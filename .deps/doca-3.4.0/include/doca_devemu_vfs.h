/*
 * Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_devemu_vfs.h
 * @page doca_devemu_vfs
 * @defgroup DOCA_DEVEMU_VFS DOCA Device Emulation - Virtio FS Devices
 * @ingroup DOCA_DEVEMU
 *
 * DOCA library for emulated virtio FS devices
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VFS_H_
#define DOCA_DEVEMU_VFS_H_

#include <stdint.h>

#include <doca_buf.h>
#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>
#include <doca_devemu_vfs_dev.h>
#include <doca_devemu_vfs_fuse_kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing DOCA Virtio FS global configurations.
 * This structure is used by Virtio FS device emulation applications and services.
 */
struct doca_devemu_vfs_cfg;

/**
 * @brief Opaque structure representing emulated Virtio FS pci device.
 * This structure extends the core doca_devemu_virtio_dev structure.
 * This structure is used by Virtio FS device emulation applications and services.
 */
struct doca_devemu_vfs_dev;

/**
 * @brief Opaque structure representing emulated Virtio FS pci device type.
 * This structure extends the core doca_devemu_virtio_type structure.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_vfs_type;

/**
 * @brief Opaque structure representing emulated Virtio FS pci device IO context.
 * This structure extends the core doca_devemu_virtio_io structure.
 * This structure is used by Virtio FS device emulation applications and services.
 */
struct doca_devemu_vfs_io;

/**
 * @brief Opaque structure representing Virtio FS FUSE request.
 * This structure is used by Virtio FS device emulation applications and services.
 */
struct doca_devemu_vfs_fuse_req;

/*********************************************************************************************************************
 * DOCA devemu Virtio FS configuration API
 *********************************************************************************************************************/

/**
 * @brief Initialize the DOCA devemu Virtio FS.
 *
 * This is the global initialization function for DOCA devemu Virtio FS.
 * Must be invoked before creating Virtio FS devices, functions and IO context's.
 * This is a one time call, used for initialization and global configurations.
 *
 * @param [in] cfg
 * DOCA devemu Virtio FS configuration structure.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_init(struct doca_devemu_vfs_cfg *cfg);

/**
 * @brief Teardown the DOCA devemu Virtio FS.
 *
 * Release all the resources initialized by doca_devemu_vfs_init().
 * Must be invoked at the teardown stage of the Virtio FS device emulation application, before it exits.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_teardown(void);

/**
 * @brief Create DOCA devemu Virtio FS configuration structure. The new structure is populated with the default
 * configuration.
 *
 * @param [out] cfg
 * DOCA devemu Virtio FS configuration structure.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_cfg_create(struct doca_devemu_vfs_cfg **cfg);

/**
 * @brief Destroy DOCA devemu Virtio FS configuration structure.
 *
 * @param [in] cfg
 * The DOCA devemu Virtio FS configuration structure to destroy.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_cfg_destroy(struct doca_devemu_vfs_cfg *cfg);

/**
 * @brief Register a DOCA device with the Virtio FS configuration.
 *
 * @details The DOCA device must support the DOCA Virtio FS type. For example, this can be verified by calling
 * doca_devemu_vfs_cap_is_default_vfs_type_supported() with the device info. If unsupported, subsequent call to
 * doca_devemu_vfs_init() will fail.  Once successfully added, and after doca_devemu_vfs_init(), the DOCA device can be
 * associated with a valid Virtio FS type (For example, via doca_devemu_vfs_find_default_vfs_type_by_dev()).
 *
 * @param [in] cfg
 * The DOCA devemu Virtio FS configuration structure to modify.
 * @param [in] dev
 * The DOCA device instance to be registered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_cfg_add_dev(struct doca_devemu_vfs_cfg *cfg, struct doca_dev *dev);

/**
 * @brief Remove a DOCA device from the Virtio FS configuration.
 *
 * @details The DOCA device must have been previously added via doca_devemu_vfs_cfg_add_dev(). After removal, the
 * device will no longer be associated with the Virtio FS configuration, and subsequent initialization, using
 * doca_devemu_vfs_init() will not include it.
 *
 * @param [in] cfg
 * The DOCA devemu Virtio FS configuration structure to modify.
 * @param [in] dev
 * The DOCA device instance to be removed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_cfg_rm_dev(struct doca_devemu_vfs_cfg *cfg, struct doca_dev *dev);

/**
 * @brief Get the size of the user data buffer that will be allocated for each doca_devemu_vfs_notification_req on behalf
 * of the user. This buffer will be valid and used by the user upon receiving new doca_devemu_vfs_notification_req. The
 * buffer will become invalid after doca_devemu_vfs_notification_req completion.
 *
 * @param [in] cfg
 * The DOCA devemu Virtio FS configuration structure to query.
 * @param [out] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vfs_notification_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_cfg_get_notification_req_user_data_size(const struct doca_devemu_vfs_cfg *cfg,
		uint32_t *req_user_data_size);

/**
 * @brief Set the size of the user data buffer that will be allocated for each doca_devemu_vfs_notification_req on behalf
 * of the user. This buffer will be valid and used by the user upon receiving new doca_devemu_vfs_notification_req. The
 * buffer will become invalid after doca_devemu_vfs_notification_req completion.
 *
 * @param [in] cfg
 * The DOCA devemu Virtio FS configuration structure to modify.
 * @param [in] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vfs_notification_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 * @note This function is mutually exclusive with:
 *  - doca_devemu_vfs_cfg_set_vfs_fuse_req_user_data_size()
 * If this mutual exclusivity condition is not met, doca_devemu_vfs_init() will fail.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_cfg_set_notification_req_user_data_size(struct doca_devemu_vfs_cfg *cfg,
		uint32_t req_user_data_size);

/**
 * @brief Get the size of the user data buffer that will be allocated for each doca_devemu_vfs_fuse_req on behalf
 * of the user. This buffer will be valid and used by the user upon receiving new doca_devemu_vfs_fuse_req. The
 * buffer will become invalid after doca_devemu_vfs_fuse_req completion.
 *
 * @param [in] cfg
 * The DOCA devemu Virtio FS configuration structure to query.
 * @param [out] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vfs_fuse_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'cfg' or 'req_user_data_size' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_cfg_get_vfs_fuse_req_user_data_size(const struct doca_devemu_vfs_cfg *cfg,
		uint32_t *req_user_data_size);

/**
 * @brief Set the size of the user data buffer that will be allocated for each doca_devemu_vfs_fuse_req on behalf of
 * the user. This buffer will be valid and used by the user upon receiving new doca_devemu_vfs_fuse_req. The buffer
 * will become invalid after doca_devemu_vfs_fuse_req completion.
 *
 * @param [in] cfg
 * The DOCA devemu Virtio FS configuration structure to modify.
 * @param [in] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each doca_devemu_vfs_fuse_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'cfg' is NULL
 * @note This function is mutually exclusive with:
 *  - doca_devemu_vfs_cfg_set_vfs_req_user_data_size()
 *  - doca_devemu_vfs_cfg_set_notification_req_user_data_size()
 * If this mutual exclusivity condition is not met, doca_devemu_vfs_init() will fail.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_cfg_set_vfs_fuse_req_user_data_size(struct doca_devemu_vfs_cfg *cfg,
		uint32_t req_user_data_size);

/**
 * @brief Retrieve the minimum FUSE protocol version supported by the library.
 *
 * If the host driver reports a lower version than the returned minimum, the device will not allow a FUSE session to
 * be opened.
 *
 * @param [out] major
 * The minimum supported FUSE major version.
 * @param [out] minor
 * The minimum supported FUSE minor version.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'major' or 'major' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_get_minimal_fuse_version_supported(uint32_t *major, uint32_t *minor);


#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VFS_H_ */
