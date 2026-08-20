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

#ifndef DOCA_STA_IO_NON_OFFLOAD_H_
#define DOCA_STA_IO_NON_OFFLOAD_H_

#include <stdint.h>
#include <stdbool.h>

#include <doca_error.h>
#include <doca_types.h>

#include <doca_sta_handle.h>
#include <doca_sta_task.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_sta_io;

/**
 * @brief Function to execute on new STA IO non-offload command notification.
 *
 * @details This function is called by doca_pe_progress() when a related task receives a new non-offload command from
 * DPA.
 *
 * @param [in] qp_handle
 * STA QP handle that received the command
 * @param [in] user_data
 * User data associated with the QP
 * @param [in] nvme_cmd
 * NVMe command buffer
 * @param [in] payload
 * The payload data (if any) belonging to the NVMe command
 * @param [in] payload_len
 * The length of the payload. The maximum length can be obtained by calling doca_sta_get_max_io_size()
 * @param [in] payload_valid
 * Indicates whether the NVMe command has been retrieved from the initiator and stored in the payload buffer
 * @param [in] non_offload_user_data
 * User data attached to the NVMe command
 */
typedef void (*doca_sta_io_non_offload_cb_t)(struct doca_sta_qp_handle *qp_handle,
					     union doca_data user_data,
					     const uint8_t *nvme_cmd,
					     uint8_t *payload,
					     uint32_t payload_len,
					     bool payload_valid,
					     union doca_data non_offload_user_data);

/**
 * @brief Configure the non-offload callback.
 * The callback will be issued when a new NVMeoF capsule cannot be offloaded by the STA engine.
 *
 * @param [in] sta_io
 * Pointer to doca_sta_io instance
 * @param [in] non_offload_cb
 * Non-offload callback function
 * @param [in] user_data
 * User data to be passed to the callback
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_non_offload_register_cb(struct doca_sta_io *sta_io,
						 doca_sta_io_non_offload_cb_t non_offload_cb,
						 union doca_data user_data);

/**
 * @brief Set the STA IO non-offload RDMA WRITE tasks configuration.
 *
 * @param [in] sta_io
 * The STA IO context to configure
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
doca_error_t doca_sta_io_task_non_offload_set_rdma_write_send_conf(struct doca_sta_io *sta_io,
								   doca_sta_task_completion_cb_t task_completion_cb,
								   doca_sta_task_completion_cb_t task_error_cb);

/**
 * @brief Set the STA IO non-offload RDMA READ tasks configuration.
 *
 * @param [in] sta_io
 * The STA IO context to configure
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
doca_error_t doca_sta_io_task_non_offload_set_rdma_read_conf(struct doca_sta_io *sta_io,
							     doca_sta_task_completion_cb_t task_completion_cb,
							     doca_sta_task_completion_cb_t task_error_cb);

/**
 * @brief Allocate and initialize a STA IO RDMA WRITE with RDMA SEND task.
 *
 * @param [in] sta_io
 * The STA IO context to configure
 * @param [in] user_data
 * User data to attach to the task
 * @param [in] qp_handle
 * The handle of the QP to use for the task
 * @param [in] completion
 * NVMeF completion buffer
 * @param [in] non_offload_user_data
 * User data attached to the original NVMe command
 * @param [out] task
 * Pointer to a doca_sta_producer_task_send instance that will be populated with input parameters
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_task_non_offload_rdma_write_send_alloc_init(struct doca_sta_io *sta_io,
								     union doca_data user_data,
								     struct doca_sta_qp_handle *qp_handle,
								     doca_sta_nvme_completion_t completion,
								     union doca_data non_offload_user_data,
								     struct doca_sta_producer_task_send **task);

/**
 * @brief Allocate and initialize a STA IO RDMA SEND task.
 *
 * @param [in] sta_io
 * The STA IO context to configure
 * @param [in] user_data
 * User data to attach to the task
 * @param [in] qp_handle
 * The handle of the QP to use for the task
 * @param [in] completion
 * NVMeF completion buffer
 * @param [in] non_offload_user_data
 * User data attached to the original NVMe command
 * @param [out] task
 * Pointer to a doca_sta_producer_task_send instance that will be populated with input parameters
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_task_non_offload_rdma_send_alloc_init(struct doca_sta_io *sta_io,
							       union doca_data user_data,
							       struct doca_sta_qp_handle *qp_handle,
							       doca_sta_nvme_completion_t completion,
							       union doca_data non_offload_user_data,
							       struct doca_sta_producer_task_send **task);

/**
 * @brief Allocate and initialize a STA IO RDMA READ task.
 *
 * @param [in] sta_io
 * The STA IO context to configure
 * @param [in] user_data
 * User data to attach to the task
 * @param [in] qp_handle
 * The handle of the QP to use for the task
 * @param [in] non_offload_user_data
 * User data attached to the original NVMe command
 * @param [out] task
 * Pointer to a doca_sta_producer_task_send instance that will be populated with input parameters
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_task_non_offload_rdma_read_alloc_init(struct doca_sta_io *sta_io,
							       union doca_data user_data,
							       struct doca_sta_qp_handle *qp_handle,
							       union doca_data non_offload_user_data,
							       struct doca_sta_producer_task_send **task);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_IO_NON_OFFLOAD_H_ */
