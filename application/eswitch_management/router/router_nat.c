#include "router_nat.h"

#include <string.h>

#define ETH_HEADER_LEN 14U
#define IPV4_MIN_HEADER_LEN 20U
#define TCP_MIN_HEADER_LEN 20U
#define UDP_HEADER_LEN 8U
#define ICMP_ECHO_HEADER_LEN 8U
#define IPPROTO_ICMP_VALUE 1U
#define IPPROTO_TCP_VALUE 6U
#define IPPROTO_UDP_VALUE 17U
#define ICMP_ECHO_REPLY 0U
#define ICMP_ECHO_REQUEST 8U

enum packet_direction {
  PACKET_OUTBOUND,
  PACKET_INBOUND,
};

struct packet_view {
  uint8_t *ip;
  uint8_t *l4;
  size_t ip_header_length;
  size_t l4_length;
  size_t frame_length;
  uint8_t protocol;
  uint32_t source_ip;
  uint32_t destination_ip;
  uint16_t source_port;
  uint16_t destination_port;
};

static uint16_t read16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

static uint32_t read32(const uint8_t *p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
         (uint32_t)p[2] << 8 | p[3];
}

static void write16(uint8_t *p, uint16_t value) {
  p[0] = (uint8_t)(value >> 8);
  p[1] = (uint8_t)value;
}

static void write32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)(value >> 24);
  p[1] = (uint8_t)(value >> 16);
  p[2] = (uint8_t)(value >> 8);
  p[3] = (uint8_t)value;
}

static uint32_t checksum_add(uint32_t sum, const uint8_t *bytes,
                             size_t length) {
  while (length >= 2) {
    sum += read16(bytes);
    bytes += 2;
    length -= 2;
  }
  if (length)
    sum += (uint16_t)bytes[0] << 8;
  return sum;
}

static uint16_t checksum_finish(uint32_t sum) {
  while (sum >> 16)
    sum = (sum & UINT16_MAX) + (sum >> 16);
  return (uint16_t)~sum;
}

static uint16_t ipv4_checksum(const uint8_t *ip, size_t length) {
  return checksum_finish(checksum_add(0, ip, length));
}

static uint16_t transport_checksum(const struct packet_view *view) {
  uint32_t sum = 0;
  uint8_t pseudo[4] = {0, view->protocol,
                       (uint8_t)(view->l4_length >> 8),
                       (uint8_t)view->l4_length};

  sum = checksum_add(sum, view->ip + 12, 8);
  sum = checksum_add(sum, pseudo, sizeof(pseudo));
  sum = checksum_add(sum, view->l4, view->l4_length);
  return checksum_finish(sum);
}

static uint16_t icmp_checksum(const struct packet_view *view) {
  return checksum_finish(checksum_add(0, view->l4, view->l4_length));
}

static enum router_nat_result packet_copy_and_parse(
    const uint8_t *frame, size_t length, uint8_t *output, size_t capacity,
    enum packet_direction direction, struct packet_view *view) {
  uint16_t total_length;
  uint16_t fragment;

  if (!frame || !output || !view || length < ETH_HEADER_LEN + IPV4_MIN_HEADER_LEN ||
      capacity < length || frame[12] != 0x08 || frame[13] != 0x00)
    return ROUTER_NAT_INVALID;
  memcpy(output, frame, length);
  memset(view, 0, sizeof(*view));
  view->ip = output + ETH_HEADER_LEN;
  if ((view->ip[0] >> 4) != 4 || (view->ip[0] & 0x0fU) < 5)
    return ROUTER_NAT_INVALID;
  view->ip_header_length = (size_t)(view->ip[0] & 0x0fU) * 4U;
  if (length < ETH_HEADER_LEN + view->ip_header_length ||
      ipv4_checksum(view->ip, view->ip_header_length) != 0)
    return ROUTER_NAT_INVALID;
  total_length = read16(view->ip + 2);
  if (total_length < view->ip_header_length ||
      total_length > length - ETH_HEADER_LEN)
    return ROUTER_NAT_INVALID;
  fragment = read16(view->ip + 6);
  if ((fragment & 0x3fffU) != 0)
    return ROUTER_NAT_UNSUPPORTED;
  view->protocol = view->ip[9];
  view->l4 = view->ip + view->ip_header_length;
  view->l4_length = total_length - view->ip_header_length;
  if ((view->protocol == IPPROTO_TCP_VALUE &&
       view->l4_length < TCP_MIN_HEADER_LEN) ||
      (view->protocol == IPPROTO_UDP_VALUE &&
       view->l4_length < UDP_HEADER_LEN) ||
      (view->protocol == IPPROTO_ICMP_VALUE &&
       view->l4_length < ICMP_ECHO_HEADER_LEN))
    return ROUTER_NAT_INVALID;
  if (view->protocol != IPPROTO_ICMP_VALUE &&
      view->protocol != IPPROTO_TCP_VALUE &&
      view->protocol != IPPROTO_UDP_VALUE)
    return ROUTER_NAT_UNSUPPORTED;
  if (view->protocol == IPPROTO_TCP_VALUE) {
    size_t tcp_header_length = (size_t)(view->l4[12] >> 4) * 4U;

    if (tcp_header_length < TCP_MIN_HEADER_LEN ||
        tcp_header_length > view->l4_length || transport_checksum(view) != 0)
      return ROUTER_NAT_INVALID;
  } else if (view->protocol == IPPROTO_UDP_VALUE) {
    if (read16(view->l4 + 4) != view->l4_length ||
        (read16(view->l4 + 6) != 0 && transport_checksum(view) != 0))
      return ROUTER_NAT_INVALID;
  } else {
    uint8_t expected_type = direction == PACKET_OUTBOUND
                                ? ICMP_ECHO_REQUEST
                                : ICMP_ECHO_REPLY;

    if (view->l4[0] != expected_type || view->l4[1] != 0)
      return ROUTER_NAT_UNSUPPORTED;
    if (icmp_checksum(view) != 0)
      return ROUTER_NAT_INVALID;
  }
  view->frame_length = ETH_HEADER_LEN + total_length;
  view->source_ip = read32(view->ip + 12);
  view->destination_ip = read32(view->ip + 16);
  if (view->protocol == IPPROTO_ICMP_VALUE) {
    uint16_t identifier = read16(view->l4 + 4);

    view->source_port = direction == PACKET_OUTBOUND ? identifier : 0;
    view->destination_port = direction == PACKET_INBOUND ? identifier : 0;
  } else {
    view->source_port = read16(view->l4);
    view->destination_port = read16(view->l4 + 2);
  }
  return ROUTER_NAT_TRANSLATED;
}

static void update_checksums(struct packet_view *view) {
  uint16_t checksum_offset;
  uint16_t value;
  bool udp_checksum_disabled = view->protocol == IPPROTO_UDP_VALUE &&
                               read16(view->l4 + 6) == 0;

  view->ip[10] = 0;
  view->ip[11] = 0;
  write16(view->ip + 10, ipv4_checksum(view->ip, view->ip_header_length));
  if (view->protocol == IPPROTO_ICMP_VALUE) {
    view->l4[2] = 0;
    view->l4[3] = 0;
    write16(view->l4 + 2, icmp_checksum(view));
    return;
  }
  if (udp_checksum_disabled)
    return;
  checksum_offset = view->protocol == IPPROTO_TCP_VALUE ? 16U : 6U;
  view->l4[checksum_offset] = 0;
  view->l4[checksum_offset + 1] = 0;
  value = transport_checksum(view);
  if (view->protocol == IPPROTO_UDP_VALUE && value == 0)
    value = UINT16_MAX;
  write16(view->l4 + checksum_offset, value);
}

/* Hash explicit host-order fields, never struct padding. Full key equality
 * below remains authoritative, including under hash collisions. */
static size_t tuple_bucket(uint16_t vr, uint8_t protocol, uint32_t ip,
                           uint16_t port, uint32_t remote, uint16_t remote_port) {
  uint32_t h = UINT32_C(2166136261);
  const uint32_t fields[] = {vr, protocol, ip, port, remote, remote_port};
  for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
    h ^= fields[i];
    h *= UINT32_C(16777619);
    h ^= h >> 16;
  }
  return h & (ROUTER_NAT_BUCKETS - 1U);
}

static size_t session_bucket(const struct router_nat_session *s, unsigned index) {
  return tuple_bucket(s->vr_id, s->protocol,
                      index == 0 ? s->inside_ip : s->public_ip,
                      index == 0 ? s->inside_port : s->public_port,
                      index == 2 ? 0 : s->remote_ip,
                      index == 2 ? 0 : s->remote_port);
}

static void index_insert(struct router_nat_table *table,
                         struct router_nat_session *s) {
  uint16_t slot = (uint16_t)(s - table->entries + 1);
  for (unsigned index = 0; index < 3; index++) {
    size_t bucket = session_bucket(s, index);
    s->index_next[index] = table->buckets[index][bucket];
    table->buckets[index][bucket] = slot;
  }
}

static void index_remove(struct router_nat_table *table,
                         struct router_nat_session *s) {
  uint16_t slot = (uint16_t)(s - table->entries + 1);
  for (unsigned index = 0; index < 3; index++) {
    uint16_t *link = &table->buckets[index][session_bucket(s, index)];
    while (*link && *link != slot)
      link = &table->entries[*link - 1].index_next[index];
    if (*link)
      *link = s->index_next[index];
  }
}

static struct router_nat_session *find_outbound(
    struct router_nat_table *table, uint16_t vr_id,
    const struct packet_view *view) {
  size_t bucket = tuple_bucket(vr_id, view->protocol, view->source_ip,
                              view->source_port, view->destination_ip,
                              view->destination_port);
  for (uint16_t slot = table->buckets[0][bucket]; slot;
       slot = table->entries[slot - 1].index_next[0]) {
    struct router_nat_session *s = &table->entries[slot - 1];
    if (s->used && s->vr_id == vr_id && s->protocol == view->protocol &&
        s->inside_ip == view->source_ip &&
        s->inside_port == view->source_port &&
        s->remote_ip == view->destination_ip &&
        s->remote_port == view->destination_port)
      return s;
  }
  return NULL;
}

static struct router_nat_session *find_inbound(
    struct router_nat_table *table, uint16_t vr_id,
    const struct packet_view *view) {
  size_t bucket = tuple_bucket(vr_id, view->protocol, view->destination_ip,
                              view->destination_port, view->source_ip,
                              view->source_port);
  for (uint16_t slot = table->buckets[1][bucket]; slot;
       slot = table->entries[slot - 1].index_next[1]) {
    struct router_nat_session *s = &table->entries[slot - 1];
    if (s->used && s->vr_id == vr_id && s->protocol == view->protocol &&
        s->public_ip == view->destination_ip &&
        s->public_port == view->destination_port &&
        s->remote_ip == view->source_ip &&
        s->remote_port == view->source_port)
      return s;
  }
  return NULL;
}

static bool port_in_use(const struct router_nat_table *table, uint16_t vr_id,
                        uint8_t protocol, uint32_t public_ip,
                        uint16_t public_port) {
  for (size_t i = 0; i < table->port_forward_count; i++) {
    const struct router_port_forward *r = &table->port_forwards[i];
    if (r->vr_id == vr_id && r->protocol == protocol &&
        r->public_ip == public_ip && r->public_port == public_port)
      return true;
  }
  size_t bucket = tuple_bucket(vr_id, protocol, public_ip, public_port, 0, 0);
  for (uint16_t slot = table->buckets[2][bucket]; slot;
       slot = table->entries[slot - 1].index_next[2]) {
    const struct router_nat_session *s = &table->entries[slot - 1];
    if (s->used && s->vr_id == vr_id && s->protocol == protocol &&
        s->public_ip == public_ip && s->public_port == public_port)
      return true;
  }
  return false;
}

static struct router_nat_session *allocate_session(
    struct router_nat_table *table, const struct router_nat_policy *policy,
    uint32_t public_ip, const struct packet_view *view,
    const struct router_nat_inside *inside, uint64_t now_ns) {
  struct router_nat_session *free_session = NULL;
  uint32_t range = (uint32_t)policy->port_last - policy->port_first + 1U;
  uint16_t start;

  for (size_t i = 0; i < ROUTER_NAT_MAX_SESSIONS; i++)
    if (!table->entries[i].used) { free_session = &table->entries[i]; break; }
  if (!free_session)
    return NULL;
  start = table->next_port;
  if (start < policy->port_first || start > policy->port_last)
    start = policy->port_first;
  for (uint32_t offset = 0; offset < range; offset++) {
    uint16_t candidate = (uint16_t)(policy->port_first +
        ((uint32_t)(start - policy->port_first) + offset) % range);
    if (port_in_use(table, policy->vr_id, view->protocol, public_ip,
                    candidate))
      continue;
    *free_session = (struct router_nat_session){
      .used=true,.vr_id=policy->vr_id,.protocol=view->protocol,
      .public_interface_id=policy->interface_id,
      .inside_ip=view->source_ip,.inside_port=view->source_port,
      .remote_ip=view->destination_ip,.remote_port=view->destination_port,
      .public_ip=public_ip,.public_port=candidate,.inside=*inside,
      .created_ns=now_ns,.last_seen_ns=now_ns};
    table->next_port = candidate == policy->port_last ? policy->port_first
                                                       : candidate + 1U;
    index_insert(table, free_session);
    table->count++;
    table->stats.sessions_created++;
    return free_session;
  }
  return NULL;
}

void router_nat_init(struct router_nat_table *table) {
  if (table)
    memset(table, 0, sizeof(*table));
}

void router_nat_set_port_forwards(struct router_nat_table *table,
                                  const struct router_config *config) {
  if (!table || !config) return;
  table->port_forward_count = config->port_forward_count;
  memcpy(table->port_forwards, config->port_forwards,
         config->port_forward_count * sizeof(config->port_forwards[0]));
}

enum router_nat_result router_nat_outbound(
    struct router_nat_table *table, const struct router_nat_policy *policy,
    uint32_t public_ip, const struct router_nat_inside *inside,
    const uint8_t *frame, size_t length, uint64_t now_ns,
    uint8_t *output, size_t capacity,
    const struct router_nat_session **session) {
  struct packet_view view;
  struct router_nat_session *s;
  enum router_nat_result result;

  if (session) *session = NULL;
  if (!table || !inside || (!policy && !inside->vr_id) ||
      (policy && (!public_ip || !policy->vr_id ||
       policy->port_first < 1024 || policy->port_first > policy->port_last)))
    return ROUTER_NAT_INVALID;
  result = packet_copy_and_parse(frame, length, output, capacity,
                                 PACKET_OUTBOUND, &view);
  if (result != ROUTER_NAT_TRANSLATED) {
    if (result == ROUTER_NAT_UNSUPPORTED) table->stats.unsupported_packets++;
    else table->stats.invalid_packets++;
    return result;
  }
  s = find_outbound(table, policy ? policy->vr_id : inside->vr_id, &view);
  if (!s && policy)
    s = allocate_session(table, policy, public_ip, &view, inside, now_ns);
  if (s && !policy && !s->port_forward)
    return ROUTER_NAT_NOT_APPLICABLE;
  if (!s) {
    if (!policy) return ROUTER_NAT_NOT_APPLICABLE;
    table->stats.port_allocation_failures++;
    return ROUTER_NAT_FULL;
  }
  write32(view.ip + 12, s->public_ip);
  if (view.protocol == IPPROTO_ICMP_VALUE)
    write16(view.l4 + 4, s->public_port);
  else
    write16(view.l4, s->public_port);
  view.source_ip = s->public_ip;
  view.source_port = s->public_port;
  update_checksums(&view);
  /* Refresh the reverse-delivery identity after an inside MAC/port move. The
   * translated 5-tuple stays stable, while replies follow the latest packet
   * that proved ownership of the session's original direction. */
  s->inside = *inside;
  s->last_seen_ns = now_ns;
  s->original_packets++;
  table->stats.outbound_packets++;
  if (s->port_forward)
    table->stats.port_forward_outbound_packets++;
  if (view.protocol == IPPROTO_ICMP_VALUE)
    table->stats.icmp_echo_outbound_packets++;
  if (session) *session = s;
  return ROUTER_NAT_TRANSLATED;
}

enum router_nat_result router_nat_inbound(
    struct router_nat_table *table, uint16_t vr_id,
    const uint8_t *frame, size_t length, uint64_t now_ns,
    uint8_t *output, size_t capacity,
    const struct router_nat_session **session) {
  struct packet_view view;
  struct router_nat_session *s;
  enum router_nat_result result;

  if (session) *session = NULL;
  if (!table || !vr_id)
    return ROUTER_NAT_INVALID;
  result = packet_copy_and_parse(frame, length, output, capacity,
                                 PACKET_INBOUND, &view);
  if (result != ROUTER_NAT_TRANSLATED) {
    if (result == ROUTER_NAT_UNSUPPORTED) table->stats.unsupported_packets++;
    else table->stats.invalid_packets++;
    return result;
  }
  s = find_inbound(table, vr_id, &view);
  if (!s) {
    table->stats.reverse_misses++;
    return ROUTER_NAT_NOT_APPLICABLE;
  }
  write32(view.ip + 16, s->inside_ip);
  if (view.protocol == IPPROTO_ICMP_VALUE)
    write16(view.l4 + 4, s->inside_port);
  else
    write16(view.l4 + 2, s->inside_port);
  view.destination_ip = s->inside_ip;
  view.destination_port = s->inside_port;
  update_checksums(&view);
  s->last_seen_ns = now_ns;
  s->reply_packets++;
  table->stats.inbound_packets++;
  if (view.protocol == IPPROTO_ICMP_VALUE)
    table->stats.icmp_echo_inbound_packets++;
  if (session) *session = s;
  return ROUTER_NAT_TRANSLATED;
}

enum router_nat_result router_nat_port_forward_inbound(
    struct router_nat_table *table, const struct router_config *config,
    uint16_t vr_id, uint16_t public_interface_id,
    const uint8_t *frame, size_t length, uint64_t now_ns,
    uint8_t *output, size_t capacity,
    const struct router_nat_session **session) {
  struct packet_view view;
  const struct router_port_forward *rule = NULL;
  struct router_nat_session *s;
  enum router_nat_result result;

  if (session) *session = NULL;
  if (!table || !config || !vr_id || !public_interface_id)
    return ROUTER_NAT_INVALID;
  result = packet_copy_and_parse(frame, length, output, capacity,
                                 PACKET_INBOUND, &view);
  if (result != ROUTER_NAT_TRANSLATED)
    return result;
  if (view.protocol != IPPROTO_TCP_VALUE &&
      view.protocol != IPPROTO_UDP_VALUE)
    return ROUTER_NAT_NOT_APPLICABLE;
  for (size_t i = 0; i < config->port_forward_count; i++) {
    const struct router_port_forward *candidate = &config->port_forwards[i];
    if (candidate->vr_id == vr_id &&
        candidate->interface_id == public_interface_id &&
        candidate->public_ip == view.destination_ip &&
        candidate->protocol == view.protocol &&
        candidate->public_port == view.destination_port) {
      rule = candidate;
      break;
    }
  }
  if (!rule) return ROUTER_NAT_NOT_APPLICABLE;

  s = find_inbound(table, vr_id, &view);
  if (s && !s->port_forward) return ROUTER_NAT_NOT_APPLICABLE;
  if (!s) {
    struct packet_view return_view = {
      .protocol = view.protocol,
      .source_ip = rule->private_ip,
      .source_port = rule->private_port,
      .destination_ip = view.source_ip,
      .destination_port = view.source_port};
    /* Two public mappings cannot safely share the same return 5-tuple.
     * Fail closed rather than guessing which public port to restore. */
    if (find_outbound(table, vr_id, &return_view) != NULL) {
      table->stats.port_forward_full++;
      return ROUTER_NAT_FULL;
    }
    for (size_t i = 0; i < ROUTER_NAT_MAX_SESSIONS; i++)
      if (!table->entries[i].used) { s = &table->entries[i]; break; }
    if (!s) {
      table->stats.port_forward_full++;
      return ROUTER_NAT_FULL;
    }
    *s = (struct router_nat_session){
      .used = true, .port_forward = true, .vr_id = vr_id,
      .public_interface_id = public_interface_id,
      .port_forward_rule_id = rule->rule_id,
      .protocol = view.protocol,
      .inside_ip = rule->private_ip, .inside_port = rule->private_port,
      .remote_ip = view.source_ip, .remote_port = view.source_port,
      .public_ip = rule->public_ip, .public_port = rule->public_port,
      .created_ns = now_ns, .last_seen_ns = now_ns};
    index_insert(table, s);
    table->count++;
    table->stats.sessions_created++;
    table->stats.port_forward_sessions_created++;
  }
  write32(view.ip + 16, s->inside_ip);
  write16(view.l4 + 2, s->inside_port);
  view.destination_ip = s->inside_ip;
  view.destination_port = s->inside_port;
  update_checksums(&view);
  s->last_seen_ns = now_ns;
  s->reply_packets++;
  table->stats.inbound_packets++;
  table->stats.port_forward_inbound_packets++;
  if (session) *session = s;
  return ROUTER_NAT_TRANSLATED;
}

void router_nat_port_forward_reject(struct router_nat_table *table,
                                    const struct router_nat_session *session) {
  if (!table || !session)
    return;
  uintptr_t base = (uintptr_t)table->entries;
  uintptr_t address = (uintptr_t)session;
  if (address < base || address - base >= sizeof(table->entries) ||
      (address - base) % sizeof(table->entries[0]) != 0)
    return;
  struct router_nat_session *s = &table->entries[
      (address - base) / sizeof(table->entries[0])];
  if (!s->used || !s->port_forward || s->hardware_active)
    return;
  index_remove(table, s);
  memset(s, 0, sizeof(*s));
  table->count--;
}

void router_nat_age(struct router_nat_table *table, uint64_t now_ns) {
  if (!table) return;
  for (size_t i = 0; i < ROUTER_NAT_MAX_SESSIONS; i++) {
    struct router_nat_session *s = &table->entries[i];
    uint64_t timeout;
    if (!s->used || s->hardware_active) continue;
    if (s->protocol == IPPROTO_TCP_VALUE)
      timeout = ROUTER_NAT_TCP_IDLE_NS;
    else if (s->protocol == IPPROTO_UDP_VALUE)
      timeout = ROUTER_NAT_UDP_IDLE_NS;
    else
      timeout = ROUTER_NAT_ICMP_IDLE_NS;
    if (now_ns - s->last_seen_ns <= timeout) continue;
    index_remove(table, s);
    *s = (struct router_nat_session){0};
    table->count--;
    table->stats.sessions_aged++;
  }
}

void router_nat_flush(struct router_nat_table *table, uint16_t vr_id) {
  if (!table) return;
  for (size_t i=0;i<ROUTER_NAT_MAX_SESSIONS;i++) {
    if(!table->entries[i].used ||
       (vr_id != 0 && table->entries[i].vr_id!=vr_id) ||
       table->entries[i].hardware_active) continue;
    index_remove(table, &table->entries[i]);
    table->entries[i]=(struct router_nat_session){0};
    table->count--;
  }
}

void router_nat_session_set_hardware_active(
    const struct router_nat_session *session, bool active) {
  /* Session storage has one dataplane owner.  The public translation API
   * returns a const view so packet callers cannot mutate tuple keys; this
   * narrow lifecycle hook is the sole exception. */
  if (session != NULL)
    ((struct router_nat_session *)session)->hardware_active = active;
}
