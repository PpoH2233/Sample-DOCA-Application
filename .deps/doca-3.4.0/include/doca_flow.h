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
 * @file doca_flow.h
 * @page doca_flow
 * @defgroup DOCA_FLOW DOCA Flow
 * DOCA HW offload flow library. For more details please refer to the user guide
 * on DOCA devzone.
 *
 * @{
 */

#ifndef DOCA_FLOW_H_
#define DOCA_FLOW_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <doca_compat.h>
#include <doca_error.h>

#include <doca_flow_net.h>
#include <doca_flow_crypto.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <doca_ctsd_compat_versions.h>

/**
 * doca flow lib set version if not set by client app
 */
#ifndef LIB_FLOW_VERSION
#define LIB_FLOW_VERSION LIB_FLOW_VERSION_LATEST
#endif

/**
 * doca flow lib versions
 */
#define LIB_FLOW_VERSION_1 1

/**
 * doca flow lib latest version
 */
#define LIB_FLOW_VERSION_LATEST LIB_FLOW_VERSION_1

struct doca_dev;
struct doca_dev_rep;

/**
 * @brief doca flow port struct
 */
struct doca_flow_port;

/**
 * @brief doca flow pipeline struct
 */
struct doca_flow_pipe;

/**
 * @brief doca flow pipeline entry struct
 */
struct doca_flow_pipe_entry;

/**
 * @brief doca flow target struct
 */
struct doca_flow_target;

/**
 * @brief doca flow parser struct
 */
struct doca_flow_parser;

/**
 * @brief doca flow definitions struct
 */
struct doca_flow_definitions;

/**
 * @brief doca flow global configurations
 */
struct doca_flow_cfg;

/**
 * @brief doca flow port configuration
 */
struct doca_flow_port_cfg;

/**
 * @brief pipeline configuration
 */
struct doca_flow_pipe_cfg;

/**
 * @brief tune configuration
 */
struct doca_flow_tune_cfg;

/**
 * @brief Resource mode types
 */
enum doca_flow_resource_mode {
	DOCA_FLOW_RESOURCE_MODE_SYSTEM,
	DOCA_FLOW_RESOURCE_MODE_PORT,
};

/**
 * @brief Resource types
 */
enum doca_flow_resource {
	DOCA_FLOW_RESOURCE_METER,
	/**< meter type */
	DOCA_FLOW_RESOURCE_COUNTER,
	/**< counter type */
	DOCA_FLOW_RESOURCE_RSS,
	/**< rss type */
	DOCA_FLOW_RESOURCE_PSP,
	/**< psp action type */
	DOCA_FLOW_RESOURCE_ENCAP,
	/**< encap type */
	DOCA_FLOW_RESOURCE_DECAP,
	/**< decap type */
	DOCA_FLOW_RESOURCE_IPSEC_SA,
	/**< ipsec SA type */
	DOCA_FLOW_RESOURCE_COUNTER_CT,
	/**< CT counter type */
};

/**
 * @brief Set number of resource
 *
 * Set the number of resource per type
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @param [in] resource
 * Type of resource
 * @param [in] nr_resources
 * Number of resources
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_cfg_set_nr_resources(struct doca_flow_port_cfg *cfg,
						 enum doca_flow_resource resource,
						 uint32_t nr_resources);
/**
 * @brief Shared resource supported types
 */
enum doca_flow_shared_resource_type {
	DOCA_FLOW_SHARED_RESOURCE_METER,
	/**< Shared meter type */
	DOCA_FLOW_SHARED_RESOURCE_COUNTER,
	/**< Shared counter type */
	DOCA_FLOW_SHARED_RESOURCE_RSS,
	/**< Shared rss type */
	DOCA_FLOW_SHARED_RESOURCE_PSP,
	/**< Shared psp action type */
	DOCA_FLOW_SHARED_RESOURCE_ENCAP,
	/**< Shared encap type */
	DOCA_FLOW_SHARED_RESOURCE_DECAP,
	/**< Shared decap type */
	DOCA_FLOW_SHARED_RESOURCE_IPSEC_SA,
	/**< Shared ipsec SA type */
};

/**
 * @brief doca flow entry flags
 * multiple flags can be ORed together, however,
 * only one of DOCA_FLOW_ENTRY_FLAGS_NO_WAIT or DOCA_FLOW_ENTRY_FLAGS_WAIT_FOR_BATCH can be set.
 */
enum doca_flow_entry_flags {
	DOCA_FLOW_ENTRY_FLAGS_NO_WAIT = (1u << 0),
	/**< entry will not be buffered */
	DOCA_FLOW_ENTRY_FLAGS_WAIT_FOR_BATCH = (1u << 1),
	/**< entry will be buffered */
};

/**
 * @brief Set a flag in doca_flow_flags
 * @param flags
 * Flags variable (lvalue) to modify
 * @param flag
 * Flag to set (e.g., DOCA_FLOW_ENTRY_FLAGS_NO_WAIT)
 */
#define DOCA_FLOW_FLAGS_SET(flags, flag) ((flags) |= (flag))

/**
 * @brief Unset a flag in doca_flow_flags
 * @param flags
 * Flags variable (lvalue) to modify
 * @param flag
 * Flag to unset (e.g., DOCA_FLOW_ENTRY_FLAGS_NO_WAIT)
 */
#define DOCA_FLOW_FLAGS_UNSET(flags, flag) ((flags) &= ~(flag))

/**
 * @brief Check if a flag is set in doca_flow_flags
 * @param flags
 * Flags value to check
 * @param flag
 * Flag to check (e.g., DOCA_FLOW_ENTRY_FLAGS_NO_WAIT)
 * @return
 * Non-zero if flag is set, zero otherwise
 */
#define DOCA_FLOW_FLAGS_IS_SET(flags, flag) (((flags) & (flag)) != 0)

/**
 * @brief doca flow pipe operation
 */
enum doca_flow_pipe_op {
	DOCA_FLOW_PIPE_OP_CONGESTION_REACHED,
	/**< Pipe congestion percentage level reached */
	DOCA_FLOW_PIPE_OP_RESIZED,
	/**< Pipe resize completion */
	DOCA_FLOW_PIPE_OP_DESTROYED,
	/**< Pipe destroy completion */
};

/**
 * @brief doca flow pipe status
 */
enum doca_flow_pipe_status {
	DOCA_FLOW_PIPE_STATUS_SUCCESS = 1,
	/**< The operation was completed successfully. */
	DOCA_FLOW_PIPE_STATUS_ERROR,
	/**< The operation failed. */
};

/**
 * @brief doca flow entry operation
 */
enum doca_flow_entry_op {
	DOCA_FLOW_ENTRY_OP_ADD,
	/**< Add entry */
	DOCA_FLOW_ENTRY_OP_DEL,
	/**< Delete entry */
	DOCA_FLOW_ENTRY_OP_UPD,
	/**< Update entry */
	DOCA_FLOW_ENTRY_OP_AGED,
	/**< Aged entry */
};

/**
 * @brief doca flow entry status
 */
enum doca_flow_entry_status {
	DOCA_FLOW_ENTRY_STATUS_IN_PROCESS,
	/* The operation is in progress. */
	DOCA_FLOW_ENTRY_STATUS_SUCCESS,
	/* The operation was completed successfully. */
	DOCA_FLOW_ENTRY_STATUS_ERROR,
	/* The operation failed. */
};

/**
 * @brief rss hash function type
 */
enum doca_flow_rss_hash_function {
	DOCA_FLOW_RSS_HASH_FUNCTION_TOEPLITZ,		/**< Toeplitz */
	DOCA_FLOW_RSS_HASH_FUNCTION_SYMMETRIC_TOEPLITZ, /**< Toeplitz with sorted source and destination */
};

/**
 * @brief doca flow rss resource configuration
 */
struct doca_flow_resource_rss_cfg {
	uint32_t outer_flags;
	/**< rss offload outer types */
	uint32_t inner_flags;
	/**< rss offload inner types */
	uint16_t *queues_array;
	/**< rss queues array */
	int nr_queues;
	/**< number of queues */
	enum doca_flow_rss_hash_function rss_hash_func;
	/**< hash function */
};

/**
 * @brief doca flow pipe process callback
 */
typedef void (*doca_flow_pipe_process_cb)(struct doca_flow_pipe *pipe,
					  enum doca_flow_pipe_status status,
					  enum doca_flow_pipe_op op,
					  void *user_ctx);

/**
 * @brief doca flow entry process callback
 */
typedef void (*doca_flow_entry_process_cb)(struct doca_flow_pipe_entry *entry,
					   uint16_t pipe_queue,
					   enum doca_flow_entry_status status,
					   enum doca_flow_entry_op op,
					   void *user_ctx);

/**
 * @brief doca flow shared resource unbind callback
 */
typedef void (*doca_flow_shared_resource_unbind_cb)(enum doca_flow_shared_resource_type,
						    uint32_t shared_resource_id,
						    void *bindable_obj);

/**
 * @brief doca flow pipe type
 */
enum doca_flow_pipe_type {
	DOCA_FLOW_PIPE_BASIC,
	/**< Flow pipe */
	DOCA_FLOW_PIPE_CONTROL,
	/**< Control pipe */
	DOCA_FLOW_PIPE_LPM,
	/**< longest prefix match (LPM) pipe */
	DOCA_FLOW_PIPE_CT,
	/**< Connection Tracking pipe */
	DOCA_FLOW_PIPE_ACL,
	/**< ACL pipe */
	DOCA_FLOW_PIPE_ORDERED_LIST,
	/**< Ordered list pipe */
	DOCA_FLOW_PIPE_HASH,
	/**< Hash pipe */
};

/**
 * @brief doca flow pipe domain
 */
enum doca_flow_pipe_domain {
	DOCA_FLOW_PIPE_DOMAIN_DEFAULT = 0,
	/**< Default pipe domain for actions on ingress traffic */
	DOCA_FLOW_PIPE_DOMAIN_SECURE_INGRESS,
	/**< Pipe domain for secure actions on ingress traffic */
	DOCA_FLOW_PIPE_DOMAIN_EGRESS,
	/**< Pipe domain for actions on egress traffic */
	DOCA_FLOW_PIPE_DOMAIN_SECURE_EGRESS,
	/**< Pipe domain for actions on egress traffic */
};

/**
 * Max meta scratch pad size in 32-bit resolution
 */
#define DOCA_FLOW_META_SCRATCH_PAD_MAX 10

/**
 * Max meta data size in bytes, including pkt_meta.
 */
#define DOCA_FLOW_META_MAX ((DOCA_FLOW_META_SCRATCH_PAD_MAX + 1) * 4)

/**
 * Max usage of actions memory size in byte
 */
#define DOCA_FLOW_MAX_ENTRY_ACTIONS_MEM_SIZE (128)

/**
 * @brief meter mark color
 */
enum __attribute__((__packed__)) doca_flow_meter_color {
	DOCA_FLOW_METER_COLOR_RED = 0,
	/**< Meter marking packet color as red */
	DOCA_FLOW_METER_COLOR_YELLOW,
	/**< Meter marking packet color as yellow */
	DOCA_FLOW_METER_COLOR_GREEN,
	/**< Meter marking packet color as green */
};

/**
 * @brief doca flow l2 valid type for parser meta
 */
enum doca_flow_l2_meta {
	DOCA_FLOW_L2_META_NO_VLAN = 0,
	/**< no vlan present */
	DOCA_FLOW_L2_META_MULTI_VLAN,
	/**< multiple vlan present */
	DOCA_FLOW_L2_META_SINGLE_VLAN,
	/**< single vlan present */
	DOCA_FLOW_L2_META_CUSTOM_VLAN,
	/**< custom vlan present */
};

/**
 * @brief doca flow l3 valid type for parser meta
 */
enum doca_flow_l3_meta {
	DOCA_FLOW_L3_META_NONE = 0,
	/**< l3 type is none of the below */
	DOCA_FLOW_L3_META_IPV4,
	/**< l3 type is ipv4 */
	DOCA_FLOW_L3_META_IPV6,
	/**< l3 type is ipv6 */
};

/**
 * @brief doca flow l4 valid type for parser meta
 */
enum doca_flow_l4_meta {
	DOCA_FLOW_L4_META_NONE = 0,
	/**< l4 type is none of the below */
	DOCA_FLOW_L4_META_TCP,
	/**< l4 type is tcp */
	DOCA_FLOW_L4_META_UDP,
	/**< l4 type is udp */
	DOCA_FLOW_L4_META_ICMP,
	/**< l4 type is icmp or icmp6 */
	DOCA_FLOW_L4_META_ESP,
	/**< l4 type is esp */
};

/**
 * @brief doca flow psp/ipsec syndrome valid values for parser meta
 */
enum doca_flow_crypto_syndrome {
	DOCA_FLOW_CRYPTO_SYNDROME_OK,
	/**<  Decryption and authentication success */
	DOCA_FLOW_CRYPTO_SYNDROME_ICV_FAIL,
	/**<  Authentication failure */
	DOCA_FLOW_CRYPTO_SYNDROME_BAD_TRAILER,
	/**<  Trailer overlaps with headers */
};

/**
 * @brief DOCA Flow pipe map algorithm
 */
enum doca_flow_pipe_hash_map_algorithm {
	DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_HASH = 1 << 0,
	/**< Hash algorithm - default */
	DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_RANDOM = 1 << 1,
	/**< Random algorithm */
	DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_IDENTITY = 1 << 2,
	/**< Direct mapping algorithm */
	DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_ROUND_ROBIN = 1 << 3,
	/**< Round robin algorithm */
	DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING = 1 << 4,
	/**< Flooding algorithm */
	DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_SELECT_ENABLED = 1 << 5,
	/**< Select enabled map entry */
};

/**
 * @brief doca flow meta data
 *
 * Meta data known as scratch data can be used to match or modify within pipes.
 * Meta data can be set with value in previous pipes and match in later pipes.
 * User can customize meta data structure as long as overall size doesn't exceed limit.
 * To match meta data, mask must be specified when creating pipe.
 * Struct must be aligned to 32 bits.
 * No initial value for Meta data, must match after setting value.
 */
struct doca_flow_meta {
	doca_be32_t pkt_meta;				 /**< Shared with application via packet. */
	doca_be32_t u32[DOCA_FLOW_META_SCRATCH_PAD_MAX]; /**< Programmable user data. */
};

/**
 * @brief doca flow parser meta data
 *
 * Parser meta data known as read-only hardware data that can be used to match.
 */
struct doca_flow_parser_meta {
	uint16_t port_id; /**< Logical port ID provided in @ref doca_flow_port_cfg_set_port_id. */
	/**
	 * Matches a random value.
	 * This value is not based on the packet data/headers.
	 * Application shouldn't assume that this value is kept during the packet lifetime.
	 */
	doca_be16_t random;
	uint8_t ipsec_syndrome;			/**< IPsec decrypt/authentication syndrome. */
	uint8_t ipsec_ar_syndrome;		/**< IPsec anti-replay syndrome. */
	uint8_t psp_syndrome;			/**< PSP decrypt/authentication syndrome. */
	enum doca_flow_meter_color meter_color; /**< Meter colors: Green, Yellow, Red. */
	enum doca_flow_l2_meta outer_l2_type;	/**< Outermost L2 packet type. */
	enum doca_flow_l3_meta outer_l3_type;	/**< Outermost L3 packet type. */
	enum doca_flow_l4_meta outer_l4_type;	/**< Outermost L4 packet type. */
	enum doca_flow_l2_meta inner_l2_type;	/**< Innermost L2 packet type. */
	enum doca_flow_l3_meta inner_l3_type;	/**< Innermost L3 packet type. */
	enum doca_flow_l4_meta inner_l4_type;	/**< Innermost L4 packet type. */
	uint8_t outer_ip_fragmented;		/**< Whether outer IP packet is fragmented. */
	uint8_t inner_ip_fragmented;		/**< Whether inner IP packet is fragmented. */
	uint8_t outer_l3_ok;			/**< Whether outer L3 layer is valid. */
	uint8_t outer_ip4_checksum_ok;		/**< Whether outer IPv4 checksum is valid. */
	uint8_t outer_l4_ok;			/**< Whether outer L4 layer is valid. */
	uint8_t outer_l4_checksum_ok;		/**< Whether outer L4 checksum is valid. */
	uint8_t inner_l3_ok;			/**< Whether inner L3 layer is valid. */
	uint8_t inner_ip4_checksum_ok;		/**< Whether inner IPv4 checksum is valid. */
	uint8_t inner_l4_ok;			/**< Whether inner L4 layer is valid. */
	uint8_t inner_l4_checksum_ok;		/**< Whether inner L4 checksum is valid. */
};

/**
 * @brief doca flow match flags
 */
enum doca_flow_match_tcp_flags {
	DOCA_FLOW_MATCH_TCP_FLAG_FIN = (1 << 0),
	/**< match tcp packet with Fin flag */
	DOCA_FLOW_MATCH_TCP_FLAG_SYN = (1 << 1),
	/**< match tcp packet with Syn flag */
	DOCA_FLOW_MATCH_TCP_FLAG_RST = (1 << 2),
	/**< match tcp packet with Rst flag */
	DOCA_FLOW_MATCH_TCP_FLAG_PSH = (1 << 3),
	/**< match tcp packet with Psh flag */
	DOCA_FLOW_MATCH_TCP_FLAG_ACK = (1 << 4),
	/**< match tcp packet with Ack flag */
	DOCA_FLOW_MATCH_TCP_FLAG_URG = (1 << 5),
	/**< match tcp packet with Urg flag */
	DOCA_FLOW_MATCH_TCP_FLAG_ECE = (1 << 6),
	/**< match tcp packet with Ece flag */
	DOCA_FLOW_MATCH_TCP_FLAG_CWR = (1 << 7),
	/**< match tcp packet with Cwr flag */
};

/**
 * Max number of vlan headers.
 */
#define DOCA_FLOW_VLAN_MAX 2

/**
 * @brief doca flow l2 valid headers
 */
enum doca_flow_l2_valid_header {
	DOCA_FLOW_L2_VALID_HEADER_VLAN_0 = (1 << 0),
	/**< first vlan */
	DOCA_FLOW_L2_VALID_HEADER_VLAN_1 = (1 << 1),
	/**< second vlan */
};

/**
 * @brief doca flow packet format
 */
struct doca_flow_header_format {
	struct doca_flow_header_eth eth;
	/**< ether head */
	uint16_t l2_valid_headers;
	/**< indicate which headers are valid */
	struct doca_flow_header_eth_vlan eth_vlan[DOCA_FLOW_VLAN_MAX];
	/**< vlan header array*/
	enum doca_flow_l3_type l3_type;
	/**< layer 3 protocol type */
	union {
		struct doca_flow_header_ip4 ip4;
		/**< ipv4 head */
		struct doca_flow_header_ip6 ip6;
		/**< ipv6 head */
	};
	enum doca_flow_l4_type_ext l4_type_ext;
	/**< l4 layer extend type */
	union {
		struct doca_flow_header_icmp icmp;
		/**< icmp header */
		struct doca_flow_header_udp udp;
		/**< udp header */
		struct doca_flow_header_tcp tcp;
		/**< tcp header */
		struct doca_flow_header_l4_port transport;
		/**< transport layer source and destination port */
		struct doca_flow_header_roce_v2 roce_v2;
		/**< ROCEv2 header */
	};
};

/**
 * @brief doca flow header format for entropy
 */
struct doca_flow_entropy_format {
	enum doca_flow_l3_type l3_type;
	/**< layer 3 protocol type */
	union {
		struct doca_flow_header_ip4 ip4;
		/**< ipv4 head */
		struct doca_flow_header_ip6 ip6;
		/**< ipv6 head */
	};
	bool is_transport;
	/**< hint if next layer is transport or not */
	struct doca_flow_header_l4_port transport;
	/**< transport layer source and destination port */
};

/**
 * @brief doca flow matcher information
 */
struct doca_flow_match {
	struct doca_flow_meta meta;
	/**< Programmable meta data. */
	struct doca_flow_parser_meta parser_meta;
	/**< Read-only meta data. */
	struct doca_flow_header_format outer;
	/**< outer layer header format */
	struct doca_flow_tun tun;
	/**< tunnel info */
	struct doca_flow_header_format inner;
	/**< inner layer header format */
};

/**
 * @brief doca flow compare operation
 */
enum doca_flow_compare_op {
	DOCA_FLOW_COMPARE_EQ,
	/**< Equal compare. */
	DOCA_FLOW_COMPARE_NE,
	/**< Not equal compare. */
	DOCA_FLOW_COMPARE_LT,
	/**< Less than compare. */
	DOCA_FLOW_COMPARE_LE,
	/**< Less equal compare. */
	DOCA_FLOW_COMPARE_GT,
	/**< Great than compare. */
	DOCA_FLOW_COMPARE_GE,
	/**< Great equal compare. */
};

/**
 * @brief Action descriptor field
 *
 * Field based on a string that is composed out of struct members separated by a dot.
 *
 * The 1st segment determines the field location in packet "outer", "inner", "tunnel".
 * The 2nd segment determines the protocol.
 * The 3rd segment determines the field.
 *
 * E.g.
 * "outer.eth.src_mac"
 * "tunnel.gre.protocol"
 * "inner.ipv4.next_proto"
 */
struct doca_flow_desc_field {
	const char *field_string;
	/**< Field selection by string. */
	uint32_t bit_offset;
	/**< Field bit offset. */
};

/**
 * @brief doca flow match condition information
 */
struct doca_flow_match_condition {
	enum doca_flow_compare_op operation;
	/**< Condition compare operation. */
	union {
		struct {
			struct doca_flow_desc_field a;
			/**< Field descriptor A. */
			struct doca_flow_desc_field b;
			/**< Field descriptor B. */
			uint32_t width;
			/**< Field width. */
		} field_op;
	};
};

/**
 * @brief doca flow encap data information
 */
struct doca_flow_encap_action {
	struct doca_flow_header_format outer;
	/**< outer header format */
	struct doca_flow_tun tun;
	/**< tunnel info */
};

/**
 * @brief doca flow push VLAN data information
 */
struct doca_flow_push_vlan_action {
	doca_be16_t eth_type;
	/**< eth type to be written in the eth header before the VLAN */
	struct doca_flow_header_eth_vlan vlan_hdr;
	/**< Vlan header that will be pushed */
};

/**
 * @brief doca flow push action type
 */
enum doca_flow_push_action_type {
	DOCA_FLOW_PUSH_ACTION_VLAN,
};

/**
 * @brief doca flow push data information
 */
struct doca_flow_push_action {
	enum doca_flow_push_action_type type;
	/**< header type to push */
	union {
		struct doca_flow_push_vlan_action vlan;
	};
};

/**
 * @brief doca flow nat64 action
 */
struct doca_flow_nat64_action {
	enum doca_flow_l3_type original_l3_type;
	/**< original header's layer 3 type */
};

/**
 * @brief doca flow resource type
 */
enum doca_flow_resource_type {
	DOCA_FLOW_RESOURCE_TYPE_NONE,
	DOCA_FLOW_RESOURCE_TYPE_SHARED,
	DOCA_FLOW_RESOURCE_TYPE_NON_SHARED
};

/**
 * PSP decryption reserved id, must be used at pipe creation and entry addition
 */
#define DOCA_FLOW_PSP_DECRYPTION_ID UINT32_MAX

/**
 * @brief doca flow crypto action information
 */
struct doca_flow_crypto_action {
	enum doca_flow_crypto_action_type action_type;
	/**< crypto action type - none/encrypt/decrypt */
	enum doca_flow_crypto_resource_type resource_type;
	/**< crypto action resource - none/ipsec_sa/psp */
	union {
		struct {
			bool sn_en;
			/**< Enable SN/ESN generation on egress and antireplay on ingress */
		} ipsec_sa;
	};
	uint32_t crypto_id;
	/**< shared resource id represents session */
};

/**
 * @brief doca flow crypto encap action information
 */
struct doca_flow_crypto_encap_action {
	enum doca_flow_crypto_encap_action_type action_type;
	/**< action type - encap or decap */
	enum doca_flow_crypto_encap_net_type net_type;
	/**< network type - mode, protocol, header */
	uint16_t icv_size;
	/**< trailer size in bytes */
	uint16_t data_size;
	/**< reformat header length in bytes */
	uint8_t encap_data[DOCA_FLOW_CRYPTO_HEADER_LEN_MAX];
	/**< reformat header data to insert */
};

/**
 * @brief doca flow encap resource configuration
 */
struct doca_flow_resource_encap_cfg {
	bool is_l2;
	/**< L2 or L3 tunnel flavor */
	struct doca_flow_encap_action encap;
	/**< Encap data */
};

/**
 * @brief doca flow decap resource configuration
 */
struct doca_flow_resource_decap_cfg {
	bool is_l2;
	/**< L2 or L3 tunnel flavor */
	struct doca_flow_header_eth eth;
	/**< ether head for is_l2 is false */
	uint16_t l2_valid_headers;
	/**< indicate which headers are valid */
	struct doca_flow_header_eth_vlan eth_vlan[DOCA_FLOW_VLAN_MAX];
	/**< vlan header array for is_l2 is false */
};

/**
 * @brief doca flow actions information
 */
struct doca_flow_actions {
	enum doca_flow_resource_type decap_type;
	/**< type of decap */
	union {
		struct doca_flow_resource_decap_cfg decap_cfg;
		/**< config for non_shared decap */
		uint32_t shared_decap_id;
		/**< action for shared decap */
	};
	bool pop_vlan;
	/**< when true, pop the outer VLAN header */
	struct doca_flow_meta meta;
	/**< modify meta data, pipe action as mask */
	struct doca_flow_parser_meta parser_meta;
	/**< copy from read-only meta data, pipe action as mask */
	struct doca_flow_header_format outer;
	/**< modify outer headers */
	struct doca_flow_tun tun;
	/**< modify tunnel headers*/
	enum doca_flow_resource_type encap_type;
	/**< type of encap */
	union {
		struct doca_flow_resource_encap_cfg encap_cfg;
		/**< config for non_shared encap */
		uint32_t shared_encap_id;
		/**< action for shared encap */
	};
	bool has_push;
	/**< when true, push header */
	struct doca_flow_push_action push;
	/**< push header data information */
	struct doca_flow_nat64_action nat64;
	/**< nat64 action */
	bool has_crypto_encap;
	/**< when true, do crypto reformat header/trailer */
	struct doca_flow_crypto_encap_action crypto_encap;
	/**< header/trailer reformat data information */
	struct doca_flow_crypto_action crypto;
	/**< crypto action information */
};

/**
 * @brief doca flow crypto actions information
 */
struct doca_flow_crypto {
	bool has_crypto_encap;
	/**< when true, do crypto reformat header/trailer */
	struct doca_flow_crypto_encap_action crypto_encap;
	/**< header/trailer reformat data information */
	struct doca_flow_crypto_action crypto;
	/**< crypto action information */
};

/**
 * @brief doca flow target type
 */
enum doca_flow_target_type {
	DOCA_FLOW_TARGET_KERNEL,
};

/**
 * @brief forwarding action type
 */
enum doca_flow_fwd_type {
	DOCA_FLOW_FWD_NONE = 0,
	/**< No forward action be set */
	DOCA_FLOW_FWD_RSS,
	/**< Forwards packets to rss */
	DOCA_FLOW_FWD_PORT,
	/**< Forwards packets to one port */
	DOCA_FLOW_FWD_PIPE,
	/**< Forwards packets to another pipe */
	DOCA_FLOW_FWD_DROP,
	/**< Drops packets */
	DOCA_FLOW_FWD_TARGET,
	/**< Forwards packets to target */
	DOCA_FLOW_FWD_ORDERED_LIST_PIPE,
	/**< Forwards packet to a specific entry in an ordered list pipe. */
	DOCA_FLOW_FWD_HASH_PIPE,
	/**< Forwards packet to a specific algorithm in hash pipe. */
	DOCA_FLOW_FWD_CHANGEABLE = 100
	/**< Forward is specified at entry creation. */
};

/**
 * @brief rss offload types
 */
enum doca_rss_type {
	DOCA_FLOW_RSS_IPV4 = (1 << 0),
	/**< rss by ipv4 header */
	DOCA_FLOW_RSS_IPV6 = (1 << 1),
	/**< rss by ipv6 header */
	DOCA_FLOW_RSS_UDP = (1 << 2),
	/**< rss by udp header */
	DOCA_FLOW_RSS_TCP = (1 << 3),
	/**< rss by tcp header */
	DOCA_FLOW_RSS_ESP = (1 << 4),
	/**< rss by esp header */
	DOCA_FLOW_RSS_IPV4_SRC = (1 << 5),
	/**< rss by ipv4 src */
	DOCA_FLOW_RSS_IPV4_DST = (1 << 6),
	/**< rss by ipv4 dst */
	DOCA_FLOW_RSS_IPV6_SRC = (1 << 7),
	/**< rss by ipv6 src */
	DOCA_FLOW_RSS_IPV6_DST = (1 << 8),
	/**< rss by ipv6 dst */
	DOCA_FLOW_RSS_UDP_SRC = (1 << 9),
	/**< rss by udp src */
	DOCA_FLOW_RSS_UDP_DST = (1 << 10),
	/**< rss by udp dst */
	DOCA_FLOW_RSS_TCP_SRC = (1 << 11),
	/**< rss by tcp src */
	DOCA_FLOW_RSS_TCP_DST = (1 << 12),
	/**< rss by tcp dst */
	DOCA_FLOW_RSS_AUTO = (1 << 30),
	/**< rss according to how DOCA Flow sees fit */
};

/**
 * @brief forwarding configuration
 */
struct doca_flow_fwd {
	enum doca_flow_fwd_type type;
	/**< indicate the forwarding type */
	union {
		struct {
			enum doca_flow_resource_type rss_type;
			/**< rss forwarding type */
			union {
				struct doca_flow_resource_rss_cfg rss;
				/**< non-shared rss configuration */
				uint32_t shared_rss_id;
				/**< shared rss id */
			};
		};
		/**< rss configuration information */
		struct {
			uint16_t port_id;
			/**< destination port id */
		};
		/**< port configuration information */
		struct {
			struct doca_flow_pipe *next_pipe;
			/**< next pipe pointer */
		};
		/**< next pipe configuration information */
		struct {
			/** Ordered list pipe to select an entry from. */
			struct doca_flow_pipe *pipe;
			/** Index of the ordered list pipe entry. */
			uint32_t idx;
		} ordered_list_pipe;
		/**< next ordered list pipe configuration */
		struct {
			struct doca_flow_target *target;
			/**< pointer to target handler */
		};
		/**< target configuration information */
		struct {
			struct doca_flow_pipe *pipe;
			/** Hash pipe to forward. */
			enum doca_flow_pipe_hash_map_algorithm algorithm;
			/** Algorithm to use */
		} hash_pipe;
		/**< next hash pipe configuration */
	};
};

/**
 * @brief Traffic meter algorithms
 */
enum doca_flow_meter_algorithm_type {
	DOCA_FLOW_METER_ALGORITHM_TYPE_RFC2697,
	/**< Single Rate Three Color Marker - IETF RFC 2697. */
	DOCA_FLOW_METER_ALGORITHM_TYPE_RFC2698,
	/**< Two Rate Three Color Marker - IETF RFC 2698. */
	DOCA_FLOW_METER_ALGORITHM_TYPE_RFC4115,
	/**< Two Rate Three Color Marker - IETF RFC 4115. */
};

/**
 * @brief Traffic meter limit type: per bytes or per packets for all
 * meter parameters: cir, cbs, eir, ebs.
 */
enum doca_flow_meter_limit_type {
	DOCA_FLOW_METER_LIMIT_TYPE_BYTES = 0,
	/**< Meter parameters per bytes */
	DOCA_FLOW_METER_LIMIT_TYPE_PACKETS,
	/**< Meter parameters packets */
};

/**
 * @brief Traffic meter init color mode when creating a pipe or entry: blind
 * (fixed as green) or aware (configurable value).
 */
enum doca_flow_meter_color_mode {
	DOCA_FLOW_METER_COLOR_MODE_BLIND = 0,
	/**< Meter action init color is green. */
	DOCA_FLOW_METER_COLOR_MODE_AWARE,
	/**< Meter action init color is configured. */
};

/**
 * @brief doca flow meter resource configuration
 */
struct doca_flow_resource_meter_cfg {
	enum doca_flow_meter_limit_type limit_type;
	/**< Meter rate limit type: bytes / packets per second */
	enum doca_flow_meter_color_mode color_mode;
	/**< Meter color mode: blind / aware */
	enum doca_flow_meter_algorithm_type alg;
	/**< Meter algorithm by RFCs */
	uint64_t cir;
	/**< Committed Information Rate (bytes or packets per second). */
	uint64_t cbs;
	/**< Committed Burst Size (bytes or packets). */
	union {
		struct {
			uint64_t ebs;
			/** Excess Burst Size (EBS) (bytes or packets). */
		} rfc2697;
		struct {
			uint64_t pir;
			/**< Peak Information Rate (bytes or packets per seconds). */
			uint64_t pbs;
			/**< Peak Burst Size (bytes or packets). */
		} rfc2698;
		struct {
			uint64_t eir;
			/**< Excess Information Rate (bytes or packets per seconds). */
			uint64_t ebs;
			/**< Excess Burst Size (EBS) (bytes or packets). */
		} rfc4115;
	};
};

/**
 * @brief doca flow psp resource configuration
 */
struct doca_flow_resource_psp_cfg {
	struct doca_flow_crypto_key_cfg key_cfg;
	/**< PSP key configuration */
};

/**
 * @brief doca flow ipsec SA resource configuration
 */
struct doca_flow_resource_ipsec_sa_cfg {
	struct doca_flow_crypto_key_cfg key_cfg;
	/**< IPSec key configuration */
	uint32_t salt;
	/**< salt value */
	uint64_t implicit_iv;
	/**< implicit IV value */
	enum doca_flow_crypto_icv_len icv_len;
	/**< ICV value */
	enum doca_flow_crypto_sn_offload_type sn_offload_type;
	/**< SN offload type - increment or anti-replay */
	enum doca_flow_crypto_replay_win_size win_size;
	/**< Anti-replay window size - only valid when using DOCA_FLOW_CRYPTO_SN_OFFLOAD_AR */
	bool esn_en;
	/**< Enable extended sequence number */
	uint64_t sn_initial;
	/**< Initial sequence number */
	uint32_t lifetime_threshold;
	/**<  When SN reaches this threshold, all passing packets will return a relevant syndrome */
};

/**
 * @brief doca flow shared resource configuration
 */
struct doca_flow_shared_resource_cfg {
	union {
		struct doca_flow_resource_meter_cfg meter_cfg;
		struct doca_flow_resource_rss_cfg rss_cfg;
		struct doca_flow_resource_psp_cfg psp_cfg;
		struct doca_flow_resource_encap_cfg encap_cfg;
		struct doca_flow_resource_decap_cfg decap_cfg;
		struct doca_flow_resource_ipsec_sa_cfg ipsec_sa_cfg;
	};
};

/**
 * @brief doca monitor action configuration
 */
struct doca_flow_monitor {
	enum doca_flow_resource_type meter_type;
	/**< Type of meter configuration. */
	union {
		struct {
			enum doca_flow_meter_limit_type limit_type;
			/**< Meter rate limit type: bytes / packets per second */
			uint64_t cir;
			/**< Committed Information Rate (bytes/second). */
			uint64_t cbs;
			/**< Committed Burst Size (bytes). */
		} non_shared_meter;
		struct {
			uint32_t shared_meter_id;
			/**< shared meter id */
			enum doca_flow_meter_color meter_init_color;
			/**< meter initial color */
		} shared_meter;
	};

	enum doca_flow_resource_type counter_type;
	/**< Type of counter configuration. */
	union {
		struct {
			uint32_t shared_counter_id;
			/**< shared counter id */
		} shared_counter;
	};

	uint32_t aging_sec;
	/**< aging time in seconds.*/
};

/**
 * @brief action type enumeration
 */
enum doca_flow_action_type {
	DOCA_FLOW_ACTION_AUTO = 0,
	/* Derived from pipe actions. */
	DOCA_FLOW_ACTION_ADD,
	/* Add field value from pipe actions or flow entry. */
	DOCA_FLOW_ACTION_COPY,
	/* Copy field to another field. */
};

/**
 * @brief action description
 */
struct doca_flow_action_desc {
	enum doca_flow_action_type type; /**< type */
	union {
		struct {
			struct doca_flow_desc_field src; /* Source info to copy from. */
			struct doca_flow_desc_field dst; /* Or destination info to copy to. */
			uint32_t width;			 /* Bit width to copy */
		} field_op;
	};
};

/**
 * @brief action descriptor array
 */
struct doca_flow_action_descs {
	uint8_t nb_action_desc;
	/**< maximum number of action descriptor array. */
	struct doca_flow_action_desc *desc_array;
	/**< action descriptor array pointer. */
};

/**
 * @brief Type of an ordered list element
 */
enum doca_flow_ordered_list_element_type {
	DOCA_FLOW_ORDERED_LIST_ELEMENT_ACTIONS,
	/**
	 * Ordered list element is struct doca_flow_actions,
	 * and/or actions mask associated with the current element,
	 * or corresponding doca_flow_action_desc.
	 */
	DOCA_FLOW_ORDERED_LIST_ELEMENT_MONITOR,
	/** Ordered list element is struct doca_flow_monitor. */
	DOCA_FLOW_ORDERED_LIST_ELEMENT_NAT64,
	/** Ordered list element is struct doca_flow_nat64_action. */
	DOCA_FLOW_ORDERED_LIST_ELEMENT_CRYPTO,
	/** Ordered list element is struct doca_flow_crypto. */
	DOCA_FLOW_ORDERED_LIST_ELEMENT_EXTERNAL_ACTIONS,
	/** Ordered list element is external actions (void * to pipe-level handle or entry struct). */
};

/**
 * @brief Ordered list elements
 */
struct doca_flow_ordered_list_element {
	enum doca_flow_ordered_list_element_type type;
	/**< Types of DOCA Flow structures each of the elements is pointing to. */
	union {
		struct {
			struct doca_flow_actions *actions;
			/* Pointer to DOCA Flow actions. */
			struct doca_flow_actions *actions_mask;
			/* Pointer to DOCA Flow actions masks. */
			struct doca_flow_action_descs *action_descs;
			/* Pointer to DOCA Flow actions descriptors. */
		};
		struct doca_flow_monitor *monitor;
		/* Pointer to DOCA Flow monitor. */
		struct doca_flow_nat64_action *nat64;
		/* Pointer to DOCA Flow nat64 action. */
		struct doca_flow_crypto *crypto;
		/* Pointer to DOCA Flow crypto. */
		void *external_actions;
		/* Pointer to external action handle (pipe create) or entry struct (entry add). */
	};
};

/**
 * @brief Ordered list configuration
 */
struct doca_flow_ordered_list {
	uint32_t idx;
	/**< List index among the lists of the pipe.
	 * At pipe creation, it must match the list position in the array of lists.
	 * At entry insertion, it determines which list to use.
	 */
	uint32_t size;
	/**< Number of elements in the list. */
	struct doca_flow_ordered_list_element *elements;
	/**< An array of DOCA flow structure pointers, depending on types. */
};

/**
 * @brief flow resource query
 */
struct doca_flow_resource_query {
	union {
		struct {
			uint64_t total_bytes;
			/**< total bytes hit this flow */
			uint64_t total_pkts;
			/**< total packets hit this flow */
		} counter;
		struct {
			uint64_t current_sn;
			/**< Current SN for encrypt, lower bound of AR window for decrypt */
		} ipsec_sa;
	};
};

/**
 * @brief Geneve TLV option class mode
 */
enum doca_flow_parser_geneve_opt_mode {
	DOCA_FLOW_PARSER_GENEVE_OPT_MODE_IGNORE,
	/**< class is ignored. */
	DOCA_FLOW_PARSER_GENEVE_OPT_MODE_FIXED,
	/**< class is fixed (the class defines the option along with the type). */
	DOCA_FLOW_PARSER_GENEVE_OPT_MODE_MATCHABLE,
	/**< class is matching per flow. */
};

/**
 * @brief Default miss type
 */
enum doca_flow_port_default_miss_type {
	DOCA_FLOW_PORT_DEFAULT_MISS_KERNEL = 0,
	DOCA_FLOW_PORT_DEFAULT_MISS_RSS,
	DOCA_FLOW_PORT_DEFAULT_MISS_DROP,
};

/**
 * @brief User configuration structure using to create parser for single GENEVE TLV option.
 */
struct doca_flow_parser_geneve_opt_cfg {
	enum doca_flow_parser_geneve_opt_mode match_on_class_mode;
	/**< Indicator about class field role in this option. */
	doca_be16_t option_class;
	/**< The class of the GENEVE TLV option. */
	uint8_t option_type;
	/**< The type of the GENEVE TLV option. */
	uint8_t option_len;
	/**< The length of the GENEVE TLV option data in DW granularity. */
	doca_be32_t data_mask[DOCA_FLOW_GENEVE_DATA_OPTION_LEN_MAX];
	/**< Data mask describing which DWs should be sampled. */
};

/**
 * @brief Defines the operation states for a port instance.
 */
enum doca_flow_port_operation_state {
	DOCA_FLOW_PORT_OPERATION_STATE_ACTIVE,
	/**< This instance actively handles incoming and outgoing traffic */
	DOCA_FLOW_PORT_OPERATION_STATE_ACTIVE_READY_TO_SWAP,
	/**< This instance actively handles traffic when no other active instance is available */
	DOCA_FLOW_PORT_OPERATION_STATE_STANDBY,
	/**< This instance handles traffic only when no active or active_ready_to_swap instance is available */
	DOCA_FLOW_PORT_OPERATION_STATE_UNCONNECTED,
	/**< This instance does not handle traffic, regardless of the state of other instances */
};

/**
 * @brief Defines the tune profiles
 */
enum doca_flow_tune_profile {
	DOCA_FLOW_TUNE_PROFILE_NONE = 0,
	/**< Tune profile none */
	DOCA_FLOW_TUNE_PROFILE_PIPELINE = 30,
	/**< Tune profile pipeline */
	DOCA_FLOW_TUNE_PROFILE_FULL = 60,
	/**< Tune profile full */
};

/**
 * @brief Initialize the doca flow.
 *
 * This is the global initialization function for doca flow. It
 * initializes all resources used by doca flow.
 *
 * Must be invoked first before any other function in this API.
 * this is a one time call, used for doca flow initialization and
 * global configurations.
 *
 * @param [in] cfg
 * Port configuration, see doca_flow_cfg for details.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported configuration.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_STABLE
doca_error_t doca_flow_init(struct doca_flow_cfg *cfg);

/**
 * @brief Destroy the doca flow.
 *
 * Release all the resources used by doca flow.
 *
 * Must be invoked at the end of the application, before it exits.
 */
DOCA_STABLE
void doca_flow_destroy(void);

/**
 * @brief Start a doca port.
 *
 * Start a port with the given configuration. Will create one port in
 * the doca flow layer, allocate all resources used by this port, and
 * create the default offload logic for traffic.
 *
 * @param [in] cfg
 * Port configuration, see doca_flow_cfg for details.
 * @param [out] port
 * Port handler on success.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported port type.
 * - DOCA_ERROR_NOT_PERMITTED - operation not permitted.
 * - DOCA_ERROR_ALREADY_EXIST - another instance has the same operation state.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_STABLE
doca_error_t doca_flow_port_start(const struct doca_flow_port_cfg *cfg, struct doca_flow_port **port);

/**
 * @brief Stop a doca port.
 *
 * Stop the port, disable the traffic, destroy the doca port,
 * free all resources of the port.
 *
 * @param [in] port
 * Port struct.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_IN_USE - port resources in use.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_STABLE
doca_error_t doca_flow_port_stop(struct doca_flow_port *port);

/**
 * @brief pair two doca flow ports.
 *
 * This API should be used to pair two doca ports. This pair should be the
 * same as the actual physical layer paired information. Those two pair
 * ports have no order, a port cannot be paired with itself.
 *
 * In this API, default behavior will be handled according to each modes.
 * In VNF mode, pair information will be translated to queue action to
 * redirect packets to it's pair port. In REMOTE_VNF mode,
 * default rules will be created to redirect packets between 2 pair ports.
 *
 * @param [in] port
 * Pointer to doca flow port.
 * @param [in] pair_port
 * Pointer to the pair port.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - not supported in the current run mode.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */

DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_pair(struct doca_flow_port *port, struct doca_flow_port *pair_port);

/**
 * @brief Get pointer of user private data.
 *
 * User can manage specific data structure in port structure.
 * The size of the data structure is given on port configuration.
 * See doca_flow_cfg for more details.
 *
 * @param [in] port
 * Port struct.
 * @return
 * Private data head pointer.
 */
DOCA_STABLE
uint8_t *doca_flow_port_priv_data(struct doca_flow_port *port);

/**
 * @brief Modifies the operation state of a port instance.
 *
 * This function changes the operation state of the given port instance.
 * If the port instance already has the required state, the function returns DOCA_SUCCESS without performing any
 * operations.
 *
 * @warning Two instances cannot be in the same operation state simultaneously except
 *          DOCA_FLOW_PORT_OPERATION_STATE_UNCONNECTED.
 *
 * @param [in] port
 * Pointer to the DOCA port instance.
 * @param [in] state
 * The desired operation state for the port instance.
 * @return
 * DOCA_SUCCESS - if successful.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - not supported in the current run mode.
 * - DOCA_ERROR_NOT_PERMITTED - operation not permitted.
 * - DOCA_ERROR_ALREADY_EXIST - another instance exists with the same state.
 * - DOCA_ERROR_UNKNOWN - unknown error.
 */
DOCA_STABLE
doca_error_t doca_flow_port_operation_state_modify(struct doca_flow_port *port,
						   enum doca_flow_port_operation_state state);

/**
 * @brief Configure a single shared resource.
 *
 * This API can be used by bounded and unbounded resources.
 *
 * @param [in] type
 * Shared resource type.
 * @param [in] id
 * Shared resource id.
 * @param [in] cfg
 * Pointer to a shared resource configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported shared resource type.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_shared_resource_set_cfg(enum doca_flow_shared_resource_type type,
					       uint32_t id,
					       struct doca_flow_shared_resource_cfg *cfg);

/**
 * @brief Binds a bulk of shared resources to a bindable object.
 *
 * Binds a bulk of shared resources from the same type to a bindable object.
 * Currently the bindable objects are ports and pipes.
 *
 * @param [in] type
 * Shared resource type.
 * @param [in] res_array
 * Array of shared resource IDs.
 * @param [in] res_array_len
 * Shared resource IDs array length.
 * @param [in] bindable_obj
 * Pointer to an allowed bindable object, use NULL to bind globally.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported shared resource type.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_STABLE
doca_error_t doca_flow_shared_resources_bind(enum doca_flow_shared_resource_type type,
					     uint32_t *res_array,
					     uint32_t res_array_len,
					     void *bindable_obj);

/**
 * @brief Extract information about shared counter
 *
 * Query an array of shared objects of a specific type.
 *
 * @param [in] type
 * Shared object type.
 * @param [in] res_array
 * Array of shared objects IDs to query.
 * @param [in] data_array
 * Data array retrieved by the query.
 * @param [in] array_len
 * Number of objects and their query results in their arrays (same number).
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported shared resource type.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_shared_resources_query(enum doca_flow_shared_resource_type type,
					      uint32_t *res_array,
					      struct doca_flow_resource_query *data_array,
					      uint32_t array_len);

/**
 * @brief Create one new pipe.
 *
 * Create new pipeline to match and offload specific packets, the pipe
 * configuration includes the following components:
 *
 *     match: Match one packet by inner or outer fields.
 *     match_mask: The mask for the matched items.
 *     actions: Includes the modify specific packets fields, Encap and
 *                  Decap actions.
 *     monitor: Includes Count, Age, and Meter actions.
 *     fwd: The destination of the matched action, include RSS, Hairpin,
 *             Port, and Drop actions.
 *
 * This API will create the pipe, but would not start the HW offload.
 *
 * @param [in] cfg
 * Pipe configuration.
 * @param [in] fwd
 * Fwd configuration for the pipe.
 * @param [in] fwd_miss
 * Fwd_miss configuration for the pipe. NULL for no fwd_miss.
 * When creating a pipe if there is a miss and fwd_miss configured,
 * packet steering should jump to it.
 * @param [out] pipe
 * Pipe handler on success.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported pipe type.
 * - DOCA_ERROR_DRIVER - driver error.
 * - DOCA_ERROR_TOO_BIG - pipe specs exceed capability
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_create(const struct doca_flow_pipe_cfg *cfg,
				   const struct doca_flow_fwd *fwd,
				   const struct doca_flow_fwd *fwd_miss,
				   struct doca_flow_pipe **pipe);

/**
 * @brief doca flow pipe resize number of entries changed callback.
 *
 * @param [in] pipe_user_ctx
 * Pointer to pipe user context.
 * @param [out] nr_entries
 * Changed value for pipe's number of entries.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - memory error.
 */
typedef doca_error_t (*doca_flow_pipe_resize_nr_entries_changed_cb)(void *pipe_user_ctx, uint32_t nr_entries);

/**
 * @brief doca flow pipe entry relocation callback.
 *
 * Called for each entry that reached its destination after resize.
 * User is allowed to switch the context to a new pointer.
 *
 * @param [in] pipe_user_ctx
 * Pointer to pipe user context.
 * @param [in] pipe_queue
 * Pipe queue id.
 * @param [in] entry_user_ctx
 * Pointer to entry user context.
 * @param [out] new_entry_user_ctx
 * Pointer to new entry user context.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - memory error.
 */
typedef doca_error_t (*doca_flow_pipe_resize_entry_relocate_cb)(void *pipe_user_ctx,
								uint16_t pipe_queue,
								void *entry_user_ctx,
								void **new_entry_user_ctx);

/**
 * @brief Resize pipe
 *
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] new_congestion_level
 * Pushback the pipe current congestion level to a new value.
 * @param [in] nr_entries_changed_cb
 * Number of entries after resize.
 * @param [in] entry_relocation_cb
 * Entry relocate behavior.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - driver error.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_resize(struct doca_flow_pipe *pipe,
				   uint8_t new_congestion_level,
				   doca_flow_pipe_resize_nr_entries_changed_cb nr_entries_changed_cb,
				   doca_flow_pipe_resize_entry_relocate_cb entry_relocation_cb);

/**
 * @brief Add one new entry to a pipe.
 *
 * When a packet matches a single pipe, will start HW offload. The pipe only
 * defines which fields to match. When offloading, we need detailed information
 * from packets, or we need to set some specific actions that the pipe did not
 * define. The parameters include:
 *
 *    match: The packet detail fields according to the pipe definition.
 *    actions: The real actions according to the pipe definition.
 *    monitor: Defines the monitor actions if the pipe did not define it.
 *    fwd: Define the forward action if the pipe did not define it.
 *
 * This API will do the actual HW offload, with the information from the fields
 * of the input packets.
 *
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] match
 * Pointer to match, indicate specific packet match information.
 * @param [in] action_idx
 * Index according to place provided on creation.
 * @param [in] actions
 * Pointer to modify actions, indicate specific modify information.
 * @param [in] monitor
 * Pointer to monitor actions.
 * @param [in] fwd
 * Pointer to fwd actions.
 * @param [in] flags
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_entry_flags.
 * @param [in] usr_ctx
 * Pointer to the user context. This context is associated with the entry and will
 * be used during update and removal operations.
 * Therefore, the pointer must remain valid for as long as the entry is valid.
 * @param [out] entry
 * Pipe entry handler on success.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - driver error.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_basic_add_entry(uint16_t pipe_queue,
					    struct doca_flow_pipe *pipe,
					    const struct doca_flow_match *match,
					    uint8_t action_idx,
					    const struct doca_flow_actions *actions,
					    const struct doca_flow_monitor *monitor,
					    const struct doca_flow_fwd *fwd,
					    uint32_t flags,
					    void *usr_ctx,
					    struct doca_flow_pipe_entry **entry);

/**
 * @brief Update the pipe entry with new actions.
 *
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] action_idx
 * Index according to place provided on creation.
 * @param [in] actions
 * Pointer to modify actions, indicate specific modify information.
 * @param [in] monitor
 * Pointer to monitor actions.
 * @param [in] fwd
 * Pointer to fwd actions.
 * @param [in] flags
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_entry_flags.
 * @param [in] entry
 * The pipe entry to be updated.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported pipe type.
 * - DOCA_ERROR_AGAIN - resource temporarily unavailable, try again
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_basic_update_entry(uint16_t pipe_queue,
					       struct doca_flow_pipe *pipe,
					       uint8_t action_idx,
					       const struct doca_flow_actions *actions,
					       const struct doca_flow_monitor *monitor,
					       const struct doca_flow_fwd *fwd,
					       uint32_t flags,
					       struct doca_flow_pipe_entry *entry);

/**
 * @brief Add one new entry to a control pipe.
 *
 * Refer to doca_flow_pipe_basic_add_entry.
 *
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] match
 * Pointer to match, indicate specific packet match information.
 * @param [in] match_mask
 * Pointer to match mask information.
 * @param [in] condition
 * Pointer to match condition information.
 * @param [in] actions
 * Pointer to modify actions, indicate specific modify information.
 * @param [in] actions_mask
 * Pointer to modify actions' mask, indicate specific modify information.
 * @param [in] action_descs
 * action descriptions
 * @param [in] monitor
 * Pointer to monitor actions.
 * @param [in] priority
 * Priority value.
 * @param [in] fwd
 * Pointer to fwd actions.
 * @param [in] usr_ctx
 * Pointer to the user context. This context is associated with the entry and will
 * be used during update and removal operations.
 * Therefore, the pointer must remain valid for as long as the entry is valid.
 * @param [out] entry
 * Pipe entry handler on success.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported pipe type.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_control_add_entry(uint16_t pipe_queue,
					      struct doca_flow_pipe *pipe,
					      const struct doca_flow_match *match,
					      const struct doca_flow_match *match_mask,
					      const struct doca_flow_match_condition *condition,
					      const struct doca_flow_actions *actions,
					      const struct doca_flow_actions *actions_mask,
					      const struct doca_flow_action_descs *action_descs,
					      const struct doca_flow_monitor *monitor,
					      uint32_t priority,
					      const struct doca_flow_fwd *fwd,
					      void *usr_ctx,
					      struct doca_flow_pipe_entry **entry);

/**
 * @brief Add one new entry to a lpm pipe.
 *
 * This API will populate the lpm entries
 *
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] match
 * Pointer to match, indicate specific packet match information.
 * @param [in] match_mask
 * Pointer to match mask information.
 * @param [in] action_idx
 * Index according to place provided on creation.
 * @param [in] actions
 * Pointer to modify actions, indicate specific modify information.
 * @param [in] monitor
 * Pointer to monitor actions.
 * @param [in] fwd
 * Pointer to fwd actions.
 * @param [in] flags
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_entry_flags.
 * @param [in] usr_ctx
 * Pointer to the user context. This context is associated with the entry and will
 * be used during update and removal operations.
 * Therefore, the pointer must remain valid for as long as the entry is valid.
 * @param [out] entry
 * Pipe entry handler on success.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - driver error.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_lpm_add_entry(uint16_t pipe_queue,
					  struct doca_flow_pipe *pipe,
					  const struct doca_flow_match *match,
					  const struct doca_flow_match *match_mask,
					  uint8_t action_idx,
					  const struct doca_flow_actions *actions,
					  const struct doca_flow_monitor *monitor,
					  const struct doca_flow_fwd *fwd,
					  uint32_t flags,
					  void *usr_ctx,
					  struct doca_flow_pipe_entry **entry);

/**
 * @brief Update the lpm pipe entry with new actions.
 *
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] action_idx
 * Index according to place provided on creation.
 * @param [in] actions
 * Pointer to modify actions, indicate specific modify information.
 * @param [in] monitor
 * Pointer to monitor actions.
 * @param [in] fwd
 * Pointer to fwd actions.
 * @param [in] flags
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_entry_flags.
 * @param [in] entry
 * The pipe entry to be updated.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - driver error.
 * - DOCA_ERROR_AGAIN - resource temporarily unavailable, try again
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_lpm_update_entry(uint16_t pipe_queue,
					     struct doca_flow_pipe *pipe,
					     uint8_t action_idx,
					     const struct doca_flow_actions *actions,
					     const struct doca_flow_monitor *monitor,
					     const struct doca_flow_fwd *fwd,
					     uint32_t flags,
					     struct doca_flow_pipe_entry *entry);

/**
 * Add an entry to the ordered list pipe.
 *
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] pipe
 * Pipe handle.
 * @param [in] idx
 * Unique entry index. It is the user's responsibility to ensure uniqueness.
 * @param [in] ordered_list
 * Ordered list with pointers to struct doca_flow_actions and struct doca_flow_monitor
 * at the same indices as they were at the pipe creation time.
 * If the configuration contained an element of struct doca_flow_action_descs,
 * the corresponding array element is ignored and can be NULL.
 * @param [in] fwd
 * Entry forward configuration.
 * @param [in] flags
 * Entry insertion flags.
 * @param [in] user_ctx
 * Pointer to the user context. This context is associated with the entry and will
 * be used during removal operation.
 * Therefore, the pointer must remain valid for as long as the entry is valid.
 * @param[out] entry
 * The entry inserted.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_DRIVER - driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_ordered_list_add_entry(uint16_t pipe_queue,
						   struct doca_flow_pipe *pipe,
						   uint32_t idx,
						   const struct doca_flow_ordered_list *ordered_list,
						   const struct doca_flow_fwd *fwd,
						   uint32_t flags,
						   void *user_ctx,
						   struct doca_flow_pipe_entry **entry);
/**
 * @brief Add one new entry to a acl pipe.
 *
 * This API will populate the acl entries
 *
 * @param pipe_queue
 * Queue identifier.
 * @param pipe
 * Pointer to pipe.
 * @param match
 * Pointer to match, indicate specific packet match information.
 * @param match_mask
 * Pointer to match mask information.
 * @param [in] action_idx
 * Index according to place provided on creation.
 * @param [in] actions
 * Pointer to modify actions, indicate specific modify information.
 * @param priority
 * Priority value
 * @param fwd
 * Pointer to fwd actions.
 * @param flags
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_entry_flags.
 * @param usr_ctx
 * Pointer to the user context. This context is associated with the entry and will
 * be used during update and removal operations.
 * Therefore, the pointer must remain valid for as long as the entry is valid.
 * @param[out] entry
 * The entry inserted.
 * @return
 * Pipe entry handler on success, NULL otherwise and error is set.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_acl_add_entry(uint16_t pipe_queue,
					  struct doca_flow_pipe *pipe,
					  const struct doca_flow_match *match,
					  const struct doca_flow_match *match_mask,
					  uint8_t action_idx,
					  const struct doca_flow_actions *actions,
					  const uint32_t priority,
					  const struct doca_flow_fwd *fwd,
					  uint32_t flags,
					  void *usr_ctx,
					  struct doca_flow_pipe_entry **entry);

/**
 * @brief Update the acl pipe entry with new actions.
 *
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] action_idx
 * Index according to place provided on creation.
 * @param [in] actions
 * Pointer to modify actions, indicate specific modify information.
 * @param [in] fwd
 * Pointer to fwd actions.
 * @param [in] flags
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_entry_flags.
 * @param [in] entry
 * The pipe entry to be updated.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - driver error.
 * - DOCA_ERROR_AGAIN - resource temporarily unavailable, try again
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_acl_update_entry(uint16_t pipe_queue,
					     struct doca_flow_pipe *pipe,
					     uint8_t action_idx,
					     const struct doca_flow_actions *actions,
					     const struct doca_flow_fwd *fwd,
					     uint32_t flags,
					     struct doca_flow_pipe_entry *entry);

/**
 * @brief Add one new entry to an hash pipe.
 *
 * Refer to doca_flow_pipe_basic_add_entry.
 *
 * @param pipe_queue
 * Queue identifier.
 * @param pipe
 * Pointer to pipe.
 * @param entry_index
 * Static index in pipe for this entry.
 * @param action_idx
 * Index according to place provided on creation.
 * @param actions
 * Pointer to modify actions, indicate specific modify information.
 * @param monitor
 * Pointer to monitor actions.
 * @param fwd
 * Pointer to forward actions.
 * @param flags
 * Flow entry will be pushed to HW immediately or not. enum doca_flow_entry_flags.
 * @param usr_ctx
 * Pointer to the user context. This context is associated with the entry and will
 * be used during removal operation.
 * Therefore, the pointer must remain valid for as long as the entry is valid.
 * @param [out] entry
 * Pipe entry handler on success.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - driver error.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_hash_add_entry(uint16_t pipe_queue,
					   struct doca_flow_pipe *pipe,
					   uint32_t entry_index,
					   uint8_t action_idx,
					   const struct doca_flow_actions *actions,
					   const struct doca_flow_monitor *monitor,
					   const struct doca_flow_fwd *fwd,
					   uint32_t flags,
					   void *usr_ctx,
					   struct doca_flow_pipe_entry **entry);

/**
 * @brief Free one pipe entry.
 *
 * This API will free the pipe entry and cancel HW offload. The
 * Application receives the entry pointer upon creation and if can
 * call this function when there is no more need for this offload.
 * For example, if the entry aged, use this API to free it.
 *
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] flags
 * Flow entry will be removed from hw immediately or not. enum doca_flow_entry_flags.
 * @param [in] entry
 * The pipe entry to be removed.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported pipe type.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_remove_entry(uint16_t pipe_queue, uint32_t flags, struct doca_flow_pipe_entry *entry);

/**
 * @brief calc the hash for a given match on a given pipe.
 *
 * Calculates the hash value for a given pipe assuming the that the match
 * parameter holds the values that the HW will see.
 *
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] match
 * Pointer to match, indicate specific packet match information.
 * @param [out] hash
 * The calculated hash on success.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - driver error.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_calc_hash(struct doca_flow_pipe *pipe, const struct doca_flow_match *match, uint32_t *hash);

/**
 * @brief Calculate the entropy.
 *
 * Calculate the entropy as it would have been calculated by the HW.
 *
 * @param [in] port
 * The given port for the entropy calculation.
 * @param [in] header
 * Pointer to the header that holds the fields that are the base for the
 * entropy calculation.
 * @param [out] entropy
 * Used to return the calculated entropy. It will be written in network order.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_calc_entropy(struct doca_flow_port *port,
					 struct doca_flow_entropy_format *header,
					 uint16_t *entropy);

/**
 * @brief Destroy one pipe
 *
 * Destroy the pipe, and the pipe entries that match this pipe.
 *
 * @param [in] pipe
 * Pointer to pipe.
 */
DOCA_STABLE
void doca_flow_pipe_destroy(struct doca_flow_pipe *pipe);

/**
 * @brief Flush pipes of one port
 *
 * Destroy all pipes and all pipe entries belonging to the port.
 *
 * @param [in] port
 * Pointer to doca flow port.
 */
DOCA_STABLE
void doca_flow_port_pipes_flush(struct doca_flow_port *port);

/**
 * @brief Dump pipe information (not including the entries)
 *
 * @param [in] pipe
 * Pointer to doca flow pipe.
 * @param [in] f
 * The output file of the pipe information.
 */
DOCA_EXPERIMENTAL
void doca_flow_pipe_dump(struct doca_flow_pipe *pipe, FILE *f);

/**
 * @brief Extract information about specific entry
 *
 * Query the packet statistics about specific pipe entry
 *
 * @param [in] entry
 * The pipe entry to query.
 * @param [in] data
 * Data retrieved by the query.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_STABLE
doca_error_t doca_flow_resource_query_entry(struct doca_flow_pipe_entry *entry, struct doca_flow_resource_query *data);

/**
 * @brief Extract information about pipe miss entry
 *
 * Query the packet statistics about specific pipe miss entry
 *
 * @param [in] pipe
 * The pipe to query.
 * @param [in] data
 * Data retrieved by the query.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_STABLE
doca_error_t doca_flow_resource_query_pipe_miss(struct doca_flow_pipe *pipe, struct doca_flow_resource_query *data);

/**
 * @brief Update the forward miss action.
 *
 * @param [in] pipe
 * The pipe to update its miss action.
 * @param [in] fwd_miss
 * A new fwd_miss configuration for the pipe.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported forward request.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_update_miss(struct doca_flow_pipe *pipe, const struct doca_flow_fwd *fwd_miss);

/**
 * @brief Handle aging of entries.
 *
 * Process aged entries, the user will get a notification in
 * the callback.
 *
 * Handling of aged entries can take too much time, so we split each cycle
 * to small chunks that are limited by some time quota.
 *
 * As long as the function doesn't return -1, more entries
 * are pending processing for this cycle.
 *
 * @param [in] port
 * Port to handle aging
 * @param [in] queue
 * Queue identifier.
 * @param [in] quota
 * Max time quota in micro seconds handle aging, 0: no limit.
 * @param [in] max_entries
 * Max entries for this function to handle aging, 0: no limit.
 * @return
 * > 0 the number of aged entries.
 * 0 no aged entries in current call.
 * -1 full cycle done.
 */
DOCA_STABLE
int doca_flow_aging_handle(struct doca_flow_port *port, uint16_t queue, uint64_t quota, uint64_t max_entries);

/**
 * @brief Process entries in queue.
 *
 * The application must invoke this function in order to complete
 * the flow rule offloading and to receive the flow rule operation status.
 *
 * @param [in] port
 * Port
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] timeout
 * Max time in micro seconds for this function to process entries.
 * Process once if timeout is 0
 * @param [in] max_processed_entries
 * Flow entries number to process
 * If it is 0, it will proceed until timeout.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_DRIVER - driver error.
 */
DOCA_STABLE
doca_error_t doca_flow_entries_process(struct doca_flow_port *port,
				       uint16_t pipe_queue,
				       uint64_t timeout,
				       uint32_t max_processed_entries);

/**
 * @brief Get entry's status
 *
 * @param [in] entry
 * pipe entry
 * @return
 * entry's status
 */
DOCA_STABLE
enum doca_flow_entry_status doca_flow_pipe_entry_get_status(struct doca_flow_pipe_entry *entry);

/**
 * @brief Get doca flow switch port
 *
 * @param [in] port
 * The port for which to get the switch port. If NULL, get the first switch
 * port created.
 * The application could use this function to get the doca switch port, then
 * create pipes and pipe entries on this port.
 * @return
 * The parent switch port number or NULL if none found
 *
 */
DOCA_STABLE
struct doca_flow_port *doca_flow_port_switch_get(const struct doca_flow_port *port);

/**
 * @brief Prepare an MPLS label header in big-endian.
 *
 * @note: All input variables are in cpu-endian.
 *
 * @param [in] label
 * The label value - 20 bits.
 * @param [in] traffic_class
 * Traffic class - 3 bits.
 * @param [in] ttl
 * Time to live - 8 bits
 * @param [in] bottom_of_stack
 * Whether this MPLS is bottom of stack.
 * @param [out] mpls
 * Pointer to MPLS structure to fill.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_mpls_label_encode(uint32_t label,
					 uint8_t traffic_class,
					 uint8_t ttl,
					 bool bottom_of_stack,
					 struct doca_flow_header_mpls *mpls);

/**
 * @brief Decode an MPLS label header.
 *
 * @note: All output variables are in cpu-endian.
 *
 * @param [in] mpls
 * Pointer to MPLS structure to decode.
 * @param [out] label
 * Pointer to fill MPLS label value.
 * @param [out] traffic_class
 * Pointer to fill MPLS traffic class value.
 * @param [out] ttl
 * Pointer to fill MPLS TTL value.
 * @param [out] bottom_of_stack
 * Pointer to fill whether this MPLS is bottom of stack.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_mpls_label_decode(const struct doca_flow_header_mpls *mpls,
					 uint32_t *label,
					 uint8_t *traffic_class,
					 uint8_t *ttl,
					 bool *bottom_of_stack);

/**
 * @brief Creates GENEVE TLV parser for the selected port.
 *
 * This function must be called before creation of any pipe using GENEVE option.
 *
 * This API is port oriented, but the configuration is done once for all ports under the same
 * physical device. Each port should call this API before using GENEVE options, but it must use
 * the same options in the same order inside the list.
 *
 * Each physical device has 7 DWs for GENEVE TLV options. Each nonzero element in 'data_mask'
 * array consumes one DW, and choosing matchable mode for class consumes additional one.
 * Calling this API for second port under same physical device doesn't consume more DW, it uses
 * same configuration.
 *
 * @param [in] port
 * Pointer to doca flow port.
 * @param [in] tlv_list
 * A list of GENEVE TLV options to create parser for them.
 * @param [in] nb_options
 * The number of options in TLV list.
 * @param [out] parser
 * Parser handler on success.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported configuration.
 * - DOCA_ERROR_ALREADY_EXIST - physical device already has parser, by either same or another port.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_parser_geneve_opt_create(const struct doca_flow_port *port,
						const struct doca_flow_parser_geneve_opt_cfg tlv_list[],
						uint8_t nb_options,
						struct doca_flow_parser **parser);

/**
 * @brief Destroy GENEVE TLV parser.
 *
 * This function must be called after last use of GENEVE option and before port closing.
 *
 * @param [in] parser
 * Pointer to parser to be destroyed.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_IN_USE - one of options is in used by a pipe.
 * - DOCA_ERROR_DRIVER - there is no valid GENEVE TLV parser in this handle.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_parser_geneve_opt_destroy(struct doca_flow_parser *parser);

/**
 * @brief Get doca flow forward target.
 *
 * @param [in] type
 * Target type.
 * @param [out] target
 * Target handler on success
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported type.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_get_target(enum doca_flow_target_type type, struct doca_flow_target **target);

/**
 * @brief Create Doca Flow Tune configuration struct.
 *
 * Create and allocate DOCA Flow Tune configuration struct
 *
 * @param [in] tune_cfg
 * DOCA Flow Tune configuration
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_cfg_create(struct doca_flow_tune_cfg **tune_cfg);

/**
 * @brief Destroy DOCA Flow Tune configuration struct
 *
 * Free and destroy the DOCA Flow Tune configuration struct
 *
 * @param [in] tune_cfg
 * DOCA Flow Tune configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_cfg_destroy(struct doca_flow_tune_cfg *tune_cfg);

/**
 * @brief Set tune profile
 *
 * Set DOCA Flow Tune profile
 *
 * @param [in] tune_cfg
 * DOCA Flow Tune configuration
 * @param [in] profile
 * DOCA Flow Tune profile
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_tune_cfg_set_profile(struct doca_flow_tune_cfg *tune_cfg, enum doca_flow_tune_profile profile);

/**
 * @brief Create DOCA Flow configuration struct.
 *
 * Create and allocate DOCA Flow configuration struct
 *
 * @param [out] cfg
 * DOCA Flow global configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_create(struct doca_flow_cfg **cfg);

/**
 * @brief Destroy DOCA Flow configuration struct
 *
 * Free and destroy the DOCA Flow configuration struct
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_destroy(struct doca_flow_cfg *cfg);

/**
 * @brief Set tune configuration
 *
 * @param [in] cfg
 * DOCA Flow global configuration
 * @param [in] tune_cfg
 * Tune level configuration struct
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_cfg_set_tune_cfg(struct doca_flow_cfg *cfg, struct doca_flow_tune_cfg *tune_cfg);

/**
 * @brief Set pipe queues
 *
 * Set the pipe's number of queues for each offload thread
 *
 * @param [in] cfg
 * DOCA Flow global configuration
 * @param [in] pipe_queues
 * Pipe queues
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_pipe_queues(struct doca_flow_cfg *cfg, uint16_t pipe_queues);

/**
 * @brief Set number of counters to configure
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] nr_counters
 * Number of counters to configure
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_nr_counters(struct doca_flow_cfg *cfg, uint32_t nr_counters);

/**
 * @brief Set number of traffic meters to configure
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] nr_meters
 * Number of traffic meters to configure
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_nr_meters(struct doca_flow_cfg *cfg, uint32_t nr_meters);

/**
 * @brief Set number of pre-configured collisions
 *
 * Get the number of pre-configured collisions for the acl module
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] nr_acl_collisions
 * Number pre-configured collisions
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_nr_acl_collisions(struct doca_flow_cfg *cfg, uint8_t nr_acl_collisions);

/**
 * @brief Set DOCA mode args
 *
 * Set the DOCA Flow architecture mode switch, vnf
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] mode_args
 * DOCA Flow architecture mode
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_mode_args(struct doca_flow_cfg *cfg, const char *mode_args);

/**
 * @brief Set number of shared resource
 *
 * Set the number of shared resource per type
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] nr_shared_resource
 * Number of shared resource
 * @param [in] type
 * Type of shared resource
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_nr_shared_resource(struct doca_flow_cfg *cfg,
						  uint32_t nr_shared_resource,
						  enum doca_flow_shared_resource_type type);

/**
 * @brief Set number of pre-configured queue_size
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [out] queue_depth
 * Number of pre-configured queue_size
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_queue_depth(struct doca_flow_cfg *cfg, uint32_t queue_depth);

/**
 * @brief Set callback for pipe process completion
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] cb
 * Callback for pipe process completion
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_cb_pipe_process(struct doca_flow_cfg *cfg, doca_flow_pipe_process_cb cb);

/**
 * @brief Set callback for entry create/destroy
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] cb
 * Callback for entry create/destroy
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_cb_entry_process(struct doca_flow_cfg *cfg, doca_flow_entry_process_cb cb);

/**
 * @brief Set callback for unbinding of a shared resource
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] cb
 * Callback for unbinding of a shared resource
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_cb_shared_resource_unbind(struct doca_flow_cfg *cfg,
							 doca_flow_shared_resource_unbind_cb cb);

/**
 * @brief Set RSS hash key
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] rss_key
 * RSS hash key
 * @param [in] rss_key_len
 * Length of the RSS hash key in bytes
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY -  memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_rss_key(struct doca_flow_cfg *cfg, const uint8_t *rss_key, uint32_t rss_key_len);

/**
 * @brief Set the definition object.
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] defs
 * A valid doca flow definitions object.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_cfg_set_definitions(struct doca_flow_cfg *cfg, const struct doca_flow_definitions *defs);

/**
 * @brief Set resource mode
 *
 * Set doca flow's resource mode - system or port. The default is system mode.
 *
 * @param [in] cfg
 * DOCA Flow global configuration.
 * @param [in] mode
 * Doca flow resource mode to use
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_cfg_set_resource_mode(struct doca_flow_cfg *cfg, const enum doca_flow_resource_mode mode);

/**
 * @brief Create DOCA Flow port configuration struct.
 *
 * Create and allocate DOCA Flow port configuration struct
 *
 * @param [out] cfg
 * DOCA Flow port configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_port_cfg_create(struct doca_flow_port_cfg **cfg);

/**
 * @brief Destroy DOCA Flow port configuration struct
 *
 * Free and destroy the DOCA Flow port configuration struct
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_port_cfg_destroy(struct doca_flow_port_cfg *cfg);

/**
 * @brief Set devargs
 *
 * Set specific configuration per port type
 *
 * @param [in] cfg
 * DOCA Flow port configuration
 * @param [in] devargs
 * Specific configuration per port type
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_port_cfg_set_devargs(struct doca_flow_port_cfg *cfg, const char *devargs);

/**
 * @brief Set user private data size
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @param [in] priv_data_size
 * User private data size
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_port_cfg_set_priv_data_size(struct doca_flow_port_cfg *cfg, uint16_t priv_data_size);

/**
 * @brief Set port's device
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @param [in] dev
 * Device
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_port_cfg_set_dev(struct doca_flow_port_cfg *cfg, struct doca_dev *dev);

/**
 * @brief Set port's device for representor.
 *
 * Calling this function also indicates the port is representor.
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @param [in] dev
 * Doca device representor.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_port_cfg_set_dev_rep(struct doca_flow_port_cfg *cfg, struct doca_dev_rep *dev);

/**
 * @brief Set the logical port ID.
 *
 * Assigns a logical port ID to the specified DOCA Flow port.
 *
 * @param [in] cfg
 * Pointer to the DOCA Flow port configuration.
 * @param [in] port_id
 * Logical port ID to assign to the port.
 * @return
 * - DOCA_SUCCESS on success.
 * - Error code on failure:
 *   - DOCA_ERROR_INVALID_VALUE if the input is invalid.
 */
DOCA_STABLE
doca_error_t doca_flow_port_cfg_set_port_id(struct doca_flow_port_cfg *cfg, uint16_t port_id);

/**
 * @brief Disable SN offload for ipsec - Anti-replay and sn increment will not be activated.
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_port_cfg_set_ipsec_sn_offload_disable(struct doca_flow_port_cfg *cfg);

/**
 * @brief Set default rules operation state.
 *
 * @note If this setter is not called, the default state is DOCA_FLOW_PORT_OPERATION_STATE_ACTIVE.
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @param [in] state
 * The desired operation state for the port instance.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_port_cfg_set_operation_state(struct doca_flow_port_cfg *cfg,
						    enum doca_flow_port_operation_state state);

/**
 * @brief Set max memory size used by actions.
 *
 * Set max memory which will be used by actions in this port.
 * Default is zero.
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @param [in] size
 * Actions memory size in bytes, power of 2 recommended but not mandatory.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_cfg_set_actions_mem_size(struct doca_flow_port_cfg *cfg, uint32_t size);

/**
 * @brief Set service threads execution cpu core.
 *
 * Set cpu core which will be used by service threads in this port.
 * Default is zero.
 * Current supported service threads:
 *  counters cache threads
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @param [in] core
 * The cpu core number.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_cfg_set_service_threads_core(struct doca_flow_port_cfg *cfg, uint32_t core);

/**
 * @brief Set service threads max execution cycle.
 *
 * Defines the minimum time between two consecutive thread cycle executions in port.
 * Default is 1000 ms (1 sec).
 * Current supported service threads:
 *  counters cache threads
 *
 * @param [in] cfg
 * DOCA Flow port configuration.
 * @param [in] cycle_ms
 * The service threads cycle in ms.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_cfg_set_service_threads_cycle(struct doca_flow_port_cfg *cfg, uint32_t cycle_ms);

/**
 * @brief Update default miss target
 *
 * Update default miss target for the port.
 *
 * @param [in] port
 * DOCA Flow port.
 * @param [in] default_miss
 * Default miss target..
 * @param [in] counter_en
 * Counter enable, taken from the user counters resources.
 * Can only be set on first usage, for more details see programming guide.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_update_default_miss(struct doca_flow_port *port,
						enum doca_flow_port_default_miss_type default_miss,
						bool counter_en);

/**
 * @brief Default miss type
 *
 * @param [in] port
 * DOCA Flow port.
 * @param [in] data
 * Data retrieved by the query.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_query_default_miss(struct doca_flow_port *port, struct doca_flow_resource_query *data);

/**
 * @brief Configure a single shared resource.
 *
 * @param [in] port
 * Pointer to doca flow port the shared resource belong to
 * @param [in] type
 * Shared resource type.
 * @param [in] shared_resource_id
 * Shared resource id.
 * @param [in] cfg
 * Pointer to a shared resource configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported shared resource type.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_shared_resource_set_cfg(struct doca_flow_port *port,
						    enum doca_flow_shared_resource_type type,
						    uint32_t shared_resource_id,
						    struct doca_flow_shared_resource_cfg *cfg);

/**
 * @brief Get a resource id for shared usage
 *
 * @param [in] port
 * Pointer to doca flow port the shared resource belong to
 * @param [in] type
 * Resource type.
 * @param [in,out] shared_resource_id
 * Shared resource id.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_EMPTY - no available resource id.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported shared resource type.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_shared_resource_get(struct doca_flow_port *port,
						enum doca_flow_shared_resource_type type,
						uint32_t *shared_resource_id);
/**
 * @brief Put a shared resource id back
 *
 * @param [in] port
 * Pointer to doca flow port the shared resource belong to
 * @param [in] type
 * Resource type.
 * @param [in] shared_resource_id
 * Shared resource id.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported shared resource type.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_shared_resource_put(struct doca_flow_port *port,
						enum doca_flow_shared_resource_type type,
						uint32_t shared_resource_id);
/**
 * @brief Extract information about shared counter
 *
 * Query an array of shared objects of a specific type.
 *
 * @param [in] port
 * Pointer to doca flow port the shared resource belong to
 * @param [in] type
 * Shared object type.
 * @param [in] res_array
 * Array of shared objects IDs to query.
 * @param [in] data_array
 * Data array retrieved by the query.
 * @param [in] array_len
 * Number of objects and their query results in their arrays (same number).
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported shared resource type.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_port_shared_resources_query(struct doca_flow_port *port,
						   enum doca_flow_shared_resource_type type,
						   uint32_t *res_array,
						   struct doca_flow_resource_query *data_array,
						   uint32_t array_len);

/**
 * @brief Create DOCA Flow pipe configuration struct.
 *
 * Create and allocate DOCA Flow pipe configuration struct and set port
 *
 * @param [out] cfg
 * DOCA Flow pipe configuration.
 * @param [in] port
 * DOCA Flow port.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_create(struct doca_flow_pipe_cfg **cfg, struct doca_flow_port *port);

/**
 * @brief Destroy DOCA Flow pipe configuration struct
 *
 * Free and destroy the DOCA Flow pipe configuration struct
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_destroy(struct doca_flow_pipe_cfg *cfg);

/**
 * @brief Set pipe's match and match mask
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] match
 * DOCA Flow match
 * @param [in] match_mask
 * DOCA Flow match mask
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_match(struct doca_flow_pipe_cfg *cfg,
					  const struct doca_flow_match *match,
					  const struct doca_flow_match *match_mask);

/**
 * @brief Set pipe's actions, actions mask and actions descriptor
 *
 * Set pipe's actions, actions mask and actions descriptor.
 * nr_actions must not be zero, and actions must not be NULL.
 * actions_masks and action_descs can be NULL, meaning not set.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] actions
 * DOCA Flow actions array
 * @param [in] actions_masks
 * DOCA Flow actions mask array
 * @param [in] action_descs
 * DOCA Flow actions descriptor array
 * @param [in] nr_actions
 * Number of actions
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_actions(struct doca_flow_pipe_cfg *cfg,
					    struct doca_flow_actions *const *actions,
					    struct doca_flow_actions *const *actions_masks,
					    struct doca_flow_action_descs *const *action_descs,
					    size_t nr_actions);

/**
 * @brief Set pipe's monitor
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] monitor
 * DOCA Flow monitor
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_monitor(struct doca_flow_pipe_cfg *cfg, const struct doca_flow_monitor *monitor);

/**
 * @brief Set pipe's ordered lists
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] ordered_lists
 * DOCA Flow ordered lists array
 * @param [in] nr_ordered_lists
 * Number of ordered lists
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_ordered_lists(struct doca_flow_pipe_cfg *cfg,
						  struct doca_flow_ordered_list *const *ordered_lists,
						  size_t nr_ordered_lists);

/**
 * @brief Set pipe's name
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] name
 * Pipe name
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_name(struct doca_flow_pipe_cfg *cfg, const char *name);

/**
 * @brief Sets the label for the given pipe
 *
 * The label is a text string that can be used to identify or describe the pipe.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] label
 * A string containing the label to be set.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_cfg_set_label(struct doca_flow_pipe_cfg *cfg, const char *label);

/**
 * @brief Set pipe's type
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] type
 * DOCA Flow pipe type
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_type(struct doca_flow_pipe_cfg *cfg, enum doca_flow_pipe_type type);

/**
 * @brief Set pipe's domain
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] domain
 * DOCA Flow pipe steering domain
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_ALREADY_EXIST - domain was already set.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_domain(struct doca_flow_pipe_cfg *cfg, enum doca_flow_pipe_domain domain);

/**
 * @brief Set if pipe is root or not
 *
 * Set if pipe is root or not. If true it means the pipe is a root pipe executed on packet arrival.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] is_root
 * If the pipe is root.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_is_root(struct doca_flow_pipe_cfg *cfg, bool is_root);

/**
 * @brief Set pipe's maximum number of flow rules.
 *
 * Set pipe's maximum number of flow rules, default is 8k if not set.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] nr_entries
 * Maximum number of flow rules
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_nr_entries(struct doca_flow_pipe_cfg *cfg, uint32_t nr_entries);

/**
 * @brief Set if the pipe supports the resize operation
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] is_resizable
 * If the pipe is resizable.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_is_resizable(struct doca_flow_pipe_cfg *cfg, bool is_resizable);

/**
 * @brief Set pipe_queue as excluded in the pipe.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] pipe_queue
 * The pipe_queue to exclude
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_cfg_set_excluded_queue(struct doca_flow_pipe_cfg *cfg, uint16_t pipe_queue);

/**
 * @brief Set to enable pipe's miss counter
 *
 * Set to enable pipe's missed flows counter, can be queried with doca_flow_resource_query_pipe_miss().
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] miss_counter
 * If to enable miss counter
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_miss_counter(struct doca_flow_pipe_cfg *cfg, bool miss_counter);

/**
 * @brief Set pipe's congestion level threshold
 *
 * Set congestion threshold for pipe in percentage (0,100] - pipe notification.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] congestion_level_threshold
 * congestion level threshold
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_congestion_level_threshold(struct doca_flow_pipe_cfg *cfg,
							       uint8_t congestion_level_threshold);

/**
 * @brief Set pipe's user context
 *
 * Set pipe's user context - pipe notification.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] user_ctx
 * User context
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_STABLE
doca_error_t doca_flow_pipe_cfg_set_user_ctx(struct doca_flow_pipe_cfg *cfg, void *user_ctx);

/**
 * @brief Set pipe map algorithm - supported only in hash pipe
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] algorithm_flags
 * Algorithms to use in the pipe. enum doca_flow_pipe_hash_map_algorithm.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_cfg_set_hash_map_algorithm(struct doca_flow_pipe_cfg *cfg, uint32_t algorithm_flags);

/**
 * @brief Set CT pipe's session counts
 *
 * Configure the number of CT sessions for IPv4, IPv6, and total.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] nb_ipv4
 * Number of IPv4 sessions.
 * @param [in] nb_ipv6
 * Number of IPv6 sessions.
 * @param [in] nb_total
 * Total number of sessions (0 = sum of IPv4 + IPv6).
 * @return
 * DOCA_SUCCESS in case of success, error number otherwise
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_cfg_set_ct_connections(struct doca_flow_pipe_cfg *cfg,
						   uint32_t nb_ipv4,
						   uint32_t nb_ipv6,
						   uint32_t nb_total);

/**
 * @brief Set maximum number of connections per zone for CT pipe.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] max_connections_per_zone
 * Max number of CT connections per zone, default 0x200000.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_cfg_set_ct_max_connections_per_zone(struct doca_flow_pipe_cfg *cfg,
								uint32_t max_connections_per_zone);

/**
 * @brief Set duplication filter size for CT pipe.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] dup_filter_sz
 * Number of connections to cache in duplication filter.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_cfg_set_ct_dup_filter_size(struct doca_flow_pipe_cfg *cfg, uint32_t dup_filter_sz);

/**
 * @brief Ordered list forwarding mode
 */
enum doca_flow_ordered_list_fwd_mode {
	DOCA_FLOW_ORDERED_LIST_FWD_MODE_INDEX,
	/**< Ordered list entry index is taken from the forwarding configuration */
	DOCA_FLOW_ORDERED_LIST_FWD_MODE_META,
	/**< Ordered list entry index is taken from the metadata */
};

/**
 * @brief Set forwarding mode for ordered list pipe.
 *
 * @param [in] cfg
 * DOCA Flow pipe configuration.
 * @param [in] fwd_mode
 * Ordered list fwd mode.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_pipe_cfg_set_ordered_list_fwd_mode(struct doca_flow_pipe_cfg *cfg,
							  enum doca_flow_ordered_list_fwd_mode fwd_mode);

// CTSD_Integration - Add compatibility header
#include "doca_ctsd_flow.h"
#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_FLOW_H_ */
