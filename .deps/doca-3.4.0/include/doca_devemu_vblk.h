/*
 * Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_devemu_vblk.h
 * @page doca_devemu_vblk
 * @defgroup VBLK Vblk library
 *
 * DOCA library for emulated virtio block devices
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VBLK_H_
#define DOCA_DEVEMU_VBLK_H_

#include <stdint.h>
#include <stdbool.h>

#include <linux/virtio_blk.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing emulated Virtio Block IO context.
 * This structure extends the core doca_devemu_virtio_io structure.
 * This structure is used by Virtio Block device emulation applications and services.
 */
struct doca_devemu_vblk_io;

/**
 * @brief Opaque structure representing Virtio Block request.
 * This structure is used by Virtio Block device emulation applications and services.
 */
struct doca_devemu_vblk_req;

/**
 * @brief Opaque structure representing emulated Virtio Block offload engine.
 * This structure extends the core doca_devemu_virtio_offload_engine structure.
 * This structure is used by Virtio Block device emulation applications and services.
 */
struct doca_devemu_vblk_offload_engine;

/**
 * @brief Opaque structure representing Virtio Block request virtqueue.
 * This structure is used by Virtio Block device emulation applications and services.
 */
struct doca_devemu_vblk_req_vq;

/*********************************************************************************************************************
 * DOCA devemu Virtio Block configuration API
 *********************************************************************************************************************/

/**
 * @brief Initialize the DOCA devemu Virtio Block.
 *
 * This is the global initialization function for DOCA devemu Virtio Block.
 * Must be invoked before creating Virtio Block devices, functions and IO context's.
 * This is a one time call, used for initialization and global configurations.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_init(void);

/**
 * @brief Teardown the DOCA devemu Virtio Block.
 *
 * Release all the resources initialized by doca_devemu_vblk_init().
 * Must be invoked at the teardown stage of the Virtio Block device emulation application, before it exits.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_teardown(void);

/**
 * @brief Register a DOCA device with the DOCA devemu Virtio Block for management.
 *
 * @details The DOCA device must support the DOCA Virtio Block type management. If unsupported, subsequent call to
 * doca_devemu_vblk_init() will fail. Once successfully added, and after doca_devemu_vblk_init(), the DOCA device can be
 * associated with a valid Virtio Block type.
 *
 * @param [in] dev
 * The DOCA device instance to be registered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_add_dev(struct doca_dev *dev);

/**
 * @brief Remove a DOCA device from the DOCA devemu Virtio Block.
 *
 * @details The DOCA device must have been previously added via doca_devemu_vblk_add_dev(). After removal, the
 * device will no longer be associated with the Virtio Block, and subsequent initialization, using
 * doca_devemu_vblk_init() will not include it.
 *
 * @param [in] dev
 * The DOCA device instance to be removed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_rm_dev(struct doca_dev *dev);

/**
 * @brief Get the size of the user data buffer that will be allocated for each doca_devemu_vblk_req on behalf of the
 * user. This buffer will be valid and used by the user upon receiving new doca_devemu_vblk_req. The buffer will become
 * invalid after doca_devemu_vblk_req completion.
 *
 * @param [out] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vblk_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'req_user_data_size' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_get_vblk_req_user_data_size(uint32_t *req_user_data_size);

/**
 * @brief Set the size of the user data buffer that will be allocated for each doca_devemu_vblk_req on behalf of the
 * user. This buffer will be valid and available to the user upon receiving new doca_devemu_vblk_req. The buffer will
 * become invalid after doca_devemu_vblk_req completion. If called multiple times, only the last call before
 * doca_devemu_vblk_init() takes effect.
 *
 * @param [in] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vblk_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_set_vblk_req_user_data_size(uint32_t req_user_data_size);

/**
 * @brief Set the maximum queue size for DOCA devemu Virtio Block VQs.
 *
 * @details Sets the maximum queue size allowed for DOCA devemu Virtio Block virtqueues. The value must
 * conform to doca_devemu_vblk_cap_get_max_queue_size(). If called multiple
 * times, only the last call made before doca_devemu_vblk_init() takes effect.
 *
 * @param [in] size
 * The maximum queue size to set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_NOT_PERMITTED - 'size' does not conform to
 *   doca_devemu_vblk_cap_get_max_queue_size().
 * - DOCA_ERROR_NOT_SUPPORTED - If called after doca_devemu_vblk_init().
 * - DOCA_ERROR_INVALID_VALUE - 'size' does not conform to the Virtio
 *   specification.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_set_max_queue_size(uint16_t size);

/**
 * @brief Get the maximum queue size for DOCA devemu Virtio Block VQs.
 *
 * @param [out] size
 * The maximum queue size.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_get_max_queue_size(uint16_t *size);

/**
 * @brief Set the maximum number of segments that can be used in a single request for DOCA
 * devemu Virtio Block VQs.
 *
 * @details Sets the maximum number of segments that can be used in a single request for DOCA
 * devemu Virtio Block virtqueues. The value must conform to doca_devemu_vblk_cap_get_max_seg_max().
 * If called multiple times, only the last call made before doca_devemu_vblk_init() takes effect.
 *
 * @param [in] seg_max
 * The maximum number of segments to set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_NOT_PERMITTED - 'seg_max' does not conform to
 *   doca_devemu_vblk_cap_get_max_seg_max().
 * - DOCA_ERROR_NOT_SUPPORTED - If called after doca_devemu_vblk_init().
 * - DOCA_ERROR_INVALID_VALUE - 'seg_max' does not conform to the Virtio
 *   specification.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_set_seg_max(uint16_t seg_max);

/**
 * @brief Get the maximum number of segments that can be used in a single request for DOCA
 * devemu Virtio Block VQs.
 *
 * @param [out] seg_max
 * The maximum number of segments.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'seg_max' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_get_seg_max(uint16_t *seg_max);

/**
 * @brief Set the datapath on DPA for Virtio Block devices.
 *
 * @details When set to 1, the datapath will run on the DPA. DPA mode is recommended when CPU cores are heavily loaded.
 * If called multiple times, only the last call before doca_devemu_vblk_init() takes effect.
 *
 * @param [in] datapath_on_dpa
 * Set to 1 to configure the datapath to run on the DPA. Otherwise, set to 0.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_NOT_PERMITTED - If called after doca_devemu_vblk_init().
 */
DOCA_EXPERIMENTAL doca_error_t doca_devemu_vblk_set_datapath_on_dpa(uint8_t datapath_on_dpa);

/**
 * @brief Retrieves whether the datapath was configured to run on the DPA for Virtio Block devices.
 *
 * @param [out] datapath_on_dpa
 * 1 if the datapath was configured to run on the DPA, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'datapath_on_dpa' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_get_datapath_on_dpa(uint8_t *datapath_on_dpa);

/*********************************************************************************************************************
 * DOCA devemu Virtio Block capability query API
 *********************************************************************************************************************/

/**
 * @brief Get the maximum queue size for Virtio Block VQs supported by the device.
 *
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] size
 * The maximum supported queue size.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'size' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_cap_get_max_queue_size(const struct doca_devinfo *devinfo, uint16_t *size);

/**
 * @brief Check whether configuring the indirect descriptor feature for Virtio Block
 * devices is supported by the device.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] supported
 * True if configuring the indirect descriptor feature is supported, and false otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'supported' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_cap_is_indir_descs_supported(const struct doca_devinfo *devinfo, uint8_t *supported);

/**
 * @brief Get the maximum value that can be configured in the seg_max register
 * for Virtio Block devices associated with the device.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_seg_max
 * The maximum supported seg_max value.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_seg_max' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_cap_get_max_seg_max(const struct doca_devinfo *devinfo, uint16_t *max_seg_max);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VBLK_H_ */
