/*
 * Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_clock.h
 * @page doca_clock
 * @defgroup DOCA_CLOCK DOCA Clock
 * @ingroup DOCACore
 * The DOCA Clock is used for exposing low-level timestamp-related functionality including reading timestamps and
 * cross-timestamping.
 *
 * @{
 */
#ifndef DOCA_CLOCK_H_
#define DOCA_CLOCK_H_

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * DOCA Clock Context
 ******************************************************************************/

/**
 * Forward declarations
 */
struct doca_clock;
struct doca_dev;
struct doca_devinfo;

/**
 * Clock types that may be used
 */
#define DOCA_CLOCK_NIC_FREE_RUNNING 0x1		   /**< NIC free-running clock (secs/nsecs) */
#define DOCA_CLOCK_NIC_REAL_TIME (0x1 << 1)	   /**< NIC real-time clock (secs/nsecs)*/
#define DOCA_CLOCK_NIC_DPA_TIMER (0x1 << 8)	   /**< NIC DPA (Data Path Accelerator) clock (secs/nsecs)*/
#define DOCA_CLOCK_HOST_COUNTER_CYCLES (0x1 << 16) /**< Host counter (raw counter, x86_64: RDTSC/ARM: CNTVCT_EL0) */
#define DOCA_CLOCK_HOST_REAL_TIME (0x1 << 17)	   /**< Host real-time clock (secs/nsecs) */
#define DOCA_CLOCK_HOST_MONOTONIC (0x1 << 18)	   /**< Host monotonic clock (secs/nsecs) */
#define DOCA_CLOCK_HOST_MONOTONIC_RAW (0x1 << 19)  /**< Host raw monotonic clock (secs/nsecs) */

/**
 * Time formats that may be returned
 */
union doca_clock_timespec_t {
	struct timespec ts; /**< Standard timestamp defined in time.h */
	uint64_t counter;   /**< Raw timestamp counter */
};

/**
 * @brief Check if given device supports free-running NIC clock.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case free-running NIC clock is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not free-running NIC clock.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_clock_cap_nic_free_running_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device supports real time NIC clock.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case real time NIC clock is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not real time NIC clock.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_clock_cap_nic_real_time_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device supports DPA timer NIC clock.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case DPA timer NIC clock is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not DPA timer NIC clock.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_clock_cap_nic_dpa_timer_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Initialize and create a doca clock for timestamp operations.
 *
 * @param [in] dev
 * The device to attach to the doca_clock instance.
 * @param [out] clock
 * Pointer to pointer to be set to point to the created doca_clock instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - clock device not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_clock_create(const struct doca_dev *dev, struct doca_clock **clock);

/**
 * @brief Release resources and destroy a previously created device clock.​
 *
 * @param [in] clock
 * doca_clock instance to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_clock_destroy(struct doca_clock *clock);

/**
 * @brief Get a specific clock/counter value.
 *
 * @param [in] clock
 * doca_clock instance to read clock value from.
 * @param [in] clock_type
 * Type of clock to be read - represented by DOCA_CLOCK_*
 * @param [out] clock_ts
 * Value of the requested clock.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_SUPPORTED - requested clock type is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_clock_get_timestamp(const struct doca_clock *clock,
				      uint64_t clock_type,
				      union doca_clock_timespec_t *clock_ts);

/**
 * @brief Get a specific clock/counter value.
 *
 * @param [in] clock
 * doca_clock instance to read clock value from.
 * @param [in] primary_clock_type
 * Primary type of clock to be read. Can refer to either NIC or host. Represented by DOCA_CLOCK_*.
 * @param [in] nic_clock_type
 * Type of NIC clock to be read. Should refer to ones of the NIC clocks (FR, RT, DPA). Represented by DOCA_CLOCK_NIC_*.
 * @param [out] primary_clock_ts
 * Value of the requested primary clock.
 * @param [out] nic_clock_ts
 * Value of the requested NIC clock.
 * @param [out] error_margin_nsecs
 * Accuracy achieved in cross timestamp reading. Will be 0 for exact clock readings.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_SUPPORTED - one of more requested clock type is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_clock_get_crosstimestamp(const struct doca_clock *clock,
					   uint64_t primary_clock_type,
					   uint64_t nic_clock_type,
					   union doca_clock_timespec_t *primary_clock_ts,
					   union doca_clock_timespec_t *nic_clock_ts,
					   uint64_t *error_margin_nsecs);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_TIME_H_ */
