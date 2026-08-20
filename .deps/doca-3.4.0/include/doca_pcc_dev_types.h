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
 * @defgroup DOCA_PCC_DEVICE DOCA PCC Device
 * DOCA PCC Device library. For more details please refer to the user guide on DOCA devzone.
 *
 * @ingroup DOCA_PCC
 *
 * @{
 */

#ifndef DOCA_PCC_DEV_TYPES_H_
#define DOCA_PCC_DEV_TYPES_H_

/**
 * @brief declares that we are compiling for the DPA Device
 *
 * @note Must be defined before the first API use/include of DOCA
 */
#define DOCA_DPA_DEVICE

#include <doca_pcc_dev_data_structures.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CC event type
 */
typedef enum {
	DOCA_PCC_DEV_EVNT_NULL = 0,		     /**< Unspecified event type */
	DOCA_PCC_DEV_EVNT_FW = 1,		     /**< Deprecated - not used */
	DOCA_PCC_DEV_EVNT_ROCE_CNP = 2,		     /**< RoCE CNP (Congestion Notification Packet) received */
	DOCA_PCC_DEV_EVNT_ROCE_TX = 3,		     /**< TX packet burst transition ended */
	DOCA_PCC_DEV_EVNT_ROCE_ACK = 4,		     /**< RoCE ACK Packet received */
	DOCA_PCC_DEV_EVNT_ROCE_NACK = 5,	     /**< RoCE NACK Packet received */
	DOCA_PCC_DEV_EVNT_RTT = 6,		     /**< RTT probe response packet event */
	DOCA_PCC_DEV_EVNT_ROCE_TX_FOR_ACK_NACK = 31, /**< ROCE TX event for ACK/NACK packets */
} doca_pcc_dev_event_type_enum;

/**
 * @brief CC Nack event subtypes
 */
typedef enum {
	DOCA_PCC_DEV_NACK_EVNT_NULL = 0,     /**< Unspecified NACK type */
	DOCA_PCC_DEV_NACK_EVNT_RNR = 1,	     /**< RNR (Receiver Not Ready) NACK received */
	DOCA_PCC_DEV_NACK_EVNT_OOS = 2,	     /**< OOS (Out of Sequence) NACK received */
	DOCA_PCC_DEV_NACK_EVNT_DUP_READ = 3, /**< Duplicated Read (with same PSN) NACK received */
} doca_pcc_dev_nack_event_sub_type_enum;

/**
 * @brief CC algorithm results
 */
typedef struct doca_pcc_dev_results {
	uint32_t rate; /**< Tx rate of the CC flow */
	union {
		uint32_t rtt_req;   /**< rtt request bit */
		uint32_t probe_req; /**< probe request bit */
	};
	uint32_t probe_type_slot; /**< probe type slot to indicate which probe to send */
#if __NV_DPA >= __NV_DPA_CX9
	doca_pcc_dev_results_extra_t *extra; /**< internal fields */
#endif
} doca_pcc_dev_results_t;

#if __NV_DPA >= __NV_DPA_CX8
#define DOCA_PCC_DEV_ACK_NACK_TX_EVENT_DISABLED_SUPPORTED (1) /**< Flag to check if disabling ACK/NACK is supported */
#else
#define DOCA_PCC_DEV_ACK_NACK_TX_EVENT_DISABLED_SUPPORTED (0)
#endif

/**
 * @brief TX Flag: Ack expected
 */
#define DOCA_PCC_DEV_TX_FLAG_ACK_EXPECTED (1 << 0)

/**
 * @brief TX Flag: Overloaded:
 */
#define DOCA_PCC_DEV_TX_FLAG_OVERLOADED (1 << 1)

/**
 * @brief TX Flag: RTT packet sent
 */
#define DOCA_PCC_DEV_TX_FLAG_RTT_REQ_SENT (1 << 2)

/**
 * @brief TX Flag: Is requestor
 */
#define DOCA_PCC_DEV_TX_FLAG_IS_REQUESTOR (1 << 5)

/**
 * @brief defines the fixed point fraction size of the rate limiter
 */
#define DOCA_PCC_DEV_LOG_MAX_RATE (20) /* rate format in fixed point 20 */

/**
 * @brief Max rate in rate limiter fixed point
 */
#define DOCA_PCC_DEV_MAX_RATE (1U << DOCA_PCC_DEV_LOG_MAX_RATE)

/**
 * @brief Default rate. The user overrides the default in the user algo function
 */
#define DOCA_PCC_DEV_DEFAULT_RATE ((DOCA_PCC_DEV_MAX_RATE >> 8) > (1) ? (DOCA_PCC_DEV_MAX_RATE >> 8) : (1))

/**
 * @brief Max number of algo slots supported by the lib
 */
#define DOCA_PCC_DEV_MAX_NUM_USER_SLOTS (8)

/**
 * @brief Reserved algo slot for internal algo provided by the lib.
 */
#define DOCA_PCC_DEV_ALGO_SLOT_INTERNAL (0xF)

/**
 * @brief Max number of algos supported by the lib
 */
#define DOCA_PCC_DEV_MAX_NUM_ALGOS (8)

/**
 * @brief Reserved algo index for internal algo provided by the lib.
 */
#define DOCA_PCC_DEV_ALGO_INDEX_INTERNAL (0xF)

/**
 * @brief Max number of parameters per algo supported by the lib
 */
#define DOCA_PCC_DEV_MAX_NUM_PARAMS_PER_ALGO (0x2C)

/**
 * @brief Max number of counters per algo supported by the lib
 */
#define DOCA_PCC_DEV_MAX_NUM_COUNTERS_PER_ALGO (0x3F)

#if __NV_DPA >= __NV_DPA_CX9
/**
 * @brief QP is requestor
 */
#define DOCA_PCC_DEV_QP_IS_REQUESTOR (1 << 5)
#endif

/**
 * @brief Algorithm metadata structure
 */
struct doca_pcc_dev_algo_meta_data {
	uint32_t algo_id;	     /**< algo unique identifier */
	uint32_t algo_major_version; /**< algo major version */
	uint32_t algo_minor_version; /**< algo minor version */
	uint32_t algo_desc_size;     /**< size of description string (null terminated) */
	uint64_t algo_desc_addr;     /**< pointer to description string */
};

/**
 * @brief Max number of counter groups supported by the lib
 */
#define DOCA_PCC_DEV_MAX_NUM_COUNTER_GROUP (15)

/**
 * @brief Min number of counter groups supported by the lib
 */
#define DOCA_PCC_DEV_MIN_NUM_COUNTER_GROUP (1)

/**
 * @brief Default number of counter groups supported by the lib
 */
#define DOCA_PCC_DEV_DEFAULT_NUM_COUNTER_GROUP (2)

/**
 * @brief Default counter group id for algo
 */
#define DOCA_PCC_DEV_DEFAULT_COUNTER_GROUP_ID (0)

/**
 * @brief Edges increasing mode of histogram
 */
typedef enum {
	DOCA_PCC_DEV_HISTOGRAM_MODE_LINEAR,	 // equally spaced bins
	DOCA_PCC_DEV_HISTOGRAM_MODE_EXPONENTIAL, // exponential growth
	DOCA_PCC_DEV_HISTOGRAM_MODE_FREE,	 // arbitrary edges
} doca_pcc_dev_histogram_mode_t;

/**
 * @brief Max number of histograms supported per port
 */
#define DOCA_PCC_DEV_MAX_NUM_HISTOGRAMS (DOCA_PCC_DEV_MAX_NUM_COUNTER_GROUP)

/**
 * @brief Default number of histograms supported per port
 */
#define DOCA_PCC_DEV_DEFAULT_NUM_HISTOGRAMS (0)

/**
 * @brief Max number of bins supported in each histogram
 */
#define DOCA_PCC_DEV_HISTOGRAM_MAX_BINS (32)

/** Accumulate (sum) values across all counter sets (default). See doca_pcc_dev_set_counter_type(). */
#define DOCA_PCC_DEV_COUNTER_TYPE_ACCUMULATE (0U)

/** Take the minimum value across all counter sets. See doca_pcc_dev_set_counter_type(). */
#define DOCA_PCC_DEV_COUNTER_TYPE_MIN (1U)

/** Take the maximum value across all counter sets. See doca_pcc_dev_set_counter_type(). */
#define DOCA_PCC_DEV_COUNTER_TYPE_MAX (2U)

#ifdef __cplusplus
}
#endif

#endif /* DOCA_PCC_DEV_TYPES_H_ */

/** @} */
