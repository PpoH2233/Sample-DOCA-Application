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
 * @file doca_flow_custom_header_graph.h
 * @page doca_flow_custom_header_graph
 * @defgroup DOCA_FLOW DOCA Flow
 * DOCA flow custom header graph. For more details please refer to the user guide
 * on DOCA devzone.
 *
 * @{
 */

#ifndef DOCA_FLOW_CUSTOM_HEADER_GRAPH_H_
#define DOCA_FLOW_CUSTOM_HEADER_GRAPH_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <doca_compat.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief doca dev struct
 */
struct doca_dev;

/**
 * @brief doca flow custom header struct
 */
struct doca_flow_custom_header;

/**
 * @brief doca flow custom header graph struct
 */
struct doca_flow_custom_header_graph;

/**
 * @brief doca flow custom header graph arc struct
 */
struct doca_flow_custom_header_graph_arc;

/**
 * @brief Function for create the parse graph object
 *
 * @param[out] graph to return the parser graph handle.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_graph_create(struct doca_flow_custom_header_graph **graph);

/**
 * @brief Binds the in-mem graph to ibv_context (DOCA device).
 *
 * A bound graph cannot be altered.
 * This API can be called multiple times with the same graph in-mem
 * for multiple DOCA devices. Also, multiple graphs can be bound
 * to the same DOCA device.
 *
 * @param[in] graph pointer to a valid parser graph.
 * @param[in] dev pointer to the device to bind to.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_graph_bind(struct doca_flow_custom_header_graph *graph, struct doca_dev *dev);

/**
 * @brief Unbinds all bound operations.
 *
 * Unbind all bound operations used with this graph for all related DOCA devices.
 * There should be no DOCA Flow ports running, based on the DOCA devices
 * the graph was bound to.
 *
 * @param[in] graph pointer to a valid parser graph.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_graph_unbind(struct doca_flow_custom_header_graph *graph);

/**
 * @brief Destroys the parser graph ctx.
 * graph must be unbound to succeed.
 *
 * @param[in] graph pointer to a valid parser graph.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_graph_destroy(struct doca_flow_custom_header_graph *graph);

/**
 * @brief Creates the arc on the base of the parser graph ctx.
 *
 * @param[in] graph pointer to a valid parser graph.
 * @param[out] arc pointer to return created arc handle.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_graph_arc_create(struct doca_flow_custom_header_graph *graph,
						      struct doca_flow_custom_header_graph_arc **arc);
/**
 * @brief Destroys the arc, dereference attached custom headers.
 *
 * @param[in] arc pointer to the arc to be destroyed.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_graph_arc_destroy(struct doca_flow_custom_header_graph_arc *arc);

/**
 * @brief Doca flow protocol header type.
 */
enum doca_flow_custom_header_graph_node_type {
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_MAC = 5,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_IP = 10,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_IPV4 = 11,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_IPV6 = 12,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_GRE = 15,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_UDP = 20,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_MPLS = 25,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_TCP = 30,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_VXLAN_GPE = 35,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_GENEVE = 40,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_IPSEC_ESP = 45,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_PSP = 50,
	DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_FLEX = 55,
};

/**
 * @brief Set the source node arc attributes
 *
 * @param [in] arc pointer to the arc being configured.
 * @param [in] node_type type of the source node.
 * @param [in] field_data the value to match on to continue parsing
 *             from the source node to the custom header.
 * @param [in] custom_header the parameter is used to specify the custom header
 *             in case node_type is DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_FLEX.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_graph_arc_set_src(struct doca_flow_custom_header_graph_arc *arc,
						       enum doca_flow_custom_header_graph_node_type node_type,
						       uint32_t field_data,
						       struct doca_flow_custom_header *custom_header);

/**
 * @brief Set the destination node arc attributes
 *
 * @param [in] arc pointer to the arc being configured.
 * @param [in] node_type type of the destination node.
 * @param [in] custom_header the parameter is used to specify the custom header
 *             in case node_type is DOCA_FLOW_CUSTOM_HEADER_GRAPH_NODE_FLEX.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_graph_arc_set_dst(struct doca_flow_custom_header_graph_arc *arc,
						       enum doca_flow_custom_header_graph_node_type node_type,
						       struct doca_flow_custom_header *custom_header);

/**
 * @brief Set the transition arc attribute
 *
 * @param [in] arc pointer to the arc being configured.
 * @param [in] start_tunnel specifies the source node is a tunnel header and
 *             the following headers should be considered as inner ones.
 *
 * @return DOCA_SUCCESS on success and DOCA error code on failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_custom_header_graph_arc_set_transition(struct doca_flow_custom_header_graph_arc *arc,
							      bool start_tunnel);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_FLOW_CUSTOM_HEADER_GRAPH_H_ */
