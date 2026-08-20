/*
 * Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_mgmt_icm_quota.h
 * @page doca_mgmt_icm_quota
 * @defgroup DOCA_MGMT_ICM_QUOTA DOCA Management ICM Quota
 * DOCA Management - ICM Quota
 *
 * @{
 */
#ifndef DOCA_MGMT_ICM_QUOTA_H_
#define DOCA_MGMT_ICM_QUOTA_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Indicates that ICM quota limit is unlimited.
 */
#define DOCA_MGMT_ICM_QUOTA_LIMIT_UNLIMITED UINT32_MAX

/**
 * @brief Opaque structure representing a DOCA management ICM quota handle.
 */
struct doca_mgmt_icm_quota;

/**
 * @brief Check if the given DOCA management device context supports ICM quota.
 * Support for the given DOCA management device context also indicates that ICM quota is supported for its related DOCA
 * management device representor contexts.
 *
 * @param [in] dev_ctx
 * The DOCA management device context to check.
 *
 * @return
 * DOCA_SUCCESS - in case DOCA management device context supports ICM quota.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - dev_ctx parameter is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - dev_ctx does not support ICM quota.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cap_icm_quota_is_supported(struct doca_mgmt_dev_ctx *dev_ctx);

/**
 * @brief Get the maximum ICM quota limit that can be set by doca_mgmt_icm_quota_set_limit for the given DOCA management
 * device context. The maximum ICM quota limit for the given DOCA management device context also applies for its related
 * DOCA management device representor contexts.
 *
 * @param [in] dev_ctx
 * The DOCA management device context to get the maximum ICM quota limit for.
 * @param [out] max_limit
 * Pointer to a uint32_t to store the maximum ICM quota limit in granularity of 4KB. Value of
 * DOCA_MGMT_ICM_QUOTA_LIMIT_UNLIMITED indicates no limit.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - dev_ctx parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - max_limit parameter is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - dev_ctx does not support ICM quota.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cap_icm_quota_get_max_limit(struct doca_mgmt_dev_ctx *dev_ctx, uint32_t *max_limit);

/**
 * @brief Create a DOCA management ICM quota handle for the given DOCA management device context.
 *
 * @param [in] dev_ctx
 * The DOCA management device context to be associated with the ICM quota handle.
 * @param [out] handle
 * Pointer to pointer to be set to point to the created DOCA management ICM quota handle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - dev_ctx parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_create_for_dev(struct doca_mgmt_dev_ctx *dev_ctx, struct doca_mgmt_icm_quota **handle);

/**
 * @brief Create a DOCA management ICM quota handle for the given DOCA management device representor context.
 *
 * @param [in] dev_rep_ctx
 * The DOCA management device representor context to be associated with the ICM quota handle.
 * @param [out] handle
 * Pointer to pointer to be set to point to the created DOCA management ICM quota handle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - dev_rep_ctx parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_create_for_dev_rep(struct doca_mgmt_dev_rep_ctx *dev_rep_ctx,
						    struct doca_mgmt_icm_quota **handle);

/**
 * @brief Destroy a DOCA management ICM quota handle.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_destroy(struct doca_mgmt_icm_quota *handle);

/**
 * @brief Get the DOCA management device context associated with a DOCA management ICM quota handle.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle to get its associated DOCA management device context.
 * @param [out] dev_ctx
 * Pointer to pointer to a doca_mgmt_dev_ctx to store the associated DOCA management device context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - dev_ctx parameter is NULL.
 * - DOCA_ERROR_EMPTY - no associated DOCA management device context was found.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_get_dev(const struct doca_mgmt_icm_quota *handle, struct doca_mgmt_dev_ctx **dev_ctx);

/**
 * @brief Get the DOCA management device representor context associated with a DOCA management ICM quota handle.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle to get its associated DOCA management device representor context.
 * @param [out] dev_rep_ctx
 * Pointer to pointer to a doca_mgmt_dev_rep_ctx to store the associated DOCA management device representor context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - dev_rep_ctx parameter is NULL.
 * - DOCA_ERROR_EMPTY - no associated DOCA management device representor context was found.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_get_dev_rep(const struct doca_mgmt_icm_quota *handle,
					     struct doca_mgmt_dev_rep_ctx **dev_rep_ctx);

/**
 * @brief Set the limit attribute of a DOCA management ICM quota handle.
 * This attribute represents the ICM quota limit that can be allocated for the device.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle for which to set the limit attribute.
 * @param [in] limit
 * The value for the limit attribute to set. The value is in granularity of 4KB and must be equal or smaller than the
 * value reported in doca_mgmt_cap_icm_quota_get_max_limit. Value of DOCA_MGMT_ICM_QUOTA_LIMIT_UNLIMITED indicates no
 * limit.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - limit parameter is invalid.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_set_limit(struct doca_mgmt_icm_quota *handle, uint32_t limit);

/**
 * @brief Set the reset max reached attribute of a DOCA management ICM quota handle.
 * If set, the maximum ICM quota allocation that has been reached for the device will be reset.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle for which to set the reset max reached attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_set_reset_max_reached(struct doca_mgmt_icm_quota *handle);

/**
 * @brief Clear the reset max reached attribute of a DOCA management ICM quota handle.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle for which to clear the reset max reached attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_clear_reset_max_reached(struct doca_mgmt_icm_quota *handle);

/**
 * @brief Get the limit attribute of a DOCA management ICM quota handle.
 * This attribute represents the ICM quota limit that can be allocated for the device.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle for which to get the limit attribute.
 * @param [out] limit
 * Pointer to a uint32_t to store the value of the limit attribute in granularity of 4KB. Value of
 * DOCA_MGMT_ICM_QUOTA_LIMIT_UNLIMITED indicates no limit.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - limit parameter is NULL.
 * - DOCA_ERROR_EMPTY - limit attribute is not set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_get_limit(const struct doca_mgmt_icm_quota *handle, uint32_t *limit);

/**
 * @brief Get the current allocation attribute of a DOCA management ICM quota handle.
 * This attribute represents the ICM quota that is currently allocated for the device.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle for which to get the current allocation attribute.
 * @param [out] current_allocation
 * Pointer to a uint32_t to store the value of the current allocation attribute in granularity of 4KB.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - current_allocation parameter is NULL.
 * - DOCA_ERROR_EMPTY - current allocation attribute is not set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_get_current_allocation(const struct doca_mgmt_icm_quota *handle,
							uint32_t *current_allocation);

/**
 * @brief Get the max reached attribute of a DOCA management ICM quota handle.
 * This attribute represents the maximum ICM quota allocation that has been reached for the device.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle for which to get the max reached attribute.
 * @param [out] max_reached
 * Pointer to a uint32_t to store the value of the max reached attribute in granularity of 4KB.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - max_reached parameter is NULL.
 * - DOCA_ERROR_EMPTY - max reached attribute is not set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_get_max_reached(const struct doca_mgmt_icm_quota *handle, uint32_t *max_reached);

/**
 * @brief Clear all previously set attributes of a DOCA management ICM quota handle.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle to clear.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_clear(struct doca_mgmt_icm_quota *handle);

/**
 * @brief Modify ICM quota configuration with the given ICM quota handle.
 * Modify the ICM quota configuration of the device that is associated with the ICM quota handle with the attributes
 * that were set in the ICM quota handle.
 *
 * @param [in] handle
 * The DOCA management ICM quota handle with the configuration to modify.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_BAD_CONFIG - no attribute was set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_modify(const struct doca_mgmt_icm_quota *handle);

/**
 * @brief Query ICM quota configuration to the given ICM quota handle.
 * Query the ICM quota configuration of the device that is associated with the ICM quota handle.
 *
 * @param [out] handle
 * The DOCA management ICM quota handle to query the configuration to.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_icm_quota_query(struct doca_mgmt_icm_quota *handle);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_MGMT_ICM_H_ */

/** @} */
