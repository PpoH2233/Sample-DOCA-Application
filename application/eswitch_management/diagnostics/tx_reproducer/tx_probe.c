/* Standalone diagnostic. Bring-up is based on the DOCA 3.4 switch-to-wire
 * sample; no eswitch-management pipeline or RX mbuf is used here. */
#include <inttypes.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <rte_ethdev.h>
#include <rte_flow.h>
#include <rte_mbuf.h>
#include <rte_errno.h>
#include <doca_dpdk.h>
#include <flow_common.h>
#include <flow_switch_common.h>
#include "probe_packet.h"
#include "probe_match.h"

static uint8_t frame[60];
static struct rte_mempool *pool;
static volatile sig_atomic_t interrupted;
static uint32_t expected_vf;
static bool hardware_path;
static bool hardware_arp;
static bool egress_only;
static bool egress_matrix;
static bool rx_reinject;

static void stop_probe(int sig) { (void)sig; interrupted = 1; }

int tx_probe_validate_config(void)
{
    uint8_t vm[6], gw[6], ip[4], gateway[4];
    const char *vf = getenv("TX_PROBE_VF");
    const char *vm_ip = getenv("TX_PROBE_VM_IP");
    const char *gw_ip = getenv("TX_PROBE_GATEWAY_IP");
    const char *mode = getenv("TX_PROBE_PATH");
    if (mode && strcmp(mode, "sw") && strcmp(mode, "hw") && strcmp(mode, "hw-arp") &&
        strcmp(mode, "sw-egress") && strcmp(mode, "sw-egress-matrix") &&
        strcmp(mode, "rx-reinject")) {
        fprintf(stderr, "CONFIG ERROR: TX_PROBE_PATH must be sw, sw-egress, "
                "sw-egress-matrix, rx-reinject, hw or hw-arp\n");
        return -1;
    }
    hardware_arp = mode && strcmp(mode, "hw-arp") == 0;
    egress_matrix = mode && strcmp(mode, "sw-egress-matrix") == 0;
    rx_reinject = mode && strcmp(mode, "rx-reinject") == 0;
    egress_only = egress_matrix || rx_reinject || (mode && strcmp(mode, "sw-egress") == 0);
    hardware_path = hardware_arp || (mode && strcmp(mode, "hw") == 0);
    /* Explicit test scope, not a default destination. No TX before all gates. */
    if (!vf || strlen(vf) != 2 || vf[0] != '1' || vf[1] < '0' || vf[1] > '5' ||
        probe_mac(getenv("TX_PROBE_VM_MAC"), vm) != 0 ||
        probe_mac(getenv("TX_PROBE_GATEWAY_MAC"), gw) != 0 || !vm_ip || !gw_ip ||
        inet_pton(AF_INET, vm_ip, ip) != 1 || inet_pton(AF_INET, gw_ip, gateway) != 1) {
        fprintf(stderr, "CONFIG ERROR: require TX_PROBE_VF=10..15, TX_PROBE_VM_MAC, "
                "TX_PROBE_GATEWAY_MAC (unicast), TX_PROBE_VM_IP, TX_PROBE_GATEWAY_IP (IPv4).\n");
        return -1;
    }
    expected_vf = (uint32_t)strtoul(vf, NULL, 10);
    probe_arp(frame, vm, gw, ip, gateway);
    return 0;
}

doca_error_t tx_probe_validate_ports(struct flow_switch_ctx *ctx)
{
    struct doca_dev *parent = NULL;
    struct doca_dev_rep *mapped = NULL;
    struct doca_devinfo_rep *info;
    enum doca_pci_func_type type;
    uint32_t host, pf, vf;
    doca_error_t r;
    /* The sample helpers use contiguous logical IDs. Validate their 0/1
     * contract rather than silently assuming our daemon's port inventory. */
    if (!ctx->is_expert || ctx->devs_ctx.nb_devs != 1 ||
        ctx->devs_ctx.devs_manager[0].nb_reps != 1 ||
        rte_eth_dev_count_avail() != 2) {
        fprintf(stderr, "PORT ERROR: require --expert-mode and exactly one parent + one VF.\n");
        return DOCA_ERROR_INVALID_VALUE;
    }
    r = doca_dpdk_port_as_dev(0, &parent);
    if (r != DOCA_SUCCESS || parent != ctx->devs_ctx.devs_manager[0].doca_dev) {
        fprintf(stderr, "PORT ERROR: sample parent=0 mapping not satisfied\n");
        return DOCA_ERROR_BAD_STATE;
    }
    r = doca_dpdk_open_dev_rep_by_port_id(1, parent, &mapped);
    if (r != DOCA_SUCCESS) return r;
    info = doca_dev_rep_as_devinfo(mapped);
    r = doca_devinfo_rep_get_pci_func_type(info, &type);
    if (r == DOCA_SUCCESS && type != DOCA_PCI_FUNC_TYPE_VF) r = DOCA_ERROR_INVALID_VALUE;
    if (r == DOCA_SUCCESS) r = doca_devinfo_rep_get_host_index(info, &host);
    if (r == DOCA_SUCCESS) r = doca_devinfo_rep_get_pf_index(info, &pf);
    if (r == DOCA_SUCCESS) r = doca_devinfo_rep_get_vf_index(info, &vf);
    if (r == DOCA_SUCCESS) {
        fprintf(stderr, "PORT MAP: parent=0 target=1 host=%u pf=%u vf=%u expected-vf=%u\n",
                host, pf, vf, expected_vf);
        if (host != 1 || pf != 0 || vf != expected_vf) r = DOCA_ERROR_INVALID_VALUE;
    }
    doca_error_t close_result = doca_dev_rep_close(mapped);
    return r == DOCA_SUCCESS ? close_result : r;
}

/* Called only AFTER the parent's queues have been stopped/closed by main.
 * Successfully enqueued mbufs belong to DPDK, never to the send loop. */
void tx_probe_release_pool(void)
{
    if (pool) rte_mempool_free(pool);
    pool = NULL;
}

static doca_error_t root_pipe(struct doca_flow_port *sw, bool egress,
                              struct entries_status *status,
                              struct doca_flow_pipe_entry **entry,
                              struct doca_flow_pipe **egress_pipe)
{
    struct doca_flow_pipe_cfg *cfg = NULL;
    struct doca_flow_pipe *pipe = NULL;
    struct doca_flow_match match = {0}; /* Deliberate match-all test. */
    struct doca_flow_monitor mon = {0};
    struct doca_flow_fwd fwd = {0}, miss = {0};
    uint16_t rss_queue = 0;
    doca_error_t r;
    mon.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
    miss.type = DOCA_FLOW_FWD_DROP;
    fwd.type = egress && !egress_only ? DOCA_FLOW_FWD_PORT : DOCA_FLOW_FWD_DROP;
    if (egress && !egress_only) fwd.port_id = 1; /* Mapping verified before queues/Flow init. */
    if (!egress && (hardware_path || rx_reinject)) {
        /* Only selected frames from the verified VF/VM can cross domains.
         * Broadcast dst MAC is changeable (all ones), not a fixed field. */
        match.parser_meta.port_id = 1;
        memcpy(match.outer.eth.src_mac, frame, 6);
        memcpy(match.outer.eth.dst_mac, frame + 6, 6);
        match.outer.eth.type = rte_cpu_to_be_16(0x88b5);
        if (hardware_arp || rx_reinject) {
            memset(match.outer.eth.dst_mac, 0xff, 6);
            match.outer.eth.type = rte_cpu_to_be_16(0x0806);
        }
        if (rx_reinject) {
            fwd.type = DOCA_FLOW_FWD_RSS;
            fwd.rss_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
            fwd.rss.queues_array = &rss_queue;
            fwd.rss.nr_queues = 1;
            fwd.rss.inner_flags = DOCA_FLOW_RSS_AUTO;
        } else {
            fwd.type = DOCA_FLOW_FWD_PIPE;
            fwd.next_pipe = *egress_pipe;
        }
    }
    r = doca_flow_pipe_cfg_create(&cfg, sw);
    if (r != DOCA_SUCCESS) return r;
    r = set_flow_pipe_cfg(cfg, egress ? "TX_PROBE_EGRESS" :
                          (rx_reinject ? "TX_PROBE_INGRESS_RSS" :
                           (hardware_path ? "TX_PROBE_INGRESS_TO_EGRESS" : "TX_PROBE_INGRESS_DROP")),
                          DOCA_FLOW_PIPE_BASIC, true);
    if (r == DOCA_SUCCESS) r = doca_flow_pipe_cfg_set_nr_entries(cfg, 1);
    if (r == DOCA_SUCCESS && egress)
        r = doca_flow_pipe_cfg_set_domain(cfg, DOCA_FLOW_PIPE_DOMAIN_EGRESS);
    if (r == DOCA_SUCCESS) r = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
    if (r == DOCA_SUCCESS) r = doca_flow_pipe_cfg_set_monitor(cfg, &mon);
    if (r == DOCA_SUCCESS) r = doca_flow_pipe_create(cfg, &fwd, &miss, &pipe);
    doca_flow_pipe_cfg_destroy(cfg);
    if (r != DOCA_SUCCESS) return r;
    if (egress) *egress_pipe = pipe;
    probe_entry_match(&match, egress, hardware_arp || rx_reinject);
    if (!egress && (hardware_arp || rx_reinject)) {
        fprintf(stderr, "MATCH DEBUG: DOCA-3.4 implicit ingress port=1 "
                "ethertype=0x0806 dst_template=ff:ff:ff:ff:ff:ff(changeable) "
                "dst_entry=%02x:%02x:%02x:%02x:%02x:%02x fwd=EGRESS\n",
                match.outer.eth.dst_mac[0], match.outer.eth.dst_mac[1],
                match.outer.eth.dst_mac[2], match.outer.eth.dst_mac[3],
                match.outer.eth.dst_mac[4], match.outer.eth.dst_mac[5]);
    }
    return doca_flow_pipe_basic_add_entry(0, pipe, &match, 0, NULL, &mon, NULL,
                                          DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, status, entry);
}

doca_error_t flow_switch_to_wire(int nb_queues, int nb_ports, struct flow_switch_ctx *ctx)
{
    struct flow_resources resource = {0};
    uint32_t shared[SHARED_RESOURCE_NUM_VALUES] = {0};
    struct doca_flow_port *ports[2] = {0}, *sw;
    uint32_t actions[2] = {ACTIONS_MEM_SIZE(2), ACTIONS_MEM_SIZE(2)};
    struct entries_status status = {0};
    struct doca_flow_pipe_entry *egress = NULL, *ingress = NULL;
    struct doca_flow_pipe *egress_pipe = NULL;
    struct doca_flow_resource_query q = {0}, iq = {0};
    struct doca_flow_resource_query egress_before = {0}, ingress_before = {0};
    struct rte_eth_stats before = {0}, after = {0};
    uint64_t accepted = 0;
    doca_error_t r;
    if (nb_ports != 2 || nb_queues != 1) return DOCA_ERROR_INVALID_VALUE;
    resource.mode = DOCA_FLOW_RESOURCE_MODE_PORT;
    resource.nr_counters = 8;
    resource.nr_rss = 1;
    fprintf(stderr, "PROBE CONFIG: switch,hws,hairpinq_num=4,expert; metadata=%s; "
            "path=%s; EGRESS=match-all->%s; revision=egress-entry-v1\n",
            (egress_matrix || rx_reinject) ? "matrix" : "disabled",
            rx_reinject ? "rx-reinject" :
              (egress_matrix ? "sw-egress-matrix" :
               (egress_only ? "sw-egress" :
                (hardware_arp ? "hw-arp" : (hardware_path ? "hw" : "sw")))),
            egress_only ? "COUNT+DROP" : "VF");
    r = init_doca_flow(nb_queues, "switch,hws,hairpinq_num=4,expert", &resource, shared);
    if (r != DOCA_SUCCESS) return r;
    r = init_doca_flow_switch_ports(ctx->devs_ctx.devs_manager, ctx->devs_ctx.nb_devs,
                                    ports, nb_ports, actions, &resource);
    if (r != DOCA_SUCCESS) { doca_flow_destroy(); return r; }
    sw = doca_flow_port_switch_get(ports[0]);
    if (!sw) { r = DOCA_ERROR_BAD_STATE; goto out; }
    /* Destination root must exist before cross-domain ingress forwarding. */
    r = root_pipe(sw, true, &status, &egress, &egress_pipe);
    if (r == DOCA_SUCCESS) r = root_pipe(sw, false, &status, &ingress, &egress_pipe);
    if (r == DOCA_SUCCESS) r = doca_flow_entries_process(sw, 0, DEFAULT_TIMEOUT_US, 2);
    if (r != DOCA_SUCCESS) goto out;
    if (status.failure || status.nb_processed != 2) {
        fprintf(stderr, "ENTRY ERROR: processed=%d expected=2 failure=%d\n",
                status.nb_processed, status.failure);
        r = DOCA_ERROR_BAD_STATE; goto out;
    }
    r = doca_flow_resource_query_entry(egress, &q);
    if (r != DOCA_SUCCESS) goto out;
    if (!hardware_path && q.counter.total_pkts != 0) { r = DOCA_ERROR_BAD_STATE; goto out; }
    if (hardware_path) {
        signal(SIGINT, stop_probe);
        signal(SIGTERM, stop_probe);
        fprintf(stderr, "PROBE READY: path=%s software_tx=OFF; send 10 %s "
                "from VM within 30 seconds\n", hardware_arp ? "hw-arp" : "hw",
                hardware_arp ? "broadcast ARP frames using arping -b -c 10" : "synthetic 0x88b5 frames");
        for (int i = 0; i < 30 && !interrupted; ++i) {
            sleep(1);
            r = doca_flow_resource_query_entry(ingress, &iq);
            if (r == DOCA_SUCCESS) r = doca_flow_resource_query_entry(egress, &q);
            if (r != DOCA_SUCCESS) goto out;
            fprintf(stderr, "HW SAMPLE: ingress_selected=%" PRIu64 " egress_hit=%" PRIu64 "\n",
                    iq.counter.total_pkts, q.counter.total_pkts);
        }
        for (int i = 0; i < 3 && !interrupted; ++i) sleep(1);
        r = doca_flow_resource_query_entry(ingress, &iq);
        if (r == DOCA_SUCCESS) r = doca_flow_resource_query_entry(egress, &q);
        if (r != DOCA_SUCCESS) goto out;
        fprintf(stderr, "HW SUMMARY: software_tx=0 ingress_selected=%" PRIu64
                " egress_hit=%" PRIu64 " guest_delivery=UNVERIFIED\n",
                iq.counter.total_pkts, q.counter.total_pkts);
        if (interrupted || iq.counter.total_pkts != 10 || q.counter.total_pkts != 10) {
            fprintf(stderr, "HW CHECK FAILED: expected exactly 10 selected and 10 egress hits; "
                    "zero selected means this run did not exercise the cross-domain path\n");
            r = DOCA_ERROR_BAD_STATE;
        }
        goto out;
    }
    if (rx_reinject) {
        uint64_t group_hits[3] = {0};
        uint64_t rx_count = 0;
        signal(SIGINT, stop_probe);
        signal(SIGTERM, stop_probe);
        r = doca_flow_resource_query_entry(egress, &egress_before);
        if (r == DOCA_SUCCESS) r = doca_flow_resource_query_entry(ingress, &ingress_before);
        if (r != DOCA_SUCCESS) goto out;
        fprintf(stderr, "BASELINE: egress=%" PRIu64 " ingress=%" PRIu64 "\n",
                egress_before.counter.total_pkts, ingress_before.counter.total_pkts);
        fprintf(stderr, "REINJECT READY: run exactly `arping -b -c 10 -I <vf-interface> "
                "192.168.0.1` on VF10 within 30 seconds\n");
        for (int second = 0; second < 30 && accepted < 10 && !interrupted; ++second) {
            struct rte_mbuf *packets[16];
            uint16_t received = rte_eth_rx_burst(0, 0, packets, 16);
            for (uint16_t j = 0; j < received; ++j) {
                struct rte_mbuf *m = packets[j];
                rx_count++;
                if (accepted >= 10) {
                    rte_pktmbuf_free(m);
                    continue;
                }
                const bool add_meta = accepted >= 5;
                uint64_t rx_flags = m->ol_flags;
                uint16_t rx_port = m->port;
                uint32_t rx_fdir = m->hash.fdir.hi;
                if (add_meta) {
                    rte_flow_dynf_metadata_set(m, 1);
                    m->ol_flags |= RTE_MBUF_DYNFLAG_TX_METADATA;
                }
                fprintf(stderr, "REINJECT BUILD: seq=%" PRIu64 " variant=%s rx_port=%u "
                        "rx_flags=0x%016" PRIx64 " rx_fdir=%u tx_flags=0x%016" PRIx64 "\n",
                        accepted + 1, add_meta ? "rx-mbuf+meta1" : "rx-mbuf-original",
                        rx_port, rx_flags, rx_fdir, (uint64_t)m->ol_flags);
                uint16_t sent = rte_eth_tx_burst(0, 0, &m, 1);
                if (sent == 0) rte_pktmbuf_free(m);
                accepted += sent;
                sleep(1);
                r = doca_flow_resource_query_entry(egress, &q);
                if (r != DOCA_SUCCESS) goto out;
                fprintf(stderr, "REINJECT SAMPLE: accepted=%" PRIu64 " egress_delta=%" PRIu64 "\n",
                        accepted, q.counter.total_pkts - egress_before.counter.total_pkts);
                if (accepted == 5) group_hits[1] = q.counter.total_pkts - egress_before.counter.total_pkts;
                if (accepted == 10) group_hits[2] = q.counter.total_pkts - egress_before.counter.total_pkts;
            }
            if (received == 0) sleep(1);
        }
        for (int i = 0; i < 3 && !interrupted; ++i) sleep(1);
        r = doca_flow_resource_query_entry(egress, &q);
        if (r == DOCA_SUCCESS) r = doca_flow_resource_query_entry(ingress, &iq);
        if (r != DOCA_SUCCESS) goto out;
        group_hits[2] = q.counter.total_pkts - egress_before.counter.total_pkts;
        uint64_t original_hits = group_hits[1] - group_hits[0];
        uint64_t metadata_hits = group_hits[2] - group_hits[1];
        const char *diagnosis = "actual-rx-mbuf-also-misses-egress";
        if (original_hits == 5)
            diagnosis = "fresh-mbuf-lacks-rx-origin-context";
        else if (original_hits == 0 && metadata_hits == 5)
            diagnosis = "rx-reinjection-requires-tx-metadata-flag";
        fprintf(stderr, "REINJECT RESULT: received=%" PRIu64 " accepted=%" PRIu64
                " original_hits=%" PRIu64 " metadata_hits=%" PRIu64
                " ingress_delta=%" PRIu64 " diagnosis=%s\n",
                rx_count, accepted, original_hits, metadata_hits,
                iq.counter.total_pkts - ingress_before.counter.total_pkts, diagnosis);
        if (accepted != 10 || (original_hits != 5 && metadata_hits != 5)) {
            fprintf(stderr, "CHECK FAILED: exact sample-style RX mbuf reinjection did not establish EGRESS entry\n");
            r = DOCA_ERROR_BAD_STATE;
        }
        goto out;
    }
    pool = rte_pktmbuf_pool_create("tx_probe_fresh", 127, 0, 0,
                                   RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (!pool) {
        fprintf(stderr, "POOL ERROR: %s\n", rte_strerror(rte_errno));
        r = DOCA_ERROR_NO_MEMORY; goto out;
    }
    if (rte_eth_stats_get(0, &before) != 0) { r = DOCA_ERROR_BAD_STATE; goto out; }
    signal(SIGINT, stop_probe);
    signal(SIGTERM, stop_probe);
    fprintf(stderr, "PROBE READY: %s; TX begins in 5 seconds (no ping needed)\nFRAME:",
            egress_only ? "EGRESS counter-only test; guest capture not applicable" : "capture on VM now");
    for (unsigned int i = 0; i < sizeof(frame); ++i) fprintf(stderr, " %02x", frame[i]);
    fprintf(stderr, "\n");
    for (int i = 0; i < 5 && !interrupted; ++i) sleep(1);
    r = doca_flow_resource_query_entry(egress, &egress_before);
    if (r == DOCA_SUCCESS) r = doca_flow_resource_query_entry(ingress, &ingress_before);
    if (r != DOCA_SUCCESS) goto out;
    fprintf(stderr, "BASELINE: egress=%" PRIu64 " ingress=%" PRIu64 "\n",
            egress_before.counter.total_pkts, ingress_before.counter.total_pkts);
    if (egress_only && egress_before.counter.total_pkts != 0) {
        fprintf(stderr, "CHECK FAILED: EGRESS saw traffic before software TX; isolate test\n");
        r = DOCA_ERROR_BAD_STATE;
        goto out;
    }
    const int tx_count = egress_matrix ? 12 : 10;
    uint64_t matrix_hits[5] = {0};
    for (int i = 0; i < tx_count && !interrupted; ++i) {
        struct rte_mbuf *m = rte_pktmbuf_alloc(pool);
        if (!m) { r = DOCA_ERROR_NO_MEMORY; goto out; }
        void *bytes = rte_pktmbuf_append(m, sizeof(frame));
        if (!bytes) { rte_pktmbuf_free(m); r = DOCA_ERROR_NO_MEMORY; goto out; }
        memcpy(bytes, frame, sizeof(frame));
        m->ol_flags = 0;
        const char *variant = "plain";
        unsigned group = 0;
        uint32_t metadata = 0;
        if (egress_matrix) {
            group = (unsigned)i / 3;
            if (group >= 1) {
                m->port = 0;
                variant = "port0";
            }
            if (group >= 2) {
                metadata = group == 2 ? 0 : 1;
                rte_flow_dynf_metadata_set(m, metadata);
                m->ol_flags |= RTE_MBUF_DYNFLAG_TX_METADATA;
                variant = group == 2 ? "port0+meta0" : "port0+meta1";
            }
        }
        fprintf(stderr, "TX BUILD: seq=%d variant=%s parent=0 queue=0 mbuf_port=%u "
                "metadata=%u len=%u segments=%u flags=0x%016" PRIx64 "\n",
                i + 1, variant, m->port, metadata, m->pkt_len, m->nb_segs,
                (uint64_t)m->ol_flags);
        uint16_t n = rte_eth_tx_burst(0, 0, &m, 1);
        /* Never read, free or retry an accepted mbuf. */
        if (n == 0) rte_pktmbuf_free(m);
        accepted += n;
        fprintf(stderr, "TX ACCEPTED: seq=%d accepted=%u cumulative=%" PRIu64 "\n", i+1, n, accepted);
        sleep(1);
        r = doca_flow_resource_query_entry(egress, &q);
        if (r != DOCA_SUCCESS) goto out;
        fprintf(stderr, "EGRESS SAMPLE: packets=%" PRIu64 " bytes=%" PRIu64 "\n",
                q.counter.total_pkts, q.counter.total_bytes);
        if (egress_only) {
            r = doca_flow_resource_query_entry(ingress, &iq);
            if (r != DOCA_SUCCESS) goto out;
            fprintf(stderr, "PATH SAMPLE: seq=%d accepted=%" PRIu64
                    " egress_delta=%" PRIu64 " ingress_delta=%" PRIu64 "\n",
                    i + 1, accepted, q.counter.total_pkts - egress_before.counter.total_pkts,
                    iq.counter.total_pkts - ingress_before.counter.total_pkts);
            if (egress_matrix && (i % 3) == 2) {
                matrix_hits[group + 1] = q.counter.total_pkts - egress_before.counter.total_pkts;
                fprintf(stderr, "MATRIX GROUP: variant=%s accepted=3 egress_group_delta=%" PRIu64 "\n",
                        variant, matrix_hits[group + 1] - matrix_hits[group]);
            }
        }
    }
    /* Counter settling, not an entries_process-based counter refresh. */
    for (int i = 0; i < 3; ++i) sleep(1);
    r = doca_flow_resource_query_entry(egress, &q);
    if (r == DOCA_SUCCESS) r = doca_flow_resource_query_entry(ingress, &iq);
    if (r != DOCA_SUCCESS) goto out;
    if (rte_eth_stats_get(0, &after) != 0) { r = DOCA_ERROR_BAD_STATE; goto out; }
    fprintf(stderr, "PROBE SUMMARY: accepted=%" PRIu64 " parent_opackets_delta=%" PRIu64
            " parent_oerrors_delta=%" PRIu64 " egress_hit=%" PRIu64 " ingress_drop=%" PRIu64
            " guest_delivery=UNVERIFIED\n", accepted, after.opackets-before.opackets,
            after.oerrors-before.oerrors, q.counter.total_pkts, iq.counter.total_pkts);
    if (interrupted || accepted != (uint64_t)tx_count || q.counter.total_pkts != accepted)
        r = DOCA_ERROR_BAD_STATE;
    if (egress_only) {
        uint64_t hits = q.counter.total_pkts - egress_before.counter.total_pkts;
        uint64_t ingress_hits = iq.counter.total_pkts - ingress_before.counter.total_pkts;
        if (hits != (uint64_t)tx_count || ingress_hits != 0) r = DOCA_ERROR_BAD_STATE;
        fprintf(stderr, "EGRESS RESULT: %s accepted=%" PRIu64
                " egress_delta=%" PRIu64 " ingress_delta=%" PRIu64
                " action=DROP guest_delivery=NOT_APPLICABLE\n",
                r == DOCA_SUCCESS ? "PASS" : "FAIL", accepted, hits, ingress_hits);
        if (ingress_hits != 0)
            fprintf(stderr, "CHECK FAILED: ingress traffic observed; isolate traffic before attributing EGRESS hits to TX\n");
        else if (hits != accepted)
            fprintf(stderr, "CHECK FAILED: software TX acceptance does not match EGRESS entry hits\n");
        if (egress_matrix) {
            uint64_t plain = matrix_hits[1] - matrix_hits[0];
            uint64_t port0 = matrix_hits[2] - matrix_hits[1];
            uint64_t meta0 = matrix_hits[3] - matrix_hits[2];
            uint64_t meta1 = matrix_hits[4] - matrix_hits[3];
            const char *cause = "none-of-the-mbuf-variants-entered-egress";
            if (plain) cause = "plain-fresh-mbuf-works";
            else if (port0) cause = "fresh-mbuf-needs-valid-ingress-port";
            else if (meta0) cause = "missing-tx-metadata-flag";
            else if (meta1) cause = "missing-tx-metadata-flag-and-destination-value";
            fprintf(stderr, "ROOT-CAUSE MATRIX: plain=%" PRIu64 " port0=%" PRIu64
                    " meta0=%" PRIu64 " meta1=%" PRIu64 " diagnosis=%s\n",
                    plain, port0, meta0, meta1, cause);
        }
    }
out:
    if (r != DOCA_SUCCESS)
        fprintf(stderr, "PROBE ERROR: %s; accepted=%" PRIu64 " processed=%d failure=%d\n",
                doca_error_get_descr(r), accepted, status.nb_processed, status.failure);
    doca_error_t stopped = stop_doca_flow_ports(nb_ports, ports);
    doca_flow_destroy();
    return r == DOCA_SUCCESS ? stopped : r;
}
