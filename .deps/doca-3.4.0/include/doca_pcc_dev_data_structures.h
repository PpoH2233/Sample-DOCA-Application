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
 * @defgroup DOCA_PCC_DEVICE_DATA_STRUCTURES DOCA PCC Device Data Structures
 * DOCA PCC Device data structure type definitions.
 * For more details please refer to the user guide on DOCA devzone.
 *
 * @ingroup DOCA_PCC_DEVICE
 *
 * @{
 */

#ifndef DOCA_PCC_DEV_DATA_STRUCTURES_H_
#define DOCA_PCC_DEV_DATA_STRUCTURES_H_

#if __NV_DPA == __NV_DPA_BF3
#include <doca_pcc_dev_data_structure_le_bf3.h>
#elif __NV_DPA == __NV_DPA_CX8
#include <doca_pcc_dev_data_structure_le_cx8.h>
#elif __NV_DPA == __NV_DPA_CX9
#include <doca_pcc_dev_data_structure_le_cx9.h>
#else
#error "Must supply '-mcpu' compiler option on command line"
#endif

/**
 * @brief PCC algorithm context data
 *
 * Holds the per-flow algorithm context preserved across CC events.
 */
typedef struct mlnx_cc_algo_ctxt_t doca_pcc_dev_algo_ctxt_t;

/**
 * @brief PCC event attributes
 */
typedef struct mlnx_cc_attr_t doca_pcc_dev_attr_t;

/**
 * @brief PCC congestion control event
 *
 * Top-level structure describing a single CC event.
 */
typedef struct mlnx_cc_event_t doca_pcc_dev_event_t;

/**
 * @brief PCC event general attributes
 */
typedef struct mlnx_cc_event_general_attr_t doca_pcc_dev_event_general_attr_t;

/**
 * @brief RoCE TX counters
 *
 * Tracks sent bytes and sent packets for RoCE TX events.
 */
typedef struct mlnx_cc_roce_tx_cntrs_t doca_pcc_dev_roce_tx_cntrs_t;

/**
 * @brief RoCE TX event data
 */
typedef struct mlnx_cc_roce_tx_t doca_pcc_dev_roce_tx_t;

/**
 * @brief ACK/NACK/CNP extra attributes
 */
typedef struct mlnx_cc_ack_nack_cnp_extra_t doca_pcc_dev_ack_nack_cnp_extra_t;

/**
 * @brief ACK/NACK/CNP event data
 */
typedef struct mlnx_cc_ack_nack_cnp_t doca_pcc_dev_ack_nack_cnp_t;

/**
 * @brief RTT timestamp data
 */
typedef struct mlnx_cc_rtt_tstamp_t doca_pcc_dev_rtt_tstamp_t;

/**
 * @brief Firmware data
 *
 * Carries 3 dwords of firmware-supplied data associated with a CC event.
 */
typedef struct mlnx_cc_fw_data_t doca_pcc_dev_fw_data_t;

/**
 * @brief Event-specific attributes union
 *
 * Discriminated union holding the event-type-specific payload.
 */
typedef union mlnx_cc_event_spec_attr_t doca_pcc_dev_event_spec_attr_t;

/**
 * Histogram handle
 */
typedef struct user_global_data_per_port_histogram_t *doca_pcc_dev_histogram_t;

#if __NV_DPA == __NV_DPA_BF3
/**
 * @brief RoCE TX extra attributes
 */
typedef struct mlnx_cc_roce_tx_extra_t doca_pcc_dev_roce_tx_extra_t;
#elif __NV_DPA >= __NV_DPA_CX8
/**
 * @brief Event general dword2
 *
 * Contains flow QP number and TTL/hop-limit fields.
 */
typedef struct mlnx_cc_event_general_dword2_t doca_pcc_dev_event_general_dword2_t;

/**
 * @brief RTT spec data word 0
 */
typedef struct mlnx_cc_rtt_spec_data0_t doca_pcc_dev_rtt_spec_data0_t;

/**
 * @brief RTT spec data word 1
 */
typedef struct mlnx_cc_rtt_spec_data1_t doca_pcc_dev_rtt_spec_data1_t;

/**
 * @brief RTT spec data word 2
 */
typedef struct mlnx_cc_rtt_spec_data2_t doca_pcc_dev_rtt_spec_data2_t;

/**
 * @brief RTT spec data word
 */
typedef struct mlnx_cc_rtt_spec_data3_t doca_pcc_dev_rtt_spec_data3_t;
#endif /* __NV_DPA == __NV_DPA_CX8 */

#if __NV_DPA >= __NV_DPA_CX9
/**
 * @brief PCC results extra data
 */
typedef struct pcc_dev_results_extra_t doca_pcc_dev_results_extra_t;
#endif

/** @} */

#endif /* DOCA_PCC_DEV_DATA_STRUCTURES_H_ */
