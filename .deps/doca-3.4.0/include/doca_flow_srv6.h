/*
 * Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_flow_srv6.h
 * @page doca_flow_srv6
 * @defgroup DOCA_FLOW_SRV6 DOCA Flow SRv6
 * @ingroup DOCA_FLOW
 * DOCA Flow SRv6 external action support for SRH header insertion.
 *
 * @{
 */

#ifndef DOCA_FLOW_SRV6_H_
#define DOCA_FLOW_SRV6_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_flow.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief memory required for SRv6 actions is number of SRv6 different SRH * this define.
 * SRv6 memory should be added to actions memory size set in doca_flow_cfg_actions_mem_size()
 */
#define DOCA_FLOW_SRV6_ACTION_MEM_SIZE (128)

/* Forward declarations */
struct doca_flow_header_ipv6_srh;

/**
 * @brief SRv6 external action operation type.
 */
enum doca_flow_external_action_srv6_op {
	DOCA_FLOW_EXT_ACT_SRV6_OP_PUSH,
	/**< Push (insert) SRH header. */
};

/**
 * @brief SRv6 pipe action configuration.
 *
 * Passed to doca_flow_external_action_srv6_pipe_action_create() to describe
 * the action to be built into a pipe.
 */
struct doca_flow_external_action_srv6_pipe_action_cfg {
	uint16_t port_id;
	/**< Port ID. */
	enum doca_flow_external_action_srv6_op op;
	/**< Operation type. */
	uint8_t srh_size;
	/**< SRH header size in bytes (base header + segments). */
};

/**
 * @brief SRv6 per-entry action data (mandatory for PUSH operations).
 */
struct doca_flow_external_action_srv6_entry {
	struct doca_flow_header_ipv6_srh srh;
	/**< SRH header data. */
};

/**
 * @brief Register the SRv6 external action type.
 *
 * Must be called before doca_flow_init().  Registers the SRv6 external
 * action plugin and defines the SRH custom header at the engine level.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_ALREADY_EXIST - SRv6 action already registered.
 * - DOCA_ERROR_UNKNOWN - internal registration failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_external_action_srv6_register(void);

/**
 * @brief Create an SRv6 pipe action.
 *
 * @param [in] cfg
 * Pipe action configuration (operation type).
 * @param [out] external_action
 * Pointer to the created opaque action handle.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_external_action_srv6_pipe_action_create(
	struct doca_flow_external_action_srv6_pipe_action_cfg *cfg,
	void **external_action);

/**
 * @brief Destroy an SRv6 pipe action.
 *
 * @param [in] external_action
 * Opaque action handle to destroy.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_external_action_srv6_pipe_action_destroy(void *external_action);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_FLOW_SRV6_H_ */
