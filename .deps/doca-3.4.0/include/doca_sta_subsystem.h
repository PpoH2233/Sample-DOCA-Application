/*
 * Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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

#ifndef DOCA_STA_SUBSYSTEM_H_
#define DOCA_STA_SUBSYSTEM_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>

#include <doca_sta_handle.h>
#include <doca_sta_task.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_sta;
struct doca_dev;

/**
 * @brief Create a subsystem resource.
 * This is an abstraction of the NVMe subsystem.
 *
 * @param [in] sta
 * The STA context to configure
 * @param [in] nqn
 * The NVMe Qualified Name (NQN)
 * @param [out] subs_handle
 * Pointer that will be set to the created doca_sta_subs_handle instance
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_subsystem_create(struct doca_sta *sta,
				       const char *nqn,
				       struct doca_sta_subs_handle **subs_handle);

/**
 * @brief Add a network device to the subsystem.
 * This couples the subsystem with a network device.
 * The subsystem may be bound with several network devices, for example mlx5_2 and mlx5_3.
 *
 * @param [in] subs_handle
 * doca_sta_subs_handle instance
 * @param [in] dev
 * Device to use in the subsystem
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_subsystem_add_dev(struct doca_sta_subs_handle *subs_handle, const struct doca_dev *dev);

/**
 * @brief Set the STA remove namespace tasks configuration.
 *
 * @param [in] sta
 * The STA context to configure
 * @param [in] task_completion_cb
 * Callback function for task completion
 * @param [in] task_error_cb
 * Callback function for task errors
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_subsystem_task_rm_ns_set_conf(struct doca_sta *sta,
						    doca_sta_task_completion_cb_t task_completion_cb,
						    doca_sta_task_completion_cb_t task_error_cb);

/**
 * @brief Destroy a subsystem resource.
 * Should be called when the subsystem has no attached namespaces.
 *
 * @param [in] subs_handle
 * The doca_sta_subs_handle instance to be destroyed
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_subsystem_destroy(struct doca_sta_subs_handle *subs_handle);

/**
 * @brief Add a namespace to the subsystem.
 *
 * @param [in] subs_handle
 * doca_sta_subs_handle instance to add the namespace to
 * @param [in] fe_ns_id
 * The frontend namespace ID (from the NVMeoF capsule)
 * @param [in] ns_block_size
 * Namespace block size in bytes
 * @param [in] be_ns_id
 * The backend namespace ID (NVMe PCI command)
 * @param [in] be_handle
 * The doca_sta_be_handle instance (NVMe PCI disk) to add the namespace to
 * @param [out] ns_handle
 * Pointer that will be set to the created doca_sta_ns_handle instance
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_subsystem_add_ns(struct doca_sta_subs_handle *subs_handle,
				       uint32_t fe_ns_id,
				       uint32_t ns_block_size,
				       uint32_t be_ns_id,
				       struct doca_sta_be_handle *be_handle,
				       struct doca_sta_ns_handle **ns_handle);

/**
 * @brief Remove a namespace from the subsystem.
 *
 * @param [in] subs_handle
 * The doca_sta_subs_handle instance to remove the namespace from
 * @param [in] ns_handle
 * The doca_sta_ns_handle instance to remove
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_subsystem_rm_ns(struct doca_sta_subs_handle *subs_handle, struct doca_sta_ns_handle *ns_handle);

/**
 * @brief Allocate and initialize a STA remove namespace task.
 *
 * @param [in] subs_handle
 * The subsystem handle
 * @param [in] ns_handle
 * The handle of the namespace to be removed
 * @param [in] user_data
 * User data to attach to the task
 * @param [out] task
 * Pointer that will be set to a doca_sta_producer_task_send instance populated with input parameters
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_subsystem_task_rm_ns_alloc_init(struct doca_sta_subs_handle *subs_handle,
						      struct doca_sta_ns_handle *ns_handle,
						      union doca_data user_data,
						      struct doca_sta_producer_task_send **task);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_SUBSYSTEM_H_ */
