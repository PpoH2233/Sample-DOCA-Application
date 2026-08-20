/*
 * Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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

#ifndef DOCA_FLOW_DEFINITIONS_H_
#define DOCA_FLOW_DEFINITIONS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <doca_compat.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Doca flow definitions configuration struct.
 */
struct doca_flow_definitions_cfg;

/**
 * @brief Doca flow definitions struct.
 */
struct doca_flow_definitions;

/**
 * @brief Creates a definitions configuration object.
 *
 * @param [out] defs_cfg
 * Pointer to a definitions configuration.
 *
 * Return DOCA_SUCCESS on success, doca error otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_definitions_cfg_create(struct doca_flow_definitions_cfg **defs_cfg);

/**
 * @brief Destroys a definitions configuration object.
 *
 * @param [in] defs_cfg
 * Pointer to a definitions configuration.
 *
 * Return DOCA_SUCCESS on success, doca error otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_definitions_cfg_destroy(struct doca_flow_definitions_cfg *defs_cfg);

/**
 * @brief Creates a definitions object.
 *
 * @param [in] defs_cfg
 * Pointer to a definitions configuration.
 * @param [out] defs
 * Pointer to a definitions object pointer.
 * Return DOCA_SUCCESS on success, doca error otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_definitions_create(struct doca_flow_definitions_cfg *defs_cfg,
					  struct doca_flow_definitions **defs);

/**
 * @brief Add a field to the definitions object.
 *
 * @param defs
 * Pointer to a definitions object.
 * @param field_opcode_str
 * Pointer to field opcode string.
 * @param field_offset
 * Field offset.
 * @param field_length
 * Field length.
 * Return DOCA_SUCCESS on success, doca error otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_definitions_add_field(struct doca_flow_definitions *defs,
					     const char *field_opcode_str,
					     uint32_t field_offset,
					     uint32_t field_length);

/**
 * @brief Destroys a definitions object.
 *
 * @param defs
 * Pointer to a definitions object.
 */
DOCA_EXPERIMENTAL
void doca_flow_definitions_destroy(struct doca_flow_definitions *defs);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_FLOW_DEFINITIONS_H_ */
