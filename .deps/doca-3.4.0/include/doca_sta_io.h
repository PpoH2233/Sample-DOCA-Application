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

#ifndef DOCA_STA_IO_H_
#define DOCA_STA_IO_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_types.h>

#include <doca_sta_handle.h>
#include <doca_sta_task.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_pe;
struct doca_dev;

struct doca_sta;
struct doca_sta_io;

/**
 * @brief Create a DOCA STA IO context.
 *
 * Create a DOCA STA IO context attached to a DOCA STA context (control context).
 * The IO context allows the user to perform IO transactions using the offload engine.
 *
 * @param [in] sta
 * Previously created STA context
 * @param [out] sta_io
 * Pointer that will be set to the created IO context
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_create(struct doca_sta *sta, struct doca_sta_io **sta_io);

/**
 * @brief Destroy a DOCA STA IO context.
 *
 * @param [in] sta_io
 * Previously created STA IO context
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_destroy(struct doca_sta_io *sta_io);

/**
 * @brief Convert a doca_sta_io instance into a generalized context for use with doca core objects.
 *
 * @param [in] sta_io
 * Pointer to doca_sta_io instance
 *
 * @return
 * Non-NULL pointer to doca_ctx on success, NULL otherwise
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_sta_io_as_ctx(struct doca_sta_io *sta_io);

/**
 * @brief Set the STA IO disconnect tasks configuration.
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
doca_error_t doca_sta_io_task_disconnect_set_conf(struct doca_sta_io *sta_io,
						  doca_sta_task_completion_cb_t task_completion_cb,
						  doca_sta_task_completion_cb_t task_error_cb);

/**
 * @brief Allocate and initialize a STA IO disconnect task.
 *
 * @param [in] sta_io
 * The STA IO context to configure
 * @param [in] user_data
 * User data to attach to the task
 * @param [in] qp_handle
 * The handle of the QP to be disconnected
 * @param [out] task
 * Pointer that will be set to the allocated task
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_task_disconnect_alloc_init(struct doca_sta_io *sta_io,
						    union doca_data user_data,
						    struct doca_sta_qp_handle *qp_handle,
						    struct doca_sta_producer_task_send **task);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_IO_H_ */
