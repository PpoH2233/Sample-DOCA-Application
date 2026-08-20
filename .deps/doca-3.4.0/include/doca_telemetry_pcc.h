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

/**
 * @file doca_telemetry_pcc.h
 * @page DOCA_TELEMETRY_PCC
 * @defgroup DOCA_TELEMETRY_PCC DOCA Telemetry PCC
 * DOCA Telemetry Programmable Congestion Control library. For more details please refer to the user guide on DOCA
 * devzone.
 *
 * @{
 */
#ifndef DOCA_TELEMETRY_PCC_H_
#define DOCA_TELEMETRY_PCC_H_

#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 * DOCA core opaque types
 *********************************************************************************************************************/
struct doca_dev;
struct doca_devinfo;
struct doca_dev_rep;
struct doca_devinfo_rep;

/*********************************************************************************************************************
 * DOCA Telemetry PCC Context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA Telemetry PCC instance.
 */
struct doca_telemetry_pcc;

/**
 * @brief Check if given device is capable of executing telemetry PCC operations.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry PCC.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PCC.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_cap_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device representor is capable of executing telemetry PCC operations.
 * @note Representor devices may be used to access pcc telemetry on lower level bond devs not exposed as doca_devs
 *
 * @param [in] devinfo_rep
 * The DOCA device rep information.
 *
 * @return
 * DOCA_SUCCESS - in case device rep supports telemetry PCC.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device rep for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PCC.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device rep.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_cap_rep_is_supported(const struct doca_devinfo_rep *devinfo_rep);

/**
 * @brief Create a DOCA Telemetry PCC instance.
 *
 * @param [in] dev
 * The device to attach to the telemetry PCC instance.
 * @param [out] pcc
 * Pointer to pointer to be set to point to the created doca_telemetry_pcc instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_create(struct doca_dev *dev, struct doca_telemetry_pcc **pcc);

/**
 * @brief Create a DOCA Telemetry PCC instance based on a representor device.
 *
 * @param [in] dev_rep
 * The representor device to attach to the telemetry PCC instance.
 * @param [out] pcc
 * Pointer to pointer to be set to point to the created doca_telemetry_pcc instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_rep_create(struct doca_dev_rep *dev_rep, struct doca_telemetry_pcc **pcc);

/**
 * @brief Destroy doca_telemetry_pcc previously created by doca_telemetry_pcc_create().
 *
 * @param [in] pcc
 * Pointer to instance to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - pcc needs to be stopped before destroy.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_destroy(struct doca_telemetry_pcc *pcc);

/**
 * @brief Start context for pcc counter extraction.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_start(struct doca_telemetry_pcc *pcc);

/**
 * @brief Stop pcc counter extraction context.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - pcc instance doesn't require stopping.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_stop(struct doca_telemetry_pcc *pcc);

/**
 * @brief Get the maximum number of algo slots that may be populated.
 *
 * @note The cap is no longer considered valid.
 * doca_telemetry_pcc_get_max_algo_slots() should be used instead when the PCC algo is running.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] max_algo_slots
 * Maximum number of algo slots on device.
 * @note Slots are indexed from 0 to max_algo_slots-1
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_DEPRECATED
doca_error_t doca_telemetry_pcc_cap_get_max_algo_slots(const struct doca_devinfo *devinfo, uint32_t *max_algo_slots);

/**
 * @brief Get the maximum number of algo slots that may be populated in the PCC instance.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [out] max_algo_slots
 * Maximum number of algo slots on device.
 * @note Slots are indexed from 0 to max_algo_slots-1
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_max_algo_slots(struct doca_telemetry_pcc *pcc, uint32_t *max_algo_slots);

/**
 * @brief Get the id of algo on a specific slot.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [in] algo_slot
 * Slot index of the algo.
 * @param [out] algo_id
 * ID of given algo.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter or out of bounds index.
 * - DOCA_ERROR_BAD_STATE - selected slot is not populated.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_algo_id(struct doca_telemetry_pcc *pcc, uint8_t algo_slot, uint32_t *algo_id);

/**
 * @brief Get the major version number of algo on a specific slot.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [in] algo_slot
 * Slot index of the algo.
 * @param [out] major_ver
 * Major version number.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter or out of bounds index.
 * - DOCA_ERROR_BAD_STATE - selected slot is not populated.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_algo_major_version(struct doca_telemetry_pcc *pcc,
						       uint8_t algo_slot,
						       uint32_t *major_ver);

/**
 * @brief Get the minor version number of algo on a specific slot.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [in] algo_slot
 * Slot index of the algo.
 * @param [out] minor_ver
 * Minor version number.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter or out of bounds index.
 * - DOCA_ERROR_BAD_STATE - selected slot is not populated.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_algo_minor_version(struct doca_telemetry_pcc *pcc,
						       uint8_t algo_slot,
						       uint32_t *minor_ver);

/**
 * @brief Get the enable status for a given algo slot.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [in] algo_slot
 * Algo slot on device to act on.
 * @param [out] algo_enabled
 * Status of the algorithm (1 is enabled, 0 is disabled).
 * @param [out] counters_enabled
 * Status of counters for the algorithm (1 is enabled, 0 is disabled).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error (slot may be invalid).
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_algo_enable_status(struct doca_telemetry_pcc *pcc,
						       uint8_t algo_slot,
						       uint8_t *algo_enabled,
						       uint8_t *counters_enabled);

/**
 * @brief Get the maximum number of characters that be returned by an algo_info request.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] max_algo_info_len
 * Maximum length of a algo info string.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_cap_get_max_algo_info_len(const struct doca_devinfo *devinfo,
							  uint32_t *max_algo_info_len);

/**
 * @brief Get information on a given algorithm.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [in] algo_slot
 * Algo slot on device to act on.
 * @param [out] algo_info
 * Algo information - must be allocated up to a length of doca_telemetry_pcc_cap_get_max_algo_info_len().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error (slot may be invalid).
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_algo_info(struct doca_telemetry_pcc *pcc, uint8_t algo_slot, char *algo_info);

/**
 * @brief Get the maximum number of counters that the pcc instance may return.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [out] max_num_counters
 * Maximum number of 32-bit counters context can return.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_max_num_counters(struct doca_telemetry_pcc *pcc, uint32_t *max_num_counters);

/**
 * @brief Get the number of counters available in a given algo slot.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [in] algo_slot
 * Algo slot on device to act on.
 * @param [out] num_counters
 * Number of counters enabled on the algo slot.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error (slot may be invalid).
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_num_counters(struct doca_telemetry_pcc *pcc,
						 uint8_t algo_slot,
						 uint32_t *num_counters);

/**
 * @brief Get the maximum number of characters that be returned by a counter_info request.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] max_counter_info_len
 * Maximum length of a counter info string.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_cap_get_max_counter_info_len(const struct doca_devinfo *devinfo,
							     uint32_t *max_counter_info_len);

/**
 * @brief Get information on a given counter.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [in] algo_slot
 * Algo slot on device to act on.
 * @param [in] counter_id
 * ID of the counter to check - ranging from 0 to num_counters-1.
 * @param [out] counter_info
 * Counter information - must be allocated up to a length of doca_telemetry_pcc_cap_get_max_counter_info_len().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error (slot or counter_id may be invalid).
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_counter_info(struct doca_telemetry_pcc *pcc,
						 uint8_t algo_slot,
						 uint8_t counter_id,
						 char *counter_info);

/**
 * @brief Get the maximum number of counters that may be returned.
 *
 * @note The cap is no longer considered valid.
 * doca_telemetry_pcc_get_max_num_counters() should be used instead when the PCC algo is running.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] max_num_counters
 * Maximum number of 32-bit counters device can return.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_DEPRECATED
doca_error_t doca_telemetry_pcc_cap_get_max_num_counters(const struct doca_devinfo *devinfo,
							 uint32_t *max_num_counters);

/**
 * @brief Get counters for PCC algo slot.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [in] algo_slot
 * Algo slot on device to act on.
 * @param [out] counters_populated
 * Number of 'counters' array entries that have been populated - indexes 0 to counters_populated-1.
 * @param [out] counters
 * Array of 32-bit counters - must be allocated with at least doca_telemetry_pcc_get_max_num_counters() entries.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - counters and/or algo are disabled.
 * - DOCA_ERROR_DRIVER - internal driver error (slot may be invalid).
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_counters(struct doca_telemetry_pcc *pcc,
					     uint8_t algo_slot,
					     uint32_t *counters_populated,
					     uint32_t *counters);

/**
 * @brief Get and clear (reset to 0) counters for PCC algo slot.
 *
 * @param [in] pcc
 * Pointer to pcc instance.
 * @param [in] algo_slot
 * Algo slot on device to act on.
 * @param [out] counters_populated
 * Number of 'counters' array entries that have been populated (0 if counters array is NULL).
 * @param [out] counters
 * Array of 32-bit counters - must be allocated with at least doca_telemetry_pcc_get_max_num_counters() entries.
 * @note Setting the array pointer to NULL will reset the counters without returning their value
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - counters and/or algo are disabled.
 * - DOCA_ERROR_DRIVER - internal driver error (slot may be invalid).
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry pcc.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pcc_get_and_clear_counters(struct doca_telemetry_pcc *pcc,
						       uint8_t algo_slot,
						       uint32_t *counters_populated,
						       uint32_t *counters);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_TELEMETRY_PCC_H_ */

/** @} */
