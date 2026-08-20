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

#ifndef DOCA_STA_BE_H_
#define DOCA_STA_BE_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_mmap.h>

#include <doca_sta_handle.h>
#include <doca_sta_task.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_sta;
struct doca_sta_eu_ctr_entry;

/**
 * @brief Create a backend (BE) controller resource.
 * This is an abstraction of the backend device (NVMe PCI disk).
 *
 * @param [in] sta
 * The STA context to configure
 * @param [out] be_handle
 * Pointer that will be set to the created doca_sta_be_handle instance
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_be_create(struct doca_sta *sta, struct doca_sta_be_handle **be_handle);

/**
 * @brief Destroy a backend (BE) controller resource.
 *
 * @param [in] be_handle
 * The doca_sta_be_handle instance to be destroyed
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_be_destroy(struct doca_sta_be_handle *be_handle);

/**
 * @brief Add a queue to the backend (BE) controller resource.
 *
 * @param [in] be_handle
 * doca_sta_be_handle instance
 * @param [in] sq
 * Memory map containing the submission queue buffer
 * @param [in] sq_db_reg
 * Memory map containing the submission queue doorbell buffer
 * @param [in] sq_db_offset
 * Offset from the sq_db_reg memory map start to set the doorbell buffer
 * @param [in] cq
 * Memory map containing the completion queue buffer
 * @param [in] cq_db_reg
 * Memory map containing the completion queue doorbell buffer
 * @param [in] cq_db_offset
 * Offset from the cq_db_reg memory map start to set the doorbell buffer
 * @param [out] be_q_handle
 * Pointer that will be set to the created doca_sta_be_q_handle instance
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_be_add_queue(struct doca_sta_be_handle *be_handle,
				   struct doca_mmap *sq,
				   struct doca_mmap *sq_db_reg,
				   uint16_t sq_db_offset,
				   struct doca_mmap *cq,
				   struct doca_mmap *cq_db_reg,
				   uint16_t cq_db_offset,
				   struct doca_sta_be_q_handle **be_q_handle);

/**
 * @brief Set the STA destroy queue tasks configuration.
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
doca_error_t doca_sta_be_task_destroy_queue_set_conf(struct doca_sta *sta,
						     doca_sta_task_completion_cb_t task_completion_cb,
						     doca_sta_task_completion_cb_t task_error_cb);

/**
 * @brief Allocate and initialize a STA destroy queue task.
 *
 * @param [in] be_q_handle
 * The handle of the BE queue to be destroyed
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
doca_error_t doca_sta_be_destroy_queue_task_alloc_init(struct doca_sta_be_q_handle *be_q_handle,
						       union doca_data user_data,
						       struct doca_sta_producer_task_send **task);

/**
 * @brief Get the mapping information between backend queue and backend handler queue.
 * This function retrieves the mapping entries that define the relationship between
 * backend queue and their corresponding backend handler queue.
 *
 * @param [in] be_q_handle
 * The backend queue handle to get mapping information from
 * @param [out] entries
 * Pointer to a doca_sta_eu_ctr_entry array that will be populated with mapping entries
 * @param [out] num_entries
 * The number of valid entries in the entries array
 *
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_be_queue_mapping_info(const struct doca_sta_be_q_handle *be_q_handle,
						const struct doca_sta_eu_ctr_entry **entries,
						uint16_t *num_entries);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_BE_H_ */
