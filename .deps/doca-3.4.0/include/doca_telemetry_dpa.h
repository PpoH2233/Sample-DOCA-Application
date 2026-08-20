/*
 * Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_telemetry_dpa.h
 * @page DOCA_TELEMETRY_DPA
 * @defgroup DOCA_TELEMETRY_DPA DOCA telemetry DPA
 * DOCA telemetry DPA library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_TELEMETRY_DPA_H_
#define DOCA_TELEMETRY_DPA_H_

#include <doca_error.h>
#include <doca_types.h>

#define DOCA_TELEMETRY_DPA_PROCESS_NAME_SIZE (17) /**< Process name limits */
#define DOCA_TELEMETRY_DPA_THREAD_NAME_SIZE (17)  /**< Thread name limits */

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 * DOCA core opaque types
 *********************************************************************************************************************/
struct doca_dev;
struct doca_devinfo;

/*********************************************************************************************************************
 * DOCA Telemetry DPA Context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA Telemetry DPA instance.
 */
struct doca_telemetry_dpa;

/**
 * @brief Performance counters state
 */
enum doca_telemetry_dpa_counter_state {
	DOCA_TELEMETRY_DPA_COUNTER_STATE_ACTIVE = 0x1,
	DOCA_TELEMETRY_DPA_COUNTER_STATE_INACTIVE = 0x2,
	DOCA_TELEMETRY_DPA_COUNTER_STATE_RESET = 0x3
};

/**
 * @brief Performance counters type
 */
enum doca_telemetry_dpa_counter_type {
	DOCA_TELEMETRY_DPA_COUNTER_TYPE_CUMULATIVE_EVENT = 0x0, /**< Performance counter sample accumulating
									     data across all event executions */
	DOCA_TELEMETRY_DPA_COUNTER_TYPE_EVENT_TRACER = 0x1	/**< Performance counter sample storing them into a
									cyclic buffer specific to the process */
};

/**
 * @brief List of the performance event samples type
 */
enum doca_telemetry_dpa_event_sample_type {
	DOCA_TELEMETRY_DPA_EVENT_SAMPLE_TYPE_EMPTY_SAMPLE = 0x0,
	DOCA_TELEMETRY_DPA_EVENT_SAMPLE_TYPE_SCHEDULE_IN = 0x1,
	DOCA_TELEMETRY_DPA_EVENT_SAMPLE_TYPE_SCHEDULE_OUT = 0x2,
	DOCA_TELEMETRY_DPA_EVENT_SAMPLE_TYPE_BUFFER_FULL = 0xff
};

/**
 * @brief Single process structure.
 */
typedef struct doca_telemetry_dpa_process_info {
	uint32_t dpa_process_id;				 /**< Global DPA process Id */
	uint32_t num_of_threads;				 /**< Number of threads in process */
	char process_name[DOCA_TELEMETRY_DPA_PROCESS_NAME_SIZE]; /**< The name of the process */
} doca_telemetry_dpa_process_info_t;

/**
 * @brief Single thread structure.
 */
typedef struct doca_telemetry_dpa_thread_info {
	uint32_t dpa_process_id;			       /**< Global DPA process Id */
	uint32_t dpa_thread_id;				       /**< Global DPA thread Id */
	char thread_name[DOCA_TELEMETRY_DPA_THREAD_NAME_SIZE]; /**< The name of the thread */
} doca_telemetry_dpa_thread_info_t;

/**
 * @brief Single cumulative info structure.
 */
typedef struct doca_telemetry_dpa_cumul_info {
	uint32_t dpa_process_id; /**< Global DPA process Id */
	uint32_t dpa_thread_id;	 /**< Global DPA thread Id */
	uint64_t time;		 /**< Total time in ticks the thread has been active */
	uint64_t cycles;	 /**< Total execution unit cycles the thread used */
	uint64_t instructions;	 /**< Total number of instructions the thread executed */
	uint64_t num_executions; /**< Total number of thread executions */
} doca_telemetry_dpa_cumul_info_t;

/**
 * @brief Single performance events tracer structure.
 */
typedef struct doca_telemetry_dpa_event_sample {
	uint64_t timestamp;				/**< Timestamp in usec */
	uint64_t cycles;				/**< Stamp of total Execution Unit cycles */
	uint32_t instructions;				/**< Stamp of total number of instructions of this DPA EU*/
	uint32_t dpa_thread_id;				/**< Global DPA thread Id */
	uint16_t eu_id;					/**< Execution unit id */
	uint16_t sample_id_in_eu;			/**< Running sample id per Execution Unit.
							 * A single sample_id is assigned to both schedule
							 * in and out samples */
	enum doca_telemetry_dpa_event_sample_type type; /**< Type of event sample */
} doca_telemetry_dpa_event_sample_t;

/**
 * @brief Check if given device is capable of executing telemetry DPA operations.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry DPA.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry DPA.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_cap_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Create a DOCA telemetry DPA instance.
 *
 * @param [in] dev
 * The device to attach to the telemetry DPA instance.
 * @param [out] dpa
 * Pointer to pointer to be set to point to the created doca_telemetry_dpa instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_create(struct doca_dev *dev, struct doca_telemetry_dpa **dpa);

/**
 * @brief Destroy doca_telemetry_dpa previously created by doca_telemetry_dpa_create().
 *
 * @param [in] dpa
 * Pointer to instance to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - context is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_destroy(struct doca_telemetry_dpa *dpa);

/**
 * @brief Start context for telemetry DPA.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_start(struct doca_telemetry_dpa *dpa);

/**
 * @brief Stop telemetry DPA context.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - dpa instance doesn't require stopping.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_stop(struct doca_telemetry_dpa *dpa);

/**
 * @brief Read the processes info.
 * @note This function reads the processes info for a specific process or all processes.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [out] dpa_process_num
 * Number of DPA processes retrieved
 * @param [out] dpa_process_list
 * Pointer to an array of DPA processes info struct.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_NOT_FOUND - no processes found.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_read_processes_list(struct doca_telemetry_dpa *dpa,
						    uint32_t dpa_process_id,
						    uint32_t *dpa_process_num,
						    doca_telemetry_dpa_process_info_t *dpa_process_list);

/**
 * @brief Read threads info
 * @note This function reads the threads info for specific process or all processes and specific thread or all threads.
 * @note If all processes id is provided the dpa_thread_id must also be set to address all threads.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [in] dpa_thread_id
 * DPA thread id or all thread id value (this value can be queried using doca_telemetry_dpa_get_all_thread_id()).
 * @param [out] dpa_threads_num
 * Number of DPA threads retrieved
 * @param [out] dpa_thread_list
 * Pointer to an array of DPA threads info struct.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_NOT_FOUND - no processes or threads found.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_read_thread_list(struct doca_telemetry_dpa *dpa,
						 uint32_t dpa_process_id,
						 uint32_t dpa_thread_id,
						 uint32_t *dpa_threads_num,
						 doca_telemetry_dpa_thread_info_t *dpa_thread_list);

/**
 * @brief Start device counters sampling - trigger device to collect performance metrics.
 * @note This function sets event counters to ACTIVE for specific process or all processes with appropriate type.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [in] type
 * DPA counter type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_BAD_STATE - counter is already started.
 * - DOCA_ERROR_NOT_SUPPORTED - provided counter type is not supported by the DPA device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_counter_start(struct doca_telemetry_dpa *dpa,
					      uint32_t dpa_process_id,
					      enum doca_telemetry_dpa_counter_type type);

/**
 * @brief Restart device counters sampling.
 * @note This function sets event counters to RESET for specific process or all processes with appropriate type.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [in] type
 * DPA counter type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided counter type is not supported by the DPA device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_counter_restart(struct doca_telemetry_dpa *dpa,
						uint32_t dpa_process_id,
						enum doca_telemetry_dpa_counter_type type);

/**
 * @brief Stop device counters sampling.
 * @note This function sets event counters to INACTIVE for specific process or all processes with appropriate type.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [in] type
 * DPA counter type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided counter type is not supported by the DPA device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_counter_stop(struct doca_telemetry_dpa *dpa,
					     uint32_t dpa_process_id,
					     enum doca_telemetry_dpa_counter_type type);

/**
 * @brief Read list of the cumulative info counters.
 * @note This function reads the list of the cumulative info counters for a specific process or all processes and
 * specific thread or all threads.
 * @note If all processes id is provided the dpa_thread_id must also be set to address all threads.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [in] dpa_thread_id
 * DPA thread id or all thread id value (this value can be queried using doca_telemetry_dpa_get_all_thread_id()).
 * @param [out] cumul_samples_num
 * Number of cumul samples retrieved
 * @param [out] cumul_info_list
 * Pointer to an array of cumulative info counters struct.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_NOT_FOUND - no processes or samples found.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_read_cumul_info_list(struct doca_telemetry_dpa *dpa,
						     uint32_t dpa_process_id,
						     uint32_t dpa_thread_id,
						     uint32_t *cumul_samples_num,
						     doca_telemetry_dpa_cumul_info_t *cumul_info_list);

/**
 * @brief Read list of the performance event samples.
 * @note This function reads the list of the performance event samples for a specific process or all processes and
 * specific thread or all threads.
 * @note If all processes id is provided the dpa_thread_id must also be set to address all threads.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [in] dpa_thread_id
 * DPA thread id or all thread id value (this value can be queried using doca_telemetry_dpa_get_all_thread_id()).
 * @param [out] perf_event_samples_num
 * Number of perf event samples retrieved
 * @param [out] perf_event_list
 * Pointer to an array of performance event samples struct.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_NOT_FOUND - no processes or samples found.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_read_perf_event_list(struct doca_telemetry_dpa *dpa,
						     uint32_t dpa_process_id,
						     uint32_t dpa_thread_id,
						     uint32_t *perf_event_samples_num,
						     doca_telemetry_dpa_event_sample_t *perf_event_list);

/**
 * @brief Get DPA timer frequency.
 * @note This function returns the DPA timer ticks frequency, given in kHZ. Using this
 * frequency, timer ticks can be converted to running clock using the formula:
 * clock_time = ticks/dpa_timer_frequency
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [out] dpa_timer_freq
 * DPA timer frequency.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_dpa_timer_freq(struct doca_telemetry_dpa *dpa, uint32_t *dpa_timer_freq);

/**
 * @brief Get memory size (in bytes) to allocate the process list.
 * @note This function returns the memory size the user can use to allocate the doca_telemetry_dpa_process_info_t output
 * structure.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [out] dpa_process_list_size
 * Memory size (in bytes) to allocate the process list.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_process_list_size(struct doca_telemetry_dpa *dpa,
						      uint32_t dpa_process_id,
						      uint32_t *dpa_process_list_size);

/**
 * @brief Get memory size (in bytes) to allocate the thread list.
 * @note This function returns the memory size the user can use to allocate the doca_telemetry_dpa_thread_info_t output
 * structure.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [in] dpa_thread_id
 * DPA thread id or all thread id value (this value can be queried using doca_telemetry_dpa_get_all_thread_id()).
 * @param [out] dpa_thread_list_size
 * Memory size (in bytes) to allocate the thread list.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_thread_list_size(struct doca_telemetry_dpa *dpa,
						     uint32_t dpa_process_id,
						     uint32_t dpa_thread_id,
						     uint32_t *dpa_thread_list_size);

/**
 * @brief Get memory size (in bytes) to allocate the cumul samples.
 * @note This function returns the memory size the user can use to allocate the doca_telemetry_dpa_cumul_info_t output
 * structure.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id or all processes id value (this value can be queried using doca_telemetry_dpa_get_all_process_id()).
 * @param [in] dpa_thread_id
 * DPA thread id or all thread id value (this value can be queried using doca_telemetry_dpa_get_all_thread_id()).
 * @param [out] dpa_cumul_samples_size
 * Memory size (in bytes) to allocate the cumul samples.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_cumul_samples_size(struct doca_telemetry_dpa *dpa,
						       uint32_t dpa_process_id,
						       uint32_t dpa_thread_id,
						       uint32_t *dpa_cumul_samples_size);

/**
 * @brief Set maximum number of perf event samples.
 * @note This function sets the maximum number of perf event samples to retrieve.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_max_perf_event_samples
 * DPA maximum number of perf event samples.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - context is already started.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_set_max_perf_event_samples(struct doca_telemetry_dpa *dpa,
							   uint32_t dpa_max_perf_event_samples);

/**
 * @brief Get maximum number of perf event samples.
 * @note This function returns the maximum number of perf event samples to be used to allocate the
 * doca_telemetry_dpa_event_sample_t output structure.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [out] dpa_max_perf_event_samples
 * DPA maximum number of perf event samples.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_max_perf_event_samples(struct doca_telemetry_dpa *dpa,
							   uint32_t *dpa_max_perf_event_samples);

/**
 * @brief Get memory size (in bytes) to allocate the perf event samples.
 * @note This function returns the memory size the user can use to allocate the doca_telemetry_dpa_event_sample_t output
 * structure.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [out] dpa_perf_event_samples_size
 * Memory size (in bytes) to allocate the perf event samples.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_perf_event_samples_size(struct doca_telemetry_dpa *dpa,
							    uint32_t *dpa_perf_event_samples_size);

/**
 * @brief Get counter state
 * @note This function gets event counter state for specific process.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id.
 * @param [out] state
 * DPA counter state.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_counter_state(struct doca_telemetry_dpa *dpa,
						  uint32_t dpa_process_id,
						  enum doca_telemetry_dpa_counter_state *state);

/**
 * @brief Get counter state and type.
 * @note This function gets event counters type for specific process.
 *
 * @param [in] dpa
 * Pointer to dpa instance.
 * @param [in] dpa_process_id
 * DPA process id.
 * @param [out] type
 * DPA counter type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_counter_type(struct doca_telemetry_dpa *dpa,
						 uint32_t dpa_process_id,
						 enum doca_telemetry_dpa_counter_type *type);

/**
 * @brief Get all process mask.
 * @note This function gets the specific id used to address all processes simultaneously.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] all_process_id
 * DPA process id that address all processes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_all_process_id(const struct doca_devinfo *devinfo, uint32_t *all_process_id);

/**
 * @brief Get all threads mask.
 * @note This function gets the specific id used to address all threads simultaneously.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] all_thread_id
 * DPA thread id that address all processes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_dpa_get_all_thread_id(const struct doca_devinfo *devinfo, uint32_t *all_thread_id);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_TELEMETRY_DPA_H_ */

/** @} */
