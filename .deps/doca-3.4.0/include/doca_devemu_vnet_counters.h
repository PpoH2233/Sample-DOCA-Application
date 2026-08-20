/*
 * Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_devemu_vnet_counters.h
 * @page doca_devemu_vnet_counters
 * @defgroup DOCA_DEVEMU_VNET_COUNTERS DOCA Device Emulation - Virtio Network Counters
 * @ingroup DOCA_DEVEMU_VNET
 *
 * DOCA Virtio Network Counters API
 *
 * This header defines APIs for managing and querying Virtio Net virtqueues counters.
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VNET_COUNTERS_H_
#define DOCA_DEVEMU_VNET_COUNTERS_H_

#include <stdint.h>

#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_devemu_vnet_offload_engine;

/*********************************************************************************************************************
 * DOCA devemu virtio network counters structures
 *********************************************************************************************************************/

/**
 * @brief virtio tx or rx virtqueue related common counters.
 *
 * This structure contains common counters applicable to both TX queue and RX queue.
 * These counters are only applicable to TX queue and RX queue.
 */
struct doca_devemu_vnet_vq_counters_common {
	uint64_t packets; /**< number of packets transmitted or received by the tx/rx virtqueue */
	uint64_t bytes; /**< number of bytes transmitted or received by the tx/rx virtqueue */
};

/**
 * @brief RX packet size distribution counters.
 *
 * This structure contains packet size distribution counters for receive queues.
 * These counters are only applicable to RX queue (RQ).
 */
struct doca_devemu_vnet_vq_counters_rx_pkts {
	uint64_t rx_64_or_less_octet_packets;		/**< Number of packets received with size 0-64 bytes */
	uint64_t rx_65_to_127_octet_packets;		/**< Number of packets received with size 65-127 bytes */
	uint64_t rx_128_to_255_octet_packets;		/**< Number of packets received with size 128-255 bytes */
	uint64_t rx_256_to_511_octet_packets;		/**< Number of packets received with size 256-511 bytes */
	uint64_t rx_512_to_1023_octet_packets;		/**< Number of packets received with size 512-1023 bytes */
	uint64_t rx_1024_to_1522_octet_packets;		/**< Number of packets received with size 1024-1522 bytes */
	uint64_t rx_1523_to_2047_octet_packets;		/**< Number of packets received with size 1523-2047 bytes */
	uint64_t rx_2048_to_4095_octet_packets;		/**< Number of packets received with size 2048-4095 bytes */
	uint64_t rx_4096_to_8191_octet_packets;		/**< Number of packets received with size 4096-8191 bytes */
	uint64_t rx_8192_to_9022_octet_packets;		/**< Number of packets received with size 8192-9022 bytes */
};

/**
 * @brief Counters for the virtio net rx virtqueue.
 *
 * This structure contains counters for network traffic of a rx VQ.
 */
struct doca_devemu_vnet_vq_counters_rx {
	struct doca_devemu_vnet_vq_counters_common common;

	/* RX packet size distribution counters */
	struct doca_devemu_vnet_vq_counters_rx_pkts packets;
};

/**
 * @brief Counters for the virtio net tx virtqueue.
 *
 * This structure contains counters for network traffic of a tx VQ.
 */
struct doca_devemu_vnet_vq_counters_tx {
	struct doca_devemu_vnet_vq_counters_common common;
};

/*********************************************************************************************************************
 * DOCA devemu virtio network counters API
 *********************************************************************************************************************/

/**
 * @brief Counters for the virtio net virtqueues
 *
 * This object contains counters for virtio net virtqueues.
 */
struct doca_devemu_vnet_counters;

/**
 * @brief Create an empty counters for Virtio net virtqueues.
 *
 * @details Allocates an unpopulated counters structure for the associated queues of a Virtio Net offload
 * engine. Only a single instance of the counters structure is allowed per offload engine.
 *

 * @param [in] offload
 * The DOCA devemu Virtio network offload engine. Must be started.
 * @param [out] counters
 * The newly created DOCA devemu Virtio queues counters. After a successful call,
 * the counters can be used to obtain statistics for Virtio Net queues. The VQ counters do not
 * automatically reset when VQs or the offload engine are disabled. They can be cleared by calling
 * doca_devemu_vnet_counters_reset().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'offload' or 'counters' is NULL.
 * - DOCA_ERROR_NOT_PERMITTED - If 'offload' has already an associated counters structure.
 * - DOCA_ERROR_NO_MEMORY - Failed to allocate internal resources.
 * - DOCA_ERROR_BAD_STATE - If 'offload' is not started.
 * @note The returned counters should be populated using doca_devemu_vnet_counters_populate_sync().
 * The returned counters must be deallocated using doca_devemu_vnet_counters_destroy() to
 * prevent memory and resource leaks.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_counters_create(struct doca_devemu_vnet_offload_engine *offload,
					      struct doca_devemu_vnet_counters **counters);

/**
 * @brief Destroy Virtio Net virtqueue counters.
 *
 * @param [in] counters
 * The DOCA devemu Virtio network queues counters.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'counters' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_counters_destroy(struct doca_devemu_vnet_counters *counters);

/**
 * @brief Populate the counters so that it can be queried.
 *
 * @details Populates the counters, when the API completes all the counters
 * are populated for querying.
 *
 * @param [in] counters
 * The DOCA devemu Virtio network queues counters. Must not be NULL.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'counters' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_counters_populate_sync(struct doca_devemu_vnet_counters *counters);

/**
 * @brief Query the counters for a specific rx virtqueue.
 *
 * @details Retrieves the current counters for the specified rx virtqueue. The counters represent
 * values accumulated since the last reset performed with doca_devemu_vnet_counters_reset().
 *
 * @param [in] counters
 * The DOCA devemu Virtio queues counters. Must not be NULL.
 * @param [in] rx_vq_index
 * The virtqueue index of the rx virtqueue. Must be a valid rx queue index.
 * @param [out] vq_counters
 * virtqueue specific counter values.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'counters', or 'vq_counters' is NULL or rx_vq_index is invalid
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_counters_rx_vq_query(struct doca_devemu_vnet_counters *counters,
						   uint16_t rx_vq_index,
						   struct doca_devemu_vnet_vq_counters_rx *vq_counters);

/**
 * @brief Query the counters for a specific tx virtqueue.
 *
 * @details Retrieves the current counters for the specified tx virtqueue. The counters represent
 * values accumulated since the last reset performed with doca_devemu_vnet_counters_reset().
 *
 * @param [in] counters
 * The DOCA devemu Virtio queues counters. Must not be NULL.
 * @param [in] tx_vq_index
 * The virtqueue index of the tx virtqueue. Must be a valid tx queue index.
 * @param [out] vq_counters
 * virtqueue specific counter values.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'counters', or 'vq_counters' is NULL or tx_vq_index is invalid
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_counters_tx_vq_query(struct doca_devemu_vnet_counters *counters,
						   uint16_t tx_vq_index,
						   struct doca_devemu_vnet_vq_counters_tx *vq_counters);


/**
 * @brief Clears all counters.
 *
 * @details Querying counters after this operation will return only zeros.
 *
 *
 * @param [in] counters
 * The DOCA Virtio Network device counters instance. Must not be NULL.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'counters' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_counters_reset(struct doca_devemu_vnet_counters *counters);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VNET_COUNTERS_H_ */
