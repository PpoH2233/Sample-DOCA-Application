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

#ifndef DOCA_STA_IO_QP_H_
#define DOCA_STA_IO_QP_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>

#include <doca_sta_handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rdma_cm_id;
struct doca_dev;
struct doca_sta_io;
union doca_data;

/**
 * @brief Allocate an empty STA queue pair (QP).
 * The QP can be used later to accept a remote connection represented by cm_id.
 *
 * @param [in] sta_io
 * Previously created STA IO context
 * @param [in] dev
 * The device to attach to the STA QP
 * @param [out] qp_handle
 * Pointer that will be set to the STA QP handle
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_alloc(struct doca_sta_io *sta_io,
				  const struct doca_dev *dev,
				  struct doca_sta_qp_handle **qp_handle);

/**
 * @brief Accept a new remote connection.
 *
 * This function accepts (connects) a remote QP represented by cm_id to the previously
 * allocated STA QP.
 *
 * @param [in] sta_io
 * Previously created STA IO context
 * @param [in] qp_handle
 * Previously allocated STA QP
 * @param [in] dev
 * The device to attach to the STA QP
 * @param [in] cm_id
 * RDMA CM ID object that carries the remote connection details
 * @param [in] priv_data
 * The private data to send with the accept message
 * @param [in] priv_size
 * Size in bytes of the private data being sent
 * @param [in] subs_handle
 * Subsystem details
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_accept(struct doca_sta_io *sta_io,
				   struct doca_sta_qp_handle *qp_handle,
				   const struct doca_dev *dev,
				   struct rdma_cm_id *cm_id,
				   const void *priv_data,
				   uint32_t priv_size,
				   struct doca_sta_subs_handle *subs_handle);

/**
 * @brief Create a newly connected STA queue pair (QP).
 *
 * This function connects a remote QP represented by cm_id to a newly created
 * STA QP, which represents an offloaded QP.
 *
 * @param [in] sta_io
 * Previously created STA IO context
 * @param [in] dev
 * The device to attach to the STA QP
 * @param [in] cm_id
 * RDMA CM ID object that carries the remote connection details
 * @param [in] priv_data
 * The private data to send with the connect message
 * @param [in] priv_size
 * Size in bytes of the private data being sent
 * @param [in] subs_handle
 * Subsystem details
 * @param [out] qp_handle
 * Pointer that will be set to the STA QP handle
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_connect(struct doca_sta_io *sta_io,
				    const struct doca_dev *dev,
				    struct rdma_cm_id *cm_id,
				    const void *priv_data,
				    uint32_t priv_size,
				    struct doca_sta_subs_handle *subs_handle,
				    struct doca_sta_qp_handle **qp_handle);

/**
 * @brief Set the user data for the STA QP.
 *
 * @param [in] sta_io
 * Previously created STA IO context
 * @param [in] qp_handle
 * STA QP handle
 * @param [in] user_data
 * User data to set for the STA QP
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_set_user_data(struct doca_sta_io *sta_io,
					  struct doca_sta_qp_handle *qp_handle,
					  union doca_data user_data);

/**
 * @brief This method retrieves user data from a STA QP.
 *
 * @param [in] qp_handle
 * STA QP handle
 * @param [out] user_data
 * Pointer that will be set to the user data from the STA QP
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_get_user_data(const struct doca_sta_qp_handle *qp_handle, union doca_data *user_data);

/**
 * @brief Set the maximum size of the submission queue for the STA QP.
 * This should be called while handling the fabric connect command, but before completing it.
 *
 * @param [in] sta_io
 * Previously created STA IO context
 * @param [in] qp_handle
 * STA QP handle
 * @param [in] sq_size
 * Submission queue size
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_connect_set_sq_size(struct doca_sta_io *sta_io,
						struct doca_sta_qp_handle *qp_handle,
						uint16_t sq_size);

/**
 * @brief Mark the local QP as connected.
 * This must be called when both local and remote QPs are connected,
 * for example upon receiving the RDMA_CM_EVENT_ESTABLISHED event.
 *
 * @param [in] sta_io
 * Previously created STA IO context
 * @param [in] qp_handle
 * STA QP handle
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_connect_established(struct doca_sta_io *sta_io, struct doca_sta_qp_handle *qp_handle);

/**
 * @brief Destroy a previously created queue pair (QP).
 *
 * @param [in] sta_io
 * Previously created STA IO context
 * @param [in] qp_handle
 * STA QP handle
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_destroy(struct doca_sta_io *sta_io, struct doca_sta_qp_handle *qp_handle);

/**
 * @brief Set the subsystem to which the queue pair (QP) belongs.
 *
 * @param [in] sta_io
 * Previously created STA IO context
 * @param [in] subs_handle
 * Subsystem handle
 * @param [in] qp_handle
 * STA QP handle
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_add_subsystem(struct doca_sta_io *sta_io,
					  const struct doca_sta_subs_handle *subs_handle,
					  struct doca_sta_qp_handle *qp_handle);

/**
 * @brief Retrieve the queue pair ID (QPN) using the given QP handle.
 *
 * @param [in] qp_handle
 * STA QP handle
 * @param [out] qp_id
 * Pointer that will be set to the QP ID (QPN)
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_get_id(struct doca_sta_qp_handle *qp_handle, uint32_t *qp_id);

/**
 * @brief Retrieve the port ID using the given QP handle.
 *
 * @param [in] qp_handle
 * STA QP handle
 * @param [out] port_id
 * Pointer that will be set to the port ID
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_get_port_id(struct doca_sta_qp_handle *qp_handle, uint16_t *port_id);

/**
 * @brief Retrieve the group ID (completion group) to which the queue pair (QP) belongs.
 *
 * @param [in] qp_handle
 * STA QP handle
 * @param [out] group_id
 * Pointer that will be set to the group ID
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_get_group_id(struct doca_sta_qp_handle *qp_handle, uint16_t *group_id);

/**
 * @brief Retrieve the queue pair (QP) index within its group.
 *
 * @param [in] qp_handle
 * STA QP handle
 * @param [out] qp_idx
 * Pointer that will be set to the QP index within the group
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_io_qp_get_index_in_group(struct doca_sta_qp_handle *qp_handle, uint16_t *qp_idx);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_IO_QP_H_ */
