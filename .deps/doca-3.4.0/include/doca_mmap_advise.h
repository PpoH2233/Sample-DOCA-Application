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

/**
 * @file doca_mmap_advise.h
 * @page doca_mmap_advise
 * @defgroup DOCA_MMAP_ADVISE DOCA MMAP advise
 * @ingroup DOCACore
 * DOCA MMAP advise
 * DOCA MMAP advise is a context that facilitates invalidating cache.
 * @{
 */

#ifndef DOCA_MMAP_ADVISE_H_
#define DOCA_MMAP_ADVISE_H_

#include <stddef.h>
#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 * opaque types
 *********************************************************************************************************************/
struct doca_ctx;
struct doca_dev;
struct doca_devinfo;
struct doca_mmap_advise;
struct doca_buf;

/**
 * Forward declaration for MMAP advise cache invalidate task.
 *
 * This task is used to invalidate the cache of a doca_buf
 * @see Task APIs below for usage.
 */
struct doca_mmap_advise_task_invalidate_cache;

/**
 * @brief Invalidate cache task completion callback.
 *
 * @param [in] task
 * The successfully completed invalidate cache task.
 * The implementation can assume task is not NULL.
 * @param [in] task_user_data
 * Task's user data which was previously set.
 * @param [in] ctx_user_data
 * Context's user data which was previously set.
 */
typedef void (*doca_mmap_advise_task_invalidate_cache_completion_cb_t)(
	struct doca_mmap_advise_task_invalidate_cache *task,
	union doca_data task_user_data,
	union doca_data ctx_user_data);

/**
 * @brief Create an mmap advise instance.
 *
 * @param [in] dev
 * doca_dev to create the mmap_advise on
 * @param [out] mmap_advise
 * The created doca_mmap_advise instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - mmap_advise argument is a NULL pointer.
 * - DOCA_ERROR_NO_MEMORY - failed to alloc doca_mmap_advise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mmap_advise_create(struct doca_dev *dev, struct doca_mmap_advise **mmap_advise);

/**
 * @brief Destroy an mmap advise instance.
 *
 * @param [in] mmap_advise
 * doca_mmap_advise to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - mmap_advise argument is a NULL pointer.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mmap_advise_destroy(struct doca_mmap_advise *mmap_advise);

/**
 * @brief Convert a MMAP advise to a DOCA context.
 *
 * @param [in] mmap_advise
 * The doca_mmap_advise to be converted
 *
 * @return
 * The matching doca_ctx instance in case of success,
 * NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_mmap_advise_as_ctx(struct doca_mmap_advise *mmap_advise);

/**
 * @brief Check if a given device supports submitting a DOCA MMAP advise cache invalidate task.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports submitting a cache invalidate task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support submitting a cache invalidate task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mmap_advise_cap_task_cache_invalidate_is_supported(const struct doca_devinfo *devinfo);

/**
 * Get the maximum supported buffer size for cache invalidate.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @param [out] buf_size
 * The maximum supported buffer size in bytes.
 *
 * @return
 * DOCA_SUCCESS - upon success
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case of invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mmap_advise_task_cache_invalidate_get_max_buf_size(const struct doca_devinfo *devinfo,
								     uint64_t *buf_size);

/**
 * @brief Set the DOCA MMAP advise cache invalidate task configuration.
 *
 * @param [in] mmap_advise
 * The associated mmap advise.
 * @param [in] completion_cb
 * The cache invalidate task completion callback.
 * @param [in] error_cb
 * The cache invalidate task error callback.
 * @param [in] num_tasks
 * Number of cache invalidate tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received null parameter.
 * - DOCA_ERROR_NOT_PERMITTED - context not in idle state.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mmap_advise_task_invalidate_cache_set_conf(
	struct doca_mmap_advise *mmap_advise,
	doca_mmap_advise_task_invalidate_cache_completion_cb_t completion_cb,
	doca_mmap_advise_task_invalidate_cache_completion_cb_t error_cb,
	uint32_t num_tasks);

/**
 * @brief Allocate a DOCA MMAP advise cache invalidate task.
 *
 * @param [in] mmap_advise
 * The associated mmap advise.
 * @param [in] buf
 * A pointer to the doca_buf to invalidate.
 * @param [in] user_data
 * doca_data to attach the task, which is later passed to the task's completion CBs.
 * @param [out] task
 * The allocated cache invalidate task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mmap_advise_task_invalidate_cache_alloc_init(struct doca_mmap_advise *mmap_advise,
							       struct doca_buf *buf,
							       union doca_data user_data,
							       struct doca_mmap_advise_task_invalidate_cache **task);

/**
 * @brief Convert a DOCA MMAP advise invalidate cache task to a DOCA Task.
 *
 * @param [in] task
 * The doca task invalidate cache task.
 *
 * @return
 * The matching doca task in case of success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_mmap_advise_task_invalidate_cache_as_doca_task(
	struct doca_mmap_advise_task_invalidate_cache *task);

/**
 * @brief Set the doca_buf pointer of a DOCA MMAP advise cache invalidate task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] buf
 * A pointer to the doca_buf to invalidate.
 */
DOCA_EXPERIMENTAL
void doca_mmap_advise_task_invalidate_cache_set_buf(struct doca_mmap_advise_task_invalidate_cache *task,
						    struct doca_buf *buf);

/**
 * @brief Get the doca_buf pointer of a DOCA MMAP advise cache invalidate task.
 *
 * @param [in] task
 * The task to get its buf pointer.
 *
 * @return
 * The task's buf pointer.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_mmap_advise_task_invalidate_cache_get_buf(
	const struct doca_mmap_advise_task_invalidate_cache *task);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_MMAP_ADVISE_H_ */
