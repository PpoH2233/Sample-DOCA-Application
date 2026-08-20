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
 * @file doca_devemu_vnet_io.h
 * @page doca_devemu_vnet_io
 * @defgroup DOCA_DEVEMU_VNET_IO DOCA Device Emulation - Virtio Network IO Context
 * @ingroup DOCA_DEVEMU_VNET
 *
 * DOCA Virtio Network IO context
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VNET_IO_H_
#define DOCA_DEVEMU_VNET_IO_H_

#include <stdint.h>

#include <doca_buf.h>
#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>
#include <doca_devemu_virtio_io.h>
#include <doca_devemu_vnet.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA devemu Virtio Network IO context API
 *********************************************************************************************************************/

/**
 * @brief Allocate Virtio Network IO context for a DOCA Virtio Network offload engine.
 *
 * @details The responsibility of the Virtio Network IO context is to relay the requests arriving from the device driver
 * towards the Virtio Network services and applications. Additionally, it is responsible for relaying the completions
 * arriving from the Virtio Network services and applications towards the device driver. Each Virtio Network IO context is
 * associated with a single DOCA Virtio Network offload engine.
 *
 * @param [in] offload
 * DOCA Virtio Network offload engine. Must be started.
 * @param [out] io
 * The created DOCA Virtio Network IO context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_io_create_from_offload_engine(struct doca_devemu_vnet_offload_engine *offload,
							    struct doca_devemu_vnet_io **io);

/**
 * @brief Free a Virtio Network IO context.
 *
 * @param [in] io
 * The DOCA Virtio Network IO context to release. Must be idle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' is NULL
 * - DOCA_ERROR_BAD_STATE - IO context is not idle. Use doca_ctx_stop() to stop it
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_io_destroy(struct doca_devemu_vnet_io *io);

/**
 * @brief Convert DOCA Virtio Network IO context instance into DOCA context.
 *
 * @param [in] io
 * DOCA Virtio Network IO context instance. This must remain valid until after the DOCA context is no longer required.
 *
 * @return
 * doca ctx upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_devemu_vnet_io_as_ctx(struct doca_devemu_vnet_io *io);

/**
 * @brief Convert DOCA Virtio Network IO context instance into DOCA Virtio IO context.
 *
 * @param [in] io
 * DOCA Virtio Network IO context instance. This must remain valid until after the DOCA Virtio IO context is
 * no longer required.
 *
 * @return
 * doca devemu virtio device io context upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_virtio_io *doca_devemu_vnet_io_as_virtio_io(struct doca_devemu_vnet_io *io);

/*********************************************************************************************************************
 * DOCA devemu Virtio Network IO context events API
 *********************************************************************************************************************/

/**
 * @brief Function to be executed on vnet_ctrl_req event occurrence. The Ownership of the doca_devemu_vnet_ctrl_req and
 * the req_user_data moves from doca_devemu_vnet_io ctx to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] cls
 * The class of the Virtio Network control request as was written by the driver.
 * @param [in] cmd
 * The command of the Virtio Network control request as was written by the driver.
 * @param [in] req_user_data
 * The user data associated to the request.
 */
typedef void (*doca_devemu_vnet_io_event_vnet_ctrl_req_handler_cb_t)(struct doca_devemu_vnet_ctrl_req *req,
								     uint8_t cls,
								     uint8_t cmd,
								     void *req_user_data);

/**
 * @brief Register to Virtio Network control request notifications.
 *
 * Registration can be done only while IO ctx is idle. If called multiple times then only the last call will take
 * effect.
 *
 * @param [in] io
 * The DOCA Virtio Network IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'io' or 'handler' is NULL
 * - DOCA_ERROR_BAD_STATE - IO is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_io_event_vnet_ctrl_req_register(struct doca_devemu_vnet_io *io,
		doca_devemu_vnet_io_event_vnet_ctrl_req_handler_cb_t handler);

/*********************************************************************************************************************
 * DOCA devemu Virtio Network control request API
 *********************************************************************************************************************/

/**
 * @brief Complete the Virtio Network control request. The Request ownership (including all the associated buffers and
 * the req_user_data) moves from the user back to the associated IO context. The associated IO context will complete
 * the request towards the device driver according to the Virtio specification.
 *
 * @param [in] req
 * The Virtio Network control request to complete.
 * @param [in] ack
 * The ack value to be written to the driver, according to the Virtio specification.
 * @param [in] len
 * The number of bytes written into the device writable portion of the buffer described by the req.
 *
 */
DOCA_EXPERIMENTAL
void doca_devemu_vnet_ctrl_req_complete(struct doca_devemu_vnet_ctrl_req *req, uint8_t ack, uint32_t len);

/**
 * @brief Get the associated DOCA Virtio Network IO context.
 *
 * @param [in] req
 * The Virtio Network control request to query. Must not be NULL.
 *
 * @return
 * The DOCA Virtio Network IO context associated to the request.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_vnet_io *doca_devemu_vnet_ctrl_req_get_vnet_io(struct doca_devemu_vnet_ctrl_req *req);

/**
 * @brief Get the user data associated with the virtqueue for a given Virtio Network control request.
 *
 * This function must be invoked on the same thread that manages the associated IO context.
 *
 * @param [in] req
 * The Virtio Network control request to query. Must not be NULL.
 *
 * @return
 * The user data associated with the virtqueue that is bound to the IO context handling this request.
 */
DOCA_EXPERIMENTAL
void *doca_devemu_vnet_ctrl_req_get_vq_user_data(struct doca_devemu_vnet_ctrl_req *req);

/**
 * @brief Get the doca buffer representing the command-specific-data[] field of the Virtio Network control request.
 *
 * @param [in] req
 * The Virtio Network control request to query.
 *
 * @return
 * The doca buffer representing the host memory for the virtio_net_ctrl::(command-specific-data), according to the
 * Virtio specification, associated to the request on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_devemu_vnet_ctrl_req_get_data(struct doca_devemu_vnet_ctrl_req *req);

/**
 * @brief Get the number of elements in the original doca buffer linked list associated with the
 * command-specific-data[] field of the Virtio Network control request returned by
 * doca_devemu_vnet_ctrl_req_get_data().
 *
 * @param [in] req
 * The Virtio Network control request to query. Must not be NULL.
 *
 * @return
 * Number of elements in the original doca buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vnet_ctrl_req_get_data_list_len(struct doca_devemu_vnet_ctrl_req *req);

/**
 * @brief Get the total length of the original doca buffer linked list associated with the command-specific-data[]
 * field of the Virtio Network control request returned by doca_devemu_vnet_ctrl_req_get_data().
 *
 * @param [in] req
 * The Virtio Network control request to query. Must not be NULL.
 *
 * @return
 * The total length (in bytes) of all elements in the original DOCA buffer linked list. Valid only if the request
 * is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vnet_ctrl_req_get_data_len(struct doca_devemu_vnet_ctrl_req *req);

/**
 * @brief Get the doca buffer representing the command-specific-result[] field of the Virtio Network control request.
 *
 * @param [in] req
 * The Virtio Network control request to query.
 *
 * @return
 * The doca buffer representing the host memory for the virtio_net_ctrl::(command-specific-result), according to the
 * Virtio specification, associated to the request on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_devemu_vnet_ctrl_req_get_result(struct doca_devemu_vnet_ctrl_req *req);

/**
 * @brief Get the number of elements in the original doca buffer linked list associated with the
 * command-specific-result[] field of the Virtio Network control request returned by
 * doca_devemu_vnet_ctrl_req_get_result().
 *
 * @param [in] req
 * The Virtio Network control request to query. Must not be NULL.
 *
 * @return
 * Number of elements in the original doca buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vnet_ctrl_req_get_result_list_len(struct doca_devemu_vnet_ctrl_req *req);

/**
 * @brief Get the total length of the original doca buffer linked list associated with the command-specific-result[]
 * field of the Virtio Network control request returned by doca_devemu_vnet_ctrl_req_get_result().
 *
 * @param [in] req
 * The Virtio Network control request to query. Must not be NULL.
 *
 * @return
 * The total length (in bytes) of all elements in the original DOCA buffer linked list. Valid only if the request
 * is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vnet_ctrl_req_get_result_len(struct doca_devemu_vnet_ctrl_req *req);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VNET_IO_H_ */
