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
 * @file doca_telemetry_adp_retx.h
 * @page doca_telemetry_adp_retx
 * @defgroup DOCA_TELEMETRY_ADP_RETX_H_ DOCA Telemetry Adaptive Retransmission
 * DOCA Telemetry Adaptive Retransmission library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_TELEMETRY_ADP_RETX_H_
#define DOCA_TELEMETRY_ADP_RETX_H_

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
 * DOCA Telemetry Adaptive Retransmission Context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA Telemetry Adaptive Retransmission instance.
 */
struct doca_telemetry_adp_retx;

/**
 * @brief Check if given device is capable of executing Telemetry Adaptive Retransmission operations.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports doca_telemetry_adp_retx contexts.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry doca_telemetry_adp_retx.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_cap_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Create a DOCA Telemetry Adaptive Retransmission instance.
 *
 * @param [in] dev
 * The device to attach to the doca_telemetry_adp_retx instance.
 * @param [out] adp_retx
 * Pointer to pointer to be set to point to the created doca_telemetry_adp_retx instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_create(struct doca_dev *dev, struct doca_telemetry_adp_retx **adp_retx);

/**
 * @brief Destroy doca_telemetry_adp_retx previously created by doca_telemetry_adp_retx_create().
 *
 * @param [in] adp_retx
 * Pointer to instance to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx needs to be stopped before destroy.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_destroy(struct doca_telemetry_adp_retx *adp_retx);

/**
 * @brief Start doca_telemetry_adp_retx context.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter or bad configuration.
 * - DOCA_ERROR_BAD_STATE - context is already started.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_start(struct doca_telemetry_adp_retx *adp_retx);

/**
 * @brief Stop doca_telemetry_adp_retx context.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx instance is already stopped.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_stop(struct doca_telemetry_adp_retx *adp_retx);

/*********************************************************************************************************************
 * DOCA Telemetry Adaptive Retransmission Histogram
 *********************************************************************************************************************/

/**
 * Adaptive Retransmission Histogram provides timeout event metrics from active ADP algorithms running on the device.
 *
 * The histogram should be configured before doca_telemetry_adp_retx_start() is called.
 * Recording of metrics will begin on context start.
 * There is no guarantee that configuration will not be modified by external parties at run time.
 * It is recommended that steps are taken by users to ensure there is no conflict here.
 *
 */

/**
 * Time unit used by histogram bins
 */
enum doca_telemetry_adp_retx_hist_time_unit {
	DOCA_TELEMETRY_ADP_RETX_HIST_TIME_UNIT_NSEC = 0x1,
	DOCA_TELEMETRY_ADP_RETX_HIST_TIME_UNIT_USEC = 0x2,
	DOCA_TELEMETRY_ADP_RETX_HIST_TIME_UNIT_USEC_100 = 0x4,
	DOCA_TELEMETRY_ADP_RETX_HIST_TIME_UNIT_MSEC = 0x8,
};

/**
 * Width calculation mode from bin 1 onwards (bin 0 width is defined independently)
 */
enum doca_telemetry_adp_retx_hist_bin_width_mode {
	DOCA_TELEMETRY_ADP_RETX_HIST_BIN_WIDTH_MODE_FIXED = 0,	/**<  All bins are the same width */
	DOCA_TELEMETRY_ADP_RETX_HIST_BIN_WIDTH_MODE_DOUBLE = 1, /**<  Each bin is 2x the width of previous */
};

/**
 * @brief Check if given device supports telemetry Adaptive Retransmission Histogram.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports doca_telemetry_adp_retx histograms.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry adp retx histogram.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_cap_histogram_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Get the maximum number of bins supported by the Adaptive Retransmission Histogram.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] max_bins
 * Maximum number of bin supported by adp retx histogram.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry adp retx histogram.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_cap_get_hist_max_bins(const struct doca_devinfo *devinfo, uint32_t *max_bins);

/**
 * @brief Get the time units supported by the Adaptive Retransmission Histogram.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] time_units
 * Bitmap of supported time values on the device defined by enum doca_telemetry_adp_retx_hist_time_unit.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry adp retx histogram.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_cap_get_hist_time_units(const struct doca_devinfo *devinfo, uint32_t *time_units);

/*********************************************************************************************************************
 * DOCA Telemetry Adaptive Retransmission Histogram Configuration
 *********************************************************************************************************************/

/**
 * @brief Set the number of bins to use in the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [in] num_bins
 * Number of bins to configure - must be greater than 0 and not exceed doca_telemetry_adp_retx_cap_get_hist_max_bins().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_set_hist_num_bins(struct doca_telemetry_adp_retx *adp_retx, uint32_t num_bins);

/**
 * @brief Get the number of bins configured in the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] num_bins
 * Number of bins to configured.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_get_hist_num_bins(struct doca_telemetry_adp_retx *adp_retx, uint32_t *num_bins);

/**
 * @brief Set the width of bin0 in the histogram.
 *
 * Timeout events between 0 and bin0_width will be counted in bin 0.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [in] bin0_width
 * Bin width to configure (must be >0) - value is in time units defined by doca_telemetry_adp_retx_set_hist_time_unit().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_set_hist_bin0_width(struct doca_telemetry_adp_retx *adp_retx, uint16_t bin0_width);

/**
 * @brief Get the configured bin0_width from the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] bin0_width
 * Configured bin0_width value.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_get_hist_bin0_width(struct doca_telemetry_adp_retx *adp_retx,
							 uint16_t *bin0_width);

/**
 * @brief Set the width of bin1 in the histogram
 *
 * Defines the width of bin 1 and all subsequent bins (based on doca_telemetry_adp_retx_set_hist_bin_width_mode()).
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [in] bin1_width
 * Bin width to configure (must be >0) - value is in time units defined by doca_telemetry_adp_retx_set_hist_time_unit().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_set_hist_bin1_width(struct doca_telemetry_adp_retx *adp_retx, uint16_t bin1_width);

/**
 * @brief Get the configured bin1_width from the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] bin1_width
 * Configured bin1_width value.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_get_hist_bin1_width(struct doca_telemetry_adp_retx *adp_retx,
							 uint16_t *bin1_width);

/**
 * @brief Set the bin width mode to configure the histogram.
 *
 * Determines the width of bins 2 to N based on the width defined for bin 1.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [in] bin_width_mode
 * Bin width mode to configure
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_set_hist_bin_width_mode(
	struct doca_telemetry_adp_retx *adp_retx,
	enum doca_telemetry_adp_retx_hist_bin_width_mode bin_width_mode);

/**
 * @brief Get the configured bin width mode from the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] bin_width_mode
 * Configured bin_width_mode value.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_get_hist_bin_width_mode(
	struct doca_telemetry_adp_retx *adp_retx,
	enum doca_telemetry_adp_retx_hist_bin_width_mode *bin_width_mode);

/**
 * @brief Set the time unit to define the bin widths in the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [in] time_unit
 * Time unit to use the histogram bins - supported types given by doca_telemetry_adp_retx_cap_get_hist_time_units().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_set_hist_time_unit(struct doca_telemetry_adp_retx *adp_retx,
							enum doca_telemetry_adp_retx_hist_time_unit time_unit);

/**
 * @brief Get the configured time unit from the histogram
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] time_unit
 * Configured time_unit value.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_get_hist_time_unit(struct doca_telemetry_adp_retx *adp_retx,
							enum doca_telemetry_adp_retx_hist_time_unit *time_unit);

/**
 * @brief Set the VHCA ID of function to read events from.
 *
 * If configured, the histogram bins will only contain events from the given VHCA ID.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [in] vhca_id
 * VHCA ID for the histogram to record events from.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_set_hist_vhca_id(struct doca_telemetry_adp_retx *adp_retx, uint16_t vhca_id);

/**
 * @brief Remove any VHCA ID configured value from the histogram.
 *
 * If no VHCA ID is configured, the histogram will record all events from the PF associated with the app_retx instance.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_unset_hist_vhca_id(struct doca_telemetry_adp_retx *adp_retx);

/**
 * @brief Get any configured VHCA ID from the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] vhca_set
 * 1 indicates that a VHCA ID value is configured on the histogram. 0 indicates no configuration.
 * @param [out] vhca_id
 * The configure VHCA ID value. Only valid if 'vhca_set' is 1.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_get_hist_vhca_id(struct doca_telemetry_adp_retx *adp_retx,
						      uint8_t *vhca_set,
						      uint16_t *vhca_id);

/**
 * @brief Configures the histogram to reset its bin values after each read.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [in] clear_on_read
 * If 1 then bins will be reset to 0 on each read. If 0 then bins values will be maintained between reads.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_set_hist_clear_on_read(struct doca_telemetry_adp_retx *adp_retx,
							    uint8_t clear_on_read);

/**
 * @brief Get the configured clear_on_read value from the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] clear_on_read
 * Configured clear_on_read value.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_get_hist_clear_on_read(struct doca_telemetry_adp_retx *adp_retx,
							    uint8_t *clear_on_read);

/**
 * @brief Configures the histogram to read and record ADP events.
 *
 * If set, then starting the context will start histogram event recording, if not set then starting the context will not
 * start the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [in] count_enable
 * If 1 then the histogram will start on context start, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 * - DOCA_ERROR_BAD_STATE - adp_retx is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_set_hist_count_enable(struct doca_telemetry_adp_retx *adp_retx,
							   uint8_t count_enable);

/**
 * @brief Get the configured count_enable value from the histogram.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] count_enable
 * Configured count_enable value.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL or unsupported parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_get_hist_count_enable(struct doca_telemetry_adp_retx *adp_retx,
							   uint8_t *count_enable);

/*********************************************************************************************************************
 * DOCA Telemetry Adaptive Retransmission Histogram Runtime
 *********************************************************************************************************************/

/**
 * @brief Determine if the currently running histogram configuration matches that which was set by telemetry context.
 *
 * This function allows a check for configuration changes that may have been applied by external entities.
 * If changes have been found, these will be reflected in the doca_telemetry_adp_retx_get_hist_*() functions.
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] change_detected
 * 0 indicates histogram configuration is unchanged. 1 indicates an external configuration modification.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_detect_hist_conf_change(struct doca_telemetry_adp_retx *adp_retx,
							     uint8_t *change_detected);

/**
 * @brief Read current bin values of the adaptive retransmission histogram.
 *
 * The adp retx histogram will return 'bins_populated' 64-bit values.
 * Each value represents the number of timeout events that have occurred within the bin's timespan.
 * The bin configuration is determined prior to context start.
 *
 * For example,
 * 4 returned bins where X is bin0_width, Y is bin1_width, time is in nanoseconds, and mode is fixed, would give:
 *  +------------------------+------------------------+------------------------+------------------------+
 *  | bins[0]                | bins[1]                | bins[2]                | bins[3]                |
 *  | [0 to X-1]ns           | [X to (X+Y)-1]ns       | [(X+Y) to (X+2Y)-1]ns  | [(X+2Y) to inf)ns      |
 *  +------------------------+------------------------+------------------------+------------------------+
 *
 * @param [in] adp_retx
 * Pointer to doca_telemetry_adp_retx instance.
 * @param [out] bins_populated
 * Number of 'bins' array entries that have been populated - indexes 0 to bins_populated-1.
 * @param [out] bins
 * Array of 64-bit bin values - must be a pointer to at least doca_telemetry_adp_retx_cap_get_hist_max_bins() values.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_SUPPORTED - operation is not supported on doca_telemetry_adp_retx histogram context.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_adp_retx_read_hist_bins(struct doca_telemetry_adp_retx *adp_retx,
						    uint32_t *bins_populated,
						    uint64_t *bins);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_TELEMETRY_ADP_RETX_H_ */

/** @} */
