/*
 * Copyright (c) 2024-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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

#ifndef DOCA_STA_TASK_H_
#define DOCA_STA_TASK_H_

#include <stdint.h>

#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_task;
struct doca_sta;

struct doca_sta_producer_task_send;

/**
 * @brief Function to execute on STA task completion.
 *
 * @details This function is called by doca_pe_progress() when a related task is identified as completed successfully.
 * When this function is called, ownership of the task object is passed from DOCA back to the user.
 * Inside this callback, the user may decide what to do with the task object:
 * - Re-submit the task with doca_task_submit(); task object ownership is passed to DOCA
 * - Release the task with doca_task_free(); task object ownership is passed to DOCA
 * - Keep the task object for future reuse; user keeps ownership of the task object
 *
 * Inside this callback, the user should not call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * due to the mode of nested execution in doca_pe_progress(), this callback does not return an error.
 *
 * @note This callback type is utilized for both successful and failed task completions.
 *
 * @param [in] task
 * STA task that completed
 * @param [in] task_user_data
 * User data associated with the task
 */
typedef void (*doca_sta_task_completion_cb_t)(struct doca_sta_producer_task_send *task, union doca_data task_user_data);

/**
 * @brief Convert a producer send task to a doca_task.
 *
 * @param [in] task
 * The task to be converted
 *
 * @return
 * Non-NULL pointer to doca_task on success, NULL otherwise
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_sta_producer_send_task_as_task(struct doca_sta_producer_task_send *task);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_TASK_H_ */
