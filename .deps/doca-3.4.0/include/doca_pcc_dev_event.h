/*
 * Copyright (c) 2023-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */

/**
 * @defgroup DOCA_PCC_DEVICE_EVENT DOCA PCC Device Event
 * @ingroup DOCA_PCC_DEVICE
 *
 * @{
 */

#ifndef DOCA_PCC_DEV_EVENT_H_
#define DOCA_PCC_DEV_EVENT_H_

#include <doca_pcc_dev.h>

#ifdef __cplusplus
extern "C" {
#endif

// ======================================== Event access functions ========================================
// ========================================================================================================

/**
 * @brief For all events, return structure with general information such as event type, subtype, port and flags
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return doca_pcc_dev_event_general_attr_t
 */
DOCA_STABLE
FORCE_INLINE doca_pcc_dev_event_general_attr_t doca_pcc_dev_get_ev_attr(doca_pcc_dev_event_t *event)
{
	uint32_t value = __builtin_bswap32(*(uint32_t *)(&event->ev_attr));

	return *(doca_pcc_dev_event_general_attr_t *)(&value);
}

/**
 * @brief For all events, flow tag to indicate different flows
 *
 * @param[in]  event - pointer to opaque event structs
 *
 * @return 32 bit flow_tag
 */
DOCA_STABLE
FORCE_INLINE uint32_t doca_pcc_dev_get_flowtag(doca_pcc_dev_event_t *event)
{
	return __builtin_bswap32(event->flow_tag);
}

/**
 * @brief For all events, serial number of this event
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return 32 bit sn
 */
DOCA_STABLE
FORCE_INLINE uint32_t doca_pcc_dev_get_sn(doca_pcc_dev_event_t *event)
{
	return __builtin_bswap32(event->sn);
}

/**
 * @brief For all events, timestamp of this event
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return 32 bit time stamp (1 nSec granularity)
 */
DOCA_STABLE
FORCE_INLINE uint32_t doca_pcc_dev_get_timestamp(doca_pcc_dev_event_t *event)
{
	return __builtin_bswap32(event->timestamp);
}

/**
 * @brief For all events, TTL (for IPv4) or HopLimit (for IPv6) header’s field in received packet
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return ttl_hoplimit
 */
FORCE_INLINE uint32_t doca_pcc_dev_get_ttl_hoplimit(__attribute__((unused)) doca_pcc_dev_event_t *event)
{
#if __NV_DPA == __NV_DPA_BF3
	return 0;
#elif __NV_DPA >= __NV_DPA_CX8
	uint32_t value = __builtin_bswap32(*(uint32_t *)(&event->ev_dword2));

	return ((doca_pcc_dev_event_general_dword2_t *)(&value))->ttl_hoplimit;
#endif
}

/**
 * @brief For all events, return flow qpn (CX8+ all events, BF3 tx event only)
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return flow_qpn
 */
FORCE_INLINE uint32_t doca_pcc_dev_get_flow_qpn(doca_pcc_dev_event_t *event)
{
#if __NV_DPA == __NV_DPA_BF3
	uint32_t value = __builtin_bswap32(*(uint32_t *)(&event->ev_spec_attr.roce_tx.extra));

	return ((doca_pcc_dev_roce_tx_extra_t *)(&value))->flow_qpn;
#elif __NV_DPA >= __NV_DPA_CX8
	uint32_t value = __builtin_bswap32(*(uint32_t *)(&event->ev_dword2));

	return ((doca_pcc_dev_event_general_dword2_t *)(&value))->flow_qpn;
#endif
}

/**
 * @brief For FW events only, three DWORDs of FW data
 *
 * @param[in]  event - pointer to opaque event struct
 * @param[in]  n - dword index 0..2
 *
 * @return 32 bit DWORD information from FW
 */
DOCA_STABLE
FORCE_INLINE uint32_t doca_pcc_dev_get_fw_settings(doca_pcc_dev_event_t *event, int n)
{
	return __builtin_bswap32(event->ev_spec_attr.fw_data.data[n]);
}

/**
 * @brief For TX/ACK/NACK/CNP events, first coalesced event timestamp
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return 32 bit first time stamp (1 nSec granularity)
 */
DOCA_STABLE
FORCE_INLINE uint32_t doca_pcc_dev_get_roce_first_timestamp(doca_pcc_dev_event_t *event)
{
	return __builtin_bswap32(event->ev_spec_attr.ack_nack_cnp.first_timestamp);
}

/**
 * @brief For TX events only, counters including byte count and packet count
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return pcc_roce_tx_cntrs_t
 */
DOCA_STABLE
FORCE_INLINE doca_pcc_dev_roce_tx_cntrs_t doca_pcc_dev_get_roce_tx_cntrs(doca_pcc_dev_event_t *event)
{
	uint32_t value = __builtin_bswap32(*(uint32_t *)(&event->ev_spec_attr.roce_tx.cntrs));

	return *(doca_pcc_dev_roce_tx_cntrs_t *)(&value);
}

/**
 * @brief For ACK/NACK/CNP events, first coalesced event serial number
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return 32 bit first serial number
 */
DOCA_STABLE
FORCE_INLINE uint32_t doca_pcc_dev_get_roce_ack_first_sn(doca_pcc_dev_event_t *event)
{
	return __builtin_bswap32(event->ev_spec_attr.ack_nack_cnp.first_sn);
}

/**
 * @brief For ACK/NACK/CNP events, extra information including number of coalesced events
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return pcc_ack_nack_cnp_extra_t
 */
DOCA_STABLE
FORCE_INLINE doca_pcc_dev_ack_nack_cnp_extra_t doca_pcc_dev_get_ack_nack_cnp_extra(doca_pcc_dev_event_t *event)
{
	uint32_t value = __builtin_bswap32(*(uint32_t *)(&event->ev_spec_attr.ack_nack_cnp.extra));

	return *(doca_pcc_dev_ack_nack_cnp_extra_t *)(&value);
}

/**
 * @brief For RTT events only, the time when RTT request is sent
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return time stamp in 1 nSec
 */
DOCA_STABLE
FORCE_INLINE uint32_t doca_pcc_dev_get_rtt_req_send_timestamp(doca_pcc_dev_event_t *event)
{
#if __NV_DPA == __NV_DPA_BF3
	return __builtin_bswap32(event->ev_spec_attr.rtt_tstamp.req_send_timestamp);
#elif __NV_DPA >= __NV_DPA_CX8
	return __builtin_bswap32(*(uint32_t *)(&event->ev_spec_attr.rtt_tstamp.data0));
#endif
}

/**
 * @brief For RTT events only, the time when RTT request is received
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return time stamp in 1 nSec
 */
DOCA_STABLE
FORCE_INLINE uint32_t doca_pcc_dev_get_rtt_req_recv_timestamp(doca_pcc_dev_event_t *event)
{
#if __NV_DPA == __NV_DPA_BF3
	return __builtin_bswap32(event->ev_spec_attr.rtt_tstamp.req_recv_timestamp);
#elif __NV_DPA >= __NV_DPA_CX8
	return __builtin_bswap32(*(uint32_t *)(&event->ev_spec_attr.rtt_tstamp.data1));
#endif
}

/**
 * @brief For RTT events only, the time when RTT response is sent
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return time stamp in 1 nSec
 */
DOCA_STABLE
FORCE_INLINE uint32_t doca_pcc_dev_get_rtt_resp_send_timestamp(doca_pcc_dev_event_t *event)
{
#if __NV_DPA == __NV_DPA_BF3
	return __builtin_bswap32(event->ev_spec_attr.rtt_tstamp.resp_send_timestamp);
#elif __NV_DPA >= __NV_DPA_CX8
	return __builtin_bswap32(*(uint32_t *)(&event->ev_spec_attr.rtt_tstamp.data2));
#endif
}

/**
 * @brief Returns the user defined event data
 *
 * This function is to be used together with a custom user NP implementation.
 * The user can define a custom data format in the probe response packet.
 *
 * @note that no endianness swap is performed by the function.
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return A pointer to the user defined event data
 *
 */
DOCA_EXPERIMENTAL
FORCE_INLINE unsigned char *doca_pcc_dev_get_rtt_raw_data(doca_pcc_dev_event_t *event)
{
	return event->ev_spec_attr.reserved_at_0;
}

/**
 * @brief Returns the user defined event data size in bytes
 *
 *  @param[in]  event - pointer to opaque event struct
 *
 *  @return The user defined event data size in bytes
 */
DOCA_EXPERIMENTAL
FORCE_INLINE size_t doca_pcc_dev_get_rtt_raw_data_size(doca_pcc_dev_event_t *event)
{
	(void)(event);
	return 12;
}

/**
 * @brief For rtt event, rtt version
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return version
 */
FORCE_INLINE uint32_t doca_pcc_dev_get_rtt_resp_version(__attribute__((unused)) doca_pcc_dev_event_t *event)
{
#if __NV_DPA == __NV_DPA_BF3
	return 0;
#elif __NV_DPA >= __NV_DPA_CX8
	uint32_t value = __builtin_bswap32(*(uint32_t *)(&event->ev_spec_attr.rtt_tstamp.data3));

	return ((doca_pcc_dev_rtt_spec_data3_t *)(&value))->version;
#endif
}

/**
 * @brief For rtt event, port type: 0 - 25G, 1 - 40G, 2 - 50G, 3 - 100G, 4 - 200G, 5 - 400G, 6 - 800G
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return port_type
 */
FORCE_INLINE uint32_t doca_pcc_dev_get_rtt_resp_port_type(__attribute__((unused)) doca_pcc_dev_event_t *event)
{
#if __NV_DPA == __NV_DPA_BF3
	return 0;
#elif __NV_DPA >= __NV_DPA_CX8
	uint32_t value = __builtin_bswap32(*(uint32_t *)(&event->ev_spec_attr.rtt_tstamp.data1));

	return ((doca_pcc_dev_rtt_spec_data1_t *)(&value))->port_type;
#endif
}

/**
 * @brief For SACK event, the time when the data packet that triggered the sack was sent
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return SACK TX timestamp
 */
FORCE_INLINE uint32_t doca_pcc_dev_get_sack_tx_timestamp(__attribute__((unused)) doca_pcc_dev_event_t *event)
{
#if __NV_DPA <= __NV_DPA_CX8
	return 0;
#elif __NV_DPA >= __NV_DPA_CX9
	uint32_t value =
		((uint32_t)__builtin_bswap16(event->ev_spec_attr.rx_sack.attr_2.trigger_packet_tx_128ns_timestamp))
		<< 7;

	return value;
#endif
}

/**
 * @brief For TRIMM NACK event, the time when the packet that triggered the trimm nack was sent
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return TRIMM NACK TX timestamp
 */
FORCE_INLINE uint32_t doca_pcc_dev_get_trimm_nack_tx_timestamp(__attribute__((unused)) doca_pcc_dev_event_t *event)
{
#if __NV_DPA <= __NV_DPA_CX8
	return 0;
#elif __NV_DPA >= __NV_DPA_CX9
	uint32_t value = ((uint32_t)__builtin_bswap16(
				 event->ev_spec_attr.rx_trimm_nack.attr_3.trigger_packet_tx_128ns_timestamp))
			 << 7;

	return value;
#endif
}

/**
 * @brief For SACK event, marked if if the sack packet indicated congestion
 *
 * @param[in]  event - pointer to opaque event struct
 *
 * @return SACK ECN mark
 */
FORCE_INLINE uint32_t doca_pcc_dev_get_sack_ecn_mark(__attribute__((unused)) doca_pcc_dev_event_t *event)
{
#if __NV_DPA <= __NV_DPA_CX8
	return 0;
#elif __NV_DPA >= __NV_DPA_CX9
	return (event->ev_spec_attr.rx_sack.attr_1.seth_m_bits) == 0x1;
#endif
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_PCC_DEV_EVENT_H_ */
