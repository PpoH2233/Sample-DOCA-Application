/*
 * Copyright (c) 2023-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @defgroup DOCA_PCC_DEVICE_ALGO_ACCESS DOCA PCC Device Algorithm Access
 * @ingroup DOCA_PCC_DEVICE
 *
 * @{
 */

#ifndef DOCA_PCC_DEV_ALGO_ACCESS_H_
#define DOCA_PCC_DEV_ALGO_ACCESS_H_

#include <doca_pcc_dev.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opt-in to per-thread-style counters with configurable sharing
 *
 * Number of counter sets per group = ceil(num_rp_threads / threads_per_counter_set).
 * - threads_per_counter_set = 1: each thread gets its own counter set (N threads -> N sets).
 * - threads_per_counter_set = 2: every 2 threads share a set (N threads -> N/2 sets).
 * - threads_per_counter_set = K: every K threads share a set (N threads -> ceil(N/K) sets).
 *
 * The PPCC read path transparently aggregates values across all counter sets
 * using the per-counter type set by doca_pcc_dev_set_counter_type().
 *
 * Must be called from doca_pcc_dev_user_declare_memory() before memory is sized.
 *
 * @param[in]  threads_per_counter_set - number of threads that share one counter set (1 = per-thread).
 *             Must be in the range [1, doca_pcc_dev_get_max_threads_per_counter_set()].
 *
 * @return DOCA_PCC_DEV_STATUS_OK / DOCA_PCC_DEV_STATUS_FAIL
 */
DOCA_EXPERIMENTAL
doca_pcc_dev_error_t doca_pcc_dev_declare_threads_per_counter_set(uint32_t threads_per_counter_set);

/**
 * @brief Return the maximum supported threads_per_counter_set
 *
 * @return Maximum allowed threads_per_counter_set.
 */
DOCA_EXPERIMENTAL
uint32_t doca_pcc_dev_get_max_threads_per_counter_set(void);

/**
 * @brief Set the type for a specific counter
 *
 * @note Only applicable when per-thread counters are declared (see
 *       doca_pcc_dev_declare_threads_per_counter_set()).
 *
 * When per-thread counters are enabled, the PPCC read path uses this type
 * to determine how to combine values across counter sets:
 *   - ACCUMULATE: result = sum of all counter sets (default)
 *   - MIN: result = minimum across all counter sets
 *   - MAX: result = maximum across all counter sets
 *
 * Should be called from doca_pcc_dev_user_init() after algo metadata is initialized.
 *
 * @param[in]  algo_idx    - Algo identifier (same as used in doca_pcc_dev_algo_init_metadata)
 * @param[in]  counter_id  - Counter index within the algo
 * @param[in]  type        - DOCA_PCC_DEV_COUNTER_TYPE_ACCUMULATE, _MIN, or _MAX
 *
 * @return DOCA_PCC_DEV_STATUS_OK on success, DOCA_PCC_DEV_STATUS_FAIL on invalid parameters.
 */
DOCA_EXPERIMENTAL
doca_pcc_dev_error_t doca_pcc_dev_set_counter_type(uint32_t algo_idx, uint32_t counter_id, uint32_t type);

/**
 * @brief  Reaction Point target app
 *
 * To be used as an extension for the algo access API. @see DOCA_PCC_DEV_ALGO_SLOT
 */
#define DOCA_PCC_DEV_TARGET_APP_RP (0)

/**
 * @brief  Notification Point target app
 *
 * To be used as an extension for the algo access API. @see DOCA_PCC_DEV_ALGO_SLOT
 */
#define DOCA_PCC_DEV_TARGET_APP_NP (1)

/**
 * @brief wrapper to define algorithm slot per target device
 *
 * @note use for algo access API
 */
#define DOCA_PCC_DEV_ALGO_SLOT(target_app, ppcc_algo_slot) ((target_app << 16) | (ppcc_algo_slot & 0xffff))

/**
 * @brief This struct provides meta data for a pcc user algo
 */

/**
 * @brief Initialize the algo database
 *
 * This function initializes an algo data-structure.
 * Each algorithm has in index (not to be confused with the doca_pcc_dev_algo_meta_data::algo_id)
 * Algo database includes a metadata area containing basic algo information,
 * and a params and counters metadata area. The number of parameters and counters
 * is configurable at init time.
 * The space allocated for the algo data-structure using this function is visible to the PCC infrastructure.
 * This is required to allow the infrastructure to read/update param / counter information
 * directly when handling PPCC MADs or commands.
 * The user can use additional memory by allocating global variables
 *
 * This function should be called once per algo during init.
 *
 * @param[in]  algo_idx - algo index.
 * @param[in]  user_def - basic version info + pointer and size of algo description string
 * @param[in]  param_num   - max number of params (will be used to reserve param space)
 * @param[in]  counter_num - max number of counters (will be used to reserve counter space)
 *
 * @return DOCA_PCC_DEV_STATUS_FAIL if input parameters are out of range.
 */
DOCA_STABLE
doca_pcc_dev_error_t doca_pcc_dev_algo_init_metadata(uint32_t algo_idx,
						     const struct doca_pcc_dev_algo_meta_data *user_def,
						     uint32_t param_num,
						     uint32_t counter_num);

/**
 * @brief Initialize a single parameter for an algo
 *
 * This function initializes a single parameter (param_id) for a specific algo (algo_idx)
 * The param_id should be in the bounds declared by doca_pcc_dev_algo_init_metadata(...)
 * The param info is "global" to the algo on all ports. The current value of the param that is initialized
 * to the "default" value can be modified at the port level.
 *
 * @param[in]  algo_idx - Algo index.
 * @param[in]  param_id - parameter id (from 0 to doca_pcc_dev_algo_init_metadata(...).param_num)
 * @param[in]  default_value - base value.
 * @param[in]  max_value - max value that is enforced by set function.
 * @param[in]  min_value - min value that is enforced by set function..
 * @param[in]  permissions - If 1 allows value update, if 0 update is disabled.
 * @param[in]  param_desc_size - The size in bytes for the parameter descriptor string field
 * @param[in]  param_desc_addr - A pointer to the parameter descriptor string field.
 *
 * @return DOCA_PCC_DEV_STATUS_FAIL if input parameters are out of range.
 */
DOCA_STABLE
doca_pcc_dev_error_t doca_pcc_dev_algo_init_param(uint32_t algo_idx,
						  uint32_t param_id,
						  uint32_t default_value,
						  uint32_t max_value,
						  uint32_t min_value,
						  uint32_t permissions,
						  uint32_t param_desc_size,
						  uint64_t param_desc_addr);

/**
 * @brief Initialize a single counter for an algo
 *
 * This function initializes a single counter (counterid) for a specific algo (algo_idx)
 * The counter_id should be in the bounds declared by doca_pcc_dev_algo_init_metadata(...)
 * The counter info (e.g. description) is "global" to the algo on all ports.
 *
 * @param[in]  algo_idx - Algo identifier.
 * @param[in]  counter_id - counter id (from 0 to doca_pcc_dev_algo_init_metadata(...).counter_num)
 * @param[in]  max_value - max value that allowed for the counter.
 * @param[in]  permissions - If 1 allows value update, if 0 update is disabled.
 * @param[in]  counter_desc_size - The size in bytes for the counter descriptor string field
 * @param[in]  counter_desc_addr - A pointer to the counter descriptor string field.
 *
 * @return DOCA_PCC_DEV_STATUS_FAIL if input parameters are out of range.
 */
DOCA_STABLE
doca_pcc_dev_error_t doca_pcc_dev_algo_init_counter(uint32_t algo_idx,
						    uint32_t counter_id,
						    uint32_t max_value,
						    uint32_t permissions,
						    uint32_t counter_desc_size,
						    uint64_t counter_desc_addr);

/**
 * @brief Set the counter group for an algo slot
 *
 * Must be called after doca_pcc_dev_init_algo_slot with algo being enabled.
 * Calling this API overwrites the counter owner which is equivalent to using PPCC command with counter_en=1
 *
 * @param[in]  port_num - port to be initialized
 * @param[in]  algo_slot - Algo slot identifier as referred to in the PPCC command field "algo_slot"
 * @param[in]  counter_group_id - Counters group to be set
 *
 * @return DOCA_PCC_DEV_STATUS_OK on success, DOCA_PCC_DEV_STATUS_FAIL on invalid parameters.
 */
DOCA_STABLE
doca_pcc_dev_error_t doca_pcc_dev_set_slot_counter_group(uint32_t port_num,
							 uint32_t algo_slot,
							 uint32_t counter_group_id);

/**
 * @brief Initialize the algo per port database
 *
 * This function initializes the algo per port parameter database, and maps an algo_idx (global algo index)
 * to a specific slot per port.
 * This function allocates parameters and counters per port.
 * The default parameters values are taken from the algo metadata set by @ref doca_pcc_dev_algo_init_param() .
 * The counters and parameters can be get/set by the infrastructure based on MAD and access register PPCC command
 * Function MUST be called after calls to doca_pcc_dev_algo_init_param for this algo type
 *
 * @param[in]  portid - port to be initialized
 * @param[in]  algo_slot - Algo slot identifier as referred to in the PPCC command field "algo_slot"
 * if possible it should be equal to the algo_idx
 * @param[in]  algo_idx - Algo identifier.
 * @param[in]  algo_en - 1 mark algo as enabled, if 0 algo will not be reported if queried
 *
 * @return DOCA_PCC_DEV_STATUS_FAIL if input parameters are out of range.
 */
DOCA_STABLE
doca_pcc_dev_error_t doca_pcc_dev_init_algo_slot(uint32_t portid,
						 uint32_t algo_slot,
						 uint32_t algo_idx,
						 uint32_t algo_en);

/**
 * @brief Get number of counters supported per algo on the port
 *
 * @param[in]  port_num - port number algo is running on
 * @param[in]  algo_slot - Algo slot identifier as referred to in the PPCC command field "algo_slot"
 *
 * @return number of supported counters
 */
DOCA_STABLE
uint32_t doca_pcc_dev_get_counters_num(uint32_t port_num, uint32_t algo_slot);

/**
 * @brief Get pointer to counter array of a specific algo and specific port
 *
 * This retrieves the pointer to an array of counters (up to doca_pcc_dev_get_counters_num(...) counters)
 * used by algo_slot on the port, Counters need to be enabled with mlxreg PPCC command, with counter_en = 1.
 *
 * @note In per-thread counter mode (see doca_pcc_dev_declare_threads_per_counter_set()), the returned
 *       pointer refers to the counter set assigned to the calling thread.
 *
 * @param[in]  port_num - port number algo is running on
 * @param[in]  algo_slot - Algo slot identifier as referred to in the PPCC command field "algo_slot"
 *
 * @return array of 32b counters
 */
DOCA_STABLE
uint32_t *doca_pcc_dev_get_counters(uint32_t port_num, uint32_t algo_slot);

/**
 * @brief Get number of params supported per algo on the port
 *
 * @param[in]  port_num - port number algo is running on
 * @param[in]  algo_slot - Algo slot identifier as referred to in the PPCC command field "algo_slot"
 *
 * @return number of supported params
 */
DOCA_STABLE
uint32_t doca_pcc_dev_get_algo_params_num(uint32_t port_num, uint32_t algo_slot);

/**
 * @brief Get pointer to param array of a specific algo and specific port
 *
 * This retrieves the pointer to an array of param (current value) of up to doca_pcc_dev_get_algo_params_num() params
 * used by algo_slot on the port
 *
 * @param[in]  port_num - port number algo is running on
 * @param[in]  algo_slot - Algo slot identifier as referred to in the PPCC command field "algo_slot"
 *
 * @return array of 32b parameters
 */
DOCA_STABLE
uint32_t *doca_pcc_dev_get_algo_params(uint32_t port_num, uint32_t algo_slot);

/**
 * @brief Get identifier of a specific algo and specific port
 *
 * @param[in]  port_num - port number algo is running on
 * @param[in]  algo_slot - Algo slot identifier as referred to in the PPCC command field "algo_slot"
 *
 * @return Algo identifier.
 */
DOCA_STABLE
uint32_t doca_pcc_dev_get_algo_index(uint32_t port_num, uint32_t algo_slot);

/**
 * @brief Initialize a histogram instance on a given counter group and histogram ID
 *
 * @param[in]  port_num     Port number
 * @param[in]  target_app   Target app (only DOCA_PCC_DEV_TARGET_APP_RP is supported)
 * @param[in]  counter_group_id Counter group ID
 * @param[in]  histogram_id   Histogram ID (0 to DOCA_PCC_DEV_MAX_NUM_HISTOGRAMS - 1)
 * @param[in]  edges        Bin edges array (length: num_bins + 1)
 * @param[in]  num_bins     Number of bins (must <= DOCA_PCC_DEV_HISTOGRAM_MAX_BINS)
 * @param[in]  mode         Bin mode (linear / exponential / free edges)
 * @param[in]  description  User description string
 * @param[out] h            Returned histogram handle
 *
 * @return
 * DOCA_PCC_DEV_STATUS_OK: Histogram was initialized
 * DOCA_PCC_DEV_STATUS_FAIL: Invalid input or unsupported configuration
 */
DOCA_EXPERIMENTAL
doca_pcc_dev_error_t doca_pcc_dev_histogram_init(uint32_t port_num,
						 uint32_t target_app,
						 uint32_t counter_group_id,
						 uint32_t histogram_id,
						 const uint32_t *edges,
						 uint32_t num_bins,
						 doca_pcc_dev_histogram_mode_t mode,
						 const char *description,
						 doca_pcc_dev_histogram_t *h);

/**
 * @brief Clear histogram counters
 *
 * Need to disable the histogram to clear the histogram counters
 *
 * @param[in] h Histogram handle
 *
 * @return DOCA_PCC_DEV_STATUS_OK: Histogram was cleared
 * DOCA_PCC_DEV_STATUS_FAIL: Invalid input or unsupported configuration
 */
DOCA_EXPERIMENTAL
doca_pcc_dev_error_t doca_pcc_dev_histogram_clear(doca_pcc_dev_histogram_t h);

/**
 * @brief Update histogram by incrementing the bin counter for a given value
 *
 * @param[in] h      Histogram handle
 * @param[in] value  Value to record
 */
DOCA_EXPERIMENTAL
void doca_pcc_dev_histogram_update(doca_pcc_dev_histogram_t h, uint32_t value);

/**
 * @brief Get histogram description string
 *
 * @param[in] h Histogram handle
 *
 * @return Pointer to description string
 */
DOCA_EXPERIMENTAL
const char *doca_pcc_dev_histogram_get_description(doca_pcc_dev_histogram_t h);

/**
 * @brief Declare the number of histograms to be supported per port
 *
 * @note Must be called inside doca_pcc_dev_user_declare_memory. If not called, or returned failure, the library will
 * fall back to default number of histograms.
 *
 * @param[in] num_histograms - number of histograms to be supported per port
 *
 * @return DOCA_PCC_DEV_STATUS_OK: number of histograms per port is declared successfully
 * DOCA_PCC_DEV_STATUS_FAIL: failed to declare number of histograms on invalid value
 */
DOCA_EXPERIMENTAL
doca_pcc_dev_error_t doca_pcc_dev_declare_num_of_histograms(uint32_t num_histograms);

/**
 * @brief Disable histogram
 *
 * @param[in] h Histogram handle
 *
 * @return DOCA_PCC_DEV_STATUS_OK: Histogram was disabled
 * DOCA_PCC_DEV_STATUS_FAIL: Invalid input or unsupported configuration
 */
DOCA_EXPERIMENTAL
doca_pcc_dev_error_t doca_pcc_dev_histogram_disable(doca_pcc_dev_histogram_t h);

/**
 * @brief Enable histogram
 *
 * @param[in] h Histogram handle
 *
 * @return DOCA_PCC_DEV_STATUS_OK: Histogram was enabled
 * DOCA_PCC_DEV_STATUS_FAIL: Invalid input or unsupported configuration
 */
DOCA_EXPERIMENTAL
doca_pcc_dev_error_t doca_pcc_dev_histogram_enable(doca_pcc_dev_histogram_t h);

/**
 * @brief Register histogram ids for an algo
 *
 * @param[in] algo_idx Algo index
 * @param[in] hist_ids Histogram ids
 * @param[in] num_hist_ids Number of histogram ids
 *
 * @return DOCA_PCC_DEV_STATUS_OK: Histogram ids were registered
 * DOCA_PCC_DEV_STATUS_FAIL: Invalid input or unsupported configuration
 */
DOCA_EXPERIMENTAL
doca_pcc_dev_error_t doca_pcc_dev_algo_register_hist_ids(uint32_t algo_idx,
							 const uint32_t *hist_ids,
							 uint32_t num_hist_ids);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_PCC_DEV_ALGO_ACCESS_H_ */
