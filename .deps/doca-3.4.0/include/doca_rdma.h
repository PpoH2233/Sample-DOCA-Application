/*
 * Copyright (c) 2023-2025 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_rdma.h
 * @page DOCA_RDMA
 * @defgroup DOCA_RDMA DOCA RDMA
 * DOCA RDMA library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_RDMA_H_
#define DOCA_RDMA_H_

#include <stdbool.h>
#include <stdint.h>
#include <rdma/rdma_cma.h>
#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 * DOCA core opaque types
 *********************************************************************************************************************/

struct doca_buf;
struct doca_dev;
struct doca_devinfo;
struct doca_sync_event_remote_net;

/** Available transport types for RDMA */
enum doca_rdma_transport_type {
	DOCA_RDMA_TRANSPORT_TYPE_RC, /**< RC transport type */
	DOCA_RDMA_TRANSPORT_TYPE_DC, /**< DC transport type, supported only in export/connect flow and CPU datapath */
};

/** gid struct */
struct doca_rdma_gid {
	uint8_t raw[DOCA_GID_BYTE_LENGTH]; /**< The raw value of the GID */
};

/** DOCA RDMA addr type */
enum doca_rdma_addr_type {
	DOCA_RDMA_ADDR_TYPE_IPv4, /**< IPv4 type */
	DOCA_RDMA_ADDR_TYPE_IPv6, /**< IPv6 type */
	DOCA_RDMA_ADDR_TYPE_GID,  /**< GID type */
};

/*********************************************************************************************************************
 * DOCA RDMA Opaques
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA RDMA instance.
 */
struct doca_rdma;

/**
 * Opaque structure representing a DOCA RDMA address.
 */
struct doca_rdma_addr;

/**
 * Opaque structure representing a DOCA RDMA remote connection instance.
 */
struct doca_rdma_connection;

/**
 * Typedef representing a DOCA RDMA DPA handle instance.
 */
typedef uint64_t doca_dpa_dev_rdma_t;

/**
 * Opaque structure representing a DOCA RDMA GPU handle instance
 */
struct doca_gpu_dev_rdma;

/*********************************************************************************************************************
 * DOCA RDMA Context
 *********************************************************************************************************************/

/**
 * @brief Create a DOCA RDMA instance.
 *
 * @param [in] dev
 * The device to attach to the RDMA instance.
 * @param [out] rdma
 * Pointer to pointer to be set to point to the created doca_rdma instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - rdma argument is a NULL pointer.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_INITIALIZATION - failed to initialize rdma.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_create(struct doca_dev *dev, struct doca_rdma **rdma);

/**
 * @brief Destroy a DOCA RDMA instance.
 *
 * @param [in] rdma
 * Pointer to instance to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - rdma argument is a NULL pointer.
 * - DOCA_ERROR_BAD_STATE - the associated ctx was not stopped before calling doca_rdma_destroy().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_destroy(struct doca_rdma *rdma);

/**
 * @brief Convert doca_rdma instance into a generalized context for use with doca core objects.
 *
 * @param [in] rdma
 * RDMA instance. This must remain valid until after the context is no longer required.
 *
 * @return
 * Non NULL upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_rdma_as_ctx(struct doca_rdma *rdma);

/**
 * @brief Export doca_rdma connection details object
 * The doca_rdma_conn_details are used in doca_rdma_connect().
 * Can only be called after calling doca_ctx_start().
 *
 * @note The exported data contains sensitive information - please make sure to pass this data through a secure channel
 *
 * @param [in] rdma
 * Pointer doca_rdma to export connection details for.
 * @param [out] local_rdma_conn_details
 * Exported doca_rdma_conn_details object.
 * @param [out] local_rdma_conn_details_size
 * Size of exported doca_rdma_conn_details object.
 * @param [out] rdma_connection
 * Connection related data required for doca_rdma to connect.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if any of the parameters is NULL.
 * - DOCA_ERROR_BAD_STATE - if called before calling ctx_start().
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed or connection details object was corrupted.
 * - DOCA_ERROR_FULL - if all connections are being used.
 * @note stopping and restarting an RDMA context require calling doca_rdma_export() & doca_rdma_connect() again.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_export(struct doca_rdma *rdma,
			      const void **local_rdma_conn_details,
			      size_t *local_rdma_conn_details_size,
			      struct doca_rdma_connection **rdma_connection);

/**
 * @brief Connect to remote doca_rdma peer.
 * Can only be called when the ctx is in DOCA_CTX_STATE_STARTING state (after calling doca_ctx_start()).
 * Once called, doca_pe_progress() should be called, in order to transition the ctx to DOCA_CTX_STATE_RUNNING state.
 * Only after that can tasks be allocated and submitted.
 *
 * @param [in] rdma
 * Pointer to doca_rdma to connect.
 * @param [in] remote_rdma_conn_details
 * Exported doca_rdma_conn_details object from remote peer.
 * @param [in] remote_rdma_conn_details_size
 * Size of remote doca_rdma_conn_details object.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if any of the parameters is NULL.
 * - DOCA_ERROR_BAD_STATE - if context was not started or rdma instance is already connected.
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed or connection details object was corrupted.
 * @note stopping and restarting an RDMA context require calling doca_rdma_export() & doca_rdma_connect() again.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_connect(struct doca_rdma *rdma,
			       const void *remote_rdma_conn_details,
			       size_t remote_rdma_conn_details_size,
			       struct doca_rdma_connection *rdma_connection);

/**
 * @brief Set connection address object for doca_rdma.
 * The object can be queried using doca_rdma_connection_get_addr().
 *
 * @param [in] addr_type
 * According to doca_rdma_addr_type enum.
 * @param [in] address
 * Address to set the connection rdma_connection to.
 * @param [in] port
 * Port to set the connection rdma_connection to.
 * @param [out] addr
 * Address object to use in context for connection.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_addr_create(enum doca_rdma_addr_type addr_type,
				   const char *address,
				   uint16_t port,
				   struct doca_rdma_addr **addr);

/**
 * @brief Destroy connection address object for doca_rdma.
 *
 * @param [in] addr
 * Address object to be destroyed use in context for connection.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if address is actively being used
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_addr_destroy(struct doca_rdma_addr *addr);

/**
 * @brief Start listening for a connection from a remote doca_rdma peer.
 * Can be called when the ctx is in DOCA_CTX_STATE_RUNNING state.
 * Once called, doca_pe_progress() should be called, in order to evaluate possible connections requests.
 * Only after a connection is established can send tasks be allocated and submitted.
 *
 * @param [in] rdma
 * Pointer to doca_rdma to connect.
 * @param [in] port
 * Port to listen to for connection requests.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if context is in an invalid or error state.
 * - DOCA_ERROR_NOT_SUPPORTED - if the current datapath is not supported.
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed or connection details object was corrupted.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_start_listen_to_port(struct doca_rdma *rdma, uint16_t port);

/**
 * @brief End the listen process for a connection from remote doca_rdma peers.
 * Can be called when the ctx is in DOCA_CTX_STATE_RUNNING state.
 * Once called, Server just stop listening for incoming connection requests and do not disconnect any remote doca_rdma
 * peer.
 *
 * @param [in] rdma
 * Pointer to doca_rdma to connect.
 * @param [in] port
 * Port to stop listening to for connection requests.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if the current datapath is not supported.
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed or connection details object was corrupted.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_stop_listen_to_port(struct doca_rdma *rdma, uint16_t port);

/**
 * @brief Accept an incoming connection request from remote doca_rdma peer.
 * Can be called when the ctx is in DOCA_CTX_STATE_RUNNING state.
 * Only after a connection is established can send tasks be allocated and submitted.
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] private_data
 * The RDMA connection private data to be sent in RDMA CM ACCEPT.
 * @note If private data is not needed then this argument should be NULL
 * @param [in] private_data_len
 * The RDMA connection size of the private data that is sent in RDMA CM ACCEPT.
 * @note If private data is not needed then this argument should be 0
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed or connection details object was corrupted.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_connection_accept(struct doca_rdma_connection *rdma_connection,
					 void *private_data,
					 uint8_t private_data_len);

/**
 * @brief Reject an incoming connection request from remote doca_rdma peer.
 * Can be called when the ctx is in DOCA_CTX_STATE_RUNNING state.
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed or connection details object was corrupted.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_connection_reject(struct doca_rdma_connection *rdma_connection);

/**
 * @brief Connect to a remote doca_rdma peer listening for a connection.
 * Can be called when the ctx is in DOCA_CTX_STATE_RUNNING state.
 * Once called, doca_pe_progress() should be called, in order to evaluate connection response.
 * Only after a connection is established can send tasks be allocated and submitted.
 *
 * @param [in] rdma
 * Pointer to doca_rdma to connect.
 * @param [in] addr
 * Address to connect to listening for connection requests.
 * @param [in] connection_user_data
 * The doca_data supplied to the connection by the application (during connection or by a setter).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - if context is in an invalid or error state.
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if the current datapath is not supported.
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed or connection details object was corrupted.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_connect_to_addr(struct doca_rdma *rdma,
				       struct doca_rdma_addr *addr,
				       union doca_data connection_user_data);

/**
 * @brief Finalize a connection with a remote doca_rdma peer.
 * Can be called when the ctx is in DOCA_CTX_STATE_RUNNING state.
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed or connection details object was corrupted.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_connection_disconnect(struct doca_rdma_connection *rdma_connection);

/**
 * @brief Prepare the connection and perform the doca connection to client side acting as a bridge.
 * This method acts as a bridge to prepare and perform the doca connection to a connection request from
 * an application that performs the listen process by itself. This function only prepare the DOCA connection,
 * it is necessary to call doca_rdma_bridge_accept(), to continue the connection process. Can be called when
 * the ctx is in DOCA_CTX_STATE_RUNNING state.
 *
 *
 * @param [in] rdma
 * Pointer to doca_rdma to connect.
 * @param [in] cm_id
 * RDMA CM ID object that carries the connection details.
 * @note DOCA RDMA assumes ownership over the cm_id.
 * @param [out] rdma_connection
 * Connection related data required for doca_rdma to connect.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_FULL - if all connections are being used.
 * - DOCA_ERROR_BAD_STATE - if context is in an invalid or error state.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if the current datapath is not supported.
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_bridge_prepare_connection(struct doca_rdma *rdma,
						 struct rdma_cm_id *cm_id,
						 struct doca_rdma_connection **rdma_connection);

/**
 * @brief Accept the connection from client side acting as a bridge.
 * This method acts as a bridge to accept a connection request from an application that performs the listen process
 * by itself. Can be called when the ctx is in DOCA_CTX_STATE_RUNNING state. Only after a connection is established can
 * send tasks be allocated and submitted.
 *
 * @param [in] rdma
 * Pointer to doca_rdma to connect.
 * @param [in] private_data
 * The RDMA connection private data to be sent in RDMA CM ACCEPT.
 * @note If private data is not needed then this argument should be NULL
 * @param [in] private_data_len
 * The RDMA connection size of the private data that is sent in RDMA CM ACCEPT.
 * @note If private data is not needed then this argument should be 0
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if context is in an invalid or error state.
 * - DOCA_ERROR_NOT_SUPPORTED - if the current datapath is not supported.
 * - DOCA_ERROR_CONNECTION_ABORTED - if connection failed.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_bridge_accept(struct doca_rdma *rdma,
				     void *private_data,
				     uint8_t private_data_len,
				     struct doca_rdma_connection *rdma_connection);

/**
 * @brief Notify the server side of the successful established connection with client
 * Can be called when the ctx is in DOCA_CTX_STATE_RUNNING state.
 *
 * @param [in] rdma
 * Pointer to doca_rdma to connect.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if the current datapath is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_bridge_established(struct doca_rdma *rdma, struct doca_rdma_connection *rdma_connection);

/*********************************************************************************************************************
 * DOCA RDMA capabilities
 *********************************************************************************************************************/

/**
 * @brief Get the maximal recv queue size for a specific device.
 * @note This capability is not relevant when using RDMA SRQ.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [out] max_recv_queue_size
 * The maximal recv queue size for the given devinfo.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query device capabilities.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_get_max_recv_queue_size(const struct doca_devinfo *devinfo, uint32_t *max_recv_queue_size);

/**
 * Get the maximal send queue size for a specific device.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [out] max_send_queue_size
 * The of the maximal send queue size for the given devinfo.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query device capabilities.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_get_max_send_queue_size(const struct doca_devinfo *devinfo, uint32_t *max_send_queue_size);

/**
 * @brief Get the maximal buffer list length property for buffers of tasks that are sent to the remote and in which
 * linked list are supported (i.e. send, send_imm, read, write, write_imm).
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [out] max_send_buf_list_len
 * Maximal buffer list length to used for buffers that support linked list in relevant tasks, for the given devinfo.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query device capabilities.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_get_max_send_buf_list_len(const struct doca_devinfo *devinfo,
						     uint32_t *max_send_buf_list_len);

/**
 * @brief Get the maximal message size for a specific device.
 *
 * @details Note that the max_message_size reflects the device capability and not the OS capability that may be smaller.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [out] max_message_size
 * The maximal message size for the given devinfo.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query device capabilities.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_get_max_message_size(const struct doca_devinfo *devinfo, uint32_t *max_message_size);

/**
 * Get the gid table size for a specific device.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [out] gid_table_size
 * The gid table size for the given devinfo.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query device capabilities.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_get_gid_table_size(const struct doca_devinfo *devinfo, uint32_t *gid_table_size);

/**
 * Get gids for a specific device by index and number of entries.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [in] start_index
 * The first gid index of interest
 * @param [in] num_entries
 * The number of desired gid indices
 * @param [in,out] gid_array
 * A 'struct doca_rdma_gid' array of size 'num_entries', that on success will hold the desired gids.
 * Note that it is the user's responsibility to provide an array with enough entries to prevent data corruption
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query device capabilities.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_get_gid(const struct doca_devinfo *devinfo,
				   uint32_t start_index,
				   uint32_t num_entries,
				   struct doca_rdma_gid *gid_array);

/**
 * @brief Check if DOCA RDMA supports given transport type for a specific device.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [in] transport_type
 * Transport type to query support for.
 *
 * @return
 * DOCA_SUCCESS - in case the transport type is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query device capabilities
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the given transport type.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_transport_type_is_supported(const struct doca_devinfo *devinfo,
						       enum doca_rdma_transport_type transport_type);

/*********************************************************************************************************************
 * DOCA RDMA properties
 *********************************************************************************************************************/

/**
 * @brief Set send queue size property for doca_rdma.
 * The value can be queried using doca_rdma_get_send_queue_size().
 * Queue size will be rounded to the next power of 2.
 * can only be called before calling doca_ctx_start().
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] send_queue_size
 * Send queue size to use in context.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if the given size is not supported.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_send_queue_size(struct doca_rdma *rdma, uint32_t send_queue_size);

/**
 * @brief Set recv queue size property for doca_rdma.
 * The value can be queried using doca_rdma_get_recv_queue_size().
 * Queue size will be rounded to the next power of 2.
 * @note Can only be called before calling doca_ctx_start().
 * @note This property affects only GPU data-path and DPA data-path when SRQ used.
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] recv_queue_size
 * Recv queue size to use in context.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if the given size is not supported or the given RDMA was created with SRQ or on CPU
 * data-path.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_recv_queue_size(struct doca_rdma *rdma, uint32_t recv_queue_size);

/**
 * @brief Set the maximum buffer list length property for local buffers of tasks that are sent to the remote and in
 * which linked list are supported (i.e. send, send_imm, read, write, write_imm).
 * The value in use can be queried using doca_rdma_get_max_send_buf_list_len().
 * @note Can only be called before calling doca_ctx_start().
 *
 * @param [in] rdma
 * The RDMA instance to set the property for.
 * @param [in] max_send_buf_list_len
 * Maximum buffer list length to use for local buffer in relevant tasks.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_max_send_buf_list_len(struct doca_rdma *rdma, uint32_t max_send_buf_list_len);

/**
 * @brief Set transport type for doca_rdma.
 * The value can be queried using doca_rdma_get_transport_type().
 * Can only be called before calling doca_ctx_start().
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] transport_type
 * Transport type to use in context.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if the given transport type is not supported.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_transport_type(struct doca_rdma *rdma, enum doca_rdma_transport_type transport_type);

/**
 * @brief Set MTU for doca_rdma.
 * The value can be queried using doca_rdma_get_mtu().
 * Can only be called before calling doca_ctx_start().
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] mtu
 * MTU to use in context.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if the given MTU is not supported.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 * - DOCA_ERROR_UNEXPECTED - if an unexpected error has occurred.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_mtu(struct doca_rdma *rdma, enum doca_mtu_size mtu);

/**
 * @brief Set rdma permissions for doca_rdma.
 * The value can be queried using doca_rdma_get_permissions().
 * Can only be called after calling doca_ctx_dev_add() and before calling doca_ctx_start().
 * The supported permissions are the RDMA access flags.
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] permissions
 * Bitwise combination of RDMA access flags - see enum doca_access_flag
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given or non-RDMA access flags were given.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_permissions(struct doca_rdma *rdma, uint32_t permissions);

/**
 * @brief Set whether to use GRH in connection.
 * The value can be queried using doca_rdma_get_grh_enabled().
 * Can only be called before calling doca_ctx_start().
 *
 * If using IB device:
 * If GRH is disabled, the address will rely on LID only.
 * If GRH is enabled, the other side must also use GRH.
 *
 * If using ETH device, GRH must be enabled.
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] grh_enabled
 * 1 if GRH is used in doca_rdma, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 * - DOCA_ERROR_NOT_SUPPORTED - if GRH setting is not supported for the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_grh_enabled(struct doca_rdma *rdma, uint8_t grh_enabled);

/**
 * @brief Set GID index for doca_rdma.
 * The value can be queried using doca_rdma_get_gid_index().
 * Can only be called before calling doca_ctx_start().
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] gid_index
 * GID index to use in doca_rdma.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_gid_index(struct doca_rdma *rdma, uint32_t gid_index);

/**
 * @brief Set SL (service level) for doca_rdma.
 * The value can be queried using doca_rdma_get_sl().
 * Can only be called before calling doca_ctx_start().
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] sl
 * SL to use in doca_rdma.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_sl(struct doca_rdma *rdma, uint32_t sl);

/**
 * @brief Set timeout property for doca_rdma.
 * The value can be queried using doca_rdma_get_connection_request_timeout().
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] timeout
 * The timeout (in milliseconds) to be used for connection resolve related functions.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_connection_request_timeout(struct doca_rdma *rdma, uint16_t timeout);

/**
 * @brief Set user data to include in each connection.
 *
 * @details This method sets a connection user data to a context. The connection user data will be returned as a
 * parameter to connection state callbacks.
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] connection_user_data
 * doca_data to attach to the connection.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_connection_set_user_data(struct doca_rdma_connection *rdma_connection,
						union doca_data connection_user_data);

/**
 * @brief Set the maximum number of connections property for a context.
 * The value can be queried using doca_rdma_get_max_num_connections().
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] max_num_connections
 * The maximum amount of connections allowed.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - provided max_num_connections is smaller than minimum value supported.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_max_num_connections(struct doca_rdma *rdma, uint16_t max_num_connections);

/**
 * @brief Set the rnr retry count property for a context.
 * The value can be queried using doca_rdma_get_rnr_retry_count().
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [in] rnr_retry_count
 * rnr retry count index to use in doca_rdma.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - provided max_num_connections is smaller than minimum value supported.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_rnr_retry_count(struct doca_rdma *rdma, uint8_t rnr_retry_count);

/**
 * @brief Get send queue size property from doca_rdma.
 * Returns the current send_queue_size set for the doca_rdma_context.
 * The size returned is the actual size being used and might differ from the size set by the user,
 * as the size may be increased.
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] send_queue_size
 * Send queue size set in context.
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_send_queue_size(const struct doca_rdma *rdma, uint32_t *send_queue_size);

/**
 * @brief Get recv queue size property from doca_rdma.
 * Returns the current recv_queue_size set for the doca_rdma_context.
 * The size returned is the actual size being used and might differ from the size set by the user,
 * as the size may be increased.
 * @note This property affects only GPU data-path and DPA data-path when SRQ is used.
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] recv_queue_size
 * Recv queue size set in context.
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if the given RDMA was created with SRQ or on CPU data-path.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_recv_queue_size(const struct doca_rdma *rdma, uint32_t *recv_queue_size);

/**
 * @brief Get the maximum buffer list length property for local buffers of tasks that are sent to the remote and in
 * which linked list are supported (i.e. send, send_imm, read, write, write_imm).
 *
 * @param [in] rdma
 * The RDMA instance to get the property from.
 * @param [out] max_send_buf_list_len
 * Maximum buffer list length to used for local buffer in relevant tasks.
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_max_send_buf_list_len(const struct doca_rdma *rdma, uint32_t *max_send_buf_list_len);

/**
 * @brief Get transport_type property from doca_rdma.
 * Returns the current transport_type set for the doca_rdma_context.
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] transport_type
 * Transport_type set in context.
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_transport_type(const struct doca_rdma *rdma, enum doca_rdma_transport_type *transport_type);

/**
 * @brief Get the MTU property from doca_rdma.
 * Returns the current MTU set for the doca_rdma context.
 * @note If MTU wasn't set by the user explicitly (and a default value was used), it may changed upon connection.
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] mtu
 * MTU set in context.
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_mtu(const struct doca_rdma *rdma, enum doca_mtu_size *mtu);

/**
 * @brief Get permissions property from doca_rdma.
 * Returns the current permissions set for the doca_rdma_context.
 * Can only be called after calling doca_ctx_dev_add().
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] permissions
 * Bitwise combination of RDMA access flags set in context - see enum doca_access_flag
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_permissions(struct doca_rdma *rdma, uint32_t *permissions);

/**
 * @brief Get GRH setting from doca_rdma.
 * Get the current GRH setting for doca_rdma.
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] grh_enabled
 * 1 if GRH setting was used in doca_rdma, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_grh_enabled(const struct doca_rdma *rdma, uint8_t *grh_enabled);

/**
 * @brief Get GID index from doca_rdma.
 * Get the current GID index set for doca_rdma.
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] gid_index
 * GID index used in doca_rdma.
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_gid_index(const struct doca_rdma *rdma, uint32_t *gid_index);

/**
 * @brief Get SL (service level) from doca_rdma.
 * Get the current SL set for doca_rdma.
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] sl
 * SL used in doca_rdma.
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_sl(const struct doca_rdma *rdma, uint32_t *sl);

/**
 * @brief Retrieve the handle in the dpa memory space of a doca_rdma
 *
 * @param [in] rdma
 * doca_rdma context to get the dpa handle from.
 * @param [out] dpa_rdma
 * A pointer to the handle in the dpa memory space.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if called before calling ctx_start(), or if not assigned to dpa datapath.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_dpa_handle(struct doca_rdma *rdma, doca_dpa_dev_rdma_t *dpa_rdma);

/**
 * @brief Retrieve the handle in the gpu memory space of a doca_rdma
 *
 * @param [in] rdma
 * doca_rdma context to get the gpu handle from.
 * @param [out] gpu_rdma
 * A pointer to the handle in the gpu memory space.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if called before calling ctx_start(), or if not assigned to gpu datapath.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_gpu_handle(struct doca_rdma *rdma, struct doca_gpu_dev_rdma **gpu_rdma);

/**
 * @brief Get timeout property for doca_rdma.
 * Returns the current timeout set for the connection resolve related functions.
 * The size returned is the actual size being used and might differ from the size set by the user,
 * as the size may be increased.
 *
 * @param [in] rdma
 * doca_rdma context to set the property for.
 * @param [out] timeout
 * Timeout value (in milliseconds) to use in context for connection.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_connection_request_timeout(const struct doca_rdma *rdma, uint16_t *timeout);

/**
 * @brief Get connection address object from a doca_rdma_connection.
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [out] addr
 * A pointer to the address object used for this connection.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_connection_get_addr(const struct doca_rdma_connection *rdma_connection,
					   struct doca_rdma_addr **addr);

/**
 * @brief Get address object parameters from an address object.
 *
 * @param [in] addr
 * A pointer to the address object to retrieve the parameters from.
 * @param [out] addr_type
 * Address type to retrieve from address object.
 * @param [out] address
 * Address to retrieve from address object.
 * @param [out] port
 * Port to retrieve from address object.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_addr_get_params(struct doca_rdma_addr *addr,
				       enum doca_rdma_addr_type *addr_type,
				       const char **address,
				       uint16_t *port);

/**
 * @brief Get user data included in a connection.
 *
 * @details This method retrieves connection user data from a rdma connection (previously set using
 * doca_rdma_connection_set_user_data).
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [out] connection_user_data
 * Connection user data to get
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_connection_get_user_data(const struct doca_rdma_connection *rdma_connection,
						union doca_data *connection_user_data);

/**
 * @brief Get connection ID from an rdma connection
 *
 * @details This method retrieves connection ID from an rdma connection object
 *
 * @param [in] rdma_connection
 * The connection to get the property from.
 * @param [out] connection_id
 * The connection ID
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_connection_get_id(const struct doca_rdma_connection *rdma_connection, uint32_t *connection_id);

/**
 * @brief Get maximum number of connections property for doca_rdma.
 * Returns the current maximum number of connections set for a context.
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] max_num_connections
 * The maximum amount of connections allowed in this context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_max_num_connections(struct doca_rdma *rdma, uint16_t *max_num_connections);

/**
 * @brief Get rnr retry count property for doca_rdma.
 * Returns the current rnr retry count set for a context.
 *
 * @param [in] rdma
 * doca_rdma context to get the property from.
 * @param [out] rnr_retry_count
 * rnr retry count used in doca_rdma.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_get_rnr_retry_count(const struct doca_rdma *rdma, uint8_t *rnr_retry_count);

/*********************************************************************************************************************
 * DOCA RDMA Connections
 *********************************************************************************************************************/

/**
 * @brief Function to execute on connection request event.
 *
 * @details This function is called by doca_pe_progress() when a connection request is received by a server.
 * When this function is called the user (acting as a server) will receive notification of a connection request.
 * Inside this callback the user may decide whether to accept or reject this connection.
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @note The implementation can assume this value is not NULL.
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_connection_request_cb_t)(struct doca_rdma_connection *rdma_connection,
						  union doca_data ctx_user_data);

/**
 * @brief Function to execute on connection established event.
 *
 * @details This function is called by doca_pe_progress() when a connection successfully established with a server.
 * When this function is called the user (acting as a client) will receive notification of a connection established.
 * Inside this callback the user may decide to account for the established.
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @note The implementation can assume this value is not NULL.
 * @param [in] connection_user_data
 * The doca_data supplied to the connection by the application (during connection or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_connection_established_cb_t)(struct doca_rdma_connection *rdma_connection,
						      union doca_data connection_user_data,
						      union doca_data ctx_user_data);

/**
 * @brief Function to execute on connection failure event.
 *
 * @details This function is called by doca_pe_progress() when a connection fails to be established.
 * When this function is called the user (acting as a client) will receive notification of a connection failure.
 * Inside this callback the user may decide to account for the issue to decide whether to re-try or re-evaluate.
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @note The implementation can assume this value is not NULL.
 * @param [in] connection_user_data
 * The doca_data supplied to the connection by the application (during connection or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_connection_failure_cb_t)(struct doca_rdma_connection *rdma_connection,
						  union doca_data connection_user_data,
						  union doca_data ctx_user_data);

/**
 * @brief Function to execute on connection disconnection event.
 *
 * @details This function is called by doca_pe_progress() when a connection is disconnected either by server or client.
 * When this function is called the user (acting as a client) will receive notification of disconnection from an
 * established connection. Inside this callback the user may decide to account for the issue to decide whether to re-try
 * or re-evaluate. Inside this callback the user shouldn't call doca_pe_progress(). Please see doca_pe_progress() for
 * details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @note The implementation can assume this value is not NULL.
 * @param [in] connection_user_data
 * The doca_data supplied to the connection by the application (during connection or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_connection_disconnection_cb_t)(struct doca_rdma_connection *rdma_connection,
							union doca_data connection_user_data,
							union doca_data ctx_user_data);

/**
 * @brief This method set the function executed on RDMA connection events.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] doca_rdma_connect_request_cb
 * A callback function for connection request event.
 * This parameter can be NULL in case application is only a client
 * @param [in] doca_rdma_connect_established_cb
 * A callback function for connection established event.
 * @param [in] doca_rdma_connect_failure_cb
 * A callback function for connection failure event.
 * @param [in] doca_rdma_disconnect_cb
 * A callback function for connection disconnection event.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_set_connection_state_callbacks(
	struct doca_rdma *rdma,
	doca_rdma_connection_request_cb_t doca_rdma_connect_request_cb,
	doca_rdma_connection_established_cb_t doca_rdma_connect_established_cb,
	doca_rdma_connection_failure_cb_t doca_rdma_connect_failure_cb,
	doca_rdma_connection_disconnection_cb_t doca_rdma_disconnect_cb);

/*********************************************************************************************************************
 * DOCA RDMA Tasks
 *********************************************************************************************************************/

/********************************************
 * DOCA RDMA Task - Receive		    *
 ********************************************/

/**
 * @brief This task receives a message \ immediate sent from the peer.
 */
struct doca_rdma_task_receive;

/** Task receive result opcodes */
enum doca_rdma_opcode {
	DOCA_RDMA_OPCODE_RECV_SEND = 0,
	DOCA_RDMA_OPCODE_RECV_SEND_WITH_IMM,
	DOCA_RDMA_OPCODE_RECV_WRITE_WITH_IMM,
};

/**
 * @brief Function to execute on completion of a receive task.
 *
 * @details This function is called by doca_pe_progress() when a receive task is successfully identified as completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed receive task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_receive_completion_cb_t)(struct doca_rdma_task_receive *task,
						       union doca_data task_user_data,
						       union doca_data ctx_user_data);

/**
 * Check if a given device supports executing a receive task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_receive_is_supported(const struct doca_devinfo *devinfo);

/**
 * Get the maximal buffer list length for a destination buffer of a receive task, for the given devinfo and transport
 * type.
 * @note The actual limit depends on the property set for the task - either the default value or the value set using
 * doca_rdma_task_receive_set_dst_buf_list_len() prior to doca_ctx_start().
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [in] transport_type
 * The relevant transport type.
 * @param [out] max_buf_list_len
 * The maximal number of local buffers that can be chained with a destination buffer of a receive task, for the given
 * devinfo and transport type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query device capabilities.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_receive_get_max_dst_buf_list_len(const struct doca_devinfo *devinfo,
								 enum doca_rdma_transport_type transport_type,
								 uint32_t *max_buf_list_len);

/**
 * @brief This method sets the receive tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for receive tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for receive tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of receive tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_receive_set_conf(struct doca_rdma *rdma,
					     doca_rdma_task_receive_completion_cb_t successful_task_completion_cb,
					     doca_rdma_task_receive_completion_cb_t error_task_completion_cb,
					     uint32_t num_tasks);

/**
 * @brief Set the maximal destination buffer list length property for receive tasks.
 * After starting the DOCA RDMA context the length may be increased and the value in use can be queried using
 * doca_rdma_get_recv_buf_list_len().
 * @note Can only be called before calling doca_ctx_start().
 * @note Cannot exceed the value returned from doca_rdma_cap_task_receive_get_max_dst_buf_list_len().
 *
 * @param [in] rdma
 * The RDMA instance to set the property for.
 * @param [in] buf_list_len
 * buf_list_len to use in doca_rdma.
 *
 * @return
 * DOCA_SUCCESS - if property set successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if context is already started or if the given RDMA was created with SRQ.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_receive_set_dst_buf_list_len(struct doca_rdma *rdma, uint32_t buf_list_len);

/**
 * @brief Get the maximal destination buffer list length property for receive tasks.
 * The returned value is the actual value being used and might differ from the size set by the user, as it may be
 * increased.
 *
 * @param [in] rdma
 * The RDMA instance to get the property from.
 * @param [out] buf_list_len
 * buf_list_len used in doca_rdma.
 *
 * @return
 * DOCA_SUCCESS - if property retrieved successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if the given RDMA was created with SRQ.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_receive_get_dst_buf_list_len(const struct doca_rdma *rdma, uint32_t *buf_list_len);

/**
 * @brief This method allocates and initializes a receive task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] dst_buf
 * Local destination buffer, for the received data.
 * May be NULL when receiving an empty message (without data), with or without immediate.
 * @note dst_buf may be linked to other buffers, with a limit according to
 * doca_rdma_cap_task_receive_get_max_dst_buf_list_len().
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized a receive task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_receive_allocate_init(struct doca_rdma *rdma,
						  struct doca_buf *dst_buf,
						  union doca_data user_data,
						  struct doca_rdma_task_receive **task);

/**
 * @brief This method converts a receive task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The receive task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_receive_as_task(struct doca_rdma_task_receive *task);

/**
 * @brief This method sets the destination buffer of a receive task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] dst_buf
 * Local destination buffer, for the received data.
 * May be NULL when receiving an empty message (without data), with or without immediate.
 * If the destination buffer is not set by the user, it will have a default value - NULL.
 * @note dst_buf may be linked to other buffers, with a limit according to
 * doca_rdma_cap_task_receive_get_max_dst_buf_list_len().
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_receive_set_dst_buf(struct doca_rdma_task_receive *task, struct doca_buf *dst_buf);

/**
 * @brief This method gets the destination buffer of a receive task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's dst_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_receive_get_dst_buf(const struct doca_rdma_task_receive *task);

/**
 * @brief This method gets the opcode of the operation executed by the peer and received by the task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The operation executed by the peer and received.
 * @note Valid only on after completion of the task. Otherwise, undefined behavior.
 */
DOCA_EXPERIMENTAL
enum doca_rdma_opcode doca_rdma_task_receive_get_result_opcode(const struct doca_rdma_task_receive *task);

/**
 * @brief This method gets the length of data received by the task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * Total length of received data in case of completion.
 * @note Valid only on successful completion of the task. Otherwise, undefined behavior.
 */
DOCA_EXPERIMENTAL
uint32_t doca_rdma_task_receive_get_result_len(const struct doca_rdma_task_receive *task);

/**
 * @brief This method gets the immediate data received by the task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * A 32-bit immediate data value, in Big-Endian, received OOB from the peer along with the message.
 * @note Valid only on successful completion of the task and when the result opcode is
 * DOCA_RDMA_OPCODE_RECV_SEND_WITH_IMM or DOCA_RDMA_OPCODE_RECV_WRITE_WITH_IMM (retrieved using
 * doca_rdma_task_receive_get_result_opcode()).
 * Otherwise, undefined behavior.
 */
DOCA_EXPERIMENTAL
doca_be32_t doca_rdma_task_receive_get_result_immediate_data(const struct doca_rdma_task_receive *task);

/**
 * @brief This method gets the rdma connection of a receive task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 * @note Valid only in the doca_rdma_task_receive successful completion callback.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_receive_get_result_rdma_connection(
	const struct doca_rdma_task_receive *task);

/********************************************
 * DOCA RDMA Task - Send		    *
 ********************************************/

/**
 * @brief This task sends a message to the peer.
 */
struct doca_rdma_task_send;

/**
 * @brief Function to execute on completion of a send task.
 *
 * @details This function is called by doca_pe_progress() when a send task is successfully identified as completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed send task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_send_completion_cb_t)(struct doca_rdma_task_send *task,
						    union doca_data task_user_data,
						    union doca_data ctx_user_data);

/**
 * Check if a given device supports executing a send task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_send_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the send tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for send tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for send tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of send tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_send_set_conf(struct doca_rdma *rdma,
					  doca_rdma_task_send_completion_cb_t successful_task_completion_cb,
					  doca_rdma_task_send_completion_cb_t error_task_completion_cb,
					  uint32_t num_tasks);

/**
 * @brief This method allocates and initializes a send task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] src_buf
 * Local source buffer, with the data to be sent.
 * May be NULL when wishing to send an empty message (without data).
 * @note src_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized a send task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_send_allocate_init(struct doca_rdma *rdma,
					       struct doca_rdma_connection *rdma_connection,
					       const struct doca_buf *src_buf,
					       union doca_data user_data,
					       struct doca_rdma_task_send **task);

/**
 * @brief This method converts a send task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The send task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_send_as_task(struct doca_rdma_task_send *task);

/**
 * @brief This method sets the source buffer of a send task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] src_buf
 * Local source buffer, with the data to be sent.
 * May be NULL when wishing to send an empty message (without data).
 * If the source buffer is not set by the user, it will have a default value - NULL.
 * @note src_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_send_set_src_buf(struct doca_rdma_task_send *task, const struct doca_buf *src_buf);

/**
 * @brief This method gets the source buffer of a send task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's src_buf.
 */
DOCA_EXPERIMENTAL
const struct doca_buf *doca_rdma_task_send_get_src_buf(const struct doca_rdma_task_send *task);

/**
 * @brief This method sets the rdma_connection of a send task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_send_set_rdma_connection(struct doca_rdma_task_send *task,
					     struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a send task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_send_get_rdma_connection(const struct doca_rdma_task_send *task);

/********************************************
 * DOCA RDMA Task - Send with Immediate	    *
 ********************************************/

/**
 * @brief This task sends a message to the peer with a 32-bit immediate value sent OOB.
 */
struct doca_rdma_task_send_imm;

/**
 * @brief Function to execute on completion of a send with immediate task.
 *
 * @details This function is called by doca_pe_progress() when a send with immediate task is successfully identified as
 * completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed send with immediate task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_send_imm_completion_cb_t)(struct doca_rdma_task_send_imm *task,
							union doca_data task_user_data,
							union doca_data ctx_user_data);

/**
 * Check if a given device supports executing a send with immediate task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_send_imm_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the send with immediate tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for send with immediate tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for send with immediate tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of send with immediate tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_send_imm_set_conf(struct doca_rdma *rdma,
					      doca_rdma_task_send_imm_completion_cb_t successful_task_completion_cb,
					      doca_rdma_task_send_imm_completion_cb_t error_task_completion_cb,
					      uint32_t num_tasks);

/**
 * @brief This method allocates and initializes a send with immediate task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] src_buf
 * Local source buffer, with the data to be sent.
 * May be NULL when wishing to send an empty message (without data).
 * @note src_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 * @param [in] immediate_data
 * A 32-bit value, in Big-Endian, to be sent OOB to the peer along with the message.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized a send with immediate task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_send_imm_allocate_init(struct doca_rdma *rdma,
						   struct doca_rdma_connection *rdma_connection,
						   const struct doca_buf *src_buf,
						   doca_be32_t immediate_data,
						   union doca_data user_data,
						   struct doca_rdma_task_send_imm **task);

/**
 * @brief This method converts a send with immediate task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The send with immediate task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_send_imm_as_task(struct doca_rdma_task_send_imm *task);

/**
 * @brief This method sets the source buffer of a send with immediate task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] src_buf
 * Local source buffer, with the data to be sent.
 * May be NULL when wishing to send an empty message (without data).
 * If the source buffer is not set by the user, it will have a default value - NULL.
 * @note src_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_send_imm_set_src_buf(struct doca_rdma_task_send_imm *task, const struct doca_buf *src_buf);

/**
 * @brief This method gets the source buffer of a send with immediate task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's src_buf.
 */
DOCA_EXPERIMENTAL
const struct doca_buf *doca_rdma_task_send_imm_get_src_buf(const struct doca_rdma_task_send_imm *task);

/**
 * @brief This method sets the immediate data of a send with immediate task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] immediate_data
 * A 32-bit value, in Big-Endian, to be sent OOB to the peer along with the message.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_send_imm_set_immediate_data(struct doca_rdma_task_send_imm *task, doca_be32_t immediate_data);

/**
 * @brief This method gets the immediate data of a send with immediate task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's immediate_data.
 */
DOCA_EXPERIMENTAL
doca_be32_t doca_rdma_task_send_imm_get_immediate_data(const struct doca_rdma_task_send_imm *task);

/**
 * @brief This method sets the rdma_connection of a send with immediate task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_send_imm_set_rdma_connection(struct doca_rdma_task_send_imm *task,
						 struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a send with immediate task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_send_imm_get_rdma_connection(
	const struct doca_rdma_task_send_imm *task);

/********************************************
 * DOCA RDMA Task - Read		    *
 ********************************************/

/**
 * @brief This task reads data from remote memory, the memory of the peer.
 */
struct doca_rdma_task_read;

/**
 * @brief Function to execute on completion of a read task.
 *
 * @details This function is called by doca_pe_progress() when a read task is successfully identified as completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed read task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_read_completion_cb_t)(struct doca_rdma_task_read *task,
						    union doca_data task_user_data,
						    union doca_data ctx_user_data);

/**
 * Check if a given device supports executing a read task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_read_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the read tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for read tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for read tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of read tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_read_set_conf(struct doca_rdma *rdma,
					  doca_rdma_task_read_completion_cb_t successful_task_completion_cb,
					  doca_rdma_task_read_completion_cb_t error_task_completion_cb,
					  uint32_t num_tasks);

/**
 * @brief This method allocates and initializes a read task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] src_buf
 * Remote source buffer, holding the data that should be read.
 * May be NULL when wishing to read no data.
 * @note buffer lists are not supported for src_buf, only the head will be considered for this task.
 * @param [in] dst_buf
 * Local destination buffer, to which the read data will be written.
 * May be NULL when src_buf is NULL.
 * @note dst_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized a read task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_read_allocate_init(struct doca_rdma *rdma,
					       struct doca_rdma_connection *rdma_connection,
					       const struct doca_buf *src_buf,
					       struct doca_buf *dst_buf,
					       union doca_data user_data,
					       struct doca_rdma_task_read **task);

/**
 * @brief This method converts a read task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The read task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_read_as_task(struct doca_rdma_task_read *task);

/**
 * @brief This method sets the source buffer of a read task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] src_buf
 * Local source buffer, with the data to be sent.
 * May be NULL when wishing to send an empty message (without data).
 * If the source buffer is not set by the user, it will have a default value - NULL.
 * @note buffer lists are not supported for src_buf, only the head will be considered for this task.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_read_set_src_buf(struct doca_rdma_task_read *task, const struct doca_buf *src_buf);

/**
 * @brief This method gets the source buffer of a read task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's src_buf.
 */
DOCA_EXPERIMENTAL
const struct doca_buf *doca_rdma_task_read_get_src_buf(const struct doca_rdma_task_read *task);

/**
 * @brief This method sets the destination buffer of a read task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] dst_buf
 * Local destination buffer, to which the read data will be written.
 * May be NULL when src_buf is NULL.
 * If the destination buffer is not set by the user, it will have a default value - NULL.
 * @note dst_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_read_set_dst_buf(struct doca_rdma_task_read *task, struct doca_buf *dst_buf);

/**
 * @brief This method gets the destination buffer of a read task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's dst_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_read_get_dst_buf(const struct doca_rdma_task_read *task);

/**
 * @brief This method gets the length of data read by the task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * Total length of read data in case of completion.
 * @note Valid only on successful completion of the task. Otherwise, undefined behavior.
 */
DOCA_EXPERIMENTAL
uint32_t doca_rdma_task_read_get_result_len(const struct doca_rdma_task_read *task);

/**
 * @brief This method sets the rdma_connection of a read task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_read_set_rdma_connection(struct doca_rdma_task_read *task,
					     struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a read task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_read_get_rdma_connection(const struct doca_rdma_task_read *task);

/********************************************
 * DOCA RDMA Task - Write		    *
 ********************************************/

/**
 * @brief This task writes data to the remote memory, the memory of the peer.
 */
struct doca_rdma_task_write;

/**
 * @brief Function to execute on completion of a write task.
 *
 * @details This function is called by doca_pe_progress() when a write task is successfully identified as completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed write task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_write_completion_cb_t)(struct doca_rdma_task_write *task,
						     union doca_data task_user_data,
						     union doca_data ctx_user_data);

/**
 * Check if a given device supports executing a write task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_write_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the write tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for write tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for write tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of write tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_write_set_conf(struct doca_rdma *rdma,
					   doca_rdma_task_write_completion_cb_t successful_task_completion_cb,
					   doca_rdma_task_write_completion_cb_t error_task_completion_cb,
					   uint32_t num_tasks);

/**
 * @brief This method allocates and initializes a write task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] src_buf
 * Local source buffer, holding the data that should be written to the remote memory.
 * May be NULL when wishing to write no data.
 * @note src_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 * @param [in] dst_buf
 * Remote destination buffer, to which the data will be written.
 * May be NULL when src_buf is NULL.
 * @note buffer lists are not supported for dst_buf, only the head will be considered for this task.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized a write task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_write_allocate_init(struct doca_rdma *rdma,
						struct doca_rdma_connection *rdma_connection,
						const struct doca_buf *src_buf,
						struct doca_buf *dst_buf,
						union doca_data user_data,
						struct doca_rdma_task_write **task);

/**
 * @brief This method converts a write task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The write task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_write_as_task(struct doca_rdma_task_write *task);

/**
 * @brief This method sets the source buffer of a write task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] src_buf
 * Local source buffer, holding the data that should be written to the remote memory.
 * May be NULL when wishing to write no data.
 * If the source buffer is not set by the user, it will have a default value - NULL.
 * @note src_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_write_set_src_buf(struct doca_rdma_task_write *task, const struct doca_buf *src_buf);

/**
 * @brief This method gets the source buffer of a write task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's src_buf.
 */
DOCA_EXPERIMENTAL
const struct doca_buf *doca_rdma_task_write_get_src_buf(const struct doca_rdma_task_write *task);

/**
 * @brief This method sets the destination buffer of a write task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] dst_buf
 * Remote destination buffer, to which the data will be written.
 * May be NULL when src_buf is NULL.
 * If the destination buffer is not set by the user, it will have a default value - NULL.
 * @note buffer lists are not supported for dst_buf, only the head will be considered for this task.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_write_set_dst_buf(struct doca_rdma_task_write *task, struct doca_buf *dst_buf);

/**
 * @brief This method gets the destination buffer of a write task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's dst_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_write_get_dst_buf(const struct doca_rdma_task_write *task);

/**
 * @brief This method sets the rdma_connection of a write task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_write_set_rdma_connection(struct doca_rdma_task_write *task,
					      struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a write task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_write_get_rdma_connection(const struct doca_rdma_task_write *task);

/********************************************
 * DOCA RDMA Task - Write with Immediate    *
 ********************************************/

/**
 * @brief This task writes data to the remote memory, the memory of the peer, along with a 32-bit immediate value sent
 * to the peer OOB.
 */
struct doca_rdma_task_write_imm;

/**
 * @brief Function to execute on completion of a write with immediate task.
 *
 * @details This function is called by doca_pe_progress() when a write with immediate task is successfully identified as
 * completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed write with immediate task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_write_imm_completion_cb_t)(struct doca_rdma_task_write_imm *task,
							 union doca_data task_user_data,
							 union doca_data ctx_user_data);

/**
 * Check if a given device supports executing a write with immediate task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_write_imm_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the write with immediate tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for write with immediate tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for write with immediate tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of write with immediate tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_write_imm_set_conf(struct doca_rdma *rdma,
					       doca_rdma_task_write_imm_completion_cb_t successful_task_completion_cb,
					       doca_rdma_task_write_imm_completion_cb_t error_task_completion_cb,
					       uint32_t num_tasks);

/**
 * @brief This method allocates and initializes a write with immediate task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] src_buf
 * Local source buffer, holding the data that should be written to the remote memory.
 * May be NULL when wishing to write no data.
 * @note src_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 * @param [in] dst_buf
 * Remote destination buffer, to which the data will be written.
 * May be NULL when src_buf is NULL.
 * @note buffer lists are not supported for dst_buf, only the head will be considered for this task.
 * @param [in] immediate_data
 * A 32-bit value, in Big-Endian, to be sent OOB to the peer along with the write data.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized a write with immediate task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_write_imm_allocate_init(struct doca_rdma *rdma,
						    struct doca_rdma_connection *rdma_connection,
						    const struct doca_buf *src_buf,
						    struct doca_buf *dst_buf,
						    doca_be32_t immediate_data,
						    union doca_data user_data,
						    struct doca_rdma_task_write_imm **task);

/**
 * @brief This method converts a write with immediate task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The write with immediate task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_write_imm_as_task(struct doca_rdma_task_write_imm *task);

/**
 * @brief This method sets the source buffer of a write with immediate task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] src_buf
 * Local source buffer, holding the data that should be written to the remote memory.
 * May be NULL when wishing to write no data.
 * If the source buffer is not set by the user, it will have a default value - NULL.
 * @note src_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_write_imm_set_src_buf(struct doca_rdma_task_write_imm *task, const struct doca_buf *src_buf);

/**
 * @brief This method gets the source buffer of a write with immediate task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's src_buf.
 */
DOCA_EXPERIMENTAL
const struct doca_buf *doca_rdma_task_write_imm_get_src_buf(const struct doca_rdma_task_write_imm *task);

/**
 * @brief This method sets the destination buffer of a write with immediate task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] dst_buf
 * Remote destination buffer, to which the data will be written.
 * May be NULL when src_buf is NULL.
 * If the destination buffer is not set by the user, it will have a default value - NULL.
 * @note buffer lists are not supported for dst_buf, only the head will be considered for this task.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_write_imm_set_dst_buf(struct doca_rdma_task_write_imm *task, struct doca_buf *dst_buf);

/**
 * @brief This method gets the destination buffer of a write with immediate task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's dst_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_write_imm_get_dst_buf(const struct doca_rdma_task_write_imm *task);

/**
 * @brief This method sets the immediate data of a write with immediate task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] immediate_data
 * A 32-bit value, in Big-Endian, to be sent OOB to the peer along with the write data.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_write_imm_set_immediate_data(struct doca_rdma_task_write_imm *task, doca_be32_t immediate_data);

/**
 * @brief This method gets the immediate data of a write with immediate task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's immediate_data.
 */
DOCA_EXPERIMENTAL
doca_be32_t doca_rdma_task_write_imm_get_immediate_data(const struct doca_rdma_task_write_imm *task);

/**
 * @brief This method sets the rdma connection of a write with immediate task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_write_imm_set_rdma_connection(struct doca_rdma_task_write_imm *task,
						  struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a write with immediate task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_write_imm_get_rdma_connection(
	const struct doca_rdma_task_write_imm *task);

/********************************************
 * DOCA RDMA Task - Atomic Compare and Swap *
 ********************************************/

/**
 * @brief This task compares an 8-byte value in the remote memory (the memory of the peer) to a given 8-byte value.
 * If these values are equal, the remote 8-byte value is swapped with another given 8-byte value, and otherwise it is
 * left without change.
 * The original remote 8-byte value (before the swap, if occurred) is written to a given local buffer.
 *
 * @note The process of reading the original remote 8-byte value, comparing it and swapping it, is atomic.
 */
struct doca_rdma_task_atomic_cmp_swp;

/**
 * @brief Function to execute on completion of an atomic compare and swap task.
 *
 * @details This function is called by doca_pe_progress() when an atomic compare and swap task is successfully
 * identified as completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed atomic compare and swap task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_atomic_cmp_swp_completion_cb_t)(struct doca_rdma_task_atomic_cmp_swp *task,
							      union doca_data task_user_data,
							      union doca_data ctx_user_data);

/**
 * Check if a given device supports executing an atomic compare and swap task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_atomic_cmp_swp_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the atomic compare and swap tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for atomic compare and swap tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for atomic compare and swap tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of atomic compare and swap tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_atomic_cmp_swp_set_conf(
	struct doca_rdma *rdma,
	doca_rdma_task_atomic_cmp_swp_completion_cb_t successful_task_completion_cb,
	doca_rdma_task_atomic_cmp_swp_completion_cb_t error_task_completion_cb,
	uint32_t num_tasks);

/**
 * @brief This method allocates and initializes an atomic compare and swap task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] dst_buf
 * Remote destination buffer, on which the atomic 8-byte operation will be executed.
 * @note buffer lists are not supported for dst_buf, only the first 8-bytes of data in the head buffer will be
 * considered for this task.
 * @param [in] result_buf
 * Local buffer, to which the original remote 8-byte value (before the swap, if occurred) will be written.
 * @note buffer lists are not supported for result_buf, only the head will be considered for this task.
 * @param [in] cmp_data
 * An 8-byte value that will be compared to the remote 8-byte value.
 * @param [in] swap_data
 * An 8-byte value that will be written to dst_buf, overwriting it's previous data, in case cmp_data is equal to the
 * original remote 8-byte value.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized an atomic compare and swap task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_atomic_cmp_swp_allocate_init(struct doca_rdma *rdma,
							 struct doca_rdma_connection *rdma_connection,
							 struct doca_buf *dst_buf,
							 struct doca_buf *result_buf,
							 uint64_t cmp_data,
							 uint64_t swap_data,
							 union doca_data user_data,
							 struct doca_rdma_task_atomic_cmp_swp **task);

/**
 * @brief This method converts an atomic compare and swap task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The atomic compare and swap task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_atomic_cmp_swp_as_task(struct doca_rdma_task_atomic_cmp_swp *task);

/**
 * @brief This method sets the destination buffer of an atomic compare and swap task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] dst_buf
 * Remote destination buffer, on which the atomic 8-byte operation will be executed.
 * @note buffer lists are not supported for dst_buf, only the first 8-bytes of data in the head buffer will be
 * considered for this task.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_atomic_cmp_swp_set_dst_buf(struct doca_rdma_task_atomic_cmp_swp *task, struct doca_buf *dst_buf);

/**
 * @brief This method gets the destination buffer of an atomic compare and swap task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's dst_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_atomic_cmp_swp_get_dst_buf(const struct doca_rdma_task_atomic_cmp_swp *task);

/**
 * @brief This method sets the result buffer of an atomic compare and swap task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] result_buf
 * Local buffer, to which the original remote 8-byte value (before the swap, if occurred) will be written.
 * @note buffer lists are not supported for result_buf, only the head will be considered for this task.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_atomic_cmp_swp_set_result_buf(struct doca_rdma_task_atomic_cmp_swp *task,
						  struct doca_buf *result_buf);

/**
 * @brief This method gets the result buffer of an atomic compare and swap task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's result_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_atomic_cmp_swp_get_result_buf(const struct doca_rdma_task_atomic_cmp_swp *task);

/**
 * @brief This method sets the compare data of an atomic compare and swap task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] cmp_data
 * An 8-byte value that will be compared to the remote 8-byte value.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_atomic_cmp_swp_set_cmp_data(struct doca_rdma_task_atomic_cmp_swp *task, uint64_t cmp_data);

/**
 * @brief This method gets the compare data of an atomic compare and swap task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's cmp_data.
 */
DOCA_EXPERIMENTAL
uint64_t doca_rdma_task_atomic_cmp_swp_get_cmp_data(const struct doca_rdma_task_atomic_cmp_swp *task);

/**
 * @brief This method sets the swap data of an atomic compare and swap task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] swap_data
 * An 8-byte value that will be written to dst_buf, overwriting it's previous data, in case cmp_data is equal to the
 * original remote 8-byte value.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_atomic_cmp_swp_set_swap_data(struct doca_rdma_task_atomic_cmp_swp *task, uint64_t swap_data);

/**
 * @brief This method gets the swap data of an atomic compare and swap task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's swap_data.
 */
DOCA_EXPERIMENTAL
uint64_t doca_rdma_task_atomic_cmp_swp_get_swap_data(const struct doca_rdma_task_atomic_cmp_swp *task);

/**
 * @brief This method sets the rdma_connection of a atomic compare and swap task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_atomic_cmp_swp_set_rdma_connection(struct doca_rdma_task_atomic_cmp_swp *task,
						       struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a atomic compare and swap task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_atomic_cmp_swp_get_rdma_connection(
	const struct doca_rdma_task_atomic_cmp_swp *task);

/********************************************
 * DOCA RDMA Task - Atomic Fetch and Add    *
 ********************************************/

/**
 * @brief This task adds a given 8-byte value to an 8-byte value in the remote memory, the memory of the peer.
 * The original remote 8-byte value (before the addition) is written to a given local buffer.
 *
 * @note The process of reading the original remote 8-byte value and adding to it, is atomic.
 */
struct doca_rdma_task_atomic_fetch_add;

/**
 * @brief Function to execute on completion of an atomic fetch and add task.
 *
 * @details This function is called by doca_pe_progress() when an atomic fetch and add task is successfully identified
 * as completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed atomic fetch and add task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_atomic_fetch_add_completion_cb_t)(struct doca_rdma_task_atomic_fetch_add *task,
								union doca_data task_user_data,
								union doca_data ctx_user_data);

/**
 * Check if a given device supports executing an atomic fetch and add task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_atomic_fetch_add_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the atomic fetch and add tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for atomic fetch and add tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for atomic fetch and add tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of atomic fetch and add tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_atomic_fetch_add_set_conf(
	struct doca_rdma *rdma,
	doca_rdma_task_atomic_fetch_add_completion_cb_t successful_task_completion_cb,
	doca_rdma_task_atomic_fetch_add_completion_cb_t error_task_completion_cb,
	uint32_t num_tasks);

/**
 * @brief This method allocates and initializes an atomic fetch and add task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] dst_buf
 * Remote destination buffer, on which the atomic 8-byte operation will be executed.
 * @note buffer lists are not supported for dst_buf, only the first 8-bytes of data in the head buffer will be
 * considered for this task.
 * @param [in] result_buf
 * Local buffer, to which the original remote 8-byte value (before the addition) will be written.
 * @note buffer lists are not supported for result_buf, only the head will be considered for this task.
 * @param [in] add_data
 * An 8-byte value that will be added to the remote 8-byte value in dst_buf.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized an atomic fetch and add task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_atomic_fetch_add_allocate_init(struct doca_rdma *rdma,
							   struct doca_rdma_connection *rdma_connection,
							   struct doca_buf *dst_buf,
							   struct doca_buf *result_buf,
							   uint64_t add_data,
							   union doca_data user_data,
							   struct doca_rdma_task_atomic_fetch_add **task);

/**
 * @brief This method converts an atomic fetch and add task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The atomic fetch and add task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_atomic_fetch_add_as_task(struct doca_rdma_task_atomic_fetch_add *task);

/**
 * @brief This method sets the destination buffer of an atomic fetch and add task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] dst_buf
 * Remote destination buffer, on which the atomic 8-byte operation will be executed.
 * @note buffer lists are not supported for dst_buf, only the first 8-bytes of data in the head buffer will be
 * considered for this task.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_atomic_fetch_add_set_dst_buf(struct doca_rdma_task_atomic_fetch_add *task,
						 struct doca_buf *dst_buf);

/**
 * @brief This method gets the destination buffer of an atomic fetch and add task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's dst_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_atomic_fetch_add_get_dst_buf(const struct doca_rdma_task_atomic_fetch_add *task);

/**
 * @brief This method sets the result buffer of an atomic fetch and add task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] result_buf
 * Local buffer, to which the original remote 8-byte value (before the addition) will be written.
 * @note buffer lists are not supported for result_buf, only the head will be considered for this task.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_atomic_fetch_add_set_result_buf(struct doca_rdma_task_atomic_fetch_add *task,
						    struct doca_buf *result_buf);

/**
 * @brief This method gets the result buffer of an atomic fetch and add task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's result_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_atomic_fetch_add_get_result_buf(const struct doca_rdma_task_atomic_fetch_add *task);

/**
 * @brief This method sets the add data of an atomic fetch and add task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] add_data
 * An 8-byte value that will be added to the remote 8-byte value in dst_buf.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_atomic_fetch_add_set_add_data(struct doca_rdma_task_atomic_fetch_add *task, uint64_t add_data);

/**
 * @brief This method gets the add data of an atomic fetch and add task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's add_data.
 */
DOCA_EXPERIMENTAL
uint64_t doca_rdma_task_atomic_fetch_add_get_add_data(const struct doca_rdma_task_atomic_fetch_add *task);

/**
 * @brief This method sets the rdma_connection of a atomic fetch and add task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_atomic_fetch_add_set_rdma_connection(struct doca_rdma_task_atomic_fetch_add *task,
							 struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a atomic fetch and add task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_atomic_fetch_add_get_rdma_connection(
	const struct doca_rdma_task_atomic_fetch_add *task);

/**********************************************
 * DOCA RDMA Task - Remote Net Sync Event Get *
 **********************************************/

/**
 * @brief This task reads the value of a remote net sync event.
 */
struct doca_rdma_task_remote_net_sync_event_get;

/**
 * @brief Function to execute on completion of a remote_net_sync_event_get task.
 *
 * @details This function is called by doca_pe_progress() when a remote_net_sync_event_get task
 * is successfully identified as completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed remote_net_sync_event_get task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_remote_net_sync_event_get_completion_cb_t)(
	struct doca_rdma_task_remote_net_sync_event_get *task,
	union doca_data task_user_data,
	union doca_data ctx_user_data);

/**
 * Check if a given device supports executing a remote_net_sync_event_get task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_remote_net_sync_event_get_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the remote_net_sync_event_get tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for remote_net_sync_event_get tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for remote_net_sync_event_get tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of remote_net_sync_event_get tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_remote_net_sync_event_get_set_conf(
	struct doca_rdma *rdma,
	doca_rdma_task_remote_net_sync_event_get_completion_cb_t successful_task_completion_cb,
	doca_rdma_task_remote_net_sync_event_get_completion_cb_t error_task_completion_cb,
	uint32_t num_tasks);

/**
 * @brief This method allocates and initializes a remote_net_sync_event_get task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] event
 * Remote net sync event to read its value.
 * @param [in] dst_buf
 * Local destination buffer, to which the read data will be written.
 * @note dst_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized a remote_net_sync_event_get task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_remote_net_sync_event_get_allocate_init(
	struct doca_rdma *rdma,
	struct doca_rdma_connection *rdma_connection,
	const struct doca_sync_event_remote_net *event,
	struct doca_buf *dst_buf,
	union doca_data user_data,
	struct doca_rdma_task_remote_net_sync_event_get **task);

/**
 * @brief This method converts a remote_net_sync_event_get task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The remote_net_sync_event_get task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_remote_net_sync_event_get_as_task(
	struct doca_rdma_task_remote_net_sync_event_get *task);

/**
 * @brief This method sets the remote net sync event of a remote_net_sync_event_get task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] event
 * Remote net sync event to read its value.
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_get_set_sync_event(struct doca_rdma_task_remote_net_sync_event_get *task,
							     const struct doca_sync_event_remote_net *event);

/**
 * @brief This method gets the remote net sync event of a remote_net_sync_event_get task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's sync_event.
 */
DOCA_EXPERIMENTAL
const struct doca_sync_event_remote_net *doca_rdma_task_remote_net_sync_event_get_get_sync_event(
	const struct doca_rdma_task_remote_net_sync_event_get *task);

/**
 * @brief This method sets the destination buffer of a remote_net_sync_event_get task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] dst_buf
 * Local destination buffer, to which the remote_net_sync_event_get data will be written.
 * @note dst_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_get_set_dst_buf(struct doca_rdma_task_remote_net_sync_event_get *task,
							  struct doca_buf *dst_buf);

/**
 * @brief This method gets the destination buffer of a remote_net_sync_event_get task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's dst_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_remote_net_sync_event_get_get_dst_buf(
	const struct doca_rdma_task_remote_net_sync_event_get *task);

/**
 * @brief This method gets the length of data read by the task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * Total length of read data in case of completion.
 * @note Valid only on successful completion of the task. Otherwise, undefined behavior.
 */
DOCA_EXPERIMENTAL
uint32_t doca_rdma_task_remote_net_sync_event_get_get_result_len(
	const struct doca_rdma_task_remote_net_sync_event_get *task);

/**
 * @brief This method sets the rdma_connection of a remote_net_sync_event_get task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_get_set_rdma_connection(struct doca_rdma_task_remote_net_sync_event_get *task,
								  struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a remote_net_sync_event_get task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_remote_net_sync_event_get_get_rdma_connection(
	const struct doca_rdma_task_remote_net_sync_event_get *task);

/********************************************************************
 * DOCA RDMA Task - Remote Net Sync Event Notify Set		    *
 ********************************************************************/

/**
 * @brief This task sets the value of a remote net sync event to a given value.
 */
struct doca_rdma_task_remote_net_sync_event_notify_set;

/**
 * @brief Function to execute on completion of a remote_net_sync_event_notify_set task.
 *
 * @details This function is called by doca_pe_progress() when a remote_net_sync_event_notify_set task
 * is successfully identified as completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed remote_net_sync_event_notify_set task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_remote_net_sync_event_notify_set_completion_cb_t)(
	struct doca_rdma_task_remote_net_sync_event_notify_set *task,
	union doca_data task_user_data,
	union doca_data ctx_user_data);

/**
 * Check if a given device supports executing a remote_net_sync_event_notify_set task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_remote_net_sync_event_notify_set_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the remote_net_sync_event_notify_set tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for remote_net_sync_event_notify_set tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for remote_net_sync_event_notify_set tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of remote_net_sync_event_notify_set tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_remote_net_sync_event_notify_set_set_conf(
	struct doca_rdma *rdma,
	doca_rdma_task_remote_net_sync_event_notify_set_completion_cb_t successful_task_completion_cb,
	doca_rdma_task_remote_net_sync_event_notify_set_completion_cb_t error_task_completion_cb,
	uint32_t num_tasks);

/**
 * @brief This method allocates and initializes a remote_net_sync_event_notify_set task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] event
 * The remote sync event to set.
 * @param [in] src_buf
 * Local source buffer, holding the value to set the remote net sync event to.
 * @note src_buf may be linked to other buffers, with a limit according to the max_send_buf_list_len property that can
 * be set or queried using doca_rdma_set_max_send_buf_list_len() \ doca_rdma_get_max_send_buf_list_len() respectfully.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized a remote_net_sync_event_notify_set task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_remote_net_sync_event_notify_set_allocate_init(
	struct doca_rdma *rdma,
	struct doca_rdma_connection *rdma_connection,
	struct doca_sync_event_remote_net *event,
	const struct doca_buf *src_buf,
	union doca_data user_data,
	struct doca_rdma_task_remote_net_sync_event_notify_set **task);

/**
 * @brief This method converts a remote_net_sync_event_notify_set task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The remote_net_sync_event_notify_set task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_remote_net_sync_event_notify_set_as_task(
	struct doca_rdma_task_remote_net_sync_event_notify_set *task);

/**
 * @brief This method sets the remote net sync event of a remote_net_sync_event_notify_set task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] event
 * The remote net sync event to set.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_notify_set_set_sync_event(
	struct doca_rdma_task_remote_net_sync_event_notify_set *task,
	struct doca_sync_event_remote_net *event);

/**
 * @brief This method gets the remote net sync event of a remote_net_sync_event_notify_set task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's remote net sync event.
 */
DOCA_EXPERIMENTAL
struct doca_sync_event_remote_net *doca_rdma_task_remote_net_sync_event_notify_set_get_sync_event(
	const struct doca_rdma_task_remote_net_sync_event_notify_set *task);

/**
 * @brief This method sets the rdma_connection of a remote_net_sync_event_notify_set task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_notify_set_set_rdma_connection(
	struct doca_rdma_task_remote_net_sync_event_notify_set *task,
	struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a remote_net_sync_event_notify_set task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_remote_net_sync_event_notify_set_get_rdma_connection(
	const struct doca_rdma_task_remote_net_sync_event_notify_set *task);

/**
 * @brief This method sets the source buffer of a remote_net_sync_event_notify_set task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] src_buf
 * Local source buffer, holding the value to set the remote net sync event to.
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_notify_set_set_src_buf(
	struct doca_rdma_task_remote_net_sync_event_notify_set *task,
	const struct doca_buf *src_buf);

/**
 * @brief This method gets the source buffer of a remote_net_sync_event_notify_set task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's src_buf.
 */
DOCA_EXPERIMENTAL
const struct doca_buf *doca_rdma_task_remote_net_sync_event_notify_set_get_src_buf(
	const struct doca_rdma_task_remote_net_sync_event_notify_set *task);

/********************************************************
 * DOCA RDMA Task - Remote Net Sync Event Notify Add    *
 ********************************************************/

/**
 * @brief This task adds a given value to the value of a remote net sync event atomically.
 * The original remote 8-byte value (before the addition) is written to a given local buffer.
 */
struct doca_rdma_task_remote_net_sync_event_notify_add;

/**
 * @brief Function to execute on completion of a remote_net_sync_event_notify_add task.
 *
 * @details This function is called by doca_pe_progress() when a remote_net_sync_event_notify_add task is
 * successfully identified as completed.
 * When this function is called the ownership of the task object passes from DOCA back to user.
 * Inside this callback the user may decide on the task object:
 * - re-submit task with doca_task_submit(); task object ownership passed to DOCA
 * - release task with doca_task_free(); task object ownership passed to DOCA
 * - keep the task object for future reuse; user keeps the ownership on the task object
 * Inside this callback the user shouldn't call doca_pe_progress().
 * Please see doca_pe_progress() for details.
 *
 * Any failure/error inside this function should be handled internally or deferred;
 * Since this function is nested in the execution of doca_pe_progress(), this callback doesn't return an error.
 *
 * @note This callback type is utilized for both successful & failed task completions.
 *
 * @param [in] task
 * The completed remote_net_sync_event_notify_add task.
 * @note The implementation can assume this value is not NULL.
 * @param [in] task_user_data
 * The doca_data supplied to the task by the application (during task allocation or by a setter).
 * @param [in] ctx_user_data
 * The doca_data supplied to the doca_ctx by the application (using a setter).
 */
typedef void (*doca_rdma_task_remote_net_sync_event_notify_add_completion_cb_t)(
	struct doca_rdma_task_remote_net_sync_event_notify_add *task,
	union doca_data task_user_data,
	union doca_data ctx_user_data);

/**
 * Check if a given device supports executing a remote_net_sync_event_notify_add task.
 *
 * @param [in] devinfo
 * The DOCA device information that should be queried.
 *
 * @return
 * DOCA_SUCCESS - in case device supports the task.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support the task.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_cap_task_remote_net_sync_event_notify_add_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief This method sets the remote_net_sync_event_notify_add tasks configuration.
 *
 * @param [in] rdma
 * The RDMA instance to config.
 * @param [in] successful_task_completion_cb
 * A callback function for remote_net_sync_event_notify_add tasks that were completed successfully.
 * @param [in] error_task_completion_cb
 * A callback function for remote_net_sync_event_notify_add tasks that were completed with an error.
 * @param [in] num_tasks
 * Number of remote_net_sync_event_notify_add tasks.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - the RDMA instance is not idle.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_remote_net_sync_event_notify_add_set_conf(
	struct doca_rdma *rdma,
	doca_rdma_task_remote_net_sync_event_notify_add_completion_cb_t successful_task_completion_cb,
	doca_rdma_task_remote_net_sync_event_notify_add_completion_cb_t error_task_completion_cb,
	uint32_t num_tasks);

/**
 * @brief This method allocates and initializes a remote_net_sync_event_notify_add task.
 *
 * @param [in] rdma
 * The RDMA instance to allocate the task for.
 * @param [in] rdma_connection
 * Connection related data required for doca_rdma to connect.
 * @param [in] event
 * Remote sync event to atomically increment by a given value.
 * @param [in] result_buf
 * Local buffer, to which the original remote sync event value (before the addition) will be written.
 * @note buffer lists are not supported for result_buf, only the head will be considered for this task.
 * @param [in] add_data
 * An 8-byte value that will be added to the remote sync event value.
 * @param [in] user_data
 * doca_data to attach to the task.
 * @param [out] task
 * On success, an allocated and initialized a remote_net_sync_event_notify_add task.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no more tasks to allocate.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_task_remote_net_sync_event_notify_add_allocate_init(
	struct doca_rdma *rdma,
	struct doca_rdma_connection *rdma_connection,
	struct doca_sync_event_remote_net *event,
	struct doca_buf *result_buf,
	uint64_t add_data,
	union doca_data user_data,
	struct doca_rdma_task_remote_net_sync_event_notify_add **task);

/**
 * @brief This method converts a remote_net_sync_event_notify_add task to a doca_task.
 *
 * @param [in] task
 * The task that should be converted.
 *
 * @return
 * The remote_net_sync_event_notify_add task converted to doca_task.
 */
DOCA_EXPERIMENTAL
struct doca_task *doca_rdma_task_remote_net_sync_event_notify_add_as_task(
	struct doca_rdma_task_remote_net_sync_event_notify_add *task);

/**
 * @brief This method sets the remote sync event of a remote_net_sync_event_notify_add task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] event
 * The remote sync event to increment atomically.
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_notify_add_set_sync_event(
	struct doca_rdma_task_remote_net_sync_event_notify_add *task,
	struct doca_sync_event_remote_net *event);

/**
 * @brief This method gets the remote sync event of a remote_net_sync_event_notify_add task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's sync event.
 */
DOCA_EXPERIMENTAL
struct doca_sync_event_remote_net *doca_rdma_task_remote_net_sync_event_notify_add_get_sync_event(
	const struct doca_rdma_task_remote_net_sync_event_notify_add *task);

/**
 * @brief This method sets the result buffer of a remote_net_sync_event_notify_add task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] result_buf
 * Local buffer, to which the original remote sync event value (before the addition) will be written.
 * @note buffer lists are not supported for result_buf, only the head will be considered for this task.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_notify_add_set_result_buf(
	struct doca_rdma_task_remote_net_sync_event_notify_add *task,
	struct doca_buf *result_buf);

/**
 * @brief This method gets the result buffer of a remote_net_sync_event_notify_add task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's result_buf.
 */
DOCA_EXPERIMENTAL
struct doca_buf *doca_rdma_task_remote_net_sync_event_notify_add_get_result_buf(
	const struct doca_rdma_task_remote_net_sync_event_notify_add *task);

/**
 * @brief This method sets the add data of a remote_net_sync_event_notify_add task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] add_data
 * An 8-byte value that will be atomically added to the remote sync event.
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_notify_add_set_add_data(
	struct doca_rdma_task_remote_net_sync_event_notify_add *task,
	uint64_t add_data);

/**
 * @brief This method gets the add data of a remote_net_sync_event_notify_add task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's add_data.
 */
DOCA_EXPERIMENTAL
uint64_t doca_rdma_task_remote_net_sync_event_notify_add_get_add_data(
	const struct doca_rdma_task_remote_net_sync_event_notify_add *task);

/**
 * @brief This method sets the rdma_connection of a remote_net_sync_event_notify_add task.
 *
 * @param [in] task
 * The task to set.
 * @param [in] rdma_connection
 * RDMA Connection to use on this task
 *
 */
DOCA_EXPERIMENTAL
void doca_rdma_task_remote_net_sync_event_notify_add_set_rdma_connection(
	struct doca_rdma_task_remote_net_sync_event_notify_add *task,
	struct doca_rdma_connection *rdma_connection);

/**
 * @brief This method gets the rdma connection of a remote_net_sync_event_notify_add task.
 *
 * @param [in] task
 * The task that should be queried.
 *
 * @return
 * The task's rdma connection.
 */
DOCA_EXPERIMENTAL
const struct doca_rdma_connection *doca_rdma_task_remote_net_sync_event_notify_add_get_rdma_connection(
	const struct doca_rdma_task_remote_net_sync_event_notify_add *task);

/*********************************************************************************************************************
 * DOCA RDMA DPA Completion context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA DPA Completion instance.
 */
struct doca_dpa_completion;

/**
 * @brief Attach DOCA RDMA to DPA completion context
 *
 * This function must be called before DOCA RDMA context is started
 *
 * @note This API is relevant only for contexts that are set on DPA datapath,
 * using doca_ctx_set_datapath_on_dpa() before calling doca_ctx_start().
 *
 * @param [in] rdma
 * doca_rdma context
 * @param[in] dpa_comp
 * DPA completion context
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - if context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_rdma_dpa_completion_attach(struct doca_rdma *rdma, struct doca_dpa_completion *dpa_comp);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_RDMA_H_ */

/** @} */
