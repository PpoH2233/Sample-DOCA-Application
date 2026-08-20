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
 * @file doca_telemetry_diag.h
 * @page DOCA_TELEMETRY_DIAG
 * @defgroup DOCA_TELEMETRY_DIAG DOCA Telemetry Diagnostics
 * DOCA Telemetry Diagnostics library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_TELEMETRY_DIAG_H_
#define DOCA_TELEMETRY_DIAG_H_

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

/*********************************************************************************************************************
 * DOCA Telemetry Diagnostics Context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA Telemetry Diagnostics instance.
 */
struct doca_telemetry_diag;

/**
 * @brief Synchronization mode of data sampling
 */
enum doca_telemetry_diag_sync_mode {
	DOCA_TELEMETRY_DIAG_SYNC_MODE_NO_SYNC = 0,    /**< Data sampling is not synchronized */
	DOCA_TELEMETRY_DIAG_SYNC_MODE_SYNC_START = 1, /**< Data sampling is synchronized for data that supports
						       *   synchronization */
};

/**
 * @brief description of data sampling mode
 */
enum doca_telemetry_diag_sample_mode {
	DOCA_TELEMETRY_DIAG_SAMPLE_MODE_SINGLE = 0,	/**< Sampling is stopped after collecting log_num_samples */
	DOCA_TELEMETRY_DIAG_SAMPLE_MODE_REPETITIVE = 1, /**< New samples overwrite old samples when the sampling buffer
							 *   is full */
	DOCA_TELEMETRY_DIAG_SAMPLE_MODE_ON_DEMAND = 2,	/**< Data is sampled once, upon query of the diag data */
};

/**
 * @brief description of the timestamp source of the sample
 *
 * FRC (free-running counter) represents an internal timer. When FRC is chosen,
 *     the correct parsing for users should be: (timestamp_h << 32) | timestamp_l.
 * RTC (real-time clock), timestamp_h (32 MSB) represents seconds, timestamp_l (32 LSB) represent sub-second time, given
 *     in Nanoseconds. Thus, when RTC is chosen, the correct parsing for users should be: ((timestamp_h * 10^9) +
 *     timestamp_l) nanosconds.
 */
enum doca_telemetry_diag_timestamp_source {
	DOCA_TELEMETRY_DIAG_TIMESTAMP_SOURCE_FRC = 0, /**< The internal timer (AKA free running timer) */
	DOCA_TELEMETRY_DIAG_TIMESTAMP_SOURCE_RTC = 1, /**< The real-time clock */
};

/**
 * @brief Defines the layout of the diagnostic data output:
 */
enum doca_telemetry_diag_output_format {
	DOCA_TELEMETRY_DIAG_OUTPUT_FORMAT_0 = 0, /**< Data ID is present in the output; timestamp per data entry; data
						  *   value size is 64-bit */
	DOCA_TELEMETRY_DIAG_OUTPUT_FORMAT_1 = 1, /**< No data ID in the output; timestamp per sample (not per data ID);
						  *   data value size is 64-bit */
	DOCA_TELEMETRY_DIAG_OUTPUT_FORMAT_2 = 2, /**< No data ID in the output; timestamp per sample (not per data ID);
						  *   data value size is 32-bit */
};

/**
 * @brief Output format 0 per-ID struct
 */
typedef struct doca_telemetry_diag_data_sample_format_0_value {
	uint64_t data_id;     /**< Data ID */
	uint32_t timestamp_h; /**< 32 MSB bits of the timestamp of the data sample time (in seconds when using timestamp
			       *   source DOCA_TELEMETRY_DIAG_TIMESTAMP_SOURCE_RTC, otherwise in nanoseconds) */
	uint32_t timestamp_l; /**< 32 LSB bits of the timestamp of this data sample time (in nanoseconds) */
	uint64_t data_value;  /**< The value of the data ID */
} doca_telemetry_diag_data_sample_format_0_value;

/**
 * @brief Output format 0 struct
 *
 * The output format when set to DOCA_TELEMETRY_DIAG_OUTPUT_FORMAT_0
 */
typedef struct doca_telemetry_diag_data_sample_format_0 {
	uint32_t sample_id;					/**< The sequence number of the sample */
	uint32_t reserved;					/**< Reserved for future use */
	doca_telemetry_diag_data_sample_format_0_value value[]; /**< Array of Diagnostic Data in Format 0 */
} doca_telemetry_diag_data_sample_format_0;

/**
 * @brief Output format 1 struct
 *
 * The output format when set to DOCA_TELEMETRY_DIAG_OUTPUT_FORMAT_1
 */
typedef struct doca_telemetry_diag_data_sample_format_1 {
	uint32_t sample_id;		    /**< The sequence number of the sample */
	uint32_t earliest_data_timestamp_h; /**< 32 MSB bits of the timestamp of the earliest data item in the sample
					     *   (in seconds when using timestamp source
					     *   DOCA_TELEMETRY_DIAG_TIMESTAMP_SOURCE_RTC, otherwise in nanoseconds) */
	uint32_t earliest_data_timestamp_l; /**< 32 LSB bits of the timestamp of the earliest data item in the sample
					     *   (in nanoseconds) */
	uint32_t latest_data_timestamp_l;   /**< 32 LSB bits of the timestamp of the latest data
					     *   item in the sample (in nanoseconds). The MSBs should be derived from
					     *   'earliest_data_timestamp_h' while considering potential wraparound */
	uint64_t data_value[];		    /**< An array of Diagnostic Data values (64 bit format). The order
					     *   of the data will be the same as the order of the requested data in
					     *   doca_telemetry_diag_apply_counters_list_by_id() */
} doca_telemetry_diag_data_sample_format_1;

/**
 * @brief Output format 2 struct
 *
 * The output format when set to DOCA_TELEMETRY_DIAG_OUTPUT_FORMAT_2
 */
typedef struct doca_telemetry_diag_data_sample_format_2 {
	uint32_t sample_id;		    /**< The sequence number of the sample */
	uint32_t earliest_data_timestamp_h; /**< 32 MSB bits of the timestamp of the earliest data item in the sample
					     *   (in seconds when using timestamp source
					     *   DOCA_TELEMETRY_DIAG_TIMESTAMP_SOURCE_RTC, otherwise in nanoseconds) */
	uint32_t earliest_data_timestamp_l; /**< 32 LSB bits of the timestamp of the earliest data item in the sample
					     *   (in nanoseconds) */
	uint32_t latest_data_timestamp_l;   /**< 32 LSB bits of the timestamp of the latest data
					     *   item in the sample (in nanoseconds). The MSBs should be derived from
					     *   'earliest_data_timestamp_h' while considering potential wraparound */
	uint32_t data_value[];		    /**< An array of Diagnostic Data values (32 bit format). The order
					     *   of the data will be the same as the order of the requested data in
					     *   doca_telemetry_diag_apply_counters_list_by_id() */
} doca_telemetry_diag_data_sample_format_2;

/**
 * @brief Check if given device is capable of executing telemetry diagnostics operations.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry diag.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry diagnostics.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_cap_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Get the maximal num of data IDs that is supported by a given device.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] max_num_data_ids
 * Maximal num of data IDs.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry diagnostics.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_cap_get_max_num_data_ids(const struct doca_devinfo *devinfo,
							  uint32_t *max_num_data_ids);

/**
 * @brief Get the maximal num (in log base 2) of samples that is supported by a given device.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] log_max_num_samples
 * Maximal num (in log base 2) of samples.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry diagnostics.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_cap_get_log_max_num_samples(const struct doca_devinfo *devinfo,
							     uint8_t *log_max_num_samples);

/**
 * @brief Check if given device supports data clear.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] data_clear
 * 1 if data_clear is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry diagnostics.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_cap_is_data_clear_supported(const struct doca_devinfo *devinfo, uint8_t *data_clear);

/**
 * @brief Check if given device supports sync start.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] sync_start
 * 1 if data sampling synchronization is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry diagnostics.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_cap_is_sync_start_supported(const struct doca_devinfo *devinfo, uint8_t *sync_start);

/**
 * @brief Check if given device supports a given sample mode.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [in] sample_mode
 * Selected sample mode to evaluate.
 * @param [out] sample_mode_supported
 * 1 if the given sample mode is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry diagnostics.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_cap_is_sample_mode_supported(const struct doca_devinfo *devinfo,
							      enum doca_telemetry_diag_sample_mode sample_mode,
							      uint8_t *sample_mode_supported);

/**
 * @brief Check if given device supports a given data timestamp source.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [in] data_timestamp_source
 * Selected data timestamp source to evaluate.
 * @param [out] timestamp_source_supported
 * 1 if the given timestamp source is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case the capability query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry diagnostics.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_cap_is_data_timestamp_source_supported(
	const struct doca_devinfo *devinfo,
	enum doca_telemetry_diag_timestamp_source data_timestamp_source,
	uint8_t *timestamp_source_supported);

/**
 * @brief Create a DOCA Telemetry Diagnostics instance.
 *
 * @param [in] dev
 * The device to attach to the telemetry diagnostics instance.
 * @param [in] force_ownership
 * 1 if forced to take ownership from another process, 0 otherwise.
 * @note ownership validation is only tested during doca_telemetry_diag context creation. Creating the
 * doca_telemetry_diag context with force_ownership flag set does not prevent other processes from accessing the
 * underlying HW resources which may result in undefined behavior. It is the user's responsibility to make sure no
 * other process is accessing those resources in parallel.
 * @param [out] diag
 * Pointer to pointer to be set to point to the created doca_telemetry_diag instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_IN_USE - another function has ownership over the diagnostics counters.
 * 			 If force_ownership is 1, cannot force ownership.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_create(struct doca_dev *dev,
					uint8_t force_ownership,
					struct doca_telemetry_diag **diag);

/**
 * @brief Destroy doca_telemetry_diag previously created by doca_telemetry_diag_create().
 *
 * @param [in] diag
 * Pointer to instance to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - diag needs to be stopped before destroy.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_destroy(struct doca_telemetry_diag *diag);

/**
 * @brief Apply device configuration.
 *
 * @param [in] diag
 * Pointer to diag instance.
 *
 * @return
 * DOCA_SUCCESS - in case of device configuration succeed.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE -  NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_apply_config(struct doca_telemetry_diag *diag);

/**
 * @brief Start device sampling - trigger device to collect metrics.
 *
 * @param [in] diag
 * Pointer to diag instance.
 *
 * @return
 * DOCA_SUCCESS - in case of sampling started successfully
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - no configuration was applied or no data ID was applied.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_start(struct doca_telemetry_diag *diag);

/**
 * @brief Restart device sampling - trigger device to collect new metrics.
 * @note Can be called only when sample mode is set to DOCA_TELEMETRY_DIAG_SAMPLE_MODE_SINGLE.
 * @note Can be called only after doca_telemetry_diag_start() and when previous sampling cycle is done.
 *
 * @param [in] diag
 * Pointer to diag instance.
 *
 * @return
 * DOCA_SUCCESS - in case of sampling started successfully
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - diag instance wasn't started or was already stopped.
 * - DOCA_ERROR_NOT_SUPPORTED - sample mode is not DOCA_TELEMETRY_DIAG_SAMPLE_MODE_SINGLE.
 * - DOCA_ERROR_AGAIN - previous sampling cycle isn't done.
 * - DOCA_ERROR_UNEXPECTED - fatal error. Please stop, reconfigure and start manually.
 */

DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_restart(struct doca_telemetry_diag *diag);

/**
 * @brief Stop device sampling.
 *
 * @param [in] diag
 * Pointer to diag instance.
 *
 * @return
 * DOCA_SUCCESS - in case of sampling stopped successfully
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - diag instance doesn't require stopping (no configuration was applied, nor was the diag
 * 			    started).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_stop(struct doca_telemetry_diag *diag);

/**
 * @brief Set output format.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] output_format
 * Set supported output format.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - trying to set the property after configuration was applied and before the ctx is stopped
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_set_output_format(struct doca_telemetry_diag *diag,
						   enum doca_telemetry_diag_output_format output_format);

/**
 * @brief Get output format.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] output_format
 * Get supported output format.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_output_format(struct doca_telemetry_diag *diag,
						   enum doca_telemetry_diag_output_format *output_format);

/**
 * @brief Set sample period.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] sample_period
 * The requested time interval between samples given in nanoseconds.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - trying to set the property after configuration was applied and before the ctx is stopped
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_set_sample_period(struct doca_telemetry_diag *diag, uint64_t sample_period);

/**
 * @brief Get sample period.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] sample_period
 * The current time interval between samples given in nanoseconds.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_sample_period(struct doca_telemetry_diag *diag, uint64_t *sample_period);

/**
 * @brief Set log max of samples.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] log_max_num_samples
 * Log (base 2) of the maximum number of samples to store on the device sampling buffer.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - trying to set the property after configuration was applied and before the ctx is stopped
 * - DOCA_ERROR_NOT_SUPPORTED - trying to set the property with a value not supported by the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_set_log_max_num_samples(struct doca_telemetry_diag *diag, uint8_t log_max_num_samples);

/**
 * @brief Get log max of samples.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] log_max_num_samples
 * Log (base 2) of the number of samples to store on the device sampling buffer.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_log_max_num_samples(struct doca_telemetry_diag *diag,
							 uint8_t *log_max_num_samples);

/**
 * @brief Set max num of data IDs.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] max_num_data_ids
 * Maximum number of data IDs that could be configured.
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - trying to set the property after configuration was applied and before the ctx is stopped
 * - DOCA_ERROR_NOT_SUPPORTED - trying to set the property with a value not supported by the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_set_max_num_data_ids(struct doca_telemetry_diag *diag, uint32_t max_num_data_ids);

/**
 * @brief Get max num of data IDs.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] max_num_data_ids
 * Maximum number of data IDs that could be configured.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_max_num_data_ids(struct doca_telemetry_diag *diag, uint32_t *max_num_data_ids);

/**
 * @brief Set synchronization mode.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] sync_mode
 * Synchronization mode of the data sampling.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - trying to set the property after configuration was applied and before the ctx is stopped
 * - DOCA_ERROR_NOT_SUPPORTED - trying to set the property with a value not supported by the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_set_sync_mode(struct doca_telemetry_diag *diag,
					       enum doca_telemetry_diag_sync_mode sync_mode);

/**
 * @brief Get synchronization mode.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] sync_mode
 * Synchronization mode of the data sampling.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_sync_mode(struct doca_telemetry_diag *diag,
					       enum doca_telemetry_diag_sync_mode *sync_mode);

/**
 * @brief Set sampling mode.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] sample_mode
 * Data sampling mode.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - trying to set the property after configuration was applied and before the ctx is stopped
 * - DOCA_ERROR_NOT_SUPPORTED - trying to set the property with a value not supported by the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_set_sample_mode(struct doca_telemetry_diag *diag,
						 enum doca_telemetry_diag_sample_mode sample_mode);

/**
 * @brief Get Sampling mode.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] sample_mode
 * Data sampling mode.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_sample_mode(struct doca_telemetry_diag *diag,
						 enum doca_telemetry_diag_sample_mode *sample_mode);

/**
 * @brief Set data clear.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] data_clear
 * If 1, counters are cleared at the beginning of each sampling period. Otherwise, 0.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - trying to set the property after configuration was applied and before the ctx is stopped
 * - DOCA_ERROR_NOT_SUPPORTED - trying to set the property with a value not supported by the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_set_data_clear(struct doca_telemetry_diag *diag, uint8_t data_clear);

/**
 * @brief Get data clear.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] data_clear
 * If 1, counters are cleared at the beginning of each sampling period. Otherwise, 0.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_data_clear(struct doca_telemetry_diag *diag, uint8_t *data_clear);

/**
 * @brief Set data timestamp source.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] data_timestamp_source
 * Defines the timer setting the sample timestamp(s).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - trying to set the property after configuration was applied and before the ctx is stopped
 * - DOCA_ERROR_NOT_SUPPORTED - trying to set the property with a value not supported by the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_set_data_timestamp_source(
	struct doca_telemetry_diag *diag,
	enum doca_telemetry_diag_timestamp_source data_timestamp_source);

/**
 * @brief Get data timestamp source.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] data_timestamp_source
 * Defines the timer setting the sample timestamp(s).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_data_timestamp_source(
	struct doca_telemetry_diag *diag,
	enum doca_telemetry_diag_timestamp_source *data_timestamp_source);

/**
 * @brief Check if a counter is supported in current configuration, by it's data ID.
 * @note This function can only be called after calling doca_telemetry_diag_apply_config() and before
 * doca_telemetry_diag_apply_counters_list_by_ids().
 * @note This function only indicates if the counter is supported by the device and in the current configuration.
 * DOCA_ERROR_NO_MEMORY, DOCA_ERROR_FULL or DOCA_ERROR_UNEXPECTED errors can still be caused by the given counter,
 * when used in doca_telemetry_diag_apply_counters_list_by_id().
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] data_id
 * Data ID to check.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - no configuration was applied or diag is started.
 * - DOCA_ERROR_NOT_SUPPORTED - data ID is not supported on device.
 * - DOCA_ERROR_BAD_CONFIG - data ID does not support sync.
 * - DOCA_ERROR_UNEXPECTED - unexpected error occurred.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_check_data_id(struct doca_telemetry_diag *diag, uint64_t data_id);

/**
 * @brief Apply the counters, by their data ID, to be queried.
 * @note This function can only be called after calling doca_telemetry_diag_apply_config() and before
 * doca_telemetry_diag_start().
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] data_ids
 * List of data IDs.
 * @param [in] num_data_ids
 * Number of data IDs in the list.
 * @param [out] counter_id_failure
 * In case of error - the first data ID in the list to cause the failure.
 * If the error is not related to a particular ID, value will be 0 (as long as the parameter is valid).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - no configuration was applied or diag is started.
 * - DOCA_ERROR_NOT_SUPPORTED - data ID is not supported on device.
 * - DOCA_ERROR_FULL - number of data IDs exceeds maximum set.
 * - DOCA_ERROR_BAD_CONFIG - data ID does not support sync.
 * - DOCA_ERROR_NO_MEMORY - missing resources to add data ID.
 * - DOCA_ERROR_UNEXPECTED - unknown error occurred.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_apply_counters_list_by_id(struct doca_telemetry_diag *diag,
							   const uint64_t *data_ids,
							   uint32_t num_data_ids,
							   uint64_t *counter_id_failure);

/**
 * @brief Get number of currently applied counters.
 * @note This function can only be called after calling doca_telemetry_diag_apply_config().
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] num_applied_counters
 * Number of data IDs in the applied counters list.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 *  - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_num_applied_counters(struct doca_telemetry_diag *diag,
							  uint32_t *num_applied_counters);

/**
 * @brief Get list of currently applied counters, by their ID.
 * @note This function can only be called after calling doca_telemetry_diag_apply_config().
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] num_data_ids
 * Number of data IDs in the data_ids list - should be larger or equal to the number returned by
 * doca_telemetry_diag_get_num_applied_counters().
 * @param [out] data_ids
 * List of currently applied data IDs.
 * @note It is the user's responsibility to allocate/free the list's memory.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 *  - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 *  - DOCA_ERROR_BAD_STATE - no configuration was applied.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_applied_counters_list_by_id(struct doca_telemetry_diag *diag,
								 uint32_t num_data_ids,
								 uint64_t *data_ids);

/**
 * @param [in] diag
 * Pointer to diag instance.
 * @param [out] sample_size
 * Size of a single sample based on the user configuration.
 * @note Valid only after calling doca_telemetry_diag_start() and before doca_telemetry_diag_stop().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_NOT_SUPPORTED -operation cannot be completed because of missing parameters.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_get_sample_size(struct doca_telemetry_diag *diag, uint32_t *sample_size);

/**
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] buf
 * Pointer to buffer allocated by the user based on doca_telemetry_diag_get_sample_size() and max_samples_to_read.
 * @param [in] max_samples_to_read
 * The maximal number of samples to be read.
 * @param [out] num_valid_samples
 * The number of valid consecutive samples that were actually read, between 0 and max_samples_to_read.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_SKIPPED - The operation completed successfully, and the output data is valid. Some previous sample/s
 * 			  were dropped and thus samples are not consecutive.
 * - DOCA_ERROR_OPERATING_SYSTEM - operating system call failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_query_counters(struct doca_telemetry_diag *diag,
						void *buf,
						uint32_t max_samples_to_read,
						uint32_t *num_valid_samples);

/**
 * @brief Reconfigure sample period, for example from low-sampling-freq to high-sampling-freq.
 * @note This function must be called when the context is started and fully configured.
 *
 * @param [in] diag
 * Pointer to diag instance.
 * @param [in] new_sample_period
 * The requested new time interval between samples given in nanoseconds.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter or invalid sample_period.
 * - DOCA_ERROR_BAD_STATE - trying to reconfigure before the diag ctx is started
 *                          or after the ctx is stopped
 *                          or no configuration was applied
 *                          or no data ID was applied.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory when reconfiguring the diag context.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_UNEXPECTED - unknown error occurred.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_diag_reconfig_sample_period(struct doca_telemetry_diag *diag, uint64_t new_sample_period);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_TELEMETRY_DIAG_H_ */

/** @} */
