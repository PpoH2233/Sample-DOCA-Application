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
 * @defgroup DOCA_PCC_DEVICE DOCA PCC Device
 * DOCA PCC Device library. For more details please refer to the user guide on DOCA devzone.
 *
 * @ingroup DOCA_PCC
 *
 * @{
 */

#ifndef DOCA_PCC_DEV_H_
#define DOCA_PCC_DEV_H_

/**
 * @brief declares that we are compiling for the DPA Device
 *
 * @note Must be defined before the first API use/include of DOCA
 */
#define DOCA_DPA_DEVICE

#include <doca_pcc_dev_common.h>
#include <doca_pcc_dev_utils.h>
#include <doca_pcc_dev_services.h>
#include <doca_pcc_dev_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Implements the internal CC algorithm provided by the lib
 *
 * The lib provides an internal built-in CC algorithm implementation.
 * The user may call this function for flows with algo_slot
 * that is not set by the user (An unknown algo_slot can be the result of running without algo negotiation)
 *
 * @note The default internal algo is only supported for algo slot DOCA_PCC_DEV_ALGO_SLOT_INTERNAL and is initiated on
 * DOCA_PCC_DEV_ALGO_INDEX_INTERNAL.
 *
 * @param[in]  algo_ctxt - @see doca_pcc_dev_user_algo
 * @param[in]  event -     @see doca_pcc_dev_user_algo
 * @param[in]  attr -      @see doca_pcc_dev_user_algo
 * @param[out] results -   @see doca_pcc_dev_user_algo
 */
DOCA_STABLE
void doca_pcc_dev_default_internal_algo(doca_pcc_dev_algo_ctxt_t *algo_ctxt,
					doca_pcc_dev_event_t *event,
					const doca_pcc_dev_attr_t *attr,
					doca_pcc_dev_results_t *results);

/**
 * @brief Entry point to the user algorithm handling code
 *
 * This code handles a single event. it receives the algorithm context,
 * the event information (opaque struct), and some attributes (algo id), and returns
 * the PCC rate
 * The event info should not be used directly through the struct. It is recommended to use
 * the supplied "getter" functions (doca_pcc_dev_event.h) to help generate more future
 * compatible code if event information placement changes
 *
 * @param[in]  algo_ctxt - pointer to user context for this flow (restored from previous iteration)
 * @param[in]  event - pointer to event data struct to be used with getter functions
 * @param[in]  attr - information about event like algo type
 * @param[out]  results - new rate information to be written to HW.
 *			  The rate is expressed as a 20b fixed point number in range (0 , 1]
 */
DOCA_STABLE
void doca_pcc_dev_user_algo(doca_pcc_dev_algo_ctxt_t *algo_ctxt,
			    doca_pcc_dev_event_t *event,
			    const doca_pcc_dev_attr_t *attr,
			    doca_pcc_dev_results_t *results);

/**
 * @brief Entry point to the user one time initialization code
 *
 * This is called on PCC process load and should initialize the data of
 * all user algorithms.
 *
 * @param[out]  disable_event_bitmask - a bitmask of events that should be discarded and not passed
 * to the event processing code
 */
DOCA_STABLE
void doca_pcc_dev_user_init(uint32_t *disable_event_bitmask);

/**
 * @brief User callback executed then parameters are set.
 *
 * Called when the parameter change was set externally.
 * The implementation should:
 *     Check the given new_parameters values. If those are correct from the algorithm perspective,
 *     assign them to the given parameter array.
 *
 * @param[in]  port_num - index of the port
 * @param[in]  algo_slot - Algo slot identifier as referred to in the PPCC command field "algo_slot"
 * if possible it should be equal to the algo_idx
 * @param[in]  param_id_base - id of the first parameter that was changed.
 * @param[in]  param_num - number of all parameters that were changed
 * @param[in]  new_param_values - pointer to an array which holds param_num number of new values for parameters
 * @param[in]  params - pointer to an array which holds beginning of the current parameters to be changed
 *
 * @return -
 * DOCA_PCC_DEV_STATUS_OK: Parameters were set
 * DOCA_PCC_DEV_STATUS_FAIL: the values (one or more) are not legal. No parameters were changed
 *
 */
DOCA_STABLE
doca_pcc_dev_error_t doca_pcc_dev_user_set_algo_params(uint32_t port_num,
						       uint32_t algo_slot,
						       uint32_t param_id_base,
						       uint32_t param_num,
						       const uint32_t *new_param_values,
						       uint32_t *params);

/**
 * @brief User callback executed to set custom header in CCMAD probe packet.
 *
 * Called in user application to change custom header only for CCMAD probe type
 *
 * @param[in]  algo_ctxt - pointer to user context for this flow (restored from previous iteration)
 * @param[in]  event - pointer to event data struct to be used with getter functions
 * @param[in]  header - header content memory address
 * @param[in]  header_size - header size length, the unit is Double Words.
 * @param[in]  wait_completed - When set, the function will return after the setting is applied, assuring that the next
 * probe packet will use the custom header content. When cleared, the function will not wait for the setting to
 * finish. It will return earlier, and the next transmitted probe may still use the previous header content.
 * @return -
 * DOCA_PCC_DEV_STATUS_OK: CCMAD probe with custom header is changed successfully
 * DOCA_PCC_DEV_STATUS_FAIL: CCMAD probe header is not changed
 *
 */
DOCA_EXPERIMENTAL
doca_pcc_dev_error_t doca_pcc_dev_custom_header_set(doca_pcc_dev_algo_ctxt_t *algo_ctxt,
						    doca_pcc_dev_event_t *event,
						    uint32_t *header,
						    uint32_t header_size,
						    uint32_t wait_completed);

/**
 * @brief User callback to initiate CC hint global caps.
 *
 * The caps will be queried by host application to initiate the hint format for the algorithm.
 * CC hint caps should be initialized by the user as a 64-byte memory block.
 *
 * @param[out] caps - CC hint caps
 */
DOCA_EXPERIMENTAL
void doca_pcc_dev_user_cc_hint_init_caps(uint8_t *caps) __attribute__((weak));

/**
 * @brief Get CC hint id.
 *
 * @param[in] event - CC event
 *
 * @return hint id
 */
DOCA_EXPERIMENTAL
uint32_t doca_pcc_dev_get_cc_hint_id(doca_pcc_dev_event_t *event);

/**
 * @brief Get CC hint update sequence number.
 *
 * @param[in] event - CC event
 * @param[in] hint_id - hint id
 *
 * @return hint update sequence number
 */
DOCA_EXPERIMENTAL
uint16_t doca_pcc_dev_get_cc_hint_usn(doca_pcc_dev_event_t *event, uint32_t hint_id);

/**
 * @brief Get CC hint data.
 *
 * @param[in] event - CC event
 * @param[in] hint_id - hint id
 *
 * @return pointer to hint data
 */
DOCA_EXPERIMENTAL
const uint8_t *doca_pcc_dev_get_cc_hint(doca_pcc_dev_event_t *event, uint32_t hint_id);

/**
 * @brief Get DSCP value.
 *
 * @note To enable this feature, need to set NV config ROCE_CC_SHAPER_COALESCE=SOURCE_QP
 * @param[in] event - CC event
 *
 * @return DSCP value (6 bits, range 0-63) of the flow of the given event.
 *    if the flow or its DSCP is not initiated, a value of 0 is returned
 */
DOCA_EXPERIMENTAL
uint32_t doca_pcc_dev_get_dscp(doca_pcc_dev_event_t *event);

/**
 * @brief Indication that an algo_idx is not assigned to an algo_slot
 */
#define DOCA_PCC_DEV_SLOT_UNASSIGNED 0x10

/**
 * @brief Declare the number of counter groups to be supported per port.
 *
 * @note Must be called inside doca_pcc_dev_user_declare_memory. If not called, or returned failure, the library will
 * fall back to default number of counter groups.
 *
 * @param[in] num_counter_groups - number of counter groups to be supported per port
 *
 * @return DOCA_PCC_DEV_STATUS_OK: number of counter groups to be supported per port is declared successfully
 * DOCA_PCC_DEV_STATUS_FAIL: failed to declare number of counter groups to be supported on invalid value
 *
 */
doca_pcc_dev_error_t doca_pcc_dev_declare_num_counter_groups(uint32_t num_counter_groups);

/**
 * @brief Call to set a specific algo to a specific slot and port.
 *
 * @note Must be called inside doca_pcc_dev_user_declare_memory
 *
 * Used to map user defined algorithms to an algo slot, mapping defined with this API must match the initialization of
 * algorithms done in doca_pcc_dev_user_init API
 *
 * @param[in]  port_num - index of the port
 * @param[in]  algo_slot - algo slot identifier as referred to in the PPCC command field "algo_slot"
 * @param[in]  algo_idx - algo index
 *
 * @return -
 * DOCA_PCC_DEV_STATUS_OK: algo slot to algo index mapping is declared successfully
 * DOCA_PCC_DEV_STATUS_FAIL: failed to declare algo on invalid algo index, algo slot or port number
 *
 */
doca_pcc_dev_error_t doca_pcc_dev_declare_slot_algo(uint32_t port_num, uint32_t algo_slot, uint8_t algo_idx);

/**
 * @brief Call to set a specific number of counters and parameters used in the algo.
 *
 * @note Must be called inside doca_pcc_dev_user_declare_memory and after doca_pcc_dev_declare_slot_algo
 *
 * @param[in]  algo_idx - algo index
 * @param[in]  param_num - number of parameters that the algo will use
 * @param[in]  counter_num - number of counters that the algo will use
 *
 * @return -
 * DOCA_PCC_DEV_STATUS_OK: algo idx is declared successfully
 * DOCA_PCC_DEV_STATUS_FAIL: failed to declare algo on invalid algo_idx
 *
 */
doca_pcc_dev_error_t doca_pcc_dev_declare_algo(uint32_t algo_idx, uint32_t param_num, uint32_t counter_num);

/**
 * @brief User callback where user declares memory usage.
 *
 * @note If used then must declare memory for all algos using both doca_pcc_dev_declare_slot_algo and
 * doca_pcc_dev_declare_algo, alternative is not to declare for any algo which causes memory to be allocated by the max
 * allowed
 *
 * Called before doca_pcc_dev_user_init
 * The implementation should:
 *     Call doca_pcc_dev_declare_algo, to set number of counters & parameters for a specific algo
 *     Call doca_pcc_dev_declare_slot_algo, to map which slot idx to be used for each algo slot and each port
 * 			DOCA_PCC_DEV_SLOT_UNASSIGNED in case a mapping is not provided
 *
 */
__attribute__((weak)) void doca_pcc_dev_user_declare_memory(void);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_PCC_DEV_H_ */

/** @} */
