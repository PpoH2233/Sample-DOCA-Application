#ifndef SWITCH_CONFIG_H
#define SWITCH_CONFIG_H

#include <stdint.h>

/*
 * Keep the first version deliberately small enough to inspect in a debugger.
 * A production switch should make these values configurable and validate them
 * against the device capabilities discovered at runtime.
 */
#define SWITCH_FLOW_QUEUE_ID 0
#define SWITCH_RX_QUEUE_ID 0
#define SWITCH_TX_QUEUE_ID 0
#define SWITCH_RX_DESC 1024
#define SWITCH_TX_DESC 1024
#define SWITCH_MBUF_COUNT 8191
#define SWITCH_MBUF_CACHE 250
#define SWITCH_PACKET_BURST 64
#define SWITCH_MAX_FDB_ENTRIES 1024
#define SWITCH_FDB_AGING_SECONDS 300
#define SWITCH_AGING_SCAN_SECONDS 1
/* If a hardware counter stays busy, allow this extra idle interval before
 * trying to retire the software FDB entry. A removed active entry is safe but
 * causes one relearn, so prefer waiting over deleting on the first query
 * failure. */
#define SWITCH_FDB_QUERY_FAILURE_GRACE_SECONDS 30
#define SWITCH_FDB_RETRY_MAX_SECONDS 30
#define SWITCH_FLOW_TIMEOUT_US 1000000
#define SWITCH_ACTIONS_MEM_SIZE (64U * 1024U)
#define SWITCH_FLOW_COUNTER_COUNT (SWITCH_MAX_FDB_ENTRIES + 64U)
#define SWITCH_FLOW_RSS_COUNT 1U

/*
 * DOCA Flow uses the mlx5 hardware steering path in switch mode.
 * fdb_def_rule_en=0 makes misses the responsibility of this application.
 */
#define SWITCH_DPDK_DEVARGS "dv_flow_en=2,fdb_def_rule_en=0,dv_xmeta_en=4"
#define SWITCH_FLOW_MODE_ARGS "switch,hws"

#endif /* SWITCH_CONFIG_H */
