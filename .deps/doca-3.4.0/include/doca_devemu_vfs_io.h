/*
 * Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_devemu_vfs_io.h
 * @page doca_devemu_vfs_io
 * @defgroup DOCA_DEVEMU_VFS_IO DOCA Device Emulation - Virtio FS IO Context
 * @ingroup DOCA_DEVEMU_VFS
 *
 * DOCA Virtio FS IO context
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VFS_IO_H_
#define DOCA_DEVEMU_VFS_IO_H_

#include <stdint.h>

#include <doca_buf.h>
#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>
#include <doca_devemu_virtio_io.h>
#include <doca_devemu_vfs.h>
#include <doca_devemu_vfs_fuse_kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA devemu Virtio FS IO context API
 *********************************************************************************************************************/

/**
 * @brief Allocate Virtio FS device IO context for a DOCA Virtio FS device.
 *
 * @details The responsibility of the Virtio FS IO context is to relay the requests arriving from the device driver
 * towards the Virtio FS services and applications. Additionally, it is responsible for relaying the completions
 * arriving from the Virtio FS services and applications towards the device driver. Each Virtio FS device IO context
 * is associated with a single DOCA Virtio FS device.
 * This function must be invoked from the same CPU core responsible for managing the IO context.
 *
 * @param [in] vfs_dev
 * DOCA Virtio FS device.
 * @param [in] progress_engine
 * The progress engine that will be used to progress the new context.
 * @param [out] io
 * The created DOCA Virtio FS device IO context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure. see doca_error_t.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_create(struct doca_devemu_vfs_dev *vfs_dev,
		struct doca_pe *progress_engine, struct doca_devemu_vfs_io **io);

/**
 * @brief Free a Virtio FS device IO context.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to release. Must be idle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' is NULL
 * - DOCA_ERROR_BAD_STATE - device IO context is not idle. Use doca_ctx_stop() to stop it
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_destroy(struct doca_devemu_vfs_io *io);

/**
 * @brief Convert DOCA Virtio FS device IO context instance into DOCA context.
 *
 * @param [in] io
 * DOCA Virtio FS device IO context instance. This must remain valid until after the DOCA context is no longer required.
 *
 * @return
 * doca ctx upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_devemu_vfs_io_as_ctx(struct doca_devemu_vfs_io *io);

/**
 * @brief Convert DOCA Virtio FS device IO context instance into DOCA Virtio device IO context.
 *
 * @param [in] io
 * DOCA Virtio FS device IO context instance. This must remain valid until after the DOCA Virtio device IO context is
 * no longer required.
 *
 * @return
 * doca devemu virtio device io context upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_virtio_io *doca_devemu_vfs_io_as_virtio_io(struct doca_devemu_vfs_io *io);

/*********************************************************************************************************************
 * DOCA devemu Virtio FS IO context FUSE events API
 *********************************************************************************************************************/

/**
 * @brief Function to be executed on FUSE_LOOKUP event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer containing the filename to be looked up as defined in the Virtio FS specification.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [in] entry
 * The virtio_fs_req::(fuse_entry_out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_lookup_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_entry_out *entry);

/**
 * @brief Register to Virtio FS FUSE_LOOKUP request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_lookup_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_lookup_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_FORGET event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] forget_in
 * The virtio_fs_req::(fuse_forget_in) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_forget_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_forget_in *forget_in);

/**
 * @brief Register to Virtio FS FUSE_FORGET request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_forget_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_forget_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_GETATTR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] getattr_in
 * The virtio_fs_req::(fuse_getattr_in) part, according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] attr_out
 * The virtio_fs_req::(fuse_attr_out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_getattr_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_getattr_in *getattr_in,
		struct fuse_out_header *out,
		struct fuse_attr_out *attr_out);

/**
 * @brief Register to Virtio FS FUSE_GETATTR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_getattr_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_getattr_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_SETATTR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] setattr_in
 * The virtio_fs_req::(fuse_setattr_in) part containing attribute modification parameters.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] attr_out
 * The virtio_fs_req::(fuse_attr_out) part containing updated file attributes.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_setattr_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_setattr_in *setattr_in,
		struct fuse_out_header *out,
		struct fuse_attr_out *attr_out);

/**
 * @brief Register to Virtio FS FUSE_SETATTR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_setattr_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_setattr_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_READLINK event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] dataout
 * The DOCA buffer containing the symbolic link path as defined in the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_readlink_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_out_header *out,
		struct doca_buf *dataout);

/**
 * @brief Register to Virtio FS FUSE_READLINK request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_readlink_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_readlink_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_SYMLINK event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer containing the target path and link name as defined in the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] entry
 * The virtio_fs_req::(fuse_entry_out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_symlink_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_entry_out *entry);

/**
 * @brief Register to Virtio FS FUSE_SYMLINK request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_symlink_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_symlink_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_MKNOD event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] mknod_in
 * The virtio_fs_req::(fuse_mknod_in) part containing parameters for node creation.
 * @param [in] datain
 * The DOCA buffer containing the name of the node to be created.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] entry
 * The virtio_fs_req::(fuse_entry_out) part containing metadata for the newly created node.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_mknod_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_mknod_in *mknod_in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_entry_out *entry);

/**
 * @brief Register to Virtio FS FUSE_MKNOD request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_mknod_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_mknod_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_MKDIR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] mkdir_in
 * The virtio_fs_req::(fuse_mkdir_in) part containing parameters for directory creation.
 * @param [in] datain
 * The DOCA buffer containing the name of the directory to be created.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] entry
 * The virtio_fs_req::(fuse_entry_out) part containing metadata for the newly created directory.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_mkdir_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_mkdir_in *mkdir_in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_entry_out *entry);

/**
 * @brief Register to Virtio FS FUSE_MKDIR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_mkdir_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_mkdir_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_UNLINK event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer containing the name of the file to be unlinked.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_unlink_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct doca_buf *datain,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_UNLINK request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_unlink_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_unlink_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_RMDIR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer containing the name of the directory to be removed.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_rmdir_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct doca_buf *datain,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_RMDIR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_rmdir_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_rmdir_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_RENAME event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] rename_in
 * The virtio_fs_req::(fuse_rename_in) part containing parameters for the rename operation.
 * @param [in] datain
 * The DOCA buffer containing both the old and new names of the file or directory to be renamed.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_rename_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_rename_in *rename_in,
		struct doca_buf *datain,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_RENAME request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_rename_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_rename_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_LINK event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] link_in
 * The virtio_fs_req::(fuse_link_in) part containing parameters for the link operation.
 * @param [in] datain
 * The DOCA buffer containing the name of the new hard link.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] entry
 * The virtio_fs_req::(fuse_entry_out) part containing metadata for the newly created hard link.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_link_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_link_in *link_in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_entry_out *entry);

/**
 * @brief Register to Virtio FS FUSE_LINK request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_link_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_link_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_OPEN event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] open_in
 * The virtio_fs_req::(fuse_open_in) part containing parameters for the open operation.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] open_out
 * The virtio_fs_req::(fuse_open_out) part containing results of the open operation.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_open_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_open_in *open_in,
		struct fuse_out_header *out,
		struct fuse_open_out *open_out);

/**
 * @brief Register to Virtio FS FUSE_OPEN request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_open_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_open_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_READ event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] read_in
 * The virtio_fs_req::(read_in) part, according to the Virtio FS specification.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [in] dataout
 * The DOCA buffer representing the 'dataout' portion of the virtio_fs_req, as defined in the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_read_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_read_in *read_in,
		struct fuse_out_header *out,
		struct doca_buf *dataout);

/**
 * @brief Register to Virtio FS FUSE_READ request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_read_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_read_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_WRITE event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] write_in
 * The virtio_fs_req::(write_in) part, according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer representing the 'datain' portion of the virtio_fs_req, as defined in the Virtio FS specification.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [in] write_out
 * The virtio_fs_req::(write_out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_write_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_write_in *write_in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_write_out *write_out);

/**
 * @brief Register to Virtio FS FUSE_WRITE request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_write_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_write_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_STATFS event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] statfs_out
 * The virtio_fs_req::(fuse_statfs_out) part containing filesystem statistics.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_statfs_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_out_header *out,
		struct fuse_statfs_out *statfs_out);

/**
 * @brief Register to Virtio FS FUSE_STATFS request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_statfs_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_statfs_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_RELEASE event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] release_in
 * The virtio_fs_req::(fuse_release_in) part containing parameters for the release operation.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_release_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_release_in *release_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_RELEASE request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_release_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_release_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_FSYNC event occurrence. Ownership of the 'req' structure and its associated
 * attributes is to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] fsync_in
 * The virtio_fs_req::(fuse_fsync_in) part containing file handle and synchronization flags.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_fsync_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_fsync_in *fsync_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_FSYNC request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_fsync_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_fsync_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_SETXATTR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] xattr_in
 * The virtio_fs_req::(fuse_setxattr_in) part containing extended attribute parameters.
 * @param [in] datain
 * The DOCA buffer containing the attribute name followed by its value.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_setxattr_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_setxattr_in *xattr_in,
		struct doca_buf *datain,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_SETXATTR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_setxattr_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_setxattr_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_GETXATTR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] xattr_in
 * The virtio_fs_req::(fuse_getxattr_in) part containing size parameters.
 * @param [in] datain
 * The DOCA buffer containing the attribute name.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] xattr_out
 * The virtio_fs_req::(fuse_getxattr_out) part containing the value size.
 * @param [out] dataout
 * The DOCA buffer for storing the attribute value.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_getxattr_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_getxattr_in *xattr_in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_getxattr_out *xattr_out,
		struct doca_buf *dataout);

/**
 * @brief Register to Virtio FS FUSE_GETXATTR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_getxattr_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_getxattr_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_LISTXATTR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] xattr_in
 * The virtio_fs_req::(fuse_getxattr_in) part according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part according to the Virtio FS specification.
 * @param [out] xattr_out
 * The virtio_fs_req::(fuse_getxattr_out) part according to the Virtio FS specification.
 * @param [out] dataout
 * The DOCA buffer storing the list of extended attribute names.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_listxattr_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_getxattr_in *xattr_in,
		struct fuse_out_header *out,
		struct fuse_getxattr_out *xattr_out,
		struct doca_buf *dataout);

/**
 * @brief Register to Virtio FS FUSE_LISTXATTR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_listxattr_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_listxattr_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_REMOVEXATTR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred from the context to the user. Method that is invoked once event is triggered.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer containing the attribute name to remove.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_removexattr_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct doca_buf *datain,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_REMOVEXATTR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_removexattr_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_removexattr_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_FLUSH event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] flush_in
 * The virtio_fs_req::(fuse_flush_in) part containing file handle and flags.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_flush_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_flush_in *flush_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_FLUSH request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_flush_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_flush_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_INIT event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] init_in
 * The virtio_fs_req::(fuse_init_in) part containing protocol negotiation parameters.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] init_out
 * The virtio_fs_req::(fuse_init_out) part containing protocol capabilities and version.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_init_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_init_in *init_in,
		struct fuse_out_header *out,
		struct fuse_init_out *init_out);

/**
 * @brief Register to Virtio FS FUSE_INIT request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_init_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_init_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_OPENDIR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] opendir_in
 * The virtio_fs_req::(fuse_open_in) part containing directory open parameters.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] opendir_out
 * The virtio_fs_req::(fuse_open_out) part containing directory handle and flags.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_opendir_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_open_in *opendir_in,
		struct fuse_out_header *out,
		struct fuse_open_out *opendir_out);

/**
 * @brief Register to Virtio FS FUSE_OPENDIR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_opendir_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_opendir_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_READDIR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] readdir_in
 * The virtio_fs_req::(read_in) part, containing parameters for directory entries according to the Virtio FS specification.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [in] dataout
 * The DOCA buffer representing the 'dataout' portion of the virtio_fs_req, as defined in the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_readdir_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_read_in *readdir_in,
		struct fuse_out_header *out,
		struct doca_buf *dataout);

/**
 * @brief Register to Virtio FS FUSE_READDIR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_readdir_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_readdir_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_RELEASEDIR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] releasedir_in
 * The virtio_fs_req::(release_in) part, containing directory handle and flags according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_releasedir_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_release_in *releasedir_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_RELEASEDIR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_releasedir_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_releasedir_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_FSYNCDIR event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] fsyncdir_in
 * The virtio_fs_req::(fsync_in) part, containing directory sync parameters according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_fsyncdir_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_fsync_in *fsyncdir_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_FSYNCDIR request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_fsyncdir_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_fsyncdir_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_GETLK event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] lk_in
 * The virtio_fs_req::(lk_in) part, containing lock query parameters according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] lk_out
 * The virtio_fs_req::(lk_out) part, containing lock status information according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_getlk_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_lk_in *lk_in,
		struct fuse_out_header *out,
		struct fuse_lk_out *lk_out);

/**
 * @brief Register to Virtio FS FUSE_GETLK request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_getlk_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_getlk_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_SETLK event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] lk_in
 * The virtio_fs_req::(lk_in) part, containing lock parameters according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_setlk_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_lk_in *lk_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_SETLK request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_setlk_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_setlk_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_SETLKW event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] lk_in
 * The virtio_fs_req::(lk_in) part, containing lock parameters according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_setlkw_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_lk_in *lk_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_SETLKW request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_setlkw_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_setlkw_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_ACCESS event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] access_in
 * The virtio_fs_req::(access_in) part, containing permission check parameters according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_access_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_access_in *access_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_ACCESS request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_access_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_access_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_CREATE event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] create_in
 * The virtio_fs_req::(create_in) part, containing file creation flags and mode according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer containing the filename to create.
 * @param [out] out
 * The virtio_fs_req::(out) part according to the Virtio FS specification.
 * @param [out] entry_out
 * The virtio_fs_req::(entry_out) part, containing metadata for the newly created file.
 * @param [out] open_out
 * The virtio_fs_req::(fuse_open_out) part according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_create_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_create_in *create_in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_entry_out *entry_out,
		struct fuse_open_out *open_out);

/**
 * @brief Register to Virtio FS FUSE_CREATE request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_create_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_create_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_INTERRUPT event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] interrupt_in
 * The virtio_fs_req::(interrupt_in) part, containing the unique ID of the request to interrupt.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_interrupt_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_interrupt_in *interrupt_in);

/**
 * @brief Register to Virtio FS FUSE_INTERRUPT request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_interrupt_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_interrupt_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_IOCTL event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] ioctl_in
 * The virtio_fs_req::(ioctl_in) part, containing ioctl command parameters according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer representing the 'datain' portion of the virtio_fs_req, as defined in the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] ioctl_out
 * The virtio_fs_req::(ioctl_out) part, containing ioctl response parameters according to the Virtio FS specification.
 * @param [out] dataout
 * The DOCA buffer representing the 'dataout' portion of the virtio_fs_req, as defined in the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_ioctl_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_ioctl_in *ioctl_in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_ioctl_out *ioctl_out,
		struct doca_buf *dataout);

/**
 * @brief Register to Virtio FS FUSE_IOCTL request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_ioctl_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_ioctl_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_POLL event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] poll_in
 * The virtio_fs_req::(poll_in) part, containing polling parameters according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] poll_out
 * The virtio_fs_req::(poll_out) part, containing polling result parameters according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_poll_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_poll_in *poll_in,
		struct fuse_out_header *out,
		struct fuse_poll_out *poll_out);

/**
 * @brief Register to Virtio FS FUSE_POLL request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_poll_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_poll_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_FALLOCATE event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] fallocate_in
 * The virtio_fs_req::(fallocate_in) part, containing allocation parameters according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_fallocate_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_fallocate_in *fallocate_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_FALLOCATE request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_fallocate_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_fallocate_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_DESTROY event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_destroy_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_DESTROY request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_destroy_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_destroy_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_NOTIFY_REPLY event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] notify_retrieve_in
 * The virtio_fs_req::(notify_retrieve_in) part, containing notification response parameters according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer containing the notification response data according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_notify_reply_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_notify_retrieve_in *notify_retrieve_in,
		struct doca_buf *datain);

/**
 * @brief Register to Virtio FS FUSE_NOTIFY_REPLY request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_notify_reply_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_notify_reply_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_BATCH_FORGET event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] batch_forget_in
 * The virtio_fs_req::(batch_forget_in) part, containing batch forget parameters according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer representing the 'datain' portion as defined in the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_batch_forget_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_batch_forget_in *batch_forget_in,
		struct doca_buf *datain);

/**
 * @brief Register to Virtio FS FUSE_BATCH_FORGET request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_batch_forget_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_batch_forget_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on READDIRPLUS event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] read_in
 * The virtio_fs_req::(read_in) part, according to the Virtio FS specification.
 * @param [in] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [in] dataout
 * The DOCA buffer representing the 'dataout' portion of the virtio_fs_req, as defined in the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_readdirplus_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_read_in *read_in,
		struct fuse_out_header *out,
		struct doca_buf *dataout);

/**
 * @brief Register to Virtio FS READDIRPLUS request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_readdirplus_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_readdirplus_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_RENAME2 event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] rename2_in
 * The virtio_fs_req::(rename2_in) part, containing rename parameters according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer containing the oldname and newname.
 * @param [out] out
 * The virtio_fs_req::(out) part according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_rename2_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_rename2_in *rename2_in,
		struct doca_buf *datain,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_RENAME2 request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_rename2_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_rename2_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_COPY_FILE_RANGE event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] copy_in
 * The virtio_fs_req::(copy_file_range_in) part, containing parameters for the copy operation.
 * @param [out] out
 * The virtio_fs_req::(out) part according to the Virtio FS specification.
 * @param [out] write_out
 * The virtio_fs_req::(write_out) part according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_copy_file_range_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_copy_file_range_in *copy_in,
		struct fuse_out_header *out,
		struct fuse_write_out *write_out);

/**
 * @brief Register to Virtio FS FUSE_COPY_FILE_RANGE request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_copy_file_range_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_copy_file_range_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_SYNCFS event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] syncfs_in
 * The virtio_fs_req::(syncfs_in) part, containing synchronization flags according to the Virtio FS specification.
 * @param [out] out
 * The virtio_fs_req::(out) part according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_syncfs_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_syncfs_in *syncfs_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_SYNCFS request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_syncfs_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_syncfs_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_LSEEK event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] lseek_in
 * The virtio_fs_req::(lseek_in) part, containing seek parameters.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] lseek_out
 * The virtio_fs_req::(lseek_out) part, containing the resulting file offset after the seek operation.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_lseek_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_lseek_in *lseek_in,
		struct fuse_out_header *out,
		struct fuse_lseek_out *lseek_out);

/**
 * @brief Register to Virtio FS FUSE_LSEEK request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_lseek_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_lseek_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_STATX event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] statx_in
 * The virtio_fs_req::(statx_in) part, containing statx parameters.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] statx_out
 * The virtio_fs_req::(statx_out) part, containing the resulting file attributes after the statx operation.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_statx_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_statx_in *statx_in,
		struct fuse_out_header *out,
		struct fuse_statx_out *statx_out);

/**
 * @brief Register to Virtio FS FUSE_STATX request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_statx_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_statx_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_TMPFILE event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] create_in
 * The virtio_fs_req::(create_in) part, containing file creation flags and mode according to the Virtio FS specification.
 * @param [in] datain
 * The DOCA buffer containing the filename to create.
 * @param [out] out
 * The virtio_fs_req::(out) part according to the Virtio FS specification.
 * @param [out] entry_out
 * The virtio_fs_req::(entry_out) part, containing metadata for the newly created file.
 * @param [out] open_out
 * The virtio_fs_req::(fuse_open_out) part according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_tmpfile_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_create_in *create_in,
		struct doca_buf *datain,
		struct fuse_out_header *out,
		struct fuse_entry_out *entry_out,
		struct fuse_open_out *open_out);

/**
 * @brief Register to Virtio FS FUSE_TMPFILE request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_tmpfile_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_tmpfile_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_BMAP event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] bmap_in
 * The virtio_fs_req::(bmap_in) part, containing bmap parameters.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 * @param [out] bmap_out
 * The virtio_fs_req::(bmap_out) part, containing the resulting block after the bmap operation.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_bmap_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_bmap_in *bmap_in,
		struct fuse_out_header *out,
		struct fuse_bmap_out *bmap_out);

/**
 * @brief Register to Virtio FS FUSE_BMAP request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_bmap_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_bmap_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_SETUPMAPPING event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] setupmapping_in
 * The virtio_fs_req::(setupmapping_in) part, containing setupmapping parameters.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_setupmapping_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_setupmapping_in *setupmapping_in,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_SETUPMAPPING request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_setupmapping_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_setupmapping_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed on FUSE_REMOVEMAPPING event occurrence. Ownership of the 'req' structure and its associated
 * attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] in
 * The virtio_fs_req::(in) part, according to the Virtio FS specification.
 * @param [in] removemapping_in
 * The virtio_fs_req::(removemapping_in) part, containing removemapping parameters.
 * @param [in] datain
 * The DOCA buffer containing an array of fuse_removemapping_one structures, each specifying the range to be unmapped.
 * @param [out] out
 * The virtio_fs_req::(out) part, according to the Virtio FS specification.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_removemapping_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_in_header *in,
		struct fuse_removemapping_in *removemapping_in,
		struct doca_buf *datain,
		struct fuse_out_header *out);

/**
 * @brief Register to Virtio FS FUSE_REMOVEMAPPING request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_removemapping_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_removemapping_req_notice_handler_cb_t handler);

/**
 * @brief Function to be executed when notification buffers are populated. Ownership of the 'req' structure and its
 * associated attributes is transferred to the user.
 *
 * @param [in] req
 * The arrived request.
 * @param [in] req_user_data
 * The user data associated to the request.
 * @param [in] out
 * The virtio_fs_notify::(out_hdr) part, according to the Virtio FS specification.
 * @param [in] dataout
 * The DOCA buffer representing the memory for the virtio_fs_notify::(outarg) part.
 */
typedef void (*doca_devemu_vfs_io_event_vfs_fuse_notification_req_notice_handler_cb_t)(struct doca_devemu_vfs_fuse_req *req,
		void *req_user_data,
		struct fuse_out_header *out,
		struct doca_buf *dataout);

/**
 * @brief Register to Virtio FS FUSE notify buffers population request notifications.
 *
 * @param [in] io
 * The DOCA Virtio FS device IO context to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'io' or 'handler' are NULL
 * - DOCA_ERROR_BAD_STATE - 'io' is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vfs_io_event_vfs_fuse_notification_req_handler_register(struct doca_devemu_vfs_io *io,
		doca_devemu_vfs_io_event_vfs_fuse_notification_req_notice_handler_cb_t handler);

/*********************************************************************************************************************
 * DOCA devemu Virtio FS FUSE request API
 *********************************************************************************************************************/

/**
 * @brief Complete the Virtio FS FUSE request. The request ownership (including the associated in_hdr, datain, out_hdr,
 * dataout and the req_user_data) moves from the user back to the associated IO context. The associated IO context will
 * complete the request towards the device driver according to the Virtio FS specification.
 *
 * @param [in] req
 * The Virtio FS FUSE request to complete.
 * @param [in] status
 * DOCA_SUCCESS - in case of success. Error code - in case of failure.
 *
 */
DOCA_EXPERIMENTAL
void doca_devemu_vfs_fuse_req_complete(struct doca_devemu_vfs_fuse_req *req, doca_error_t status);

/**
 * @brief Get the associated DOCA Virtio FS device IO context.
 *
 * @param [in] req
 * The Virtio FS FUSE request to query. Must not be NULL.
 *
 * @return
 * The DOCA Virtio FS device IO context associated to the request on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_vfs_io *doca_devemu_vfs_fuse_req_get_vfs_io(struct doca_devemu_vfs_fuse_req *req);

/**
 * @brief Get the number of elements in the original doca buffer linked list associated with the device-readable part
 * (excluding the FUSE in headers) of the Virtio FS FUSE request.
 *
 * @param [in] req
 * The Virtio FS FUSE request to query. Must not be NULL.
 *
 * @return
 * Number of elements in the original datain doca buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_fuse_req_get_datain_list_len(struct doca_devemu_vfs_fuse_req *req);

/**
 * @brief Get the total data length of the original doca buffer linked list associated with the device-readable part
 * (excluding the FUSE in headers) of the Virtio FS FUSE request.
 *
 * @param [in] req
 * The Virtio FS FUSE request to query. Must not be NULL.
 *
 * @return
 * The total data length (in bytes) of all elements in the original datain DOCA buffer linked list. Valid only if the
 * request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_fuse_req_get_datain_data_len(struct doca_devemu_vfs_fuse_req *req);

/**
 * @brief Get the number of elements in the original doca buffer linked list associated with the device-writable part
 * (excluding the FUSE out headers) of the Virtio FS FUSE request.
 *
 * @param [in] req
 * The Virtio FS FUSE request to query. Must not be NULL.
 *
 * @return
 * Number of elements in the original dataout doca buffer linked list. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_fuse_req_get_dataout_list_len(struct doca_devemu_vfs_fuse_req *req);

/**
 * @brief Get the total data length of the original doca buffer linked list associated with the device-writable part
 * (excluding the FUSE out headers) of the Virtio FS FUSE request.
 *
 * @param [in] req
 * The Virtio FS FUSE request to query. Must not be NULL.
 *
 * @return
 * The total data length (in bytes) of all elements in the original dataout DOCA buffer linked list. Valid only if the
 * request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint32_t doca_devemu_vfs_fuse_req_get_dataout_data_len(struct doca_devemu_vfs_fuse_req *req);

/**
 * @brief Get the request identifier.
 *
 * @param [in] req
 * The Virtio FS FUSE request to query. Must not be NULL.
 *
 * @return
 * The request identifier. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint64_t doca_devemu_vfs_fuse_req_get_id(struct doca_devemu_vfs_fuse_req *req);

/**
 * @brief Get the source domain identifier for the request.
 *
 * @param [in] req
 * The Virtio FS FUSE request to query. Must not be NULL.
 *
 * @return
 * The source domain identifier of the request. Valid only if the request is in the ownership of the user.
 */
DOCA_EXPERIMENTAL
uint16_t doca_devemu_vfs_fuse_req_get_src_domain_id(struct doca_devemu_vfs_fuse_req *req);


#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VFS_IO_H_ */
