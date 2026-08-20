/*
 * Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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

#ifndef DOCA_STA_H_
#define DOCA_STA_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_dpa.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_devinfo;
struct doca_dev;
struct doca_pe;
struct doca_sta;

/**
 * @brief Check if the DOCA device supports storage target acceleration (STA).
 *
 * @param [in] devinfo
 * The device to query
 *
 * @return
 * DOCA_SUCCESS - if the DOCA device has STA support
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_cap_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Create a DOCA STA context.
 *
 * This function creates a DOCA STA context given a DOCA device.
 * The context represents a program on the STA that is referenced
 * by the host process that called the context creation API.
 *
 * @param [in] dev
 * DOCA device
 * @param [out] sta
 * Pointer that will be set to the created doca_sta instance
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_create(struct doca_dev *dev, struct doca_sta **sta);

/**
 * @brief Destroy a DOCA STA context.
 *
 * This function destroys an STA context created by doca_sta_create().
 *
 * @param [in] sta
 * Previously created STA context
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_destroy(struct doca_sta *sta);

/**
 * @brief Add support for the STA functionality on a given device.
 *
 * @param [in] sta
 * Previously created STA context
 * @param [in] dev
 * DOCA device instance with appropriate capability
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_add_dev(struct doca_sta *sta, struct doca_dev *dev);

/**
 * @brief Set the maximum number of STA IO contexts (IO threads).
 *
 * @param [in] sta
 * Previously created STA context
 * @param [in] max_io_num
 * The maximum number of STA IO contexts
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_set_max_sta_io(struct doca_sta *sta, uint16_t max_io_num);

/**
 * @brief Get the maximum number of STA IO contexts (IO threads).
 *
 * @param [in] sta
 * Previously created STA context
 * @param [out] max_io_num
 * Pointer that will be set to the maximum number of STA IO contexts
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_sta_io(const struct doca_sta *sta, uint16_t *max_io_num);

/**
 * @brief Convert a doca_sta instance into a generalized context for use with doca core objects.
 *
 * @param [in] sta
 * Pointer to doca_sta instance
 *
 * @return
 * Non-NULL pointer to doca_ctx on success, NULL otherwise
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_sta_as_ctx(struct doca_sta *sta);

/**
 * @brief Get the maximum number of devices supported by the STA engine.
 *
 * @param [out] max_devs
 * Pointer that will be set to the maximum number of devices
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_devs(uint32_t *max_devs);

/**
 * @brief Retrieve the total number of execution units (EUs) available to the application.
 *
 * @param [in] sta
 * Previously created STA context
 * @param [out] max_eus
 * Pointer that will be set to the maximum number of available EUs
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_num_eus_available(const struct doca_sta *sta, uint32_t *max_eus);

/**
 * @brief Retrieve the maximum number of connected queue pairs (QPs) per execution unit (EU).
 *
 * @param [in] sta
 * Previously created STA context
 * @param [out] max_connected_qps
 * Pointer that will be set to the maximum number of connected QPs
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_num_connected_qp_per_eu(const struct doca_sta *sta, uint32_t *max_connected_qps);

/**
 * @brief Retrieve the maximum number of subsystems.
 *
 * @param [out] max_subsys
 * Pointer that will be set to the maximum number of subsystems
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_subsys(uint32_t *max_subsys);

/**
 * @brief Retrieve the maximum number of namespaces per subsystem.
 *
 * @param [out] max_ns_per_subsys
 * Pointer that will be set to the maximum number of namespaces per subsystem
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_ns_per_subs(uint32_t *max_ns_per_subsys);

/**
 * @brief Retrieve the maximum number of queue pairs (connections) supported by the STA library.
 *
 * @param [out] max_qps
 * Pointer that will be set to the maximum number of queue pairs
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_qps(uint32_t *max_qps);

/**
 * @brief Retrieve the maximum number of STA IO contexts (IO threads) supported by the STA library.
 *
 * @param [out] max_io_threads
 * Pointer that will be set to the maximum number of IO threads
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_io_threads(uint32_t *max_io_threads);

/**
 * @brief Retrieve the maximum NVMeoF IO size supported by the STA library.
 *
 * @param [out] max_io_size
 * Pointer that will be set to the maximum IO size
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_io_size(uint32_t *max_io_size);

/**
 * @brief Retrieve the maximum number of commands that can be handled by a single execution unit (EU).
 *
 * @param [in] sta
 * Previously created STA context
 * @param [out] max_ios
 * Pointer that will be set to the maximum number of commands
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_io_num_per_dev(const struct doca_sta *sta, uint32_t *max_ios);

/**
 * @brief Retrieve the maximum depth of the NVMeoF queue pair supported by the STA library.
 *
 * @param [in] sta
 * Previously created STA context
 * @param [out] max_io_queue_size
 * Pointer that will be set to the maximum queue pair depth
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_io_queue_size(const struct doca_sta *sta, uint32_t *max_io_queue_size);

/**
 * @brief Retrieve the I/O queue command capsule minimum supported size.
 *
 * @param [out] ioccsz
 * Pointer that will be set to the IO command capsule minimum size (in bytes).
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_min_ioccsz(uint32_t *ioccsz);

/**
 * @brief Retrieve the I/O queue command capsule maximum supported size.
 *
 * @param [out] ioccsz
 * Pointer that will be set to the IO command capsule maximum size (in bytes).
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_ioccsz(uint32_t *ioccsz);

/**
 * @brief Retrieve the I/O queue response capsule minimum supported size.
 *
 * @param [out] iorcsz
 * Pointer that will be set to the IO response capsule minimum size
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_min_iorcsz(uint32_t *iorcsz);

/**
 * @brief Retrieve the I/O queue response capsule maximum supported size.
 *
 * @param [out] iorcsz
 * Pointer that will be set to the IO response capsule maximum size
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_iorcsz(uint32_t *iorcsz);

/**
 * @brief Retrieve the offset where data starts within a capsule.
 *
 * @param [out] icdoff
 * Pointer that will be set to the in-capsule data offset
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_icdoff(uint32_t *icdoff);

/**
 * @brief Retrieve the maximum number of backends (NVMe disks) supported by the STA library.
 *
 * @param [out] max_be
 * Pointer that will be set to the maximum number of backends
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_be(uint32_t *max_be);

/**
 * @brief Retrieve the maximum number of queues that can be added to a backend.
 *
 * @param [out] max_qs_per_be
 * Pointer that will be set to the maximum number of queues per backend
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_max_qs_per_be(uint32_t *max_qs_per_be);

/**
 * @brief Check if the specified block size is supported.
 * The block size parameter is used for doca_sta_subsystem_add_ns() API.
 *
 * @param [in] block_size
 * The block size to check
 *
 * @return
 * true - if the block size is supported
 * false - if the block size is not supported
 */
DOCA_EXPERIMENTAL
bool doca_sta_is_logical_block_size_supported(uint32_t block_size);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_H_ */
