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

/**
 * @file doca_flow_tune_server.h
 * @page doca_flow_tune server
 * @defgroup DOCA_FLOW_TUNE_SERVER DOCA Flow Tune Server
 * @ingroup DOCA_FLOW
 *
 * @{
 */

#ifndef DOCA_TUNE_SERVER_H_
#define DOCA_TUNE_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <doca_flow.h>

/**
 * @brief DOCA Flow Tune Server mode.
 */
enum doca_flow_tune_server_mode {
	DOCA_FLOW_TUNE_SERVER_MODE_BASIC = 0, /**< Launch server thread */
	DOCA_FLOW_TUNE_SERVER_MODE_EXPERT,    /**< External thread and polling - no multi-threaded support */
};

/**
 * @brief DOCA Flow Tune Server global configurations.
 */
struct doca_flow_tune_server_cfg;

/**
 * @brief Create DOCA Flow Tune Server configuration struct.
 *
 * @param [out] cfg
 * DOCA Flow Tune Server global configuration ptr address.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_server_cfg_create(struct doca_flow_tune_server_cfg **cfg);

/**
 * @brief Set Tune Server configuration file path.
 *
 * @param [in] cfg
 * DOCA Flow Tune Server global configuration.
 * @param [in] path
 * Configuration file path.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_server_cfg_set_cfg_file_path(struct doca_flow_tune_server_cfg *cfg, const char *path);

/**
 * @brief Set Tune Server mode.
 *
 * @param [in] cfg
 * DOCA Flow Tune Server global configuration.
 * @param [in] mode
 * Tune Server mode.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_server_cfg_set_mode(struct doca_flow_tune_server_cfg *cfg,
						enum doca_flow_tune_server_mode mode);

/**
 * @brief Set Tune Server file descriptor (expert mode only).
 *
 * The socket family should be AF_UNIX and type SOCK_DGRAM.
 *
 * @param [in] cfg
 * DOCA Flow Tune Server global configuration.
 * @param [in] fd
 * File descriptor for the socket.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_server_cfg_set_fd(struct doca_flow_tune_server_cfg *cfg, int fd);

/**
 * @brief Destroy DOCA Flow Tune Server configuration struct.
 *
 * @param [in] cfg
 * DOCA Flow Tune Server global configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_server_cfg_destroy(struct doca_flow_tune_server_cfg *cfg);

/**
 * @brief Initialize a DOCA Flow Tune Server.
 *
 * This is the global initialization function for DOCA Flow Tune Server.
 * It initializes all resources used by DOCA Flow Tune Server.
 *
 * Should be called after doca_flow_init().
 *
 * @param [in] cfg
 * DOCA Flow Tune Server global configuration.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_NOT_SUPPORTED - functionality isn't supported in this (runtime) version.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_server_init(const struct doca_flow_tune_server_cfg *cfg);

/**
 * @brief Destroy the DOCA Flow Tune Server.
 *
 * Release all the resources used by DOCA Flow Tune Server.
 * Should be invoked before doca_flow_destroy() upon application termination.
 *
 */
DOCA_EXPERIMENTAL
void doca_flow_tune_server_destroy(void);

/**
 * @brief Process message on tune server socket (expert mode only).
 *
 * This function should be called after polling indicates the socket is ready for reading.
 * It processes incoming messages and sends responses. Uses the fd configured
 * during initialization.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input or not in expert mode.
 * - DOCA_ERROR_AGAIN - no data available on socket (EAGAIN/EWOULDBLOCK).
 * - DOCA_ERROR_IO_FAILED - I/O operation failed.
 * - DOCA_ERROR_UNKNOWN - other system errors.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_server_process_msg(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_TUNE_SERVER_H_ */
