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

#ifndef DOCA_STA_EVENT_H_
#define DOCA_STA_EVENT_H_

#include <stdint.h>
#include <stdbool.h>

#include <doca_error.h>
#include <doca_types.h>

#include <doca_sta_handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_sta;
struct doca_sta_io;
struct dev_sta_event;

struct doca_sta_event_transport_err;

/**
 * @brief Function to execute on STA event notification.
 *
 * @details This function is called by doca_pe_progress() when a related task receives an event notification from DPA.
 *
 * @note RDMA error events will be notified in the context of the doca_sta_io thread(s).
 * The same doca_sta_io thread that initiated connect for the QP.
 * Time-out events will be notified in the context of the doca_sta thread.
 *
 * @param [in] event
 * STA event that was received
 * @param [in] user_data
 * User data parameter specified during the call to doca_sta_event_register_cb()
 */
typedef void (*doca_sta_event_transport_err_cb_t)(const struct doca_sta_event_transport_err *event,
						  union doca_data user_data);

/**
 * @brief Get the QP handle for a given transport error event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_transport_err to query
 * @param [out] qp_handle
 * Pointer that will be set to the QP handle associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_transport_err_get_qp_handle(const struct doca_sta_event_transport_err *event,
							const struct doca_sta_qp_handle **qp_handle);

/**
 * @brief Get the QP operation for a given transport error event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_transport_err to query
 * @param [out] operation
 * Pointer that will be set to the QP operation associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_transport_err_get_operation(const struct doca_sta_event_transport_err *event,
							uint32_t *operation);

/**
 * @brief Get the QP syndrome for a given transport error event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_transport_err to query
 * @param [out] syndrome
 * Pointer that will be set to the QP syndrome associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_transport_err_get_syndrome(const struct doca_sta_event_transport_err *event,
						       uint32_t *syndrome);

/**
 * @brief Get the QP vendor syndrome for a given transport error event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_transport_err to query
 * @param [out] vendor_syndrome
 * Pointer that will be set to the QP vendor syndrome associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_transport_err_get_vendor_syndrome(const struct doca_sta_event_transport_err *event,
							      uint32_t *vendor_syndrome);

/**
 * @brief Configure the event callback for receiving transport error events on the STA_IO context.
 *
 * @param [in] sta_io
 * Pointer to doca_sta_io instance
 * @param [in] event_cb
 * Callback function for QP transport errors
 * @param [in] user_data
 * User data to be passed to the callback
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_event_transport_err_register_cb(struct doca_sta_io *sta_io,
							 doca_sta_event_transport_err_cb_t event_cb,
							 union doca_data user_data);

struct doca_sta_event_be_timeout;

/**
 * @brief Function to execute on backend timeout event notification.
 *
 * @param [in] event
 * Backend timeout event that was received
 * @param [in] user_data
 * User data parameter specified during the call to doca_sta_event_be_timeout_register_cb()
 */
typedef void (*doca_sta_event_be_timeout_cb_t)(const struct doca_sta_event_be_timeout *event,
					       union doca_data user_data);

/**
 * @brief Configure the event callback for receiving backend timeout events on the STA context.
 *
 * @param [in] sta
 * Pointer to doca_sta instance
 * @param [in] event_cb
 * Callback function for backend timeout events
 * @param [in] user_data
 * User data to be passed to the callback
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_be_timeout_register_cb(struct doca_sta *sta,
						   doca_sta_event_be_timeout_cb_t event_cb,
						   union doca_data user_data);

/**
 * @brief Get the backend handle for a given backend timeout event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_be_timeout to query
 * @param [out] be_handle
 * Pointer that will be set to the backend handle associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_be_timeout_get_be_handle(const struct doca_sta_event_be_timeout *event,
						     const struct doca_sta_be_handle **be_handle);

/**
 * @brief Get the backend queue handle for a given backend timeout event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_be_timeout to query
 * @param [out] be_queue_handle
 * Pointer that will be set to the backend queue handle associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_be_timeout_get_be_queue_handle(const struct doca_sta_event_be_timeout *event,
							   const struct doca_sta_be_q_handle **be_queue_handle);

struct doca_sta_event_eu_err;

/**
 * @brief Function to execute on execution unit (EU) error event notification.
 *
 * @param [in] event
 * EU error event that was received
 * @param [in] user_data
 * User data parameter specified during the call to doca_sta_event_eu_err_register_cb()
 */
typedef void (*doca_sta_event_eu_err_cb_t)(const struct doca_sta_event_eu_err *event, union doca_data user_data);

/**
 * @brief Configure the event callback for receiving execution unit (EU) error events on the STA context.
 *
 * @param [in] sta
 * Pointer to doca_sta instance
 * @param [in] event_cb
 * Callback function for EU error events
 * @param [in] user_data
 * User data to be passed to the callback
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_eu_err_register_cb(struct doca_sta *sta,
					       doca_sta_event_eu_err_cb_t event_cb,
					       union doca_data user_data);

/**
 * @brief Check if an EU error event indicates a fatal error.
 *
 * @param [in] event
 * Pointer to doca_sta_event_eu_err to query
 * @param [out] is_fatal_error
 * Pointer that will be set to true if the error is fatal, false otherwise
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_eu_err_is_fatal_error(const struct doca_sta_event_eu_err *event, bool *is_fatal_error);

/**
 * @brief Get the EU handle for a given EU error event.
 *
 * @note This function is only relevant for non-fatal errors.
 *
 * @param [in] event
 * Pointer to doca_sta_event_eu_err to query
 * @param [out] eu_handle
 * Pointer that will be set to the EU handle associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_eu_err_get_eu_handle(const struct doca_sta_event_eu_err *event,
						 const struct doca_sta_eu_handle **eu_handle);

struct doca_sta_event_cqe_notify;

enum doca_sta_cqe_notify_type {
	DOCA_STA_CQE_NOTIF_NVME_OF_VALIDATION_ERR, /**< NVMeoF capsule validation error */
	DOCA_STA_CQE_NOTIF_NVME_COMP_ERR,	   /**< NVMe pci command completion error */
};

/**
 * @brief Function to execute on cqe notify event notification.
 *
 * @param [in] event
 * CQE error event that was received
 * @param [in] user_data
 * User data parameter specified during the call to doca_sta_event_cqe_notify_register_cb()
 */
typedef void (*doca_sta_event_cqe_notify_cb_t)(const struct doca_sta_event_cqe_notify *event,
					       union doca_data user_data);

/**
 * @brief Configure the event callback for receiving cqe error events on the STA context.
 *
 * @param [in] sta
 * Pointer to doca_sta instance
 * @param [in] event_cb
 * Callback function for cqe notify events
 * @param [in] user_data
 * User data to be passed to the callback
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_cqe_notify_register_cb(struct doca_sta *sta,
						   doca_sta_event_cqe_notify_cb_t event_cb,
						   union doca_data user_data);

/**
 * @brief Get the cqe error type for a given cqe notify event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_cqe_notify to query
 * @param [out] cqe_error_type
 * Pointer that will be set to the cqe error type associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_cqe_notify_get_cqe_notify_type(const struct doca_sta_event_cqe_notify *event,
							   enum doca_sta_cqe_notify_type *cqe_error_type);

/**
 * @brief Get the cqe for a given cqe notify event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_cqe_notify to query
 * @param [out] cqe
 * Pointer that will be set to the cqe associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_cqe_notify_get_cqe(const struct doca_sta_event_cqe_notify *event,
					       doca_sta_nvme_completion_t cqe);

/**
 * @brief Get the subsystem handle a given cqe notify event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_cqe_notify to query
 * @param [out] subs_handle
 * Pointer that will be set to the subsystem handle associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_cqe_notify_get_subsystem_handle(const struct doca_sta_event_cqe_notify *event,
							    const struct doca_sta_subs_handle **subs_handle);

/**
 * @brief Get the qp handle a given cqe notify event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_cqe_notify to query
 * @param [out] qp_handle
 * Pointer that will be set to the qp handle associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_cqe_notify_get_qp_handle(const struct doca_sta_event_cqe_notify *event,
						     const struct doca_sta_qp_handle **qp_handle);

/**
 * @brief Get the capsule parameters a given cqe notify event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_cqe_notify to query
 * @param [out] opcode
 * Pointer that will be set to the opcode associated with the event
 * @param [out] cid
 * Pointer that will be set to the cid associated with the event
 * @param [out] nsid
 * Pointer that will be set to the nsid associated with the event
 * @param [out] lba
 * Pointer that will be set to the lba associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_cqe_notify_get_capsule_params(const struct doca_sta_event_cqe_notify *event,
							  uint8_t *opcode,
							  uint16_t *cid,
							  uint16_t *nsid,
							  uint64_t *lba);

/**
 * @brief Get the parameter error location for a given cqe notify event.
 *
 * @param [in] event
 * Pointer to doca_sta_event_cqe_notify to query
 * @param [out] pel
 * Pointer that will be set to the parameter error location associated with the event
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_event_cqe_notify_get_param_err_location(const struct doca_sta_event_cqe_notify *event,
							      uint16_t *pel);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_EVENT_H_ */
