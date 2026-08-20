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
 * @file doca_devemu_vnet.h
 * @page doca_devemu_vnet
 * @defgroup VNET Vnet library
 *
 * DOCA library for emulated Virtio Network devices
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VNET_H_
#define DOCA_DEVEMU_VNET_H_

#include <stdint.h>

#include <linux/virtio_net.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing emulated Virtio Net pci device.
 * This structure extends the core doca_devemu_virtio_dev structure.
 * This structure is used by Virtio Net device emulation applications and services.
 */
struct doca_devemu_vnet_dev;

/**
 * @brief Opaque structure representing emulated Virtio Net pci device type.
 * This structure extends the core doca_devemu_virtio_type structure.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_vnet_type;

/**
 * @brief Opaque structure representing emulated Virtio Net IO context.
 * This structure extends the core doca_devemu_virtio_io structure.
 * This structure is used by Virtio Net device emulation applications and services.
 */
struct doca_devemu_vnet_io;

/**
 * @brief Opaque structure representing Virtio Net control request.
 * This structure is used by Virtio Net device emulation applications and services.
 */
struct doca_devemu_vnet_ctrl_req;

/**
 * @brief Opaque structure representing emulated Virtio Net offload engine.
 * This structure extends the core doca_devemu_virtio_offload_engine structure.
 * This structure is used by Virtio Net device emulation applications and services.
 */
struct doca_devemu_vnet_offload_engine;

/**
 * @brief Opaque structure representing Virtio Net request virtqueue.
 * This structure is used by Virtio Net device emulation applications and services.
 */
struct doca_devemu_vnet_ctrl_vq;

/**
 * @brief Opaque structure representing Virtio Net receive virtqueue.
 * This structure is used by Virtio Net device emulation applications and services.
 */
struct doca_devemu_vnet_rx_vq;

/**
 * @brief Opaque structure representing Virtio Net transmit virtqueue.
 * This structure is used by Virtio Net device emulation applications and services.
 */
struct doca_devemu_vnet_tx_vq;

/*********************************************************************************************************************
 * DOCA devemu Virtio Net configuration API
 *********************************************************************************************************************/

/**
 * @brief Initialize the DOCA devemu Virtio Net.
 *
 * This is the global initialization function for DOCA devemu Virtio Net.
 * Must be invoked before creating Virtio Net objects.
 * This is a one time call, used for initialization and global configurations.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_init(void);

/**
 * @brief Teardown the DOCA devemu Virtio Net.
 *
 * Release all the resources initialized by doca_devemu_vnet_init().
 * Must be invoked at the teardown stage of the Virtio Net device emulation application, before it exits.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_teardown(void);

/**
 * @brief Register a DOCA device with the DOCA devemu Virtio Net for management.
 *
 * @details The DOCA device must support the DOCA Virtio Net type management. If unsupported, subsequent call to
 * doca_devemu_vnet_init() will fail. Once successfully added, and after doca_devemu_vnet_init(), the DOCA device can be
 * associated with a valid Virtio Net type.
 *
 * @param [in] dev
 * The DOCA device instance to be registered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_add_dev(struct doca_dev *dev);

/**
 * @brief Remove a DOCA device from the DOCA devemu Virtio Net.
 *
 * @details The DOCA device must have been previously added via doca_devemu_vnet_add_dev(). After removal, the
 * device will no longer be associated with the Virtio Net, and subsequent initialization, using
 * doca_devemu_vnet_init() will not include it.
 *
 * @param [in] dev
 * The DOCA device instance to be removed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_rm_dev(struct doca_dev *dev);

/**
 * @brief Get the size of the user data buffer that will be allocated for each doca_devemu_vnet_ctrl_req on behalf of
 * the user. This buffer will be valid and used by the user upon receiving new doca_devemu_vnet_ctrl_req. The buffer
 * will become invalid after doca_devemu_vnet_ctrl_req completion.
 *
 * @param [out] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vnet_ctrl_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'req_user_data_size' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_get_vnet_ctrl_req_user_data_size(uint32_t *req_user_data_size);

/**
 * @brief Set the size of the user data buffer that will be allocated for each doca_devemu_vnet_ctrl_req on behalf of
 * the user. This buffer will be valid and available to the user upon receiving new doca_devemu_vnet_ctrl_req. The
 * buffer will become invalid after doca_devemu_vnet_ctrl_req completion. If called multiple times, only the last call
 * before doca_devemu_vnet_init() takes effect.
 *
 * @param [in] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vnet_ctrl_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vnet_set_vnet_ctrl_req_user_data_size(uint32_t req_user_data_size);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VNET_H_ */
