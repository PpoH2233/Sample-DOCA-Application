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
 * @file doca_mgmt_cc_global_status.h
 * @page doca_mgmt_cc_global_status
 * @defgroup DOCA_MGMT_CC_GLOBAL_STATUS DOCA Management Congestion Control Global Status
 * DOCA Management - Congestion Control Global Status
 *
 * @{
 */
#ifndef DOCA_MGMT_CC_GLOBAL_STATUS_H_
#define DOCA_MGMT_CC_GLOBAL_STATUS_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Congestion control protocol type.
 */
enum doca_mgmt_cc_global_status_protocol {
	DOCA_MGMT_CC_GLOBAL_STATUS_PROTOCOL_RP,
	DOCA_MGMT_CC_GLOBAL_STATUS_PROTOCOL_NP,
};

/**
 * @brief Opaque structure representing a DOCA management cc global status handle.
 */
struct doca_mgmt_cc_global_status;

/**
 * @brief Create a DOCA management cc global status handle.
 *
 * @param [out] handle
 * Pointer to pointer to be set to point to the created DOCA management cc global status handle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_create(struct doca_mgmt_cc_global_status **handle);

/**
 * @brief Destroy a DOCA management cc global status handle.
 *
 * @param [in] handle
 * The DOCA management cc global status handle to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_destroy(struct doca_mgmt_cc_global_status *handle);

/**
 * @brief Set the protocol attribute of a DOCA management cc global status handle.
 * This attribute along with the priority attribute define the congestion control entity to operate on and must be set
 * before calling doca_mgmt_cc_global_status_set or doca_mgmt_cc_global_status_get.
 *
 * @param [in] handle
 * The DOCA management cc global status handle for which to set the protocol attribute.
 * @param [in] protocol
 * The value for the protocol attribute to set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - protocol parameter is invalid.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_set_protocol(struct doca_mgmt_cc_global_status *handle,
						     enum doca_mgmt_cc_global_status_protocol protocol);

/**
 * @brief Set the priority attribute of a DOCA management cc global status handle.
 * This attribute along with the protocol attribute define the congestion control entity to operate on and must be set
 * before calling doca_mgmt_cc_global_status_set or doca_mgmt_cc_global_status_get.
 *
 * @param [in] handle
 * The DOCA management cc global status handle for which to set the priority attribute.
 * @param [in] priority
 * The value for the priority attribute to set. Must be a value between 0 and 7.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - priority parameter is invalid.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_set_priority(struct doca_mgmt_cc_global_status *handle, uint8_t priority);

/**
 * @brief Set the enabled attribute of a DOCA management cc global status handle.
 *
 * @param [in] handle
 * The DOCA management cc global status handle for which to set the enabled attribute.
 * @param [in] enabled
 * 1 to enable congestion control, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_set_enabled(struct doca_mgmt_cc_global_status *handle, uint8_t enabled);

/**
 * @brief Get the protocol attribute of a DOCA management cc global status handle.
 *
 * @param [in] handle
 * The DOCA management cc global status handle for which to get the protocol attribute.
 * @param [out] protocol
 * Pointer to an enum doca_mgmt_cc_global_status_protocol to store the value of the protocol attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - protocol parameter is NULL.
 * - DOCA_ERROR_EMPTY - protocol attribute is not set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_get_protocol(const struct doca_mgmt_cc_global_status *handle,
						     enum doca_mgmt_cc_global_status_protocol *protocol);

/**
 * @brief Get the priority attribute of a DOCA management cc global status handle.
 *
 * @param [in] handle
 * The DOCA management cc global status handle for which to get the priority attribute.
 * @param [out] priority
 * Pointer to a uint8_t to store the value of the priority attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - priority parameter is NULL.
 * - DOCA_ERROR_EMPTY - priority attribute is not set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_get_priority(const struct doca_mgmt_cc_global_status *handle,
						     uint8_t *priority);

/**
 * @brief Get the enabled attribute of a DOCA management cc global status handle.
 *
 * @param [in] handle
 * The DOCA management cc global status handle for which to get the enabled attribute.
 * @param [out] enabled
 * Pointer to a uint8_t to store the value of the enabled attribute. 1 if congestion control is enabled, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - enabled parameter is NULL.
 * - DOCA_ERROR_EMPTY - enabled attribute is not set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_get_enabled(const struct doca_mgmt_cc_global_status *handle, uint8_t *enabled);

/**
 * @brief Clear all previously set attributes of a DOCA management cc global status handle.
 *
 * @param [in] handle
 * The DOCA management cc global status handle to clear.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_clear(struct doca_mgmt_cc_global_status *handle);

/**
 * @brief Set cc global status configuration for the given DOCA management device context.
 * Priority and protocol attributes of DOCA management cc global status handle define the congestion control entity to
 * operate on and must be set before calling this function.
 *
 * @param [in] ctx
 * The DOCA management device context for which to set the cc global status configuration.
 * @param [in] handle
 * The DOCA management cc global status handle with the configuration to set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - ctx parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_BAD_CONFIG - protocol, priority or enabled attributes were not set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_set(struct doca_mgmt_dev_ctx *ctx,
					    const struct doca_mgmt_cc_global_status *handle);

/**
 * @brief Get cc global status configuration for the given DOCA management device context.
 * Priority and protocol attributes of DOCA management cc global status handle define the congestion control entity to
 * operate on and must be set before calling this function.
 *
 * @param [in] ctx
 * The DOCA management device context for which to get the cc global status configuration.
 * @param [out] handle
 * The DOCA management cc global status handle to query the configuration into.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - ctx parameter is NULL.
 * - DOCA_ERROR_INVALID_VALUE - handle parameter is NULL.
 * - DOCA_ERROR_BAD_CONFIG - protocol or priority attributes were not set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_cc_global_status_get(struct doca_mgmt_dev_ctx *ctx, struct doca_mgmt_cc_global_status *handle);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_MGMT_CC_GLOBAL_STATUS_H_ */

/** @} */
