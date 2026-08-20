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

#ifndef DOCA_STA_STATS_H_
#define DOCA_STA_STATS_H_

#include <stdint.h>

#include <doca_error.h>

#include <doca_sta_handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_sta;

/**
 * @brief Execution unit (EU) types
 */
enum dpa_sta_eu_type {
	DOCA_STA_EU_TYPE_UNKNOWN = 0, /**< Unknown EU type */
	DOCA_STA_EU_COMP,	      /**< Completion handler (EU) type */
	DOCA_STA_EU_TX,		      /**< Transmission handler (EU) type */
	DOCA_STA_EU_BEQ,	      /**< Back-end queue handler (EU) type */
	DOCA_STA_EU_MAX,	      /**< Denotes all handler types */
};

enum dpa_sta_eu_state {
	DOCA_STA_EU_STATE_IDLE,	     /**< EU is idle */
	DOCA_STA_EU_STATE_STOPPING,  /**< EU is stopping */
	DOCA_STA_EU_STATE_STARTED,   /**< EU is started */
	DOCA_STA_EU_STATE_SUSPENDED, /**< EU is suspended */
	DOCA_STA_EU_STATE_STOPPED    /**< EU is stopped */
};

/**
 * @brief doca sta execution unit (handler) counter entry
 */
struct doca_sta_eu_ctr_entry {
	const char *name;
	/**< name of the counter */
	const uint64_t *val;
	/**< value of the counter */
};

/**
 * @brief Get counters (various statistics) for the given execution unit (EU) handle.
 * This function is used for debugging and performance investigation.
 *
 * @param [in] eu_handle
 * EU handle to get statistics for
 * @param [out] entries
 * Pointer to a doca_sta_eu_ctr_entry array that will be populated with counter entries
 * @param [out] num_entries
 * The number of valid entries in the entries array
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_eu_stats(struct doca_sta_eu_handle *eu_handle,
				   const struct doca_sta_eu_ctr_entry **entries,
				   uint16_t *num_entries);

/**
 * @brief Reset counters for the given execution unit (EU) handle.
 * This function clears all statistics counters associated with the specified EU handle.
 *
 * @param [in] eu_handle
 * EU handle whose counters should be reset
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_eu_reset_stats_handle(struct doca_sta_eu_handle *eu_handle);

/**
 * @brief Reset counters for all execution units of the specified type.
 * This function clears all statistics counters for EUs matching the given type.
 *
 * @param [in] sta
 * Pointer to doca_sta instance
 * @param [in] eu_type
 * Type of EUs whose counters should be reset
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_eu_reset_stats_type(struct doca_sta *sta, enum dpa_sta_eu_type eu_type);

/**
 * @brief Get counters about connected queue pairs (QPs) for the given execution unit (EU) handle.
 * This function is used for debugging and performance analysis of QP connections.
 *
 * @param [in] eu_handle
 * EU handle to get QP statistics for
 * @param [out] arr
 * Pointer to a doca_sta_qp_handle array that will be populated with connected QP handles
 * @param [out] arr_size
 * The number of valid entries in the array
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_eu_connect_qp_stats(struct doca_sta_eu_handle *eu_handle,
					      struct doca_sta_qp_handle **arr,
					      uint16_t *arr_size);

/**
 * @brief Retrieve counters for a specific queue handle associated with a backend handle.
 * This function is used for debugging and performance analysis of backend queues.
 *
 * @param [in] be_handle
 * Backend handle
 * @param [in] be_q_handle
 * Backend queue handle to get statistics for
 * @param [out] entries
 * Pointer to a doca_sta_eu_ctr_entry array that will be populated with counter entries
 * @param [out] num_entries
 * The number of valid entries in the entries array
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_be_queue_stats(struct doca_sta_be_handle *be_handle,
					 struct doca_sta_be_q_handle *be_q_handle,
					 const struct doca_sta_eu_ctr_entry **entries,
					 uint16_t *num_entries);

/**
 * @brief Retrieve counters for a specific namespace handle associated with a subsystem handle.
 * This function is used for debugging and performance analysis of namespaces.
 *
 * @param [in] subs_handle
 * Subsystem handle
 * @param [in] ns_handle
 * Namespace handle to get statistics for
 * @param [out] entries
 * Pointer to a doca_sta_eu_ctr_entry array that will be populated with counter entries
 * @param [out] num_entries
 * The number of valid entries in the entries array
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_ns_stats(struct doca_sta_subs_handle *subs_handle,
				   struct doca_sta_ns_handle *ns_handle,
				   const struct doca_sta_eu_ctr_entry **entries,
				   uint16_t *num_entries);

/**
 * @brief Retrieve information about running execution handlers.
 * This function is used for debugging and performance analysis of execution units.
 *
 * @param [in] sta
 * Pointer to doca_sta instance
 * @param [out] eu_handle_arr
 * Pointer to a doca_sta_eu_handle array that will be populated with EU handles
 * @param [out] arr_size
 * The number of valid entries in the array
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_eu_handle(const struct doca_sta *sta,
				    struct doca_sta_eu_handle **eu_handle_arr,
				    uint32_t *arr_size);

/**
 * @brief Get the name of an execution unit (EU).
 *
 * @param [in] eu_handle
 * EU handle to get the name for
 * @param [out] name
 * Pointer that will be set to the EU's name
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_eu_name(const struct doca_sta_eu_handle *eu_handle, const char **name);

/**
 * @brief Get the type of an execution unit (EU).
 *
 * @param [in] eu_handle
 * EU handle to get the type for
 * @param [out] type
 * Pointer that will be set to the EU's type
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_eu_type(const struct doca_sta_eu_handle *eu_handle, enum dpa_sta_eu_type *type);

/**
 * @brief Get the ID of an execution unit (EU).
 *
 * @param [in] eu_handle
 * EU handle to get the ID for
 * @param [out] eu_id
 * Pointer that will be set to the EU's ID
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_eu_id(const struct doca_sta_eu_handle *eu_handle, uint16_t *eu_id);

/**
 * @brief Get the port number that an execution unit (EU) belongs to.
 *
 * @param [in] eu_handle
 * EU handle to get the port for
 * @param [out] port
 * Pointer that will be set to the EU's port number
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_eu_port(const struct doca_sta_eu_handle *eu_handle, uint16_t *port);

/**
 * @brief Get the state of an execution unit (EU).
 *
 * @param [in] eu_handle
 * EU handle to get the state for
 * @param [out] state
 * Pointer that will be set to the EU's state
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_get_eu_state(struct doca_sta_eu_handle *eu_handle, enum dpa_sta_eu_state *state);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_STATS_H_ */
