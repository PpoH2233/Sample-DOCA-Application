#include "router_pending.h"

#include <stdlib.h>
#include <string.h>

static bool same_neighbor(const struct router_pending_packet *packet,
                          uint16_t vr_id, uint16_t interface_id,
                          uint32_t next_hop_ip) {
  return packet->used && packet->vr_id == vr_id &&
         packet->neighbor_interface_id == interface_id &&
         packet->next_hop_ip == next_hop_ip;
}

bool router_pending_enqueue(struct router_pending_queue *queue,
                            uint16_t vr_id,
                            uint16_t neighbor_interface_id,
                            uint32_t next_hop_ip,
                            uint16_t ingress_interface_id,
                            uint16_t ingress_port,
                            uint8_t link_hops,
                            uint16_t expected_egress_interface,
                            const uint8_t *frame, size_t length,
                            uint64_t now_ns) {
  struct router_pending_packet *free_packet = NULL;
  size_t neighbor_count = 0;
  uint8_t *copy;

  if (queue == NULL || vr_id == 0 || neighbor_interface_id == 0 ||
      next_hop_ip == 0 || ingress_interface_id == 0 || frame == NULL ||
      length < 14 || length > ROUTER_PENDING_MAX_FRAME)
    return false;
  for (size_t i = 0; i < ROUTER_PENDING_MAX_PACKETS; i++) {
    struct router_pending_packet *packet = &queue->entries[i];

    if (!packet->used) {
      if (free_packet == NULL)
        free_packet = packet;
      continue;
    }
    if (same_neighbor(packet, vr_id, neighbor_interface_id, next_hop_ip))
      neighbor_count++;
  }
  if (free_packet == NULL ||
      neighbor_count >= ROUTER_PENDING_PER_NEIGHBOR) {
    queue->overflow++;
    return false;
  }
  copy = malloc(length);
  if (copy == NULL) {
    queue->allocation_failures++;
    return false;
  }
  memcpy(copy, frame, length);
  *free_packet = (struct router_pending_packet){
      .used = true,
      .vr_id = vr_id,
      .neighbor_interface_id = neighbor_interface_id,
      .next_hop_ip = next_hop_ip,
      .ingress_interface_id = ingress_interface_id,
      .ingress_port = ingress_port,
      .expected_egress_interface = expected_egress_interface,
      .link_hops = link_hops,
      .length = length,
      .queued_ns = now_ns,
      .frame = copy,
  };
  queue->count++;
  queue->enqueued++;
  return true;
}

bool router_pending_take(struct router_pending_queue *queue,
                         uint16_t vr_id, uint16_t neighbor_interface_id,
                         uint32_t next_hop_ip,
                         struct router_pending_packet *packet) {
  if (queue == NULL || packet == NULL)
    return false;
  for (size_t i = 0; i < ROUTER_PENDING_MAX_PACKETS; i++) {
    struct router_pending_packet *candidate = &queue->entries[i];

    if (!same_neighbor(candidate, vr_id, neighbor_interface_id, next_hop_ip))
      continue;
    *packet = *candidate;
    *candidate = (struct router_pending_packet){0};
    queue->count--;
    queue->replayed++;
    return true;
  }
  return false;
}

size_t router_pending_expire(struct router_pending_queue *queue,
                             uint64_t now_ns) {
  size_t expired = 0;

  if (queue == NULL)
    return 0;
  for (size_t i = 0; i < ROUTER_PENDING_MAX_PACKETS; i++) {
    struct router_pending_packet *packet = &queue->entries[i];

    if (!packet->used || now_ns - packet->queued_ns <=
                             ROUTER_PENDING_TIMEOUT_NS)
      continue;
    free(packet->frame);
    *packet = (struct router_pending_packet){0};
    queue->count--;
    queue->expired++;
    expired++;
  }
  return expired;
}

void router_pending_destroy(struct router_pending_queue *queue) {
  if (queue == NULL)
    return;
  for (size_t i = 0; i < ROUTER_PENDING_MAX_PACKETS; i++)
    free(queue->entries[i].frame);
  *queue = (struct router_pending_queue){0};
}
