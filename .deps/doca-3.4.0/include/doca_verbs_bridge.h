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
 * @file doca_verbs_bridge.h
 * @page DOCA_VERBS_BRIDGE
 * @defgroup DOCA_VERBS_BRIDGE DOCA Verbs Bridge
 * @ingroup DOCA_VERBS
 * DOCA Verbs library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_VERBS_BRIDGE_H_
#define DOCA_VERBS_BRIDGE_H_

#include <doca_error.h>
#include <doca_types.h>

#include "doca_verbs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 * IB-Verbs opaque types
 *********************************************************************************************************************/
struct ibv_qp_attr;
struct ibv_context;
struct ibv_device;
struct ibv_pd;
struct ibv_recv_wr;
struct ibv_wc;
struct ibv_send_wr;

/*********************************************************************************************************************
 * DOCA Verbs Bridge
 *********************************************************************************************************************/

/**
 * @brief Create a DOCA Verbs Context instance from ibv_device.
 *
 * @param [in] ibv_dev
 * Pointer to ibv_device.
 * @param [in] flags
 * Create flags. @see define DOCA_VERBS_CONTEXT_CREATE_FLAGS_*
 * @param [out] verbs_context
 * Pointer to pointer to be set to point to the created verbs_context instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_NOT_CONNECTED - failed to open doca_verbs_context for the provided ibv_dev
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_bridge_verbs_context_create(struct ibv_device *ibv_dev,
						    uint32_t flags,
						    struct doca_verbs_context **verbs_context);

/**
 * @brief Import an ibv_ctx and create a DOCA Verbs Context from it.
 *
 * @param [in] ibv_ctx
 * Pointer to ibv_context instance.
 * @param [in] flags
 * Create flags. @see define DOCA_VERBS_CONTEXT_CREATE_FLAGS_*
 * @param [out] verbs_context
 * Pointer to pointer to be set to point to the created verbs_context instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_NOT_CONNECTED - failed to create doca_verbs_context from the provided ibv_ctx
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_bridge_verbs_context_import(struct ibv_context *ibv_ctx,
						    uint32_t flags,
						    struct doca_verbs_context **verbs_context);

/**
 * @brief Get the default doca_dev from a doca_verbs_context.
 *
 * @details This function will always return the same doca_dev. The function increments the ref count of the doca_dev,
 * expecting the user to call doca_dev_close when the doca_dev is no longer needed.
 *
 * @param [in] verbs_context
 * doca_verbs_context to get the default doca_dev from
 * @param [out] dev
 * Default doca_dev
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_bridge_verbs_context_get_default_doca_dev(const struct doca_verbs_context *verbs_context,
								  struct doca_dev **dev);

/**
 * @brief Query DOCA Verbs device attributes from ibv_context.
 *
 * @details The attributes reflect the device state at the time of the call to this function. Some attributes may change
 * during run-time and will not be reflected unless doca_verbs_bridge_verbs_query_device is called again.
 *
 * @param [in] ibv_ctx
 * Pointer to ibv_context instance.
 * @param [out] verbs_device_attr
 * Pointer to pointer to be set to point to the created doca_verbs_device_attr instance.
 * User is expected to free this object with "doca_verbs_device_attr_free()".
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_NOT_CONNECTED - failed to obtain doca_verbs_device_attr for the provided ibv_ctx.
 * - DOCA_ERROR_NOT_DRIVER - low level layer failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_bridge_verbs_query_device(struct ibv_context *ibv_ctx,
						  struct doca_verbs_device_attr **verbs_device_attr);

/**
 * @brief Import an ibv_pd and create a DOCA Verbs PD instance from it.
 *
 * @param [in] pd
 * Pointer to ibv_pd instance.
 * @param [out] verbs_pd
 * Pointer to pointer to be set to point to the created verbs_pd instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_bridge_verbs_pd_import(struct ibv_pd *pd, struct doca_verbs_pd **verbs_pd);

/**
 * @brief Get ibv_pd from doca_verbs_pd.
 *
 * @param [in] verbs_pd
 * Pointer to verbs_pd instance.
 *
 * @return
 * ibv_pd from the verbs_pd.
 */
DOCA_EXPERIMENTAL
struct ibv_pd *doca_verbs_bridge_verbs_pd_get_ibv_pd(const struct doca_verbs_pd *verbs_pd);

/**
 * @brief Get ibv_context from doca_verbs_context
 *
 * @param [in] verbs_context
 * Pointer to doca_verbs_context instance
 *
 * @return
 * ibv_context from verbs_context.
 */
DOCA_EXPERIMENTAL
struct ibv_context *doca_verbs_bridge_get_ibv_ctx(struct doca_verbs_context *verbs_context);

/**
 * @brief Populate verbs_qp_attr instance with the attributes from the given ibv_qp_attr, according to the given
 * attributes mask.
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @param [in] ibv_attr
 * Pointer to ibv_attr instance.
 *
 * @param [in] attr_mask
 * The ibv_attr mask.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_bridge_qp_attr_set(struct doca_verbs_qp_attr *verbs_qp_attr,
					   const struct ibv_qp_attr *ibv_attr,
					   int attr_mask);

/**
 * @brief Populate ibv_qp_attr with the attributes from the given verbs_qp_attr instance.
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @param [out] ibv_attr
 * Pointer to ibv_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_bridge_qp_attr_get(const struct doca_verbs_qp_attr *verbs_qp_attr,
					   struct ibv_qp_attr *ibv_attr);

/**
 * @brief Get DOCA QP attributes mask from ibv QP attributes mask.
 *
 * @param [in] ibv_mask
 * ibv_qp_attr_mask mask.
 *
 * @return
 * Verbs QP attributes mask. see define for DOCA_VERBS_QP_ATTR_*
 */
DOCA_EXPERIMENTAL
int doca_verbs_bridge_get_qp_attr_mask(int ibv_mask);

/**
 * @brief Poll number of entries from CQ.
 *
 * @param [in] verbs_cq
 * Pointer to doca_verbs_cq instance.
 * @param [in] num_entries
 * Maximum number of completions to return.
 * @param [out] ibv_wc
 * Array of at least num_entries of ibv_wc instances where completions will be returned.
 *
 * @return
 * Returns value < 0 in case of error.
 * Otherwise returns value >= 0 representing number of returned completions.
 * @note In case 0 <= return value < num_entries then CQ was emptied.
 */
DOCA_EXPERIMENTAL
int doca_verbs_bridge_poll_cq(struct doca_verbs_cq *verbs_cq, int num_entries, struct ibv_wc *ibv_wc);

/**
 * @brief Post a list of work requests to a receive queue.
 *
 * @param [in] verbs_qp
 * Pointer to doca_verbs_qp instance.
 * @param [in] wr
 * List of receive work requests.
 * @param [out] bad_wr
 * In case of error points to bad work request.
 *
 * @return
 * 0 in case of success, and the value of errno otherwise.
 */
DOCA_EXPERIMENTAL
int doca_verbs_bridge_post_recv(struct doca_verbs_qp *verbs_qp, struct ibv_recv_wr *wr, struct ibv_recv_wr **bad_wr);

/**
 * @brief Post a list of work requests to a send queue.
 *
 * @param [in] verbs_qp
 * Pointer to doca_verbs_qp instance.
 * @param [in] wr
 * List of send work requests.
 * @param [out] bad_wr
 * In case of error points to bad work request.
 *
 * @return
 * 0 in case of success, and the value of errno otherwise.
 */
DOCA_EXPERIMENTAL
int doca_verbs_bridge_post_send(struct doca_verbs_qp *verbs_qp, struct ibv_send_wr *wr, struct ibv_send_wr **bad_wr);

/**
 * @brief Get the current consumer index of the given CQ.
 *
 * @param [in] verbs_cq
 * Pointer to doca_verbs_cq instance.
 * @return
 * The current consumer index of the CQ.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_bridge_get_current_cq_ci(struct doca_verbs_cq *verbs_cq);

/**
 * @brief Set the work request for waiting on a CQ for a specific producer index.
 * The wait WR will complete after the CQ PI passes the given value. This allows forcing a QP to wait for other
 * WRs to complete. After setting the WR it can be submitted using doca_verbs_bridge_post_send().
 * The WC for this type of WR will have the opcode IBV_WC_DRIVER1.
 * This WR can only be used if the waiting QP and the target CQ are using the same UAR.
 *
 * @param [in] cq
 * Pointer to doca_verbs_cq instance to wait on.
 * @param [in] wait_pi
 * PI value to wait for.
 * @param [in] wr
 * work request object to set.
 *
 * @return
 * 0 in case of success, and the value of errno otherwise.
 */
DOCA_EXPERIMENTAL
int doca_verbs_bridge_set_wait_cq_pi_wr(struct doca_verbs_cq *cq, uint32_t wait_pi, struct ibv_send_wr *wr);

/**
 * @brief Post a list of work requests to a Shared Receive Queue.
 *
 * @note Currently, bridge datapath is supported only for linked-list type
 *
 * @param [in] verbs_srq
 * Pointer to doca_verbs_srq instance.
 * @param [in] wr
 * List of receive work requests.
 * @param [out] bad_wr
 * In case of error points to bad work request.
 *
 * @return
 * 0 in case of success, and the value of errno otherwise.
 */
DOCA_EXPERIMENTAL
int doca_verbs_bridge_post_srq_recv(struct doca_verbs_srq *verbs_srq,
				    struct ibv_recv_wr *wr,
				    struct ibv_recv_wr **bad_wr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_VERBS_BRIDGE_H_ */

/** @} */
