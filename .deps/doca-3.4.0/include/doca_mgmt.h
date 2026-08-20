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
 * @file doca_mgmt.h
 * @page doca_mgmt
 * @defgroup DOCA_MGMT DOCA Management
 * DOCA Management library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_MGMT_H_
#define DOCA_MGMT_H_

#include <stddef.h>

#include <doca_compat.h>
#include <doca_dev.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing a DOCA management device context.
 */
struct doca_mgmt_dev_ctx;

/**
 * @brief Opaque structure representing a DOCA management device representor context.
 */
struct doca_mgmt_dev_rep_ctx;

/**
 * @brief Create a DOCA management device context.
 *
 * @param [in] dev
 * The DOCA device to create the management context for.
 * @param [out] ctx
 * Pointer to pointer to be set to point to the created doca_mgmt_dev_ctx instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - invalid input parameters.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_dev_ctx_create(struct doca_dev *dev, struct doca_mgmt_dev_ctx **ctx);

/**
 * @brief Destroy a DOCA management device context.
 *
 * @param [in] ctx
 * The DOCA management device context to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_dev_ctx_destroy(struct doca_mgmt_dev_ctx *ctx);

/**
 * @brief Get the DOCA device from the DOCA management device context.
 *
 * @param [in] ctx
 * The DOCA management device context to get the device from.
 *
 * @return
 * The DOCA device of the DOCA management device context in case of success, or NULL if ctx is NULL.
 */
DOCA_EXPERIMENTAL
struct doca_dev *doca_mgmt_dev_ctx_get_doca_dev(struct doca_mgmt_dev_ctx *ctx);

/**
 * @brief Create a DOCA management device representor context.
 *
 * @param [in] dev_ctx
 * The DOCA management device context to create the management device representor context for.
 * @param [in] rep
 * The DOCA device representor to create the management device representor context for.
 * @param [out] ctx
 * Pointer to pointer to be set to point to the created doca_mgmt_dev_rep_ctx instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - invalid input parameters.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_dev_rep_ctx_create(struct doca_mgmt_dev_ctx *dev_ctx,
					  struct doca_dev_rep *rep,
					  struct doca_mgmt_dev_rep_ctx **ctx);

/**
 * @brief Create a DOCA management device representor context by PCI address.
 * This function creates a DOCA management device representor context for a VF in an alternative way that doesn't
 * require a VF representor.
 * This function should only be used in special cases where a VF representor is not available but the VF should still be
 * configurable via a struct doca_mgmt_dev_rep_ctx.
 *
 * @param [in] dev_ctx
 * The DOCA management device context of the device that the VF belongs to.
 * @param [in] pci_addr
 * The PCI address of the VF to create the management device representor context for. The PCI address format is
 * "Domain:Bus:Device.Function", such that each value is represented by HEX digits, e.g., "0000:3a:00.2".
 * @param [out] ctx
 * Pointer to pointer to be set to point to the created doca_mgmt_dev_rep_ctx instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - invalid input parameters.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_dev_rep_ctx_create_by_pci_addr(struct doca_mgmt_dev_ctx *dev_ctx,
						      const char *pci_addr,
						      struct doca_mgmt_dev_rep_ctx **ctx);

/**
 * @brief Destroy a DOCA management device representor context.
 *
 * @param [in] ctx
 * The DOCA management device representor context to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_dev_rep_ctx_destroy(struct doca_mgmt_dev_rep_ctx *ctx);

/**
 * @brief Get the DOCA device representor from the DOCA management device representor context.
 *
 * @param [in] ctx
 * The DOCA management device representor context to get the DOCA device representor from.
 *
 * @return
 * The DOCA device representor of the DOCA management device representor context in case of success, or NULL if ctx is
 * NULL.
 */
DOCA_EXPERIMENTAL
struct doca_dev_rep *doca_mgmt_dev_rep_ctx_get_doca_dev_rep(struct doca_mgmt_dev_rep_ctx *ctx);

/**
 * @brief Get the DOCA management device context from the DOCA management device representor context.
 *
 * @param [in] ctx
 * The DOCA management device representor context to get the DOCA management device context from.
 *
 * @return
 * The DOCA management device context of the DOCA management device representor context in case of success, or NULL if
 * ctx is NULL.
 */
DOCA_EXPERIMENTAL
struct doca_mgmt_dev_ctx *doca_mgmt_dev_rep_ctx_get_mgmt_dev_ctx(struct doca_mgmt_dev_rep_ctx *ctx);

/**
 * @brief Scope of a doca management command.
 */
enum doca_mgmt_cmd_scope {
	DOCA_MGMT_CMD_SCOPE_CONFIGURATION,
	DOCA_MGMT_CMD_SCOPE_DEBUG_READ_ONLY,
	DOCA_MGMT_CMD_SCOPE_DEBUG_WRITE,
	DOCA_MGMT_CMD_SCOPE_DEBUG_WRITE_FULL,
};

/**
 * @brief Send a raw command to the device over fwctl.
 *
 * @param [in] ctx
 * The DOCA management device context to send the command on.
 * @param [in] command_id
 * An identifier of the command for logging purposes.
 * @param [in] scope
 * The scope of the command to send.
 * @param [in] in_payload
 * Buffer with the input payload of the command to send.
 * @param [in] in_payload_size
 * The size of the input payload in bytes.
 * @param [out] out_payload
 * Buffer for the output payload of the command to receive.
 * @param [in] out_payload_size
 * The size of the output payload in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - invalid input parameters.
 * - DOCA_ERROR_OPERATING_SYSTEM - fwctl RPC ioctl failed.
 * - DOCA_ERROR_IO_FAILED - fwctl RPC failed by FW.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_mgmt_raw_cmd(struct doca_mgmt_dev_ctx *ctx,
			       const char *command_id,
			       enum doca_mgmt_cmd_scope scope,
			       const void *in_payload,
			       size_t in_payload_size,
			       void *out_payload,
			       size_t out_payload_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_MGMT_H_ */

/** @} */
