/*
 * Copyright (c) 2022-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_flow_ct.h
 * @page doca_flow_ct
 * @defgroup DOCA_FLOW_CT Doca Flow CT
 * @ingroup DOCA_FLOW
 * DOCA HW connection tracking library.
 *
 * @{
 */

#ifndef DOCA_FLOW_CT_H_
#define DOCA_FLOW_CT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <doca_compat.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_flow.h>
#include <doca_flow_net.h>
#include <doca_bitfield.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * invalid CT action handle
 */
#define DOCA_FLOW_CT_ACTION_HANDLE_INVALID (0xffffff00)

/**
 * Translate DPDK's MARK value to CT mark value
 */
#define DOCA_FLOW_CT_MARK_FROM_DPDK(_mark) (DOCA_BETOH32((_mark) + 1) >> 8)

/**
 * @brief CT flags
 */
enum doca_flow_ct_flags {
	DOCA_FLOW_CT_FLAG_STATS = 1u << 0,		 /**< Enable counter for internal pipes */
	DOCA_FLOW_CT_FLAG_WORKER_STATS = 1u << 1,	 /**< Enable worker counter dump */
	DOCA_FLOW_CT_FLAG_NO_AGING = 1u << 2,		 /**< Bypass aging scan */
	DOCA_FLOW_CT_FLAG_ASYMMETRIC_TUNNEL = 1u << 3,	 /**< Tunnel or non-tunnel in different direction */
	DOCA_FLOW_CT_FLAG_NO_COUNTER = 1u << 4,		 /**< Disable counter support */
	DOCA_FLOW_CT_FLAG_ITERATOR = 1u << 5,		 /**< Enable entries iterator */
	DOCA_FLOW_CT_FLAG_DUP_FILTER_UDP_ONLY = 1u << 6, /**< Apply duplication filter for UDP connections only */
	DOCA_FLOW_CT_FLAG_ORIGIN_WIRE = 1u << 7,	 /**< Origin traffic will be from wire */
	DOCA_FLOW_CT_FLAG_REPLY_WIRE = 1u << 8,		 /**< Reply traffic will be from wire */
};

/**
 * @brief doca flow ct meta data
 *
 * Regular DOCA Flow metadata with extra write-only mark field.
 */
struct doca_flow_ct_meta {
	struct doca_flow_meta flow; /**< Regular DOCA Flow metadata */
	doca_be32_t mark;	    /**< Mark id. */
};

/**
 * This callback is invoked as the final step in the lifecycle of a flow entry if the
 * DOCA_FLOW_CT_ENTRY_FLAGS_ENTRY_FINALIZE was configured. It occurs after the destroy callback and provides a last
 * opportunity to query the flow's final state before the SW context associated with the flow entry is completely
 * invalidated.
 */
typedef void (*doca_flow_ct_entry_finalize_cb)(struct doca_flow_pipe *pipe, void *entry, uint16_t queue, void *usr_ctx);

/**
 * Stats updates callback function to notify on counter changes
 */
typedef void (*doca_flow_ct_stats_update_cb)(struct doca_flow_pipe *pipe,
					     void *priv_data,
					     struct doca_flow_resource_query *origin_data,
					     struct doca_flow_resource_query *reply_data);

/**
 * @brief CT aging user plugin context
 */
struct doca_flow_ct_aging_ctx {
	uint32_t n_total_conns;	   /**< Total connections */
	uint32_t n_total_counters; /**< Total allocated counters */
	void *user_ctx;		   /**< User set context */
};

/**
 * @brief CT aging connection info
 */
union doca_flow_ct_aging_conn {
	uint32_t v;			    /**< Union value, changed on connection change or reused */
	struct {			    /**< Connection detail */
		uint32_t valid : 1;	    /**< Connection is valid */
		uint32_t ctr_origin : 1;    /**< Need origin direction counter */
		uint32_t ctr_reply : 1;	    /**< Need reply direction counter */
		uint32_t ctr_shared : 1;    /**< Need shared counter for both direction */
		uint32_t is_tcp : 1;	    /**< Connection is TCP, default to UDP */
		uint32_t conn_version : 11; /**< +1 on connection reuse */
		uint32_t timeout : 16;	    /**< Timeout in seconds */
	};
};

/**
 * @brief CT aging user plugin connection event
 */
struct doca_flow_ct_aging_conn_event {
	enum doca_flow_entry_op op;	    /**< Callback type: add, delete or update */
	uint32_t aging_conn_id;		    /**< aging global connection ID */
	union doca_flow_ct_aging_conn conn; /**< Connection info */
};

/**
 * @brief CT aging user plugin callbacks
 */
struct doca_flow_ct_aging_ops {
	doca_error_t (*aging_init_cb)(struct doca_flow_ct_aging_ctx *ctx);
	/**< Plugin init callback */
	void (*aging_shutdown_cb)(struct doca_flow_ct_aging_ctx *ctx);
	/**< Plugin shutdown callback */
	void (*conn_sync_cb)(struct doca_flow_ct_aging_ctx *ctx,
			     struct doca_flow_ct_aging_conn_event *conn,
			     uint32_t n);
	/**< Before timer, Connection sync callback for changed connections */
	void (*aging_timer_cb)(struct doca_flow_ct_aging_ctx *ctx, uint64_t current_time_s);
	/**< Callback to check timeout connections based on counter statistics */
};

/**
 * @brief doca flow ct global configuration
 */
struct doca_flow_ct_cfg;

/**
 * @brief Create CT configuration
 *
 * @param [out] cfg
 *   CT configuration.
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_create(struct doca_flow_ct_cfg **cfg);

/**
 * @brief Destroy CT configuration
 *
 * @param [in] cfg
 *   CT configuration.
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_destroy(struct doca_flow_ct_cfg *cfg);

/**
 * @brief Set CT flags
 *
 * @param [in] cfg
 *   CT configuration.
 * @param [in] flags
 *   CT flags. see enum doca_flow_ct_flags.
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_flags(struct doca_flow_ct_cfg *cfg, uint32_t flags);

/**
 * @brief Set number of hardware queues.
 *
 * @param [in] cfg
 *   CT configuration.
 * @param [in] n_queues
 *   Number of hardware queues.
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_queues(struct doca_flow_ct_cfg *cfg, uint32_t n_queues);

/**
 * @brief Set queue depth.
 *
 * @param [in] cfg
 *   CT configuration.
 * @param [in] queue_depth
 *   Queue depth. Default to 512
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_queue_depth(struct doca_flow_ct_cfg *cfg, uint32_t queue_depth);

/**
 * @brief Set number of control queues.
 *
 * @param [in] cfg
 *   CT configuration.
 * @param [in] n_ctrl_queues
 *   Number of control queues.
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_ctrl_queues(struct doca_flow_ct_cfg *cfg, uint32_t n_ctrl_queues);

/**
 * @brief Set memory for CT actions
 *
 * @param [in] cfg
 *   CT configuration.
 * @param [in] size
 *   Actions memory size in bytes, power of 2 recommended but not mandatory.
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_actions_mem_size(struct doca_flow_ct_cfg *cfg, uint32_t size);

/**
 * @brief Set entry private data size.
 *
 * @param [in] cfg
 *   CT configuration.
 * @param [in] entry_priv_data_size
 *   Entry private data size.
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_entry_private_data_size(struct doca_flow_ct_cfg *cfg, uint32_t entry_priv_data_size);

/**
 * @brief Set entry finalize callback to query connection final stats
 *
 * @param [in] cfg
 *   CT configuration.
 * @param [in] entry_finalize_cb
 *   Entry finalize callback.
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_entry_finalize_cb(struct doca_flow_ct_cfg *cfg,
						    doca_flow_ct_entry_finalize_cb entry_finalize_cb);

/**
 * @brief Set status update callback to notify on counter changes
 *
 * @param [in] cfg
 *   CT configuration.
 * @param [in] stats_update_cb
 *   Status update callback.
 * @return
 *   DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_status_update_cb(struct doca_flow_ct_cfg *cfg,
						   doca_flow_ct_stats_update_cb stats_update_cb);

/**
 * @brief Set core ID to run aging thread on
 *
 * @param [in] cfg
 *  CT configuration.
 * @param [in] aging_core
 *  Core to run aging thread on.
 * @return
 *  DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_aging_core(struct doca_flow_ct_cfg *cfg, uint32_t aging_core);

/**
 * @brief Set aging query delay in seconds
 *
 * @param [in] cfg
 *  CT configuration.
 * @param [in] aging_query_delay_s
 *  Aging query delay in seconds.
 * @return
 *  DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_aging_query_delay(struct doca_flow_ct_cfg *cfg, uint32_t aging_query_delay_s);

/**
 * @brief Set aging plugin callbacks
 *
 * @param [in] cfg
 *  CT configuration.
 * @param [in] aging_ops
 *  Aging plugin callbacks.
 * @return
 *  DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_aging_plugin_ops(struct doca_flow_ct_cfg *cfg,
						   struct doca_flow_ct_aging_ops *aging_ops);

/**
 * @brief Set CT configuration origin and reply direction.
 *
 * @param [in] cfg
 *  CT configuration.
 * @param [in] direction
 *  Direction of the CT configuration.
 * @param [in] match_inner
 *  Match inner 5-tuples.
 * @param [in] zone_match_mask
 *  Zone match mask.
 * @param [in] meta_modify_mask
 *  CT meta modify mask.
 * @return
 *  DOCA_SUCCESS in case of success, error number otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cfg_set_direction(struct doca_flow_ct_cfg *cfg,
					    bool direction,
					    bool match_inner,
					    struct doca_flow_meta *zone_match_mask,
					    struct doca_flow_ct_meta *meta_modify_mask);

/**
 * @brief Initialize the doca flow ct.
 *
 * This is the global initialization function for doca flow ct. It
 * initializes all resources used by doca flow.
 *
 * Must be invoked first before any other function in this API.
 * this is a one time call, used for doca flow ct initialization and
 * global configurations.
 *
 * Must be invoked after Doca Flow initialization, before port start.
 *
 * @param cfg
 *   CT configuration.
 * @return
 *   0 on success, a negative errno value otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_init(const struct doca_flow_ct_cfg *cfg);

/**
 * @brief Check if doca device supported by CT.
 *
 * @param devinfo
 *   Doca device info.
 * @return
 *   DOCA_SUCCESS - device supported by CT.
 *   Error code - in case of failure:
 *   - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support CT.
 *   - DOCA_ERROR_INVALID_VALUE - received invalid input.
 *   - DOCA_ERROR_DRIVER - failed to query capability support.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_cap_is_dev_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Destroy the doca flow ct.
 *
 * Release all the resources used by doca flow ct.
 *
 * Must be invoked before doca flow destroy.
 */
DOCA_EXPERIMENTAL
void doca_flow_ct_destroy(void);

/**
 * @brief Prepare meta with zone and default CT type.
 *
 * @param meta
 *   Doca flow meta.
 * @param zone
 *   Zone value.
 * @param is_reply
 *   Prepare reply direction zone in asymmetric mode.
 */
DOCA_EXPERIMENTAL
void doca_flow_ct_meta_prepare(struct doca_flow_meta *meta, uint32_t zone, bool is_reply);

/**
 * @brief Prepare meta as mask with zone and CT type.
 *
 * @param meta
 *   Doca flow meta.
 * @param is_reply
 *   Prepare reply direction zone in asymmetric mode.
 */
DOCA_EXPERIMENTAL
void doca_flow_ct_meta_mask_prepare(struct doca_flow_meta *meta, bool is_reply);

/**
 * @brief Set meta match zone data to doca_flow meta.
 *
 * @param meta
 *   doca_flow meta.
 * @param zone
 *   Zone value.
 * @param is_reply
 *   Set reply direction zone in asymmetric mode.
 */
DOCA_EXPERIMENTAL
void doca_flow_ct_meta_set_match_zone(struct doca_flow_meta *meta, uint32_t zone, bool is_reply);
/**
 * @brief Get zone data bit offset in meta data field.
 *
 * @param is_reply
 *   Reply direction in asymmetric mode.
 * @return
 *   Zone data bit offset.
 */
DOCA_EXPERIMENTAL
uint32_t doca_flow_ct_meta_get_zone_offset(bool is_reply);

/*********** start of aging plugin API ***************/

/**
 * @brief CT aging counter state
 */
struct doca_flow_ct_aging_counter_state {
	uint32_t ctr_id; /**< Counter global ID */
	bool inuse;	 /**< Counter in use */
};

/**
 * @brief Get counter state inside aging plugin timer callback
 *
 * @param ctx
 *   CT Aging callback context
 * @param [out] ctrs
 *   List of counter states with counter ID set
 * @param n
 *   Number of counter states to get
 * @return
 *   DOCA_SUCCESS in case of success, others on error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_aging_counter_state_get(struct doca_flow_ct_aging_ctx *ctx,
						  struct doca_flow_ct_aging_counter_state *ctrs,
						  uint32_t n);

/**
 * @brief Set counter state inside aging plugin timer callback
 *
 * @param ctx
 *   CT Aging callback context
 * @param ctrs
 *   List of counter states
 * @param n
 *   Number of counter states to set
 * @return
 *   DOCA_SUCCESS in case of success, others on error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_aging_counter_state_set(struct doca_flow_ct_aging_ctx *ctx,
						  struct doca_flow_ct_aging_counter_state *ctrs,
						  uint32_t n);

/**
 * @brief Counter statistics
 */
struct doca_flow_ct_aging_counter {
	uint32_t ctr_id;      /**< Counter ID */
	uint16_t last_hit_s;  /**< Last hit time in seconds */
	uint64_t total_bytes; /**< Total bytes the counter received */
	uint64_t total_pkts;  /**< Total packets the counter received */
};

/**
 * @brief Get counter statistics inside aging plugin timer callback
 *
 * @param ctx
 *   CT Aging callback context
 * @param [out] ctrs
 *   List of counter states with counter ID set
 * @param n
 *   Number of counter statistics to get
 * @return
 *   DOCA_SUCCESS in case of success, others on error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_aging_counter_query(struct doca_flow_ct_aging_ctx *ctx,
					      struct doca_flow_ct_aging_counter *ctrs,
					      uint32_t n);

/**
 * @brief Connection update information
 */
struct doca_flow_ct_aging_conn_update_info {
	uint32_t aging_conn_id;			 /**< Aging global connection ID*/
	union doca_flow_ct_aging_conn conn_info; /**< Connection info */
	uint32_t ctr_origin_id;			 /**< Origin direction counter ID, UINT32_MAX to disable counter */
	uint32_t ctr_reply_id;			 /**< Reply direction counter ID, UINT32_MAX to disable counter */
};

/**
 * @brief Update connection counter inside aging plugin timer callback
 *
 * @param ctx
 *   CT Aging callback context
 * @param info
 *   List of connection info
 * @param n
 *   Number of connections to update
 */
DOCA_EXPERIMENTAL
void doca_flow_ct_aging_conn_update(struct doca_flow_ct_aging_ctx *ctx,
				    struct doca_flow_ct_aging_conn_update_info *info,
				    uint32_t n);

/**
 * @brief Remove timeout connections inside aging plugin timer callback
 *
 * @param ctx
 *   CT Aging callback context
 * @param aging_conn_ids
 *   List of aging global connection IDs
 * @param n
 *   Number of connections to remove
 */
DOCA_EXPERIMENTAL
void doca_flow_ct_aging_conn_timeout(struct doca_flow_ct_aging_ctx *ctx, uint32_t *aging_conn_ids, uint32_t n);

/*********** end of aging plugin API ***************/

/*********** start of management mode API ***************/

/**
 * doca flow CT action
 */
struct doca_flow_ct_actions {
	enum doca_flow_resource_type resource_type;
	/**< shared/non-shared */
	union {
		uint32_t action_handle;
		/**< handle of a predefined shared action */
		struct {
			uint8_t action_idx;
			/**< action template index */
			struct doca_flow_ct_meta meta;
			/**< modify meta */
			struct doca_flow_header_l4_port l4_port;
			/**< UDP or TCP source and destination port */
			union {
				struct doca_flow_ct_ip4 ip4;
				/**< source and destination ipv4 addresses */
				struct doca_flow_ct_ip6 ip6;
				/**< source and destination ipv6 addresses */
			};
		} data;
	};
};

/**
 * @brief doca flow CT IPv4 match pattern
 */
struct doca_flow_ct_match4 {
	struct doca_flow_header_l4_port l4_port;
	/**< UDP or TCP source and destination port */
	doca_be32_t src_ip;
	/**< ip src address */
	doca_be32_t dst_ip;
	/**< ip dst address */
	doca_be32_t metadata;
	/**< metadata */
	uint8_t next_proto;
	/**< ip next protocol */
};

/**
 * @brief doca flow CT IPv6 match pattern
 */
struct doca_flow_ct_match6 {
	struct doca_flow_header_l4_port l4_port;
	/**< UDP or TCP source and destination port */
	doca_be32_t src_ip[4];
	/**< ip src address */
	doca_be32_t dst_ip[4];
	/**< ip dst address */
	doca_be32_t metadata;
	/**< metadata */
	uint8_t next_proto;
	/**< ip next protocol */
};

/**
 * @brief doca flow CT match pattern
 */
struct doca_flow_ct_match {
	union {
		struct doca_flow_ct_match4 ipv4; /**< IPv4 match pattern */
		struct doca_flow_ct_match6 ipv6; /**< IPv6 match pattern */
	};
};

/**
 * @brief doca flow CT entry operation flags
 */
enum doca_flow_ct_entry_flags {
	DOCA_FLOW_CT_ENTRY_FLAGS_NO_WAIT = (1 << 0),
	/**< entry will not be buffered, send to hardware immediately */
	DOCA_FLOW_CT_ENTRY_FLAGS_DIR_ORIGIN = (1 << 1),
	/**< apply to origin direction */
	DOCA_FLOW_CT_ENTRY_FLAGS_DIR_REPLY = (1 << 2),
	/**< apply to reply direction */
	DOCA_FLOW_CT_ENTRY_FLAGS_IPV6_ORIGIN = (1 << 3),
	/**< origin direction is IPv6, union in struct doca_flow_ct_match is ipv6 */
	DOCA_FLOW_CT_ENTRY_FLAGS_IPV6_REPLY = (1 << 4),
	/**< reply direction is IPv6, union in struct doca_flow_ct_match is ipv6 */
	DOCA_FLOW_CT_ENTRY_FLAGS_COUNTER_ORIGIN = (1 << 5),
	/**< Apply counter to origin direction */
	DOCA_FLOW_CT_ENTRY_FLAGS_COUNTER_REPLY = (1 << 6),
	/**< Apply counter to reply direction */
	DOCA_FLOW_CT_ENTRY_FLAGS_COUNTER_SHARED = (1 << 7),
	/**< Counter is shared for both direction */
	DOCA_FLOW_CT_ENTRY_FLAGS_ENTRY_FINALIZE = (1 << 8),
	/**< Enable finalize callback on entry removed */
	DOCA_FLOW_CT_ENTRY_FLAGS_ALLOC_ON_MISS = (1 << 9),
	/**< Allocate on entry not found */
	DOCA_FLOW_CT_ENTRY_FLAGS_DUP_FILTER_ORIGIN = 1u << 10,
	/**< Enable duplication filter on origin  */
	DOCA_FLOW_CT_ENTRY_FLAGS_DUP_FILTER_REPLY = 1u << 11,
	/**< Enable duplication filter on reply  */
	DOCA_FLOW_CT_ENTRY_FLAGS_STATS_UPDATES = 1u << 12,
	/**< Enable calls to stats_update_cb on counter changes  */
};

/**
 * @brief Process CT entries in queue.
 *
 * The application may invoke this function in order to complete
 * the flow rule offloading and to receive the flow rule operation status via callbacks.
 *
 * This function allows the application to ensure minimal room in the steering queue for pushing entries operations in
 * bulks.
 *
 * This function also processes entries counter reset and counter update if enabled.
 *
 * @param [in] port
 * Port
 * @param [in] pipe_queue
 * Queue identifier.
 * @param [in] min_room
 * Non-zero value: minimal room to ensure in queue. max_processed_entries must be set to same value or greater.
 * 0 to poll queue once, process any entries operation completion available.
 * @param [in] max_processed_entries
 * Flow CT entries number to process from hardware steering queue.
 * If it is 0, no limitation, process all entries available, max is queue depth.
 * @param [out] queue_room
 * If set, return queue room available after processing entries.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - invalid pipe state.
 * - DOCA_ERROR_DRIVER - error happened in driver.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported operation.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_entries_process(struct doca_flow_port *port,
					  uint16_t pipe_queue,
					  uint32_t min_room,
					  uint32_t max_processed_entries,
					  uint32_t *queue_room);

/**
 * @brief Lookup recent CT entry and create on miss.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] flags
 * operation flags, see doca_flow_ct_entry_flags.
 * @param [in] match_origin
 * match pattern in origin direction.
 * @param [in] hash_origin
 * 5 tuple hash of origin direction.
 * @param [in] match_reply
 * match pattern in reply direction, default to reverse of origin pattern.
 * @param [in] hash_reply
 * 5 tuple hash of reply direction.
 * @param [out] entry
 * pointer to save the new entry
 * @param [out] conn_found
 * whether the entry is found in recent list.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - invalid pipe state.
 * - DOCA_ERROR_NOT_SUPPORTED - unsupported operation.
 * - DOCA_ERROR_FULL - pipe is full.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_entry_prepare(uint16_t queue,
					struct doca_flow_pipe *pipe,
					uint32_t flags,
					struct doca_flow_ct_match *match_origin,
					uint32_t hash_origin,
					struct doca_flow_ct_match *match_reply,
					uint32_t hash_reply,
					struct doca_flow_pipe_entry **entry,
					bool *conn_found);

/**
 * @brief Free the CT entry that hasn't been added to CT pipe.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [out] entry
 * pointer to the CT entry
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - invalid pipe state.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_entry_prepare_rollback(uint16_t queue,
						 struct doca_flow_pipe *pipe,
						 struct doca_flow_pipe_entry *entry);

/**
 * @brief Add new entry to doca flow CT pipe.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] flags
 * operation flags, see doca_flow_ct_entry_flags.
 * @param [in] match_origin
 * match pattern in origin direction.
 * @param [in] match_reply
 * match pattern in reply direction, default to reverse of origin pattern.
 * @param [in] actions_origin
 * actions to set on origin direction
 * @param [in] actions_reply
 * actions to set on reply direction
 * @param [in] fwd_origin
 * forward for origin direction; Ignored for fixed pipe-level fwd
 * @param [in] fwd_reply
 * forward for reply direction; Ignored for fixed pipe-level fwd
 * @param [in] timeout_s
 * aging timeout in second, 0 to disable aging
 * @param [in] usr_ctx
 * user context data to associate to entry
 * @param [out] entry
 * pointer of the CT entry
 * @return
 * DOCA_SUCCESS - in case of success.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_add_entry(uint16_t queue,
				    struct doca_flow_pipe *pipe,
				    uint32_t flags,
				    struct doca_flow_ct_match *match_origin,
				    struct doca_flow_ct_match *match_reply,
				    const struct doca_flow_ct_actions *actions_origin,
				    const struct doca_flow_ct_actions *actions_reply,
				    const struct doca_flow_fwd *fwd_origin,
				    const struct doca_flow_fwd *fwd_reply,
				    uint32_t timeout_s,
				    void *usr_ctx,
				    struct doca_flow_pipe_entry *entry);

/**
 * @brief Add missing direction rule to CT connection.
 *
 * The direction must be specified via flags, must be empty when the connection created.
 * Must call `doca_flow_entries_process` to polling adding result.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] flags
 * operation flags, see doca_flow_ct_entry_flags.
 * @param [in] match
 * match pattern of the direction.
 * @param [in] actions
 * actions to set of the direction
 * @param [in] fwd
 * forward for the added direction; Ignored for fixed pipe-level fwd
 * @param [in] entry
 * pointer of the entry
 * @return
 * DOCA_SUCCESS - in case of success.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_entry_add_dir(uint16_t queue,
					struct doca_flow_pipe *pipe,
					uint32_t flags,
					struct doca_flow_ct_match *match,
					const struct doca_flow_ct_actions *actions,
					const struct doca_flow_fwd *fwd,
					struct doca_flow_pipe_entry *entry);

/**
 * @brief Update CT entry meta or counter.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] flags
 * operation flags, see doca_flow_ct_entry_flags.
 * @param [in] entry
 * The CT pipe entry to query.
 * @param [in] actions_origin
 * actions to set on origin direction
 * @param [in] actions_reply
 * actions ta to set on reply direction
 * @param [in] fwd_origin
 * forward for origin direction; NULL keeps the existing fwd
 * @param [in] fwd_reply
 * forward for reply direction; NULL keeps the existing fwd
 * @param [in] timeout_s
 * aging timeout in second, 0 to disable aging
 * @return
 * DOCA_SUCCESS - in case of success.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_update_entry(uint16_t queue,
				       struct doca_flow_pipe *pipe,
				       uint32_t flags,
				       struct doca_flow_pipe_entry *entry,
				       const struct doca_flow_ct_actions *actions_origin,
				       const struct doca_flow_ct_actions *actions_reply,
				       const struct doca_flow_fwd *fwd_origin,
				       const struct doca_flow_fwd *fwd_reply,
				       uint32_t timeout_s);

/**
 * @brief remove CT entry.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] flags
 * operation flags, see doca_flow_ct_entry_flags.
 * @param [in] entry
 * The CT pipe entry to query.
 * @return
 * DOCA_SUCCESS - in case of success.
 * DOCA_ERROR_INVALID_VALUE - in case of invalid input.
 * DOCA_ERROR_IN_PROGRESS - in case of connection is in progress of hardware processing.
 * DOCA_ERROR_NOT_FOUND - in case of entry not found or destroyed.
 * DOCA_ERROR_BAD_STATE - in case of invalid pipe or connection state.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_rm_entry(uint16_t queue,
				   struct doca_flow_pipe *pipe,
				   uint32_t flags,
				   struct doca_flow_pipe_entry *entry);

/**
 * @brief Get CT entry information.
 *
 * Get the match pattern, flags and hash of the valid CT entry before the entry is destroyed,
 * Can't be called in entry finalize callback since the entry is not valid anymore.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] flags
 * operation flags, see doca_flow_ct_entry_flags.
 * @param [in] entry
 * CT entry.
 * @param [out] match_origin
 * Pointer to save match pattern of origin direction
 * @param [out] match_reply
 * Pointer to save match pattern of reply direction
 * @param [out] entry_flags
 * Entry flags, see doca_flow_ct_entry_flags.
 * @param [out] hash_origin
 * Pointer to save hash of origin direction
 * @param [out] hash_reply
 * Pointer to save hash of reply direction
 * @return
 * DOCA_SUCCESS - in case of success.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_get_entry(uint16_t queue,
				    struct doca_flow_pipe *pipe,
				    uint32_t flags,
				    struct doca_flow_pipe_entry *entry,
				    struct doca_flow_ct_match *match_origin,
				    struct doca_flow_ct_match *match_reply,
				    uint64_t *entry_flags,
				    uint32_t *hash_origin,
				    uint32_t *hash_reply);

/**
 * @brief Extract information about specific entry
 *
 * Query the packet statistics about specific CT pipe entry
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] flags
 * operation flags, see doca_flow_ct_entry_flags.
 * @param [in] entry
 * The CT pipe entry to query.
 * @param [in] origin_data
 * Data of origin direction retrieved by the query.
 * @param [in] reply_data
 * Data of reply direction retrieved by the query.
 * @param [in] last_hit_s
 * Last hit time in the number of seconds since the Epoch.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_UNKNOWN - otherwise.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_query_entry(uint16_t queue,
				      struct doca_flow_pipe *pipe,
				      uint32_t flags,
				      struct doca_flow_pipe_entry *entry,
				      struct doca_flow_resource_query *origin_data,
				      struct doca_flow_resource_query *reply_data,
				      uint64_t *last_hit_s);

/**
 * Retrieves the user private data associated with a specific CT entry.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] entry
 * The CT pipe entry to query.
 * @return
 * A pointer to the private data associated with the CT entry.
 * NULL - in case of failure.
 */
DOCA_EXPERIMENTAL
void *doca_flow_ct_entry_get_priv_data(uint16_t queue, struct doca_flow_pipe *pipe, struct doca_flow_pipe_entry *entry);

/**
 * Retrieves the connection ID associated with a given CT entry.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] entry
 * CT entry.
 * @return
 * The connection ID associated with the CT entry.
 * UINT32_MAX - in case of failure.
 */
DOCA_EXPERIMENTAL
uint32_t doca_flow_ct_entry_get_conn_id(uint16_t queue,
					struct doca_flow_pipe *pipe,
					struct doca_flow_pipe_entry *entry);

/**
 * Retrieves the aging connection ID associated with a given CT entry.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] entry
 * CT entry.
 * @return
 * The aging connection ID associated with the CT entry.
 * UINT32_MAX - in case of failure.
 */
DOCA_EXPERIMENTAL
uint32_t doca_flow_ct_entry_get_aging_conn_id(uint16_t queue,
					      struct doca_flow_pipe *pipe,
					      struct doca_flow_pipe_entry *entry);
/**
 * @brief Retrieves the entry from CT pipe based on the given connection ID.
 *
 * @param [in] queue
 * queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] conn_id
 * CT connection ID inside queue.
 * @param [out] entry
 * CT entry retrieved by the query.
 * @param [out] priv_data
 * Private data associated with the CT entry.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - wrong pipe state.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_entry_get_by_id(uint16_t queue,
					  struct doca_flow_pipe *pipe,
					  uint32_t conn_id,
					  struct doca_flow_pipe_entry **entry,
					  void **priv_data);

/**
 * @brief Retrieves the entry from CT pipe based on the given aging connection ID.
 *
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] aging_conn_id
 * Aging connection ID.
 * @param [out] entry
 * CT entry retrieved by the query.
 * @param [out] priv_data
 * Private data associated with the CT entry.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_BAD_STATE - wrong pipe state.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_entry_get_by_aging_conn_id(struct doca_flow_pipe *pipe,
						     uint32_t aging_conn_id,
						     struct doca_flow_pipe_entry **entry,
						     void **priv_data);

/*********** end of management mode API ***************/

/**
 * @brief Add shared modify-action
 *
 * @param [in] ctrl_queue
 * control queue id.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] actions
 * list of actions data, each updated with action id
 * @param [in] nb_actions
 * number of actions to create
 * @param [out] actions_handles
 * list of handles allocated for the input actions
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input
 * - DOCA_ERROR_EMPTY - user actions pool is empty
 * - DOCA_ERROR_BAD_STATE - wrong number of HW responses
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_actions_add_shared(uint16_t ctrl_queue,
					     struct doca_flow_pipe *pipe,
					     const struct doca_flow_ct_actions actions[],
					     uint32_t nb_actions,
					     uint32_t actions_handles[]);

/**
 * @brief Update shared modify-action
 *
 * @param [in] ctrl_queue
 * control queue id.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] actions
 * list of actions data, each updated with action id
 * @param [in] nb_actions
 * number of actions to update
 * @param [in] actions_handles
 * list of handles used for the update actions
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure:
 * - DOCA_ERROR_NOT_SUPPORTED - number of update action is not supported
 * - DOCA_ERROR_BAD_STATE - wrong number of HW responses
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_actions_update_shared(uint16_t ctrl_queue,
						struct doca_flow_pipe *pipe,
						const struct doca_flow_ct_actions actions[],
						uint32_t nb_actions,
						uint32_t actions_handles[]);
/**
 * @brief Remove shared modify-action
 *
 * @param [in] ctrl_queue
 * control ctrl queue id.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] actions_handles
 * list of action ids
 * @param [in] nb_actions
 * number of actions to create
 * @return
 * DOCA_SUCCESS - always success
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_actions_rm_shared(uint16_t ctrl_queue,
					    struct doca_flow_pipe *pipe,
					    uint32_t actions_handles[],
					    uint32_t nb_actions);

/**
 * @brief Callback function for iterating over CT pipe entries
 *
 * This callback is used to iterate over all CT pipe entries in a given pipe.
 *
 * @param [in] pipe_queue
 * Pipe queue ID, offset from doca_flow.nb_queues.
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] entry
 * Pointer to CT pipe entry.
 * @param [in] iterate_usr_ctx
 * User context for the iteration callback.
 */
typedef void (*doca_flow_ct_pipe_iterate_cb)(uint16_t pipe_queue,
					     struct doca_flow_pipe *pipe,
					     struct doca_flow_pipe_entry *entry,
					     void *iterate_usr_ctx);

/**
 * @brief Iterate over all CT pipe entries in a given pipe
 *
 * This function trigger iteration over all CT pipe entries in a given pipe on all queues.
 * Call doca_flow_ct_entries_process which calls doca_flow_ct_pipe_iterate_cb to iterate entries incrementally.
 *
 * @param [in] pipe
 * Pointer to pipe.
 * @param [in] iterate_cb
 * Iterator callback.
 * @param [in] iterate_usr_ctx
 * User context for the iteration callback.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - wrong pipe state.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_flow_ct_pipe_iterate(struct doca_flow_pipe *pipe,
				       doca_flow_ct_pipe_iterate_cb iterate_cb,
				       void *iterate_usr_ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_FLOW_CT_H_ */
