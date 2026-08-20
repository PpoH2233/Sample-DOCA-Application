/*
 * Copyright (c) 2021-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_flow_custom_header.h
 * @page doca_flow_custom_header
 * @defgroup DOCA_FLOW DOCA Flow
 * DOCA flow custom header. For more details please refer to the user guide
 * on DOCA devzone.
 *
 * @{
 */

#ifndef DOCA_FLOW_CUSTOM_HEADER_H_
#define DOCA_FLOW_CUSTOM_HEADER_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <doca_compat.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief doca flow custom header struct
 */
struct doca_flow_custom_header;

/**
 * @brief doca flow custom header sampler struct
 */
struct doca_flow_custom_header_sampler;

/**
 * @brief Function for create the custom header object, representing
 * how header is parsed in packet and what header data are sampled
 *
 * @param[out] custom_header to return the created object handle.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_create(struct doca_flow_custom_header **custom_header);

/**
 * @brief Destroys the custom header object.
 * Custom header must be not attached to any existing arcs to succeed.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_destroy(struct doca_flow_custom_header *custom_header);

/**
 * @brief Configures the next header field offset in the custom header.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] bit_offset offset of the next header field in bits.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_next_hdr_set_offset(struct doca_flow_custom_header *custom_header,
							 uint32_t bit_offset);

/**
 * @brief Configures the next header field length in the custom header.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] bit_length length of the next header field in bits.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_next_hdr_set_length(struct doca_flow_custom_header *custom_header,
							 uint32_t bit_length);

/**
 * @brief Configures the custom header length as fixed one.
 * If there is no other header length field setters invoked the custom header
 * length is considered as fixed one and corresponds to the specified parameter.
 * Otherwise the fixed length value will be added to the value extracted from
 * the length field in the packet.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] fixed_length length of the custom header in bits,
 *            must be multiple of 32
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_length_field_set_fixed_length(struct doca_flow_custom_header *custom_header,
								   uint32_t fixed_length);

/**
 * @brief Configures the header length in length field mode, the header
 * length is taken from the field in the packet. The routine sets the
 * length field offset in bits, starting from the header beginning.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] bit_offset length of the custom header in bits,
 *            must be multiple of 32
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_length_field_set_offset(struct doca_flow_custom_header *custom_header,
							     uint32_t bit_offset);

/**
 * @brief Configures the header length in length field mode, the header
 * length is taken from the field in the packet. The routine sets the
 * length field length in bits.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] bit_length length of the length field in bits.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_length_field_set_length(struct doca_flow_custom_header *custom_header,
							     uint32_t bit_length);

/**
 * @brief Configures the header length in length field mode, the header
 * length is taken from the field in the packet. The routine sets the
 * length field multiplier.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] multiplier the extracted length field value will be
 * 			 multiplied by this parameter, must be power of 2.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_length_field_set_multiplier(struct doca_flow_custom_header *custom_header,
								 uint32_t multiplier);

/**
 * @brief Configure the custom header TLV options length as fixed one.
 * If there is no other TLV option length field setters invoked the TLV option
 * length is considered as fixed one and corresponds to the specified parameter.
 * Otherwise the fixed length value will be added to the value extracted from
 * the option length field in the packet.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] fixed_length length of the custom header in bits,
 *            must be multiple of 32
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_tlv_options_set_fixed_length(struct doca_flow_custom_header *custom_header,
								  uint32_t fixed_length);

/**
 * @brief Configure the custom header TLV options start offset.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] bit_offset offset in bits from header start where options begin.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_tlv_options_set_offset(struct doca_flow_custom_header *custom_header,
							    uint32_t bit_offset);

/**
 * @brief Configure the custom header TLV option type field offset.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] bit_offset offset in bits from option start where type field begins.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_tlv_options_type_set_offset(struct doca_flow_custom_header *custom_header,
								 uint32_t bit_offset);

/**
 * @brief Configure the custom header TLV option type field length.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] bit_length option type field length in bits.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_tlv_options_type_set_length(struct doca_flow_custom_header *custom_header,
								 uint32_t bit_length);

/**
 * @brief Configure the custom header TLV option length field offset.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] bit_offset offset in bits from where option begins.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_tlv_options_length_set_offset(struct doca_flow_custom_header *custom_header,
								   uint32_t bit_offset);

/**
 * @brief Configure the custom header TLV option length field offset.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] bit_length option length field in bits.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_tlv_options_length_set_length(struct doca_flow_custom_header *custom_header,
								   uint32_t bit_length);

/**
 * @brief Configure the custom header TLV option length field multiplier.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] multiplier the extracted length field value will be
 * 			 multiplied by this parameter, must be power of 2.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_tlv_options_length_set_multiplier(struct doca_flow_custom_header *custom_header,
								       uint32_t multiplier);

/**
 * @brief Create the sampler object that controls data capturing.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] custom_header pointer to a valid custom header object.
 * @param[in] sampler pointer to return created sampler object.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_sampler_create(struct doca_flow_custom_header *custom_header,
						    struct doca_flow_custom_header_sampler **sampler);

/**
 * @brief Destroy the sampler object.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] sampler pointer to valid sampler object.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_sampler_destroy(struct doca_flow_custom_header_sampler *sampler);

/**
 * @brief Configure the sampler data field offset.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] sampler pointer to a valid sampler.
 * @param[in] bit_offset offset of the data field being captured to match on or modify,
 *            in bits, starting from the header beginning.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_sampler_set_offset(struct doca_flow_custom_header_sampler *sampler,
							uint32_t bit_offset);

/**
 * @brief Configure the sampler data field length.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] sampler pointer to a valid sampler.
 * @param[in] bit_length the data field length in bits.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_sampler_set_length(struct doca_flow_custom_header_sampler *sampler,
							uint32_t bit_length);

/**
 * @brief Configure the sampler to capture TLV option of the specified type.
 * This routine can't be invoked once custom header is used in arc.
 *
 * @param[in] sampler pointer to a valid sampler.
 * @param[in] option_type option type to be captured by the sampler.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_sampler_enable_is_options(struct doca_flow_custom_header_sampler *sampler,
							       uint32_t option_type);
/**
 * @brief doca flow custom header tunnel mode - inner, outer, first
 */
enum doca_flow_custom_header_sampler_tunnel_mode {
	DOCA_FLOW_CUSTOM_HEADER_SAMPLER_TUNNEL_MODE_OUTER,
	DOCA_FLOW_CUSTOM_HEADER_SAMPLER_TUNNEL_MODE_INNER,
	DOCA_FLOW_CUSTOM_HEADER_SAMPLER_TUNNEL_MODE_FIRST,
};

/**
 * @brief Configure the sampler tunnel mode, what header to capture - inner,
 * outer, first encountered, etc.
 *
 * @param[in] sampler pointer to a valid sampler.
 * @param[in] tunnel_mode what kind of header to capture by the sampler.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_sampler_set_tunnel_mode(
	struct doca_flow_custom_header_sampler *sampler,
	enum doca_flow_custom_header_sampler_tunnel_mode tunnel_mode);

/**
 * @brief Get the opcode string to match on the data captured by sampler.
 *
 * @param[in] sampler pointer to a valid sampler.
 * @param[in] opcode_str pointer to return pointer to the const string.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_sampler_get_match_definition(struct doca_flow_custom_header_sampler *sampler,
								  const char **opcode_str);

/**
 * @brief Get the opcode string for the modify action on the data presented by sampler.
 *
 * @param[in] sampler pointer to a valid sampler.
 * @param[in] opcode_str pointer to return pointer to the const string.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_sampler_get_action_definition(struct doca_flow_custom_header_sampler *sampler,
								   const char **opcode_str);

/**
 * @brief Get the opcode string to match on the valid capture bit.
 *
 * @param[in] sampler pointer to a valid sampler.
 * @param[in] opcode_str pointer to return pointer to the const string.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_sampler_get_valid_bit_definition(struct doca_flow_custom_header_sampler *sampler,
								      const char **opcode_str);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_FLOW_CUSTOM_HEADER_H_ */
