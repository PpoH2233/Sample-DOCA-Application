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
 * @file doca_mgmt_device_caps_general.h
 * @page doca_mgmt_device_caps_general
 * @defgroup DOCA_MGMT_DEVICE_CAPS_GENERAL DOCA Management Device Capabilities General
 * DOCA Management - Device Capabilities General
 *
 * @{
 */
#ifndef DOCA_MGMT_DEVICE_CAPS_GENERAL_H_
#define DOCA_MGMT_DEVICE_CAPS_GENERAL_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing a DOCA management device caps general handle.
 */
struct doca_mgmt_device_caps_general;

/**
 * @brief Create a DOCA management device caps general handle.
 *
 * @param [out] handle
 * Pointer to pointer to be set to point to the created DOCA management device caps general handle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_device_caps_general_create(struct doca_mgmt_device_caps_general **handle);

/**
 * @brief Destroy a DOCA management device caps general handle.
 *
 * @param [in] handle
 * The DOCA management device caps general handle to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_device_caps_general_destroy(struct doca_mgmt_device_caps_general *handle);

/**
 * @brief Set the data direct attribute of a DOCA management device caps general handle.
 *
 * @param [in] handle
 * The DOCA management device caps general handle for which to set the data direct attribute.
 * @param [in] data_direct
 * 1 to enable data direct, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_device_caps_general_set_data_direct(struct doca_mgmt_device_caps_general *handle,
							   uint8_t data_direct);

/**
 * @brief Get the data direct attribute of a DOCA management device caps general handle.
 *
 * @param [in] handle
 * The DOCA management device caps general handle for which to get the data direct attribute.
 * @param [out] data_direct
 * Pointer to a uint8_t to store the value of the data direct attribute. 1 if data direct is enabled, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - data_direct parameter is NULL.
 * - DOCA_ERROR_EMPTY - data direct attribute is not set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_device_caps_general_get_data_direct(const struct doca_mgmt_device_caps_general *handle,
							   uint8_t *data_direct);

/**
 * @brief Clear all previously set attributes of a DOCA management device caps general handle.
 *
 * @param [in] handle
 * The DOCA management device caps general handle to clear.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_device_caps_general_clear(struct doca_mgmt_device_caps_general *handle);

/**
 * @brief Set device caps general configuration for the given DOCA management device representor context.
 *
 * @param [in] ctx
 * The DOCA management device representor context for which to set the device caps general configuration.
 * @param [in] handle
 * The DOCA management device caps general handle with the configuration to set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - ctx parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_BAD_CONFIG - data direct attribute was not set.
 * - DOCA_ERROR_NOT_SUPPORTED - setting device representor configuration is not supported.
 * - DOCA_ERROR_IN_USE - the device representor is already initialized.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_device_caps_general_set(struct doca_mgmt_dev_rep_ctx *ctx,
					       const struct doca_mgmt_device_caps_general *handle);

/**
 * @brief Get device caps general configuration for the given DOCA management device representor context.
 *
 * @param [in] ctx
 * The DOCA management device representor context for which to get the device caps general configuration.
 * @param [out] handle
 * The DOCA management device caps general handle to query the configuration into.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - ctx parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - getting device representor configuration is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_device_caps_general_get(struct doca_mgmt_dev_rep_ctx *ctx,
					       struct doca_mgmt_device_caps_general *handle);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_MGMT_DEVICE_CAPS_GENERAL_H_ */

/** @} */
