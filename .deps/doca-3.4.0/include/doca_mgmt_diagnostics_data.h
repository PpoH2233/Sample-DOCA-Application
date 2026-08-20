/*
 * Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_mgmt_diagnostics_data.h
 * @page doca_mgmt_diagnostics_data
 * @defgroup DOCA_MGMT_DIAGNOSTICS_DATA DOCA Management Diagnostics Data
 * DOCA Management - Diagnostics Data
 *
 * @{
 */
#ifndef DOCA_MGMT_DIAGNOSTICS_DATA_H_
#define DOCA_MGMT_DIAGNOSTICS_DATA_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing a DOCA management diagnostics data handle.
 */
struct doca_mgmt_diagnostics_data;

/**
 * @brief Check if the given DOCA management device context supports diagnostics data multi-domain.
 * @param[in] dev_ctx
 * The DOCA management device context to check.
 * @return
 * DOCA_SUCCESS - if diagnostics data multi-domain is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - dev_ctx parameter is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - the device does not support diagnostics data.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cap_diagnostics_data_multi_domain_is_supported(struct doca_mgmt_dev_ctx *dev_ctx);

/**
 * @brief Create a DOCA management diagnostics data handle.
 *
 * @param [out] handle
 * Pointer to pointer to be set to point to the created DOCA management diagnostics data handle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_diagnostics_data_create(struct doca_mgmt_diagnostics_data **handle);

/**
 * @brief Destroy a DOCA management diagnostics data handle.
 *
 * @param [in] handle
 * The DOCA management diagnostics data handle to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_diagnostics_data_destroy(struct doca_mgmt_diagnostics_data *handle);

/**
 * @brief Set the multi-domain attribute of a DOCA management diagnostics data handle.
 *
 * @param [in] handle
 * The DOCA management diagnostics data handle.
 * @param [in] multi_domain
 * 1 to enable multi-domain; 0 to disable.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_diagnostics_data_set_multi_domain(struct doca_mgmt_diagnostics_data *handle,
							 uint8_t multi_domain);

/**
 * @brief Get the multi-domain value of a DOCA management diagnostics data handle.
 *
 * @param [in] handle
 * The DOCA management diagnostics data handle.
 * @param [out] multi_domain
 * Pointer to uint8_t to store the multi-domain value- 1 if multi-domain is enabled; 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle or multi_domain parameter is NULL.
 * - DOCA_ERROR_EMPTY - multi_domain was not set or queried on the handle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_diagnostics_data_get_multi_domain(const struct doca_mgmt_diagnostics_data *handle,
							 uint8_t *multi_domain);

/**
 * @brief Clear all previously set attributes of a DOCA management diagnostics data handle.
 *
 * @param [in] handle
 * The DOCA management diagnostics data handle to clear.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_diagnostics_data_clear(struct doca_mgmt_diagnostics_data *handle);

/**
 * @brief Modify diagnostics data configuration with the given handle.
 *
 * @param [in] dev_ctx
 * The DOCA management device context.
 * @param [in] handle
 * The handle with the configuration to modify.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - dev_ctx or handle is NULL.
 * - DOCA_ERROR_BAD_CONFIG - No attribute was set on the handle.
 * - DOCA_ERROR_NOT_PERMITTED - One or more diagnostics data domains have ownership.
 * - DOCA_ERROR_AGAIN - One or more diagnostics data domains have been given ownership during the modify operation.
 * - DOCA_ERROR_IO_FAILED - configuration command failed on the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_diagnostics_data_modify_for_dev(struct doca_mgmt_dev_ctx *dev_ctx,
						       const struct doca_mgmt_diagnostics_data *handle);

/**
 * @brief Query diagnostics data configuration to the given handle.
 *
 * @param [in] dev_ctx
 * The DOCA management device context.
 * @param [out] handle
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - dev_ctx or handle is NULL.
 * - DOCA_ERROR_IO_FAILED - configuration command failed on the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_diagnostics_data_query_for_dev(struct doca_mgmt_dev_ctx *dev_ctx,
						      struct doca_mgmt_diagnostics_data *handle);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_MGMT_DIAGNOSTICS_DATA_H_ */

/** @} */
