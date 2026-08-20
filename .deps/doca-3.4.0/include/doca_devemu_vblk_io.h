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
 * @file doca_devemu_vblk_io.h
 * @page doca_devemu_vblk_io
 * @defgroup DOCA_DEVEMU_VBLK_IO DOCA Device Emulation - Virtio Block IO Context
 * @ingroup DOCA_DEVEMU_VBLK
 *
 * DOCA Virtio Block IO context
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VBLK_IO_H_
#define DOCA_DEVEMU_VBLK_IO_H_

#include <stdint.h>

#include <doca_buf.h>
#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>
#include <doca_devemu_virtio_io.h>
#include <doca_devemu_virtio_tlp.h>
#include <doca_devemu_vblk.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA devemu Virtio Block IO context API
 *********************************************************************************************************************/

/**
 * @brief Allocate Virtio Block IO context for a DOCA Virtio Block offload engine.
 *
 * This function must be invoked on the ARM core that manages the IO context.
 *
 * @details The responsibility of the Virtio Block IO context is to relay the requests arriving from the device driver
 * towards the Virtio Block services and applications. Additionally, it is responsible for relaying the completions
 * arriving from the Virtio Block services and applications towards the device driver. Each Virtio Block IO context is
 * associated with a single DOCA Virtio Block offload engine.
 *
 * @param [in] offload
 * DOCA Virtio Block offload engine. Must be started.
 * @param [out] io
 * The created DOCA Virtio Block IO context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_io_create_from_offload_engine(struct doca_devemu_vblk_offload_engine *offload,
							    struct doca_devemu_vblk_io **io);

/**
 * @brief Free a Virtio Block IO context.
 *
 * @param [in] io
 * The DOCA Virtio Block IO context to release. Must be idle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' is NULL
 * - DOCA_ERROR_BAD_STATE - IO context is not idle. Use doca_ctx_stop() to stop it
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_io_destroy(struct doca_devemu_vblk_io *io);

/**
 * @brief Convert DOCA Virtio Block IO context instance into DOCA context.
 *
 * @param [in] io
 * DOCA Virtio Block IO context instance. This must remain valid until after the DOCA context is no longer required.
 *
 * @return
 * doca ctx upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_devemu_vblk_io_as_ctx(struct doca_devemu_vblk_io *io);

/**
 * @brief Convert DOCA Virtio Block IO context instance into DOCA Virtio IO context.
 *
 * @param [in] io
 * DOCA Virtio Block IO context instance. This must remain valid until after the DOCA Virtio IO context is
 * no longer required.
 *
 * @return
 * doca devemu virtio device io context upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_virtio_io *doca_devemu_vblk_io_as_virtio_io(struct doca_devemu_vblk_io *io);

/*********************************************************************************************************************
 * DOCA devemu Virtio Block IO context events API
 *********************************************************************************************************************/

/**
 * @brief Function to be executed on vblk_req event occurrence. The Ownership of the doca_devemu_vblk_req and the
 * req_user_data moves from doca_devemu_vblk_io ctx to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] type
 * The type of the Virtio Block request as was written by the driver.
 * @param [in] sector
 * The sector number of the Virtio Block request as was written by the driver.
 * @param [in] req_user_data
 * The user data associated to the request.
 */
typedef void (*doca_devemu_vblk_io_event_vblk_req_handler_cb_t)(struct doca_devemu_vblk_req *req,
								uint32_t type,
								uint64_t sector,
								void *req_user_data);

/**
 * @brief Register to Virtio Block request notifications.
 *
 * Registration can be done only while IO ctx is idle. If called multiple times then only the last call will take
 * effect.
 *
 * @param [in] io
 * The DOCA Virtio Block IO context to be associated with the event. Must be idle.
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
doca_error_t doca_devemu_vblk_io_event_vblk_req_register(struct doca_devemu_vblk_io *io,
		doca_devemu_vblk_io_event_vblk_req_handler_cb_t handler);

/*********************************************************************************************************************
 * DOCA devemu Virtio Block request API
 *********************************************************************************************************************/

/**
 * @brief Complete the Virtio Block request. The Request ownership (including the associated datain, dataout and
 * req_user_data) moves from the user back to the associated IO context. The associated IO context will complete the
 * request towards the device driver according to the virtio fs specification.
 *
 * @param [in] req
 * The Virtio Block request to complete.
 * @param [in] status
 * The status value to be written to the driver, according to the Virtio specification.
 * @param [in] len
 * The number of bytes written into the device writable portion of the buffer described by the req.
 *
 */
DOCA_EXPERIMENTAL
void doca_devemu_vblk_req_complete(struct doca_devemu_vblk_req *req, uint8_t status, uint32_t len);

/**
 * @brief Get the associated DOCA Virtio Block IO context.
 *
 * @param [in] req
 * The Virtio Block request to query. Must not be NULL.
 *
 * @return
 * The DOCA Virtio Block IO context associated to the request.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_vblk_io *doca_devemu_vblk_req_get_vblk_io(struct doca_devemu_vblk_req *req);

/**
 * @brief Get the user data associated with the virtqueue for a given Virtio Block request.
 *
 * This function must be invoked on the same thread that manages the associated IO context.
 *
 * @param [in] req
 * The Virtio Block request to query. Must not be NULL.
 *
 * @return
 * The user data associated with the virtqueue that is bound to the IO context handling this request.
 */
DOCA_EXPERIMENTAL
void *doca_devemu_vblk_req_get_vq_user_data(struct doca_devemu_vblk_req *req);

/**
 * @brief Get the doca buffer representing the data[] field of the Virtio Block request.
 *
 * This function should be issued during scheduling the request towards the execution context that will be handling
 * the doca request.
 *
 * @param [in] req
 * The Virtio Block request to query.
 *
 * @return
 * The doca buffer representing the host memory for the virtio_blk_req::(data), according to the virtio specification,
 * associated to the request on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_devemu_vblk_req_get_data(struct doca_devemu_vblk_req *req);

/**
 * @brief Get the number of elements in the original doca buffer linked list associated with the data[] field of the
 * Virtio Block request returned by doca_devemu_vblk_req_get_data().
 *
 * @param [in] req
 * The Virtio Block request to query. Must not be NULL.
 *
 * @return
 * Number of elements in the original doca buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vblk_req_get_data_list_len(struct doca_devemu_vblk_req *req);

/**
 * @brief Get the total data length of the original doca buffer linked list associated with the data[] field of the
 * Virtio Block request returned by doca_devemu_vblk_req_get_data().
 *
 * @param [in] req
 * The Virtio Block request to query. Must not be NULL.
 *
 * @return
 * The total data length (in bytes) of all elements in the original DOCA buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vblk_req_get_data_len(struct doca_devemu_vblk_req *req);


#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VBLK_IO_H_ */
