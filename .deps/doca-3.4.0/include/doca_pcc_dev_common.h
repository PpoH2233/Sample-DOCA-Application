/*
 * Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @defgroup DOCA_PCC_DEVICE_COMMON DOCA PCC Device Common
 * DOCA PCC Device library. For more details please refer to the user guide on DOCA devzone.
 *
 * @ingroup DOCA_PCC_DEVICE
 *
 * @{
 */

#ifndef DOCA_PCC_DEV_COMMON_H_
#define DOCA_PCC_DEV_COMMON_H_

/**
 * @brief declares that we are compiling for the DPA Device
 *
 * @note Must be defined before the first API use/include of DOCA
 */
#define DOCA_DPA_DEVICE

#include <stdint.h>
#include <stddef.h>
#include <doca_compat.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief force inline wrapper
 */
#define ALWAYS_INLINE __attribute__((always_inline))

/**
 * @brief static inline wrapper
 */
#define FORCE_INLINE static inline ALWAYS_INLINE

/**
 * @brief API functions return status
 */
typedef enum {
	DOCA_PCC_DEV_STATUS_OK = 0,   /**< completed successfully */
	DOCA_PCC_DEV_STATUS_FAIL = 1, /**< Failed */
} doca_pcc_dev_error_t;

/**
 * @brief NIC counter types
 */
typedef enum {
	DOCA_PCC_DEV_NIC_COUNTER_TYPE_TX_BYTES = 0x2, /**< TX bytes NIC counter type */
} doca_pcc_dev_nic_port_counter_types_t;

/**
 * @brief Generates a unique ID using the logical port number, counter type, and plane parameters for identifying
 * specific port counters
 */
#define DOCA_PCC_DEV_GET_PORT_COUNTER_ID(port, type, plane) \
	(((port & 0xF)) | ((type & 0xF) << 4) | ((plane & 0XF) << 8) | ((1) << 24))

/**
 * @brief Max number of NIC ports supported by the lib
 */
#define DOCA_PCC_DEV_MAX_NUM_PORTS (__NV_DPA >= __NV_DPA_CX8 ? 8 : 4)

/**
 * @brief Prepare a list of counters to read
 *
 * The list is stored in kernel memory. A single counters config per process is supported.
 *
 * @warning process crashes in case of:
 *      counters_ids too large
 *      bad pointers of values, counter_ids
 *      unknown counter
 *
 * @note
 * - arrays memory must be defined in global or heap memory only.
 * - it is recommended to configure counters in the port info change callback. @see
 * doca_pcc_dev_user_port_info_changed()
 *
 * @param[in]  counter_ids - An array of counter ids.
 * @param[in]  num_counters - number of counters in the counter_ids array
 * @param[out] values - buffer to store counters values (32b) read by doca_pcc_dev_nic_counters_sample()
 */
DOCA_EXPERIMENTAL
void doca_pcc_dev_nic_counters_config(uint32_t *counter_ids, uint32_t num_counters, uint32_t *values);

/**
 * @brief Sample counters according to the prior configuration call
 *
 * @warning process crashes in case of:
 *      doca_pcc_dev_nic_counters_config() never called
 *
 * Sample counter_ids, num_counters and values buffer provided in the last successful call to
 * doca_pcc_dev_nic_counters_config().
 * This call ensures fastest sampling on a pre-checked counter ids and buffers.
 */
DOCA_EXPERIMENTAL ALWAYS_INLINE void doca_pcc_dev_nic_counters_sample(void);

/**
 * @brief Get mask of initiated logical ports
 * @note - it is recommended to get logical ports mask in the port info change callback. @see
 * doca_pcc_dev_user_port_info_changed()
 *
 * @return - ports mask
 */
DOCA_EXPERIMENTAL
uint32_t doca_pcc_dev_get_logical_ports(void);

/**
 * @brief Get number of available planes per a given port
 * @note - it is recommended to get port planes in the port info change callback. @see
 * doca_pcc_dev_user_port_info_changed()
 *
 * @param[in] portid - port number
 *
 * @return - number of port planes
 */
DOCA_EXPERIMENTAL
uint32_t doca_pcc_dev_get_port_planes(uint32_t portid);

/**
 * @brief Get speed in Gbps units per a given port
 * @note - it is recommended to get port speed in the port info change callback. @see
 * doca_pcc_dev_user_port_info_changed()
 *
 * @param[in] portid - port number
 *
 * @return - port speed
 */
DOCA_EXPERIMENTAL
uint32_t doca_pcc_dev_get_port_speed(uint32_t portid);

/**
 * @brief User callback triggered on a port state change
 *
 * The user may implement this function to initiate port related operations, E.G counter sampling, or parameters changes
 * @note: Implementation of this function is optional.
 *
 * @param[in] portid - changed port id
 *
 */
DOCA_EXPERIMENTAL
void doca_pcc_dev_user_port_info_changed(uint32_t portid) __attribute__((weak));

#ifdef __cplusplus
}
#endif

#endif /* DOCA_PCC_DEV_COMMON_H_ */

/** @} */
