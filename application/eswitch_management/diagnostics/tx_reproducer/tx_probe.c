/* Standalone diagnostic. Bring-up is based on the DOCA 3.4 switch-to-wire
 * sample; no eswitch-management pipeline or RX mbuf is used here. */
#include <inttypes.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_errno.h>
#include <doca_dpdk.h>
#include <flow_common.h>
#include <flow_switch_common.h>
#include "probe_packet.h"

static uint8_t frame[60];
static struct rte_mempool *pool;
static volatile sig_atomic_t interrupted;
static uint32_t expected_vf;

static void stop_probe(int sig) { (void)sig; interrupted = 1; }

int tx_probe_validate_config(void)
{
    uint8_t vm[6], gw[6], ip[4], gateway[4];
    const char *vf = getenv("TX_PROBE_VF");
    const char *vm_ip = getenv("TX_PROBE_VM_IP");
    const char *gw_ip = getenv("TX_PROBE_GATEWAY_IP");
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
                              struct doca_flow_pipe_entry **entry)
{
    struct doca_flow_pipe_cfg *cfg = NULL;
    struct doca_flow_pipe *pipe = NULL;
    struct doca_flow_match match = {0}; /* Deliberate match-all test. */
    struct doca_flow_monitor mon = {0};
    struct doca_flow_fwd fwd = {0}, miss = {0};
    doca_error_t r;
    mon.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
    miss.type = DOCA_FLOW_FWD_DROP;
    fwd.type = egress ? DOCA_FLOW_FWD_PORT : DOCA_FLOW_FWD_DROP;
    if (egress) fwd.port_id = 1; /* Mapping verified before queues/Flow init. */
    r = doca_flow_pipe_cfg_create(&cfg, sw);
    if (r != DOCA_SUCCESS) return r;
    r = set_flow_pipe_cfg(cfg, egress ? "TX_PROBE_EGRESS" : "TX_PROBE_INGRESS_DROP",
                          DOCA_FLOW_PIPE_BASIC, true);
    if (r == DOCA_SUCCESS) r = doca_flow_pipe_cfg_set_nr_entries(cfg, 1);
    if (r == DOCA_SUCCESS && egress)
        r = doca_flow_pipe_cfg_set_domain(cfg, DOCA_FLOW_PIPE_DOMAIN_EGRESS);
    if (r == DOCA_SUCCESS) r = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
    if (r == DOCA_SUCCESS) r = doca_flow_pipe_cfg_set_monitor(cfg, &mon);
    if (r == DOCA_SUCCESS) r = doca_flow_pipe_create(cfg, &fwd, &miss, &pipe);
    doca_flow_pipe_cfg_destroy(cfg);
    if (r != DOCA_SUCCESS) return r;
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
    struct doca_flow_resource_query q = {0}, iq = {0};
    struct rte_eth_stats before = {0}, after = {0};
    uint64_t accepted = 0;
    doca_error_t r;
    if (nb_ports != 2 || nb_queues != 1) return DOCA_ERROR_INVALID_VALUE;
    resource.mode = DOCA_FLOW_RESOURCE_MODE_PORT;
    resource.nr_counters = 8;
    resource.nr_rss = 1;
    fprintf(stderr, "PROBE CONFIG: switch,hws,hairpinq_num=4,expert; metadata=disabled; "
            "packets=10 interval=1s; DEFAULT=DROP; EGRESS=match-all->VF\n");
    r = init_doca_flow(nb_queues, "switch,hws,hairpinq_num=4,expert", &resource, shared);
    if (r != DOCA_SUCCESS) return r;
    r = init_doca_flow_switch_ports(ctx->devs_ctx.devs_manager, ctx->devs_ctx.nb_devs,
                                    ports, nb_ports, actions, &resource);
    if (r != DOCA_SUCCESS) { doca_flow_destroy(); return r; }
    sw = doca_flow_port_switch_get(ports[0]);
    if (!sw) { r = DOCA_ERROR_BAD_STATE; goto out; }
    r = root_pipe(sw, false, &status, &ingress);
    if (r == DOCA_SUCCESS) r = root_pipe(sw, true, &status, &egress);
    if (r == DOCA_SUCCESS) r = doca_flow_entries_process(sw, 0, DEFAULT_TIMEOUT_US, 2);
    if (r != DOCA_SUCCESS) goto out;
    if (status.failure || status.nb_processed != 2) {
        fprintf(stderr, "ENTRY ERROR: processed=%d expected=2 failure=%d\n",
                status.nb_processed, status.failure);
        r = DOCA_ERROR_BAD_STATE; goto out;
    }
    r = doca_flow_resource_query_entry(egress, &q);
    if (r != DOCA_SUCCESS) goto out;
    if (q.counter.total_pkts != 0) { r = DOCA_ERROR_BAD_STATE; goto out; }
    pool = rte_pktmbuf_pool_create("tx_probe_fresh", 127, 0, 0,
                                   RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (!pool) {
        fprintf(stderr, "POOL ERROR: %s\n", rte_strerror(rte_errno));
        r = DOCA_ERROR_NO_MEMORY; goto out;
    }
    if (rte_eth_stats_get(0, &before) != 0) { r = DOCA_ERROR_BAD_STATE; goto out; }
    signal(SIGINT, stop_probe);
    signal(SIGTERM, stop_probe);
    fprintf(stderr, "PROBE READY: capture on VM now; TX begins in 5 seconds (no ping needed)\nFRAME:");
    for (unsigned int i = 0; i < sizeof(frame); ++i) fprintf(stderr, " %02x", frame[i]);
    fprintf(stderr, "\n");
    for (int i = 0; i < 5 && !interrupted; ++i) sleep(1);
    for (int i = 0; i < 10 && !interrupted; ++i) {
        struct rte_mbuf *m = rte_pktmbuf_alloc(pool);
        if (!m) { r = DOCA_ERROR_NO_MEMORY; goto out; }
        void *bytes = rte_pktmbuf_append(m, sizeof(frame));
        if (!bytes) { rte_pktmbuf_free(m); r = DOCA_ERROR_NO_MEMORY; goto out; }
        memcpy(bytes, frame, sizeof(frame));
        m->ol_flags = 0;
        fprintf(stderr, "TX BUILD: seq=%d parent=0 queue=0 target=1 len=%u segments=%u flags=0\n",
                i + 1, m->pkt_len, m->nb_segs);
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
    if (interrupted || accepted != 10 || q.counter.total_pkts != accepted)
        r = DOCA_ERROR_BAD_STATE;
out:
    if (r != DOCA_SUCCESS)
        fprintf(stderr, "PROBE ERROR: %s; accepted=%" PRIu64 " processed=%d failure=%d\n",
                doca_error_get_descr(r), accepted, status.nb_processed, status.failure);
    doca_error_t stopped = stop_doca_flow_ports(nb_ports, ports);
    doca_flow_destroy();
    return r == DOCA_SUCCESS ? stopped : r;
}
