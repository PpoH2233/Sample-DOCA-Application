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
 * @file doca_verbs.h
 * @page DOCA_VERBS
 * @defgroup DOCA_VERBS DOCA Verbs
 * DOCA Verbs library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_VERBS_H_
#define DOCA_VERBS_H_

#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 * DOCA core opaque types
 *********************************************************************************************************************/
struct doca_devinfo;
struct doca_dev;
struct doca_dev_rep;
union doca_data;
struct doca_umem;
struct doca_uar;

/*********************************************************************************************************************
 * DOCA libraries opaque types
 *********************************************************************************************************************/
struct doca_dpa;
struct doca_dpa_completion;

/*********************************************************************************************************************
 * DOCA Verbs opaque types
 *********************************************************************************************************************/
/**
 * Opaque structure representing a DOCA Verbs Context instance.
 */
struct doca_verbs_context;
/**
 * Opaque structure representing a DOCA Verbs Device Attributes instance.
 */
struct doca_verbs_device_attr;
/**
 * Opaque structure representing a DOCA Verbs PD instance.
 */
struct doca_verbs_pd;
/**
 * Opaque structure representing a DOCA Verbs QP Init Attributes instance.
 */
struct doca_verbs_qp_init_attr;
/**
 * Opaque structure representing a DOCA Verbs QP Attributes instance.
 */
struct doca_verbs_qp_attr;
/**
 * Opaque structure representing a DOCA Verbs CQ Attributes instance.
 */
struct doca_verbs_cq_attr;
/**
 * Opaque structure representing a DOCA Verbs SRQ Init Attributes
 */
struct doca_verbs_srq_init_attr;
/**
 * Opaque structure representing a DOCA Verbs ETH SQ Init Attributes
 */
struct doca_verbs_eth_sq_init_attr;
/**
 * Opaque structure representing a DOCA Verbs ETH RQ Init Attributes
 */
struct doca_verbs_eth_rq_init_attr;
/**
 * Opaque structure representing a DOCA Verbs AH attributes instance.
 */
struct doca_verbs_ah_attr;
/**
 * Opaque structure representing a DOCA Verbs Completion Channel instance.
 */
struct doca_verbs_comp_channel;
/**
 * Opaque structure representing a DOCA Verbs Completion Queue instance.
 */
struct doca_verbs_cq;
/**
 * Opaque structure representing a DOCA Verbs Queue Pair instance.
 */
struct doca_verbs_qp;
/**
 * Opaque structure representing a DOCA Verbs Shared Receive Queue instance.
 */
struct doca_verbs_srq;
/**
 * Opaque structure representing a DOCA Verbs Ethernet Send Queue instance.
 */
struct doca_verbs_eth_sq;
/**
 * Opaque structure representing a DOCA Verbs Ethernet Receive Queue instance.
 */
struct doca_verbs_eth_rq;

/**
 * @brief Type representing a doca_verbs_srq handle on the DPA.
 */
typedef uint64_t doca_dpa_dev_verbs_srq_t;

/**
 * @brief Type representing a doca_verbs_qp handle on the DPA.
 */
typedef uint64_t doca_dpa_dev_verbs_qp_t;

/**
 * @brief Type representing a doca_verbs_eth_sq handle on the DPA.
 */
typedef uint64_t doca_dpa_dev_verbs_eth_sq_t;

/**
 * @brief Type representing a doca_verbs_eth_rq handle on the DPA.
 */
typedef uint64_t doca_dpa_dev_verbs_eth_rq_t;

/**
 * Opaque structure representing a DOCA Verbs CC Group instance.
 */
struct doca_verbs_cc_group;
/**
 * Opaque structure representing a DOCA Verbs CC Group attributes.
 */
struct doca_verbs_cc_group_attr;
/**
 * Opaque structure representing a DOCA Verbs CC Group caps.
 */
struct doca_verbs_cc_group_caps;

/**
 * @brief Asynchronous event type - QP Fatal.
 */
#define DOCA_VERBS_EVENT_QP_FATAL 0x0
/**
 * @brief Asynchronous event type - QP REQ Error.
 */
#define DOCA_VERBS_EVENT_QP_REQ_ERR 0x1
/**
 * @brief Asynchronous event type - QP Access Error.
 */
#define DOCA_VERBS_EVENT_QP_ACCESS_ERR 0x2
/**
 * @brief Asynchronous event type - CQ Error.
 */
#define DOCA_VERBS_EVENT_CQ_ERR 0x3

/**
 * @brief Verbs RC QP type define.
 */
#define DOCA_VERBS_QP_TYPE_RC 0x0

/**
 * @brief Verbs UC QP type define.
 */
#define DOCA_VERBS_QP_TYPE_UC 0x1

/**
 * @brief Verbs Time Stamp Source default.
 */
#define DOCA_VERBS_TS_SOURCE_DEFAULT 0x0

/**
 * @brief Verbs Time Stamp Source free running.
 */
#define DOCA_VERBS_TS_SOURCE_FREE_RUNNING 0x1

/**
 * @brief Verbs Time Stamp Source real time.
 */
#define DOCA_VERBS_TS_SOURCE_REAL_TIME 0x2

/**
 * @brief Verbs QP state.
 */
enum doca_verbs_qp_state {
	DOCA_VERBS_QP_STATE_RST = 0x0,
	DOCA_VERBS_QP_STATE_INIT = 0x1,
	DOCA_VERBS_QP_STATE_RTR = 0x2,
	DOCA_VERBS_QP_STATE_RTS = 0x3,
	DOCA_VERBS_QP_STATE_ERR = 0x4,
};

/**
 * @brief Verbs address type.
 */
enum doca_verbs_addr_type {
	DOCA_VERBS_ADDR_TYPE_IPv4,	/**< IPv4 type */
	DOCA_VERBS_ADDR_TYPE_IPv6,	/**< IPv6 type */
	DOCA_VERBS_ADDR_TYPE_IB_GRH,	/**< IB with GRH type */
	DOCA_VERBS_ADDR_TYPE_IB_NO_GRH, /**< IB without GRH type */
};

/**
 * @brief Verbs SRQ type.
 */
enum doca_verbs_srq_type {
	DOCA_VERBS_SRQ_TYPE_LINKED_LIST,
	DOCA_VERBS_SRQ_TYPE_CONTIGUOUS,
};

/**
 * @brief DOCA Verbs QP Atomic Mode.
 */
enum doca_verbs_qp_atomic_mode {
	DOCA_VERBS_QP_ATOMIC_MODE_NONE = 0x0,
	DOCA_VERBS_QP_ATOMIC_MODE_IB_SPEC = 0x1,
	DOCA_VERBS_QP_ATOMIC_MODE_ONLY_8BYTES = 0x2,
	DOCA_VERBS_QP_ATOMIC_MODE_UP_TO_8BYTES = 0x3,
	DOCA_VERBS_QP_ATOMIC_MODE_UP_TO_16BYTES = 0x4,
	DOCA_VERBS_QP_ATOMIC_MODE_UP_TO_32BYTES = 0x5,
	DOCA_VERBS_QP_ATOMIC_MODE_UP_TO_64BYTES = 0x6,
	DOCA_VERBS_QP_ATOMIC_MODE_UP_TO_128BYTES = 0x7,
	DOCA_VERBS_QP_ATOMIC_MODE_UP_TO_256BYTES = 0x8
};

/**
 * @brief DOCA Verbs QP scatter to CQE type.
 */
enum doca_verbs_qp_scatter_to_cqe {
	DOCA_VERBS_QP_SCATTER_TO_CQE_NONE = 0x0,
	DOCA_VERBS_QP_SCATTER_TO_CQE_32B = 0x1
};

/**
 * @brief DOCA Verbs QP Send DBR Mode.
 */
enum doca_verbs_qp_send_dbr_mode {
	DOCA_VERBS_QP_SEND_DBR_MODE_DBR_VALID = 0x0,
	DOCA_VERBS_QP_SEND_DBR_MODE_NO_DBR_EXT = 0x1, /**< DBREC is not updated and DB is rung by SW. */
	DOCA_VERBS_QP_SEND_DBR_MODE_NO_DBR_INT = 0x2, /**< DBREC is not updated and DB is rung by device (DPA). */
};

/**
 * @brief Verbs QP attributes
 *
 * @details These defines can be used with doca_verbs_qp_modify() to set QP attributes.
 * These attributes are used in several QP state transition commands.
 *
 * For each command below there are optional and required attributes depending on QP type:
 * - *->rst:
 *		QP type RC:
 *			required: next_state
 *			optional: NONE
 *		QP type UC:
 *			required: next_state
 *			optional: NONE
 * - *->err:
 *		QP type RC:
 *			required: next_state
 *			optional: NONE
 *		QP type UC:
 *			required: next_state
 *			optional: NONE
 * - rst->init:
 * 		QP type RC:
 *			required: next_state, allow_remote_write, allow_remote_read, atomic_mode, pkey_index, port_num
 *			optional: NONE
 * 		QP type UC:
 * 			required: next_state, allow_remote_write, pkey_index, port_num
 * 			optional: NONE
 * - init->init:
 *		QP type RC:
 *			required: NONE
 *			optional: allow_remote_write, allow_remote_read, atomic_mode, pkey_index, port_num
 *		QP type UC:
 *			required: NONE
 *			optional: allow_remote_write, pkey_index, port_num
 * - init->rtr:
 *		QP type RC:
 *			required: next_state, rq_psn, dest_qp_num, path_mtu, ah_attr, min_rnr_timer, max_dest_rd_atomic
 *			optional: allow_remote_write, allow_remote_read, atomic_mode, pkey_index
 *		QP type UC:
 *			required: next_state, rq_psn, dest_qp_num, path_mtu, ah_attr
 *			optional: allow_remote_write, pkey_index
 * - rtr->rts:
 *		QP type RC:
 *			required: next_state, sq_psn, ack_timeout, retry_cnt, rnr_retry, max_rd_atomic
 *			optional: allow_remote_write, min_rnr_timer, atomic_mode
 *		QP type UC:
 *			required: next_state, sq_psn,
 *			optional: allow_remote_write
 * - rts->rts:
 *		QP type RC:
 *			required: NONE
 *			optional: allow_remote_write, allow_remote_read, atomic_mode, min_rnr_timer, ah_attr
 *		QP type UC:
 *			required: NONE
 *			optional: allow_remote_write, ah_attr
 *
 */
/**
 * @brief Allow Remote Write attribute.
 */
#define DOCA_VERBS_QP_ATTR_ALLOW_REMOTE_WRITE (1 << 0)
/**
 * @brief Allow Remote Read attribute.
 */
#define DOCA_VERBS_QP_ATTR_ALLOW_REMOTE_READ (1 << 1)
/**
 * @brief PKEY Index attribute.
 */
#define DOCA_VERBS_QP_ATTR_PKEY_INDEX (1 << 2)
/**
 * @brief Minimum RNR Timer attribute.
 */
#define DOCA_VERBS_QP_ATTR_MIN_RNR_TIMER (1 << 3)
/**
 * @brief Port Number attribute.
 */
#define DOCA_VERBS_QP_ATTR_PORT_NUM (1 << 4)
/**
 * @brief Next State attribute.
 */
#define DOCA_VERBS_QP_ATTR_NEXT_STATE (1 << 5)
/**
 * @brief Current State attribute.
 */
#define DOCA_VERBS_QP_ATTR_CURRENT_STATE (1 << 6)
/**
 * @brief Path MTU attribute.
 */
#define DOCA_VERBS_QP_ATTR_PATH_MTU (1 << 7)
/**
 * @brief RQ PSN attribute.
 */
#define DOCA_VERBS_QP_ATTR_RQ_PSN (1 << 8)
/**
 * @brief SQ PSN attribute.
 */
#define DOCA_VERBS_QP_ATTR_SQ_PSN (1 << 9)
/**
 * @brief Destination QP attribute.
 */
#define DOCA_VERBS_QP_ATTR_DEST_QP_NUM (1 << 10)
/**
 * @brief ACK Timeout attribute.
 */
#define DOCA_VERBS_QP_ATTR_ACK_TIMEOUT (1 << 11)
/**
 * @brief Retry Counter attribute.
 */
#define DOCA_VERBS_QP_ATTR_RETRY_CNT (1 << 12)
/**
 * @brief RNR Retry attribute.
 */
#define DOCA_VERBS_QP_ATTR_RNR_RETRY (1 << 13)
/**
 * @brief AH attribute.
 */
#define DOCA_VERBS_QP_ATTR_AH_ATTR (1 << 14)
/**
 * @brief Atomic Mode attribute.
 */
#define DOCA_VERBS_QP_ATTR_ATOMIC_MODE (1 << 15)
/**
 * @brief The maximum number of outstanding RDMA Read/Atomic requests that a single QP is allowed to initiate
 * concurrently.
 */
#define DOCA_VERBS_QP_ATTR_MAX_QP_RD_ATOMIC (1 << 16)
/**
 * @brief The maximum number of incoming RDMA Read/Atomic requests that a single QP can handle concurrently as a
 * responder.
 */
#define DOCA_VERBS_QP_ATTR_MAX_DEST_RD_ATOMIC (1 << 17)

/**
 * @brief Verbs QP MP attributes - CC Group
 *
 * @note These attributes extend the QP attributes defined in doca_verbs.h
 *
 * @details Can be used with doca_verbs_qp_modify() to set CC group attribute in init->rtr command.
 */
#define DOCA_VERBS_QP_ATTR_CC_GROUP (1 << 18)

/**
 * @brief The capability for atomic operations.
 */
enum doca_verbs_atomic_cap {
	DOCA_VERBS_ATOMIC_CAP_NONE = 0x0, /* Atomic operations are not supported at all */
	DOCA_VERBS_ATOMIC_CAP_HCA = 0x1,  /* Atomicity is guaranteed only between QPs on this device */
	DOCA_VERBS_ATOMIC_CAP_GLOB = 0x2, /* Atomicity is guaranteed between this device and any other component,
						  such as CPUs, I/O devices, and other RDMA devices */
};

/**
 * @brief Asynchronous event.
 */
struct doca_verbs_async_event {
	union {
		struct doca_verbs_qp *qp; /**< pointer to QP in case of QP related event */
		struct doca_verbs_cq *cq; /**< pointer to CQ in case of CQ related event */
		void *reserved;		  /**< reserved for doca use */
	} element;			  /**< Union to include different objects */
	int event_type;			  /**< event type. see define DOCA_VERBS_EVENT_* */
};

/**
 * @brief GID struct.
 */
struct doca_verbs_gid {
	uint8_t raw[DOCA_GID_BYTE_LENGTH]; /**< The raw value of the GID */
};

/**
 * @brief Ethernet Send Queue state.
 */
enum doca_verbs_eth_sq_state {
	DOCA_VERBS_ETH_SQ_STATE_RST = 0x0, /* Ethernet Send Queue is in reset state */
	DOCA_VERBS_ETH_SQ_STATE_RDY = 0x1, /* Ethernet Send Queue is in ready state */
	DOCA_VERBS_ETH_SQ_STATE_ERR = 0x2, /* Ethernet Send Queue is in error state */
};

/**
 * @brief Ethernet Receive Queue state.
 */
enum doca_verbs_eth_rq_state {
	DOCA_VERBS_ETH_RQ_STATE_RST = 0x0, /* Ethernet Receive Queue is in reset state */
	DOCA_VERBS_ETH_RQ_STATE_RDY = 0x1, /* Ethernet Receive Queue is in ready state */
	DOCA_VERBS_ETH_RQ_STATE_ERR = 0x2, /* Ethernet Receive Queue is in error state */
};

/**
 * @brief Verbs QP ordering semantic.
 */
enum doca_verbs_qp_ordering_semantic {
	DOCA_VERBS_QP_ORDERING_SEMANTIC_IBTA = 0x0,
	DOCA_VERBS_QP_ORDERING_SEMANTIC_OOO_RW = 0x1,
	DOCA_VERBS_QP_ORDERING_SEMANTIC_OOO_ALL = 0x2,
};

/**
 * @brief Default flags for doca_verbs_context object creation.
 */
#define DOCA_VERBS_CONTEXT_CREATE_FLAGS_NONE 0

/**
 * @brief Create a DOCA Verbs Context instance from doca_devinfo.
 *
 * @details This method shall create a new instance of doca_verbs_context (in contrast to doca_dev_open that will
 * open the same device for a doca_devinfo).
 * Follow the below procedure if you want to use a doca_dev and doca_verbs_context that share the same resources:
 * 1. Open a doca_verbs_context (using doca_verbs_context_create)
 * 2. Create doca_verbs_pd (using doca_verbs_pd_create)
 * 3. Create doca_dev from the verbs_pd (using doca_verbs_pd_as_doca_dev)
 *
 * @param [in] devinfo
 * Pointer to doca_devinfo.
 * @param [in] flags
 * Create flags. @see define DOCA_VERBS_CONTEXT_CREATE_FLAGS_*
 * @param [out] verbs_context
 * Pointer to pointer to be set to point to the created verbs_context instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_NOT_CONNECTED - failed to open doca_verbs_context for the provided devinfo
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_context_create(struct doca_devinfo *devinfo,
				       uint32_t flags,
				       struct doca_verbs_context **verbs_context);

/**
 * @brief Destroy a DOCA Verbs Context instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_context_destroy(struct doca_verbs_context *verbs_context);

/**
 * @brief Export a verbs context handle.
 *
 * @details to be used by doca_verbs_context_import_from_handle, most likely in another process.
 * If the handle is exported to another process it needs to be duplicated and SCM_RIGHTS should be set.
 *
 * @param [in] verbs_context
 * Verbs context to get the handle from
 *
 * @return context handle
 */
DOCA_EXPERIMENTAL
int doca_verbs_context_export_handle(const struct doca_verbs_context *verbs_context);

/**
 * @brief Import verbs context from an exported handle
 *
 * @see doca_verbs_context_export_handle for exporting.
 *
 * @param [in] handle
 * handle to import the verbs context from
 * @param [in] flags
 * Create flags. Expecting same flags that were used to create the original context. @see
 * DOCA_VERBS_CONTEXT_CREATE_FLAGS_*
 * @param [out] verbs_context
 * Imported verbs context
 *
 * @return
 * DOCA_SUCCESS - in case context was imported successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_DRIVER - Failed to import the verbs context.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_context_import_from_handle(int handle,
						   uint32_t flags,
						   struct doca_verbs_context **verbs_context);

/**
 * @brief Create a DOCA Verbs PD instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 * @param [out] verbs_pd
 * Pointer to pointer to be set to point to the created verbs_pd instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_pd_create(struct doca_verbs_context *verbs_context, struct doca_verbs_pd **verbs_pd);

/**
 * @brief Destroy a DOCA Verbs PD instance.
 *
 * @param [in] verbs_pd
 * Pointer to verbs_pd instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_pd_destroy(struct doca_verbs_pd *verbs_pd);

/**
 * @brief Create a doca_dev from doca_verbs_pd
 *
 * @details The doca_dev PD will be the same PD as the input verbs_pd. Every call to this function will return the
 * same doca_dev for a specific verbs_pd. Use doca_dev_close to close the doca_dev.
 *
 * @param [in] verbs_pd
 * Pointer to verbs_pd instance.
 * @param [out] dev
 * Pointer to doca_dev to create
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_pd_as_doca_dev(struct doca_verbs_pd *verbs_pd, struct doca_dev **dev);

/**
 * @brief Export a pd handle.
 *
 * @details to be used by doca_verbs_pd_import_from_handle, most likely in another process.
 *
 * @param [in] verbs_pd
 * Protection domain to get the handle from
 *
 * @return pd handle
 */
DOCA_EXPERIMENTAL
int doca_verbs_pd_export_handle(const struct doca_verbs_pd *verbs_pd);

/**
 * @brief Import pd from an exported handle
 *
 * @see doca_verbs_pd_export_handle for exporting.
 *
 * @param [in] verbs_context
 * Verbs context to import the PD.
 * @param [in] handle
 * handle to import the pd from
 * @param [out] verbs_pd
 * Imported pd
 *
 * @return
 * DOCA_SUCCESS - in case pd was imported successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_DRIVER - Failed to import the pd.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_pd_import_from_handle(struct doca_verbs_context *verbs_context,
					      int handle,
					      struct doca_verbs_pd **verbs_pd);

/**
 * @brief Create a DOCA Verbs QP Init Attributes instance.
 *
 * @param [out] verbs_qp_init_attr
 * Pointer to pointer to be set to point to the created verbs_qp_init_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_create(struct doca_verbs_qp_init_attr **verbs_qp_init_attr);

/**
 * @brief Destroy a DOCA Verbs QP Init Attributes instance.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_destroy(struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set pd attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] pd
 * pd attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_pd(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
					    struct doca_verbs_pd *pd);

/**
 * @brief Get pd attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * pd attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_pd *doca_verbs_qp_init_attr_get_pd(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set send_cq attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] send_cq
 * send_cq attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_send_cq(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						 struct doca_verbs_cq *send_cq);

/**
 * @brief Get send_cq attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * send_cq attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_cq *doca_verbs_qp_init_attr_get_send_cq(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set receive_cq attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] receive_cq
 * receive_cq attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_receive_cq(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						    struct doca_verbs_cq *receive_cq);

/**
 * @brief Get receive_cq attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * receive_cq attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_cq *doca_verbs_qp_init_attr_get_receive_cq(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set sq_sig_all attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] sq_sig_all
 * sq_sig_all attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_sq_sig_all(struct doca_verbs_qp_init_attr *verbs_qp_init_attr, int sq_sig_all);

/**
 * @brief Get sq_sig_all attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * sq_sig_all attribute.
 */
DOCA_EXPERIMENTAL
int doca_verbs_qp_init_attr_get_sq_sig_all(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set sq_wr attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] sq_wr
 * sq_wr attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_sq_wr(struct doca_verbs_qp_init_attr *verbs_qp_init_attr, uint32_t sq_wr);

/**
 * @brief Get sq_wr attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * sq_wr attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_sq_wr(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set rq_wr attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] rq_wr
 * rq_wr attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_rq_wr(struct doca_verbs_qp_init_attr *verbs_qp_init_attr, uint32_t rq_wr);

/**
 * @brief Get rq_wr attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * rq_wr attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_rq_wr(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set send_max_sges attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] send_max_sges
 * send_max_sges attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_send_max_sges(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						       uint32_t send_max_sges);

/**
 * @brief Get send_max_sges attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * send_max_sges attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_send_max_sges(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set receive_max_sges attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] receive_max_sges
 * receive_max_sges attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_receive_max_sges(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
							  uint32_t receive_max_sges);

/**
 * @brief Get receive_max_sges attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * receive_max_sges attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_receive_max_sges(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set max_inline_data attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] max_inline_data
 * max_inline_data attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_max_inline_data(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
							 uint32_t max_inline_data);

/**
 * @brief Get max_inline_data attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * max_inline_data attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_max_inline_data(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set user_index attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] user_index
 * user_index attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_user_index(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						    uint32_t user_index);

/**
 * @brief Get user_index attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * user_index attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_user_index(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set qp_type attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] qp_type
 * qp_type attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_qp_type(struct doca_verbs_qp_init_attr *verbs_qp_init_attr, uint32_t qp_type);

/**
 * @brief Get qp_type attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * qp_type attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_qp_type(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set external umem attributes for verbs_qp_init_attr.
 *
 * Setting these attributes means that the user wants to create and provide the umem by himself,
 * in compare with the default mode where the umem is created internally.
 * In that case it is the user responsibility to allocate enough memory for the umem and to free it.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] external_umem
 * External umem instance.
 * @param [in] external_umem_offset
 * The offset in the external umem buffer to set the Work Queue
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_external_umem(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						       struct doca_umem *external_umem,
						       uint64_t external_umem_offset);

/**
 * @brief Get external umem attributes from verbs_qp_init_attr.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [out] external_umem
 * External umem instance.
 * @param [out] external_umem_offset
 * The offset in the external umem buffer to set the Work Queue
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_get_external_umem(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						       struct doca_umem **external_umem,
						       uint64_t *external_umem_offset);

/**
 * @brief Set external DBR umem attributes for verbs_qp_init_attr.
 *
 * Setting these attributes means that the user wants to create and provide the dbr umem by himself,
 * in compare with the default mode where the dbr umem is created internally.
 * In that case it is the user responsibility to allocate enough memory for the umem and to free it.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] external_dbr_umem
 * External dbr umem instance.
 * @param [in] external_dbr_umem_offset
 * The offset in the external dbr umem buffer to set the DBR
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_external_dbr_umem(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
							   struct doca_umem *external_dbr_umem,
							   uint64_t external_dbr_umem_offset);

/**
 * @brief Get external DBR umem attributes from verbs_qp_init_attr.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [out] external_dbr_umem
 * External dbr umem instance.
 * @param [out] external_dbr_umem_offset
 * The offset in the external dbr umem buffer to set the DBR
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_get_external_dbr_umem(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
							   struct doca_umem **external_dbr_umem,
							   uint64_t *external_dbr_umem_offset);

/**
 * @brief Set external uar attribute for verbs_qp_init_attr.
 *
 * Setting these attribute means that the user wants to create and provide the uar by himself,
 * in compare with the default mode where the uar is created internally.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] external_uar
 * External uar instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_external_uar(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						      struct doca_uar *external_uar);

/**
 * @brief Get external uar attribute from verbs_qp_init_attr.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [out] external_uar
 * External uar instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_get_external_uar(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						      struct doca_uar **external_uar);

/**
 * @brief Set core_direct_master attribute for verbs_qp_attr.
 * @note To enable this QP attribute, external datapath must be enabled.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] core_direct_master
 * core_direct_master attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_core_direct_master(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
							    uint8_t core_direct_master);

/**
 * @brief Get core_direct_master attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * core_direct_master attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_qp_init_attr_get_core_direct_master(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set qp_context attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] qp_context
 * qp_context attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_qp_context(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						    void *qp_context);

/**
 * @brief Get qp_context attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * qp_context attribute.
 */
DOCA_EXPERIMENTAL
void *doca_verbs_qp_init_attr_get_qp_context(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set srq attribute for verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] srq
 * srq attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_srq(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
					     struct doca_verbs_srq *srq);

/**
 * @brief Get srq attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * srq attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_srq *doca_verbs_qp_init_attr_get_srq(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set DPA context for verbs_qp_init_attr.
 * This API binds the DOCA Verbs QP to a DPA device, and datapath is expected to be executed on the DPA.
 *
 * @note If used, the following is expected from the user:
 *       - Enable external datapath with doca_verbs_qp_init_attr_set_external_datapath_en()
 *       - Create an QP DPA handle with doca_verbs_qp_get_dpa_handle()
 *
 *       The following APIs must not be used if a DPA context is set:
 *       - doca_verbs_qp_init_attr_set_external_umem() / doca_verbs_qp_init_attr_set_wq_umem()
 *       - doca_verbs_qp_init_attr_set_external_dbr_umem() / doca_verbs_qp_init_attr_set_dbr_umem()
 *       - doca_verbs_qp_init_attr_set_external_uar() / doca_verbs_qp_init_attr_set_uar_id()
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to qp_init_attr instance.
 * @param [in] dpa_ctx
 * Pointer to DPA context instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_dpa(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
					     struct doca_dpa *dpa_ctx);

/**
 * @brief Get DPA context from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * Pointer to DPA context instance.
 */
DOCA_EXPERIMENTAL
struct doca_dpa *doca_verbs_qp_init_attr_get_dpa(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set send_dpa_completion attribute for verbs_qp_init_attr
 * This API is used to attach a send DPA completion to the QP.
 * This means that handling of send completion will be executed on the DPA.
 *
 * @note This API is mutually exclusive with doca_verbs_qp_init_attr_set_send_cq().
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] send_dpa_completion
 * send_dpa_completion attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_send_dpa_completion(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
							     struct doca_dpa_completion *send_dpa_completion);

/**
 * @brief Get send_dpa_completion attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * Pointer to send_dpa_completion instance.
 */
DOCA_EXPERIMENTAL
struct doca_dpa_completion *doca_verbs_qp_init_attr_get_send_dpa_completion(
	const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set receive_dpa_completion attribute for verbs_qp_init_attr
 * This API is used to attach a receive DPA completion to the QP.
 * This means that handling of receive completion will be executed on the DPA.
 *
 * @note This API is mutually exclusive with doca_verbs_qp_init_attr_set_receive_cq().
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] receive_dpa_completion
 * receive_dpa_completion attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_receive_dpa_completion(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
								struct doca_dpa_completion *receive_dpa_completion);

/**
 * @brief Get receive_dpa_completion attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * Pointer to receive_dpa_completion instance.
 */
DOCA_EXPERIMENTAL
struct doca_dpa_completion *doca_verbs_qp_init_attr_get_receive_dpa_completion(
	const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set external_datapath_en attribute for verbs_qp_init_attr
 *
 * If set then the bridge datapath APIs must not be used.
 * If not set then bridge datapath APIs can be used, and the following limitations apply:
 * - Set user_index attribute not supported
 * - Scatter to CQE not supported
 * Value should be same for QP and both send_cq, and receive_cq that are connected to it.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] external_datapath_en
 * external_datapath_en attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_external_datapath_en(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
							      uint8_t external_datapath_en);

/**
 * @brief Get external_datapath_en attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * external_datapath_en attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_qp_init_attr_get_external_datapath_en(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set DBR UMEM ID and DBR UMEM offset to verbs_qp_init_attr
 * @note This is not a mandatory setter. If called, then no internal DBR UMEM will be created.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] dbr_umem_id
 * DBR UMEM ID.
 * @param [in] dbr_umem_offset
 * DBR UMEM offset.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_dbr_umem(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						  uint32_t dbr_umem_id,
						  uint64_t dbr_umem_offset);

/**
 * @brief Get DBR UMEM ID from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * DBR UMEM ID.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_dbr_umem_id(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Get DBR UMEM offset from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * DBR UMEM offset.
 */
DOCA_EXPERIMENTAL
uint64_t doca_verbs_qp_init_attr_get_dbr_umem_offset(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set WQ UMEM ID and WQ UMEM offset to verbs_qp_init_attr
 * @note This is not a mandatory setter. If called, then no internal WQ UMEM will be created.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] wq_umem_id
 * WQ UMEM ID.
 * @param [in] wq_umem_offset
 * WQ UMEM offset.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_wq_umem(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						 uint32_t wq_umem_id,
						 uint64_t wq_umem_offset);

/**
 * @brief Get WQ UMEM ID from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * WQ UMEM ID.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_wq_umem_id(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Get WQ UMEM offset from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * WQ UMEM offset.
 */
DOCA_EXPERIMENTAL
uint64_t doca_verbs_qp_init_attr_get_wq_umem_offset(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set UAR ID to verbs_qp_init_attr
 * @note This is not a mandatory setter. If called, then no internal UAR will be created/used.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] uar_id
 * UAR ID.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_uar_id(struct doca_verbs_qp_init_attr *verbs_qp_init_attr, uint32_t uar_id);

/**
 * @brief Get UAR ID from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * UAR ID.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_uar_id(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set PDN to verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] pdn
 * PDN.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_pdn(struct doca_verbs_qp_init_attr *verbs_qp_init_attr, uint32_t pdn);

/**
 * @brief Get PDN from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * PDN.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_pdn(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Enable the scatter to CQE for responder.
 * For small message sizes, the received data will be scattered inline in the CQE and not in the
 * buffer associated with the RDMA Recv.
 * This attribute is currently supported only for the GPU data path.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] cqe_inline
 * 1 to enable, 0 to disable.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_cqe_inline(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						    enum doca_verbs_qp_scatter_to_cqe cqe_inline);

/**
 * @brief Get RDMA Recv CQE inline 32 bytes for recv from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * 1 if enabled, 0 if disabled.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_qp_init_attr_get_cqe_inline(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set Send DBR Mode for verbs_qp_init_attr.
 * This attribute is currently supported only for the GPU data path.
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] send_dbr_mode
 * Send DBR Mode attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_send_dbr_mode(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
						       enum doca_verbs_qp_send_dbr_mode send_dbr_mode);

/**
 * @brief Get Send DBR Mode attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * Send DBR Mode attribute.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_qp_send_dbr_mode doca_verbs_qp_init_attr_get_send_dbr_mode(
	const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Get send CQ number from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * send CQ number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_send_cqn(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Get receive CQ number from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * receive CQ number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_init_attr_get_receive_cqn(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set rcx_type attribute for verbs_qp_init_attr
 * @note This is an extension to doca_verbs_qp_init_attr_set_* functions that's relevant only to RCX QP
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] rcx_type
 * rcx_type attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_rcx_type(struct doca_verbs_qp_init_attr *verbs_qp_init_attr, uint8_t rcx_type);

/**
 * @brief Get rcx_type attribute from verbs_qp_init_attr
 * @note This is an extension to doca_verbs_qp_init_attr_get_* functions that's relevant only to RCX QP
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * rcx_type attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_qp_init_attr_get_rcx_type(const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Set ordering semantic attribute for verbs_qp_init_attr
 * @note Not setting ordering semantic doesn't guarantee ordering semantic didn't change (setting ECE may change it)
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [in] ordering_semantic
 * ordering semantic attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_init_attr_set_ordering_semantic(struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
							   enum doca_verbs_qp_ordering_semantic ordering_semantic);

/**
 * @brief Get ordering semantic attribute from verbs_qp_init_attr
 *
 * @param [in] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 *
 * @return
 * ordering semantic attribute.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_qp_ordering_semantic doca_verbs_qp_init_attr_get_ordering_semantic(
	const struct doca_verbs_qp_init_attr *verbs_qp_init_attr);

/**
 * @brief Create a DOCA Verbs QP Attributes instance.
 *
 * @param [out] verbs_qp_attr
 * Pointer to pointer to be set to point to the created verbs_qp_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_create(struct doca_verbs_qp_attr **verbs_qp_attr);

/**
 * @brief Destroy a DOCA Verbs QP Attributes instance.
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_destroy(struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set next_state attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] next_state
 * next_state attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_next_state(struct doca_verbs_qp_attr *verbs_qp_attr,
					       enum doca_verbs_qp_state next_state);

/**
 * @brief Get next_state attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * next_state attribute.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_qp_state doca_verbs_qp_attr_get_next_state(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set current_state attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] current_state
 * current_state attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_current_state(struct doca_verbs_qp_attr *verbs_qp_attr,
						  enum doca_verbs_qp_state current_state);

/**
 * @brief Get current_state attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * current_state attribute.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_qp_state doca_verbs_qp_attr_get_current_state(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set path_mtu attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] path_mtu
 * path_mtu attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_path_mtu(struct doca_verbs_qp_attr *verbs_qp_attr, enum doca_mtu_size path_mtu);

/**
 * @brief Get path_mtu attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * path_mtu attribute.
 */
DOCA_EXPERIMENTAL
enum doca_mtu_size doca_verbs_qp_attr_get_path_mtu(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set rq_psn attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] rq_psn
 * rq_psn attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_rq_psn(struct doca_verbs_qp_attr *verbs_qp_attr, uint32_t rq_psn);

/**
 * @brief Get rq_psn attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * rq_psn attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_attr_get_rq_psn(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set sq_psn attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] sq_psn
 * sq_psn attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_sq_psn(struct doca_verbs_qp_attr *verbs_qp_attr, uint32_t sq_psn);

/**
 * @brief Get sq_psn attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * sq_psn attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_attr_get_sq_psn(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set dest_qp_num attribute for verbs_qp_attr
 * @note The destination QP number used to establish a connection with the destination QP during the QP state
 * modification.
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] dest_qp_num
 * dest_qp_num attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_dest_qp_num(struct doca_verbs_qp_attr *verbs_qp_attr, uint32_t dest_qp_num);

/**
 * @brief Get dest_qp_num attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * dest_qp_num attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_attr_get_dest_qp_num(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set allow_remote_write attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] allow_remote_write
 * allow_remote_write attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_allow_remote_write(struct doca_verbs_qp_attr *verbs_qp_attr,
						       int allow_remote_write);

/**
 * @brief Get allow_remote_write attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * allow_remote_write attribute.
 */
DOCA_EXPERIMENTAL
int doca_verbs_qp_attr_get_allow_remote_write(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set allow_remote_read attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] allow_remote_read
 * allow_remote_read attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_allow_remote_read(struct doca_verbs_qp_attr *verbs_qp_attr, int allow_remote_read);

/**
 * @brief Get allow_remote_read attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * allow_remote_read attribute.
 */
DOCA_EXPERIMENTAL
int doca_verbs_qp_attr_get_allow_remote_read(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set atomic_mode attribute for verbs_qp_attr
 *
 * @note The doca_verbs_bridge data-path API currently does not support atomic operations.
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] atomic_mode
 * atomic_mode attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_atomic_mode(struct doca_verbs_qp_attr *verbs_qp_attr,
						enum doca_verbs_qp_atomic_mode atomic_mode);

/**
 * @brief Get atomic_mode attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * atomic_mode attribute.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_qp_atomic_mode doca_verbs_qp_attr_get_atomic_mode(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set max_rd_atomic attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] max_rd_atomic
 * max_rd_atomic attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_max_rd_atomic(struct doca_verbs_qp_attr *verbs_qp_attr, uint8_t max_rd_atomic);

/**
 * @brief Get max_rd_atomic attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * max_rd_atomic attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_qp_attr_get_max_rd_atomic(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set max_dest_rd_atomic attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] max_dest_rd_atomic
 * max_dest_rd_atomic attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_max_dest_rd_atomic(struct doca_verbs_qp_attr *verbs_qp_attr,
						       uint8_t max_dest_rd_atomic);

/**
 * @brief Get max_dest_rd_atomic attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * max_dest_rd_atomic attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_qp_attr_get_max_dest_rd_atomic(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set ah_attr attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] ah_attr
 * ah_attr attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_ah_attr(struct doca_verbs_qp_attr *verbs_qp_attr,
					    struct doca_verbs_ah_attr *ah_attr);

/**
 * @brief Get ah_attr attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * ah_attr attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_ah_attr *doca_verbs_qp_attr_get_ah_attr(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set pkey_index attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] pkey_index
 * pkey_index attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_pkey_index(struct doca_verbs_qp_attr *verbs_qp_attr, uint16_t pkey_index);

/**
 * @brief Get pkey_index attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * pkey_index attribute.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_qp_attr_get_pkey_index(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set port_num attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] port_num
 * port_num attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_port_num(struct doca_verbs_qp_attr *verbs_qp_attr, uint16_t port_num);

/**
 * @brief Get port_num attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * port_num attribute.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_qp_attr_get_port_num(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set ack_timeout attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] ack_timeout
 * ack_timeout attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_ack_timeout(struct doca_verbs_qp_attr *verbs_qp_attr, uint16_t ack_timeout);

/**
 * @brief Get ack_timeout attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * ack_timeout attribute.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_qp_attr_get_ack_timeout(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set retry_cnt attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] retry_cnt
 * retry_cnt attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_retry_cnt(struct doca_verbs_qp_attr *verbs_qp_attr, uint16_t retry_cnt);

/**
 * @brief Get retry_cnt attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * retry_cnt attribute.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_qp_attr_get_retry_cnt(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set rnr_retry attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] rnr_retry
 * rnr_retry attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_rnr_retry(struct doca_verbs_qp_attr *verbs_qp_attr, uint16_t rnr_retry);

/**
 * @brief Get rnr_retry attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * rnr_retry attribute.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_qp_attr_get_rnr_retry(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Set min_rnr_timer attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] min_rnr_timer
 * min_rnr_timer attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_min_rnr_timer(struct doca_verbs_qp_attr *verbs_qp_attr, uint16_t min_rnr_timer);

/**
 * @brief Get min_rnr_timer attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * min_rnr_timer attribute.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_qp_attr_get_min_rnr_timer(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Create a DOCA Verbs CQ Attributes instance.
 *
 * @param [out] verbs_cq_attr
 * Pointer to pointer to be set to point to the created verbs_cq_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_create(struct doca_verbs_cq_attr **verbs_cq_attr);

/**
 * @brief Destroy a DOCA Verbs CQ Attributes instance.
 *
 * @param [in] verbs_cq_attr
 * Pointer to verbs_cq_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_destroy(struct doca_verbs_cq_attr *verbs_cq_attr);

/**
 * @brief CQ entry size
 */
enum doca_verbs_cq_entry_size {
	DOCA_VERBS_CQ_ENTRY_SIZE_64 = 0,  /**< Entry size = 64 bytes. */
	DOCA_VERBS_CQ_ENTRY_SIZE_128 = 1, /**< Entry size = 128 bytes. */
};

/**
 * @brief Set entry_size attribute for doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] entry_size
 * entry size (@see doca_verbs_cq_entry_size).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_entry_size(struct doca_verbs_cq_attr *cq_attr,
					       enum doca_verbs_cq_entry_size entry_size);

/**
 * @brief Get entry_size attribute from doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 *
 * @return entry_size (@see doca_verbs_cq_entry_size).
 */
DOCA_EXPERIMENTAL
enum doca_verbs_cq_entry_size doca_verbs_cq_attr_get_entry_size(const struct doca_verbs_cq_attr *cq_attr);

/**
 * @brief Set cq_size attribute for doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] cq_size
 * cq size (num entries).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_cq_size(struct doca_verbs_cq_attr *cq_attr, uint32_t cq_size);

/**
 * @brief Get cq_size attribute from doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 *
 * @return cq size (num entries).
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_cq_attr_get_cq_size(const struct doca_verbs_cq_attr *cq_attr);

/**
 * @brief Enable cq_overrun attribute for doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] overrun
 * Set to 1 to enable or 0 to disable overrun.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_cq_overrun(struct doca_verbs_cq_attr *cq_attr, uint8_t overrun);

/**
 * @brief Get cq_overrun attribute from doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 *
 * @return overrun (1 overrun enabled, overrun disabled).
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_cq_attr_get_cq_overrun(const struct doca_verbs_cq_attr *cq_attr);

/**
 * @brief Enable cq_collapsed attribute for doca_verbs_cq_attr.
 * This attribute is currently supported only for the GPU data path.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] collapsed
 * enable or disable collapsed cq
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_cq_collapsed(struct doca_verbs_cq_attr *cq_attr, uint8_t collapsed);

/**
 * @brief Get cq_collapsed attribute from doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 *
 * @return collapsed (1 collapsed enabled, collapsed disabled).
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_cq_attr_get_cq_collapsed(const struct doca_verbs_cq_attr *cq_attr);

/**
 * @brief Set comp_channel attribute for doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] comp_channel
 * Pointer to completion channel to bind the CQ to. comp_channel may be null in case the application regrets setting a
 * completion channel.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_comp_channel(struct doca_verbs_cq_attr *cq_attr,
						 struct doca_verbs_comp_channel *comp_channel);

/**
 * @brief Get comp_channel attribute from doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 *
 * @return Pointer to completion channel binded to the CQ (may be null).
 */
DOCA_EXPERIMENTAL
struct doca_verbs_comp_channel *doca_verbs_cq_attr_get_comp_channel(const struct doca_verbs_cq_attr *cq_attr);

/**
 * @brief Set cq_context attribute for doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] cq_context
 * User data. cq_context may be null in case the application regrets setting a user data.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_cq_context(struct doca_verbs_cq_attr *cq_attr, void *cq_context);

/**
 * @brief Get cq_context attribute from doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 *
 * @return User data, may be null.
 */
DOCA_EXPERIMENTAL
void *doca_verbs_cq_attr_get_cq_context(const struct doca_verbs_cq_attr *cq_attr);

/**
 * @brief Set external umem attribute for doca_verbs_cq_attr.
 *
 * Setting this attribute means that the user wants to create and provide the umem by himself,
 * in compare with the default mode where the umem is created internally.
 * In that case it is the user responsibility to allocate enough memory for the umem and to free it.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] external_umem
 * External umem instance.
 * @param [in] external_umem_offset
 * The offset in the external umem buffer to set the Completion Queue.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_external_umem(struct doca_verbs_cq_attr *cq_attr,
						  struct doca_umem *external_umem,
						  uint64_t external_umem_offset);

/**
 * @brief Get external umem attribute from doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [out] external_umem
 * External umem instance.
 * @param [out] external_umem_offset
 * The offset in the external umem buffer to set the Completion Queue.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_get_external_umem(const struct doca_verbs_cq_attr *cq_attr,
						  struct doca_umem **external_umem,
						  uint64_t *external_umem_offset);

/**
 * @brief Set external uar attribute for doca_verbs_cq_attr.
 *
 * Setting this attribute means that the user wants to provide an external uar by himself,
 * in compare with the default mode where uar is created internally.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] external_uar
 * External uar.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_external_uar(struct doca_verbs_cq_attr *cq_attr, struct doca_uar *external_uar);

/**
 * @brief Get external uar attribute from doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 *
 * @return external_uar
 */
DOCA_EXPERIMENTAL
struct doca_uar *doca_verbs_cq_attr_get_external_uar(const struct doca_verbs_cq_attr *cq_attr);

/**
 * @brief Set DPA thread id attribute for doca_verbs_cq_attr.
 *
 * This attribute is used when user wants to attach DOCA Verbs CQ to a DPA thread.
 * 'dpa_thread_id' can be retrieved using doca_dpa_thread_get_id() API.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] dpa_thread_id
 * DPA thread id.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_dpa_thread_id(struct doca_verbs_cq_attr *cq_attr, uint32_t dpa_thread_id);

/**
 * @brief Get DPA thread id attribute from doca_verbs_cq_attr.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 *
 * @return DPA thread id.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_cq_attr_get_dpa_thread_id(const struct doca_verbs_cq_attr *cq_attr);

/**
 * @brief Create a DOCA Verbs AH attributes instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 * @param [out] verbs_ah_attr
 * Pointer to pointer to be set to point to the created verbs_ah_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_create(struct doca_verbs_context *verbs_context,
				       struct doca_verbs_ah_attr **verbs_ah_attr);

/**
 * @brief Destroy a DOCA Verbs AH attributes instance.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_destroy(struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Set gid attribute for verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 * @param [in] gid
 * gid attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_set_gid(struct doca_verbs_ah_attr *verbs_ah_attr, struct doca_verbs_gid gid);

/**
 * @brief Get gid attribute from verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * gid attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_gid doca_verbs_ah_attr_get_gid(const struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Set addr_type attribute for verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 * @param [in] addr_type
 * addr_type attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_set_addr_type(struct doca_verbs_ah_attr *verbs_ah_attr,
					      enum doca_verbs_addr_type addr_type);

/**
 * @brief Get addr_type attribute from verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * addr_type attribute.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_addr_type doca_verbs_ah_attr_get_addr_type(const struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Set dlid attribute for verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 * @param [in] dlid
 * dlid attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_set_dlid(struct doca_verbs_ah_attr *verbs_ah_attr, uint32_t dlid);

/**
 * @brief Get dlid attribute from verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * dlid attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_ah_attr_get_dlid(const struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Set sl attribute for verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 * @param [in] sl
 * sl attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_set_sl(struct doca_verbs_ah_attr *verbs_ah_attr, uint8_t sl);

/**
 * @brief Get sl attribute from verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * sl attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_ah_attr_get_sl(const struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Set sgid_index attribute for verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 * @param [in] sgid_index
 * sgid_index attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_set_sgid_index(struct doca_verbs_ah_attr *verbs_ah_attr, uint8_t sgid_index);

/**
 * @brief Get sgid_index attribute from verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * sgid_index attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_ah_attr_get_sgid_index(const struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Set static_rate attribute for verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 * @param [in] static_rate
 * static_rate attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_set_static_rate(struct doca_verbs_ah_attr *verbs_ah_attr, uint8_t static_rate);

/**
 * @brief Get static_rate attribute from verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * static_rate attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_ah_attr_get_static_rate(const struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Set hop_limit attribute for verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 * @param [in] hop_limit
 * hop_limit attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_set_hop_limit(struct doca_verbs_ah_attr *verbs_ah_attr, uint8_t hop_limit);

/**
 * @brief Get hop_limit attribute from verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * hop_limit attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_ah_attr_get_hop_limit(const struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Set traffic_class attribute for verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 * @param [in] traffic_class
 * traffic_class attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_set_traffic_class(struct doca_verbs_ah_attr *verbs_ah_attr, uint8_t traffic_class);

/**
 * @brief Get traffic_class attribute from verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * traffic_class attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_ah_attr_get_traffic_class(const struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Set udp_sport attribute for verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 * @param [in] udp_sport
 * udp_sport attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_ah_attr_set_udp_sport(struct doca_verbs_ah_attr *verbs_ah_attr, uint16_t udp_sport);

/**
 * @brief Get udp_sport attribute from verbs_ah_attr.
 *
 * @param [in] verbs_ah_attr
 * Pointer to verbs_ah_attr instance.
 *
 * @return
 * udp_sport attribute.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_ah_attr_get_udp_sport(const struct doca_verbs_ah_attr *verbs_ah_attr);

/**
 * @brief Create a DOCA Verbs Completion Channel instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 * @param [out] verbs_comp_channel
 * Pointer to pointer to be set to point to the created verbs_comp_channel instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_comp_channel_create(struct doca_verbs_context *verbs_context,
					    struct doca_verbs_comp_channel **verbs_comp_channel);

/**
 * @brief Destroy a DOCA Verbs Completion Channel instance.
 *
 * @param [in] verbs_comp_channel
 * Pointer to verbs_comp_channel instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_comp_channel_destroy(struct doca_verbs_comp_channel *verbs_comp_channel);

/**
 * @brief Get DOCA verbs completion channel handle. The handle can be used to wait for completion event (e.g. using
 * epoll)
 *
 * @param [in] verbs_comp_channel
 * Pointer to verbs_comp_channel instance.
 *
 * @return
 * completion channel handle.
 */
DOCA_EXPERIMENTAL
doca_event_handle_t doca_verbs_comp_channel_get_handle(struct doca_verbs_comp_channel *verbs_comp_channel);

/**
 * @brief Create a DOCA Verbs Completion Queue instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 * @param [in] verbs_cq_attr
 * Pointer to verbs_cq_attr instance.
 * @param [out] verbs_cq
 * Pointer to pointer to be set to point to the created verbs_cq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_create(struct doca_verbs_context *verbs_context,
				  struct doca_verbs_cq_attr *verbs_cq_attr,
				  struct doca_verbs_cq **verbs_cq);

/**
 * @brief Destroy a DOCA Verbs Completion Queue instance.
 *
 * @param [in] verbs_cq
 * Pointer to verbs_cq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_destroy(struct doca_verbs_cq *verbs_cq);

/**
 * @brief Create a DOCA Verbs Queue Pair instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 * @param [in] verbs_qp_init_attr
 * Pointer to qp_init_attr instance.
 * @param [out] verbs_qp
 * Pointer to pointer to be set to point to the created verbs_qp instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_create(struct doca_verbs_context *verbs_context,
				  struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
				  struct doca_verbs_qp **verbs_qp);

/**
 * @brief Modify a DOCA Verbs Queue Pair instance.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] attr_mask
 * Mask for QP attributes. see define for DOCA_VERBS_QP_ATTR_*
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_modify(struct doca_verbs_qp *verbs_qp,
				  struct doca_verbs_qp_attr *verbs_qp_attr,
				  int attr_mask);

/**
 * @brief Query the attributes of a DOCA Verbs Queue Pair instance.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 * @param [out] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [out] verbs_qp_init_attr
 * Pointer to verbs_qp_init_attr instance.
 * @param [out] attr_mask
 * If not NULL, mask for QP attributes that were queried. see define for DOCA_VERBS_QP_ATTR_*
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_query(struct doca_verbs_qp *verbs_qp,
				 struct doca_verbs_qp_attr *verbs_qp_attr,
				 struct doca_verbs_qp_init_attr *verbs_qp_init_attr,
				 int *attr_mask);

/**
 * @brief Get current ECE (enhanced connection establishment) of a DOCA Verbs Queue Pair instance.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 *
 * @return
 * The ECE value.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_query_ece(struct doca_verbs_qp *verbs_qp);

/**
 * @brief Set ECE (enhanced connection establishment) of a DOCA Verbs Queue Pair instance to use for next
 * configuration stage.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 * @param [in] ece
 * ECE value to use for next QP modify.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - ECE is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_set_ece(struct doca_verbs_qp *verbs_qp, uint32_t ece);

/**
 * @brief Destroy a DOCA Verbs Queue Pair instance.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_destroy(struct doca_verbs_qp *verbs_qp);

/**
 * @brief Get the next event from a DOCA Verbs Completion Channel.
 *
 * @param [in] verbs_comp_channel
 * Pointer to verbs_comp_channel instance to get the next event from.
 * @param [out] verbs_cq
 * Pointer to the verbs_cq instance associated with the event.
 * @param [out] cq_context
 * Pointer to the user-data associated with the event.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_get_cq_event(struct doca_verbs_comp_channel *verbs_comp_channel,
				     struct doca_verbs_cq **verbs_cq,
				     void **cq_context);

/**
 * @brief Get the Work Queue attributes of a DOCA Verbs Queue Pair instance.
 * @note In case of usage of doca_verbs_qp_init_attr_set_wq_umem, then the values of sq_buf/rq_buf are not valid.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 * @param [out] sq_buf
 * Pointer to Send Queue buffer.
 * @param [out] sq_num_entries
 * The number of entries in Send Queue buffer.
 * @param [out] rq_buf
 * Pointer to Receive Queue buffer.
 * @param [out] rq_num_entries
 * The number of entries in Receive Queue buffer.
 * @param [out] rwqe_size_bytes
 * Receive WQE size in bytes.
 *
 */
DOCA_EXPERIMENTAL
void doca_verbs_qp_get_wq(const struct doca_verbs_qp *verbs_qp,
			  void **sq_buf,
			  uint32_t *sq_num_entries,
			  void **rq_buf,
			  uint32_t *rq_num_entries,
			  uint32_t *rwqe_size_bytes);

/**
 * @brief Get the UAR address of a DOCA Verbs Queue Pair instance.
 * @note In case of usage of doca_verbs_qp_init_attr_set_uar_id, then this function will return an invalid address.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 *
 * @return
 * The UAR register address.
 */
DOCA_EXPERIMENTAL
void *doca_verbs_qp_get_uar_addr(const struct doca_verbs_qp *verbs_qp);

/**
 * @brief Get the DBR address of a DOCA Verbs Queue Pair instance.
 * @note In case of usage of doca_verbs_qp_init_attr_set_dbr_umem, then this function will return an invalid address.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 *
 * @return
 * The DBR address.
 */
DOCA_EXPERIMENTAL
void *doca_verbs_qp_get_dbr_addr(const struct doca_verbs_qp *verbs_qp);

/**
 * @brief Get the QP number of a DOCA Verbs Queue Pair instance.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 *
 * @return
 * The QP number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_qp_get_qpn(const struct doca_verbs_qp *verbs_qp);

/**
 * @brief Get a DPA handle for a DOCA Verbs Queue Pair instance.
 * The handle can be used to post send and receive work requests in DPA kernel for this QP.
 *
 * @note To use this API, external datapath must be enabled.
 *
 * @param [in] verbs_qp
 * Pointer to verbs_qp instance.
 * @param [in] dpa_ctx
 * Pointer to DPA context instance.
 * @param [out] dpa_qp_handle
 * Pointer to DPA QP handle to be set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_get_dpa_handle(struct doca_verbs_qp *verbs_qp,
					  struct doca_dpa *dpa_ctx,
					  doca_dpa_dev_verbs_qp_t *dpa_qp_handle);

/**
 * @brief Get the CQ number of a DOCA Verbs CQ instance.
 *
 * @param [in] verbs_cq
 * Pointer to verbs_cq instance.
 *
 * @return
 * The CQ number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_cq_get_cqn(const struct doca_verbs_cq *verbs_cq);

/**
 * @brief Get the Completion Queue attributes of a DOCA Verbs Completion Queue instance.
 *
 * @param [in] verbs_cq
 * Pointer to verbs_cq instance.
 * @param [out] cq_buf
 * Pointer to Completion Queue buffer.
 * @param [out] cq_num_entries
 * The number of entries in Completion Queue buffer.
 * @param [out] cq_entry_size
 * The size of each entry in Completion Queue buffer.
 *
 */
DOCA_EXPERIMENTAL
void doca_verbs_cq_get_wq(struct doca_verbs_cq *verbs_cq,
			  void **cq_buf,
			  uint32_t *cq_num_entries,
			  uint8_t *cq_entry_size);

/**
 * @brief Get the DBR address of a DOCA Verbs Completion Queue instance.
 *
 * @param [in] verbs_cq
 * Pointer to verbs_cq instance.
 * @param [out] uar_db_reg
 * Pointer to the UAR doorbell record
 * @param [out] ci_dbr
 * Pointer to the CI doorbell record
 * @param [out] arm_dbr
 * Pointer to the arm doorbell record
 */
DOCA_EXPERIMENTAL
void doca_verbs_cq_get_dbr_addr(struct doca_verbs_cq *verbs_cq,
				uint64_t **uar_db_reg,
				uint32_t **ci_dbr,
				uint32_t **arm_dbr);

/**
 * @brief Create a DOCA Verbs SRQ Init Attributes instance.
 *
 * @param [out] verbs_srq_init_attr
 * Pointer to pointer to be set to point to the created verbs_srq_init_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_create(struct doca_verbs_srq_init_attr **verbs_srq_init_attr);

/**
 * @brief Destroy a DOCA Verbs SRQ Init Attributes instance.
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_destroy(struct doca_verbs_srq_init_attr *verbs_srq_init_attr);

/**
 * @brief Set srq_wr attribute for verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 * @param [in] srq_wr
 * srq_wr attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_set_srq_wr(struct doca_verbs_srq_init_attr *verbs_srq_init_attr, uint32_t srq_wr);

/**
 * @brief Get srq_wr attribute from verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 *
 * @return
 * srq_wr attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_srq_init_attr_get_srq_wr(const struct doca_verbs_srq_init_attr *verbs_srq_init_attr);
/**
 * @brief Set receive_max_sges attribute for verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 * @param [in] receive_max_sges
 * receive_max_sges attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_set_receive_max_sges(struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
							   uint32_t receive_max_sges);

/**
 * @brief Get receive_max_sges attribute from verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 *
 * @return
 * receive_max_sges attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_srq_init_attr_get_receive_max_sges(const struct doca_verbs_srq_init_attr *verbs_srq_init_attr);

/**
 * @brief Set srq_type attribute for verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 * @param [in] srq_type
 * srq_type attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_set_type(struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
					       enum doca_verbs_srq_type srq_type);

/**
 * @brief Get srq_type attribute from verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 *
 * @return
 * srq_type attribute.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_srq_type doca_verbs_srq_init_attr_get_type(const struct doca_verbs_srq_init_attr *verbs_srq_init_attr);

/**
 * @brief Set pd attribute for verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 * @param [in] pd
 * pd attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_set_pd(struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
					     struct doca_verbs_pd *pd);

/**
 * @brief Get pd attribute from verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 *
 * @return
 * pd attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_pd *doca_verbs_srq_init_attr_get_pd(const struct doca_verbs_srq_init_attr *verbs_srq_init_attr);

/**
 * @brief Set external_datapath_en attribute for verbs_srq_init_attr
 *
 * If set then the bridge datapath APIs must not be used.
 * If not set then bridge datapath APIs can be used.
 *
 * @note Value should be same for QP this SRQ is attached to.
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 * @param [in] external_datapath_en
 * external_datapath_en attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_set_external_datapath_en(struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
							       uint8_t external_datapath_en);

/**
 * @brief Get external_datapath_en attribute from verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 *
 * @return
 * external_datapath_en attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_srq_init_attr_get_external_datapath_en(const struct doca_verbs_srq_init_attr *verbs_srq_init_attr);

/**
 * @brief Set external umem attributes for verbs_srq_init_attr.
 *
 * Setting these attributes means that the user wants to create and provide the umem by himself,
 * in compare with the default mode where the umem is created internally.
 * In that case it is the user responsibility to allocate enough memory for the umem and to free it.
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 * @param [in] external_umem
 * External umem instance.
 * @param [in] external_umem_offset
 * The offset in the external umem buffer to set the Work Queue
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_set_external_umem(struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
							struct doca_umem *external_umem,
							uint64_t external_umem_offset);

/**
 * @brief Get external umem attributes from verbs_srq_init_attr.
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 * @param [out] external_umem
 * External umem instance.
 * @param [out] external_umem_offset
 * The offset in the external umem buffer to set the Work Queue
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_get_external_umem(const struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
							struct doca_umem **external_umem,
							uint64_t *external_umem_offset);

/**
 * @brief Set external DBR umem attributes for verbs_srq_init_attr.
 *
 * Setting these attributes means that the user wants to create and provide the dbr umem by himself,
 * in compare with the default mode where the dbr umem is created internally.
 * In that case it is the user responsibility to allocate enough memory for the umem and to free it.
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 * @param [in] external_dbr_umem
 * External dbr umem instance.
 * @param [in] external_dbr_umem_offset
 * The offset in the external dbr umem buffer to set the DBR
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_set_external_dbr_umem(struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
							    struct doca_umem *external_dbr_umem,
							    uint64_t external_dbr_umem_offset);

/**
 * @brief Get external DBR umem attributes from verbs_srq_init_attr.
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 * @param [out] external_dbr_umem
 * External dbr umem instance.
 * @param [out] external_dbr_umem_offset
 * The offset in the external dbr umem buffer to set the DBR
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_get_external_dbr_umem(const struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
							    struct doca_umem **external_dbr_umem,
							    uint64_t *external_dbr_umem_offset);

/**
 * @brief Set DPA context for verbs_srq_init_attr.
 * This API binds the DOCA Verbs SRQ to a DPA device, and datapath is expected to be executed on the DPA.
 *
 * @note If used, the following is expected from the user:
 *       - Enable external datapath with doca_verbs_srq_init_attr_set_external_datapath_en()
 *       - Create an SRQ DPA handle with doca_verbs_srq_get_dpa_handle()
 *
 *       The following APIs must not be used if a DPA context is set:
 *       - doca_verbs_srq_init_attr_set_external_umem()
 *       - doca_verbs_srq_init_attr_set_external_dbr_umem()
 *       - doca_verbs_srq_init_attr_set_pd()
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to srq_init_attr instance.
 * @param [in] dpa_ctx
 * Pointer to DPA context instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_init_attr_set_dpa(struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
					      struct doca_dpa *dpa_ctx);

/**
 * @brief Get DPA context from verbs_srq_init_attr
 *
 * @param [in] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 *
 * @return
 * Pointer to DPA context instance.
 */
DOCA_EXPERIMENTAL
struct doca_dpa *doca_verbs_srq_init_attr_get_dpa(const struct doca_verbs_srq_init_attr *verbs_srq_init_attr);

/**
 * @brief Create a DOCA Verbs Shared Receive Queue instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 * @param [in] verbs_srq_init_attr
 * Pointer to srq_init_attr instance.
 * @param [out] verbs_srq
 * Pointer to pointer to be set to point to the created verbs_srq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_create(struct doca_verbs_context *verbs_context,
				   struct doca_verbs_srq_init_attr *verbs_srq_init_attr,
				   struct doca_verbs_srq **verbs_srq);

/**
 * @brief Destroy a DOCA RDMA Shared Receive Queue instance.
 *
 * @param [in] verbs_srq
 * Pointer to verbs_srq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_destroy(struct doca_verbs_srq *verbs_srq);

/**
 * @brief Query the attributes of a DOCA Verbs Shared Receive Queue instance.
 *
 * @param [in] verbs_srq
 * Pointer to verbs_srq instance.
 * @param [out] verbs_srq_init_attr
 * Pointer to verbs_srq_init_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_query(struct doca_verbs_srq *verbs_srq,
				  struct doca_verbs_srq_init_attr *verbs_srq_init_attr);

/**
 * @brief Get the SRQ number of a DOCA Verbs Shared Receive Queue instance.
 *
 * @param [in] verbs_srq
 * Pointer to verbs_srq instance.
 *
 * @return
 * The SRQ number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_srq_get_srqn(const struct doca_verbs_srq *verbs_srq);

/**
 * @brief Get the Work Queue attributes of a DOCA Verbs Shared Receive Queue instance.
 *
 * @param [in] verbs_srq
 * Pointer to verbs_srq instance.
 * @param [out] srq_buf
 * Pointer to Shared Receive Queue buffer.
 * @param [out] srq_num_entries
 * The number of entries in Shared Receive Queue buffer.
 * @param [out] rwqe_size_bytes
 * Receive WQE size in bytes.
 *
 */
DOCA_EXPERIMENTAL
void doca_verbs_srq_get_wq(const struct doca_verbs_srq *verbs_srq,
			   void **srq_buf,
			   uint32_t *srq_num_entries,
			   uint32_t *rwqe_size_bytes);

/**
 * @brief Get the DBR address of a DOCA Verbs Shared Receive Queue instance.
 *
 * @param [in] verbs_srq
 * Pointer to verbs_srq instance.
 *
 * @return
 * The DBR address.
 */
DOCA_EXPERIMENTAL
void *doca_verbs_srq_get_dbr_addr(const struct doca_verbs_srq *verbs_srq);

/**
 * @brief Get a DPA handle for a DOCA Verbs Shared Receive Queue instance.
 *
 * @note To use this API, external datapath must be enabled.
 *
 * @param [in] verbs_srq
 * Pointer to verbs_srq instance.
 * @param [in] dpa_ctx
 * Pointer to DPA context instance.
 * @param [out] dpa_srq_handle
 * Pointer to DPA SRQ handle to be set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_srq_get_dpa_handle(struct doca_verbs_srq *verbs_srq,
					   struct doca_dpa *dpa_ctx,
					   doca_dpa_dev_verbs_srq_t *dpa_srq_handle);

/**
 * @brief Get DOCA verbs async event handle. This handle can be used to wait for async event (e.g. using
 * epoll)
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 *
 * @return
 * async event handle.
 */
DOCA_EXPERIMENTAL
doca_event_handle_t doca_verbs_get_async_event_handle(struct doca_verbs_context *verbs_context);

/**
 * @brief Waits for the next async event of the context and returns it through the pointer async_event.
 *
 * - All async events that are retrieved must be acknowledged using "doca_verbs_ack_async_event()".
 * - This is a blocking function. If multiple threads call this function simultaneously, then when an async
 *   event occurs, only one thread will receive it, and it is not possible to predict which thread will receive it.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 * @param [in] async_event
 * Pointer to async event to retrieve.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - low level layer failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_get_async_event(struct doca_verbs_context *verbs_context,
					struct doca_verbs_async_event *async_event);

/**
 * @brief Acknowledges async event retrieved using "doca_verbs_get_async_event()".
 *
 * - This functions is thread-safe.
 *
 * @param [in] async_event
 * Pointer to async event to acknowledge.
 */
DOCA_EXPERIMENTAL
void doca_verbs_ack_async_event(struct doca_verbs_async_event *async_event);

/**
 * @brief Set external_datapath_en attribute for doca_verbs_cq_attr
 *
 * If set then the bridge datapath APIs must not be used.
 * If not set then bridge datapath APIs can be used, and the following limitations apply:
 * - Set CQ entry_size different than 64B is not supported.
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 * @param [in] external_datapath_en
 * external_datapath_en attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cq_attr_set_external_datapath_en(struct doca_verbs_cq_attr *cq_attr,
							 uint8_t external_datapath_en);

/**
 * @brief Get external_datapath_en attribute from doca_verbs_cq_attr
 *
 * @param [in] cq_attr
 * Pointer to doca_verbs_cq_attr instance.
 *
 * @return 1 if external data path is enabled, 0 if not.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_cq_attr_get_external_datapath_en(const struct doca_verbs_cq_attr *cq_attr);

/**
 * Request notification for upcoming CQ completions
 *
 * @param [in] verbs_cq
 * Pointer to the verbs_cq instance.
 * @param [in] solicited_only
 * Whether to receive notification for solicited completion events only.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_req_notify_cq(struct doca_verbs_cq *verbs_cq, int solicited_only);

/**
 * Acknowledge completion events
 *
 * Every event received from doca_verbs_get_cq_event() must be acknowledged using this API.
 * To prevent races, the CQ destroy will wait until all events are acknowledged.
 *
 * @param [in] verbs_cq
 * Pointer to the verbs_cq instance.
 * @param [in] nevents
 * The number of events to acknowledge.
 */
DOCA_EXPERIMENTAL
void doca_verbs_ack_cq_events(struct doca_verbs_cq *verbs_cq, unsigned int nevents);

/**
 * @brief Set CC group attribute for verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 * @param [in] cc_group
 * cc_group attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_qp_attr_set_cc_group(struct doca_verbs_qp_attr *verbs_qp_attr,
					     struct doca_verbs_cc_group *cc_group);

/**
 * @brief Get CC group attribute from verbs_qp_attr
 *
 * @param [in] verbs_qp_attr
 * Pointer to verbs_qp_attr instance.
 *
 * @return
 * cc_group attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_cc_group *doca_verbs_qp_attr_get_cc_group(const struct doca_verbs_qp_attr *verbs_qp_attr);

/**
 * @brief Create a DOCA Verbs CC group Attributes instance.
 *
 * @param [out] verbs_cc_group_attr
 * Pointer to pointer to be set to point to the created verbs_cc_group_attr instance.
 * User is expected to free this object with "doca_verbs_cc_group_attr_destroy()".
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_attr_create(struct doca_verbs_cc_group_attr **verbs_cc_group_attr);

/**
 * @brief Destroy a DOCA Verbs CC group Attributes instance.
 *
 * @param [in] verbs_cc_group_attr
 * Pointer to verbs_cc_group_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_attr_destroy(struct doca_verbs_cc_group_attr *verbs_cc_group_attr);

/**
 * @brief Set hint attribute for doca_verbs_cc_group_attr.
 *
 * @param [in] verbs_cc_group_attr
 * Pointer to verbs_cc_group_attr instance.
 * @param [in] hint_data
 * Pointer to hint data.
 * @param [in] hint_data_size
 * Size of hint data.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_attr_set_hint(struct doca_verbs_cc_group_attr *verbs_cc_group_attr,
					       const void *hint_data,
					       size_t hint_data_size);

/**
 * @brief Get hint attribute from doca_verbs_cc_group_attr
 *
 * @param [in] verbs_cc_group_attr
 * Pointer to verbs_cc_group_attr instance.
 * @param [out] data
 * Pointer to hint data.
 * @param [out] size
 * Pointer to hint size
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_attr_get_hint(const struct doca_verbs_cc_group_attr *verbs_cc_group_attr,
					       const void **data,
					       size_t *size);

/**
 * @brief Create a DOCA Verbs CC_group instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs context to create the CC group.
 * @param [in] verbs_cc_group_attr
 * Pointer to verbs_cc_group_attr instance.
 * @param [out] verbs_cc_group
 * Pointer to pointer to be set to point to the created verbs_cc_group instance.
 * User is expected to free this object with "doca_verbs_cc_group_destroy()".
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to create CC group
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_create(const struct doca_verbs_context *verbs_context,
					struct doca_verbs_cc_group_attr *verbs_cc_group_attr,
					struct doca_verbs_cc_group **verbs_cc_group);

/**
 * @brief Destroy a DOCA Verbs CC group instance.
 *
 * @param [in] verbs_cc_group
 * Pointer to verbs_cc_group instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to destroy CC group
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_destroy(struct doca_verbs_cc_group *verbs_cc_group);

/**
 * @brief Modify attributes for doca_verbs_cc_group.
 *
 * @param [in] verbs_cc_group
 * Pointer to doca_verbs_cc_group instance.
 * @param [in] verbs_cc_group_attr
 * Pointer to doca_verbs_cc_group_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to modify CC group
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_modify(struct doca_verbs_cc_group *verbs_cc_group,
					struct doca_verbs_cc_group_attr *verbs_cc_group_attr);

/**
 * @brief Query a DOCA Verbs CC_group instance.
 *
 * @param [in] verbs_cc_group
 * Pointer to cc group to query
 * @param [out] verbs_cc_group_attr
 * Pointer to verbs_cc_group_attr instance to fill.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query CC group
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_query(const struct doca_verbs_cc_group *verbs_cc_group,
				       struct doca_verbs_cc_group_attr *verbs_cc_group_attr);

/**
 * @brief Query CC group caps
 *
 * @param [in] verbs_context
 * Pointer to verbs context to query CC group caps
 * @param [out] cc_group_caps
 * Pointer to CC group caps
 *
 * @return
 * DOCA_SUCCESS - CC group caps query was successful.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if CC group is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_query_cc_group_caps(const struct doca_verbs_context *verbs_context,
					    struct doca_verbs_cc_group_caps **cc_group_caps);

/**
 * @brief free CC group caps
 *
 * @param [in] cc_group_caps
 * CC group caps to free
 *
 * @return
 * DOCA_SUCCESS - in case CC group caps was freed.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_caps_free(struct doca_verbs_cc_group_caps *cc_group_caps);

/**
 * @brief Get CC group caps data
 *
 * @param [in] cc_group_caps
 * CC group caps to get the data from
 * @param [out] data
 * Caps data
 * @param [out] size
 * Caps data size
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_caps_get_data(struct doca_verbs_cc_group_caps *cc_group_caps,
					       const void **data,
					       size_t *size);

/**
 * @brief Get vendor_id from CC group caps
 *
 * @param [in] cc_group_caps
 * CC group caps to get the data from
 *
 * @return
 * vendor_id.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_cc_group_caps_get_vendor_id(const struct doca_verbs_cc_group_caps *cc_group_caps);

/**
 * @brief Get format_id from CC group caps
 *
 * @param [in] cc_group_caps
 * CC group caps to get the data from
 *
 * @return
 * format_id.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_cc_group_caps_get_format_id(const struct doca_verbs_cc_group_caps *cc_group_caps);

/**
 * @brief Get hint_fields bitmap from CC group caps
 *
 * @details Returns a bitmap indicating which hint fields are supported by the PCC application.
 * Bit definitions are per vendor_id and format_id.
 * Returns 0 if the DPA application does not support hint_fields (e.g., old DPA app).
 * When non-zero, the returned value contains only the field bits (bit 0 is stripped internally).
 *
 * @param [in] cc_group_caps
 * CC group caps to get the data from
 *
 * @return
 * hint_fields bitmap. Returns 0 if cc_group_caps is NULL or hint_fields is not supported.
 */
DOCA_EXPERIMENTAL
uint64_t doca_verbs_cc_group_caps_get_hint_fields(const struct doca_verbs_cc_group_caps *cc_group_caps);

/**
 * @brief Get hint_value_range from CC group caps
 *
 * @details Returns a pointer to the hint value range data. The layout is determined by
 * vendor_id, format_id, and hint_fields.
 *
 * @param [in] cc_group_caps
 * CC group caps to get the data from
 * @param [out] data
 * Pointer to the hint_value_range data
 * @param [out] size
 * Size of the hint_value_range data in bytes
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_cc_group_caps_get_hint_value_range(const struct doca_verbs_cc_group_caps *cc_group_caps,
							   const void **data,
							   size_t *size);

/**
 * @brief Create a DOCA Verbs Ethernet SQ Attributes instance.
 *
 * @param [out] verbs_eth_sq_init_attr
 * Pointer to pointer to be set to point to the created verbs_eth_sq_init_attr instance.
 * User is expected to free this object with "doca_verbs_eth_sq_init_attr_destroy()".
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_create(struct doca_verbs_eth_sq_init_attr **verbs_eth_sq_init_attr);

/**
 * @brief Destroy a DOCA Verbs Ethernet SQ Attributes instance.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_destroy(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set PD attribute for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] pd
 * PD attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_pd(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
						struct doca_verbs_pd *pd);

/**
 * @brief Get PD attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * PD attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_pd *doca_verbs_eth_sq_init_attr_get_pd(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set CQ attribute for verbs_eth_sq_init_attr.
 *
 * @note This API is mutually exclusive with doca_verbs_eth_sq_init_attr_set_dpa_completion().
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] cq
 * CQ attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_cq(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
						struct doca_verbs_cq *cq);

/**
 * @brief Get CQ attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * CQ attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_cq *doca_verbs_eth_sq_init_attr_get_cq(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set signal all attribute for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] sig_all
 * signal all attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_sig_all(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
						     int sig_all);

/**
 * @brief Get signal all attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * signal all attribute.
 */
DOCA_EXPERIMENTAL
int doca_verbs_eth_sq_init_attr_get_sig_all(const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set WR number (number of WRs that the queue can handle simultaneously) attribute for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] wr_num
 * WR number attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_wr_num(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
						    uint32_t wr_num);

/**
 * @brief Get WR number attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * WR number attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_eth_sq_init_attr_get_wr_num(const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set max_sges attribute for verbs_eth_sq_init_attr.
 *
 * @details The max number of scatter-gather elements (specifically gather) in send work request.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] max_sges
 * max_sges attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_max_sges(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
						      uint32_t max_sges);

/**
 * @brief Get max_sges attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * max_sges attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_eth_sq_init_attr_get_max_sges(const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set max_inline_data attribute (in bytes) for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] max_inline_data
 * max_inline_data attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_max_inline_data(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
							     uint32_t max_inline_data);

/**
 * @brief Get max_inline_data attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * max_inline_data attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_eth_sq_init_attr_get_max_inline_data(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set queue_id attribute for verbs_eth_sq_init_attr.
 *
 * @note This logical queue ID is associated with Verbs' ethernet send queue when referring to it in DOCA Flow.
 *	    This logical queue ID should be unique across all send queues for a specific device (including other DOCA
 *	    libraries and DPDK queues).
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] queue_id
 * queue_id attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_queue_id(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
						      uint16_t queue_id);

/**
 * @brief Get queue_id attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * queue_id attribute.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_eth_sq_init_attr_get_queue_id(const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set user_index attribute for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] user_index
 * user_index attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_user_index(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
							uint32_t user_index);

/**
 * @brief Get user_index attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * user_index attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_eth_sq_init_attr_get_user_index(const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set user_data attribute for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] user_data
 * user_data attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_user_data(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
						       union doca_data user_data);

/**
 * @brief Get user_data attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * user_data attribute.
 */
DOCA_EXPERIMENTAL
union doca_data doca_verbs_eth_sq_init_attr_get_user_data(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set external_datapath_en attribute for verbs_eth_sq_init_attr.
 *
 * @note User needs to set external_datapath_en to true if he wants to use DPA data-path.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] external_datapath_en
 * external_datapath_en attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_external_datapath_en(
	struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
	uint8_t external_datapath_en);

/**
 * @brief Get external_datapath_en attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * external_datapath_en attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_eth_sq_init_attr_get_external_datapath_en(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set external umem attributes for verbs_eth_sq_init_attr.
 *
 * Setting these attributes means that the user wants to create and provide the umem by himself,
 * in compare with the default mode where the umem is created internally.
 * In that case it is the user responsibility to allocate enough memory for the umem and to free it.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] external_umem
 * External umem instance.
 * @param [in] external_umem_offset
 * The offset in the external umem buffer to set the Work Queue
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_external_umem(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
							   struct doca_umem *external_umem,
							   uint64_t external_umem_offset);

/**
 * @brief Get external umem attributes from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [out] external_umem
 * External umem instance.
 * @param [out] external_umem_offset
 * The offset in the external umem buffer to set the Work Queue
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_get_external_umem(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
	struct doca_umem **external_umem,
	uint64_t *external_umem_offset);

/**
 * @brief Set external DBR umem attributes for verbs_eth_sq_init_attr.
 *
 * Setting these attributes means that the user wants to create and provide the dbr umem by himself,
 * in compare with the default mode where the dbr umem is created internally.
 * In that case it is the user responsibility to allocate enough memory for the umem and to free it.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] external_dbr_umem
 * External dbr umem instance.
 * @param [in] external_dbr_umem_offset
 * The offset in the external dbr umem buffer to set the DBR
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_external_dbr_umem(
	struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
	struct doca_umem *external_dbr_umem,
	uint64_t external_dbr_umem_offset);

/**
 * @brief Get external DBR umem attributes from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [out] external_dbr_umem
 * External dbr umem instance.
 * @param [out] external_dbr_umem_offset
 * The offset in the external dbr umem buffer to set the DBR
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_get_external_dbr_umem(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
	struct doca_umem **external_dbr_umem,
	uint64_t *external_dbr_umem_offset);

/**
 * @brief Set external uar attribute for verbs_eth_sq_init_attr.
 *
 * Setting these attribute means that the user wants to create and provide the uar by himself,
 * in compare with the default mode where the uar is created internally.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] external_uar
 * External uar instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_external_uar(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
							  struct doca_uar *external_uar);

/**
 * @brief Get external uar attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [out] external_uar
 * External uar instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_get_external_uar(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
	struct doca_uar **external_uar);

/**
 * @brief Set DPA context for verbs_eth_sq_init_attr.
 * This API binds the DOCA Verbs Ethernet Send Queue to a DPA device, and datapath is expected to be executed on the
 * DPA.
 *
 * @note If used, the following is expected from the user:
 *       - Enable external datapath with doca_verbs_eth_sq_init_attr_set_external_datapath_en()
 *       - Create an Ethernet Send Queue DPA handle with doca_verbs_eth_sq_get_dpa_handle()
 *
 *       The following APIs must not be used if a DPA context is set:
 *       - doca_verbs_eth_sq_init_attr_set_external_umem()
 *       - doca_verbs_eth_sq_init_attr_set_external_dbr_umem()
 *       - doca_verbs_eth_sq_init_attr_set_external_uar()
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] dpa_ctx
 * Pointer to DPA context instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_dpa(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
						 struct doca_dpa *dpa_ctx);

/**
 * @brief Get DPA context from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * Pointer to DPA context instance.
 */
DOCA_EXPERIMENTAL
struct doca_dpa *doca_verbs_eth_sq_init_attr_get_dpa(const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set dpa_completion attribute for verbs_eth_sq_init_attr.
 * This API is used to attach a DPA completion to the Ethernet Send Queue.
 * This means that handling of completion will be executed on the DPA.
 *
 * @note This API is mutually exclusive with doca_verbs_eth_sq_init_attr_set_cq().
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] dpa_completion
 * dpa_completion attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_dpa_completion(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
							    struct doca_dpa_completion *dpa_completion);

/**
 * @brief Get dpa_completion attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * Pointer to dpa_completion instance.
 */
DOCA_EXPERIMENTAL
struct doca_dpa_completion *doca_verbs_eth_sq_init_attr_get_dpa_completion(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set flush_in_error_en attribute for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] flush_in_error_en
 * flush_in_error_en attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_flush_in_error_en(
	struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
	uint8_t flush_in_error_en);

/**
 * @brief Get flush_in_error_en attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * flush_in_error_en attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_eth_sq_init_attr_get_flush_in_error_en(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set allow_multi_pkt_send_wqe attribute for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] allow_multi_pkt_send_wqe
 * allow_multi_pkt_send_wqe attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_allow_multi_pkt_send_wqe(
	struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
	uint8_t allow_multi_pkt_send_wqe);

/**
 * @brief Get allow_multi_pkt_send_wqe attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * allow_multi_pkt_send_wqe attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_eth_sq_init_attr_get_allow_multi_pkt_send_wqe(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set plane_index attribute for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] plane_index
 * plane_index attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_plane_index(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
							 uint8_t plane_index);

/**
 * @brief Get plane_index attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * plane_index attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_eth_sq_init_attr_get_plane_index(const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Set Time Stamp source attribute for verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [in] ts_source_type
 * ts_source_type attribute. (See DOCA_VERBS_TS_SOURCE_*)
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_init_attr_set_ts_source_type(struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
							    uint8_t ts_source_type);

/**
 * @brief Get Time Stamp source attribute from verbs_eth_sq_init_attr.
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * ts_source_type attribute. (See DOCA_VERBS_TS_SOURCE_*)
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_eth_sq_init_attr_get_ts_source_type(const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Get state attribute from verbs_eth_sq_init_attr. This API is used to get the state of the Ethernet Send Queue
 * after calling doca_verbs_eth_sq_init_attr_query().
 *
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * state attribute.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_eth_sq_state doca_verbs_eth_sq_init_attr_get_state(
	const struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Create a DOCA Verbs Ethernet Send Queue instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 * @param [in] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 * @param [out] verbs_eth_sq
 * Pointer to pointer to be set to point to the created verbs_eth_sq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_create(struct doca_verbs_context *verbs_context,
				      struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr,
				      struct doca_verbs_eth_sq **verbs_eth_sq);

/**
 * @brief Destroy a DOCA Verbs Ethernet Send Queue instance.
 *
 * @param [in] verbs_eth_sq
 * Pointer to verbs_eth_sq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_destroy(struct doca_verbs_eth_sq *verbs_eth_sq);

/**
 * @brief Query the attributes of a DOCA Verbs Ethernet Send Queue instance.
 *
 * @param [in] verbs_eth_sq
 * Pointer to verbs_eth_sq instance.
 * @param [out] verbs_eth_sq_init_attr
 * Pointer to verbs_eth_sq_init_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_query(struct doca_verbs_eth_sq *verbs_eth_sq,
				     struct doca_verbs_eth_sq_init_attr *verbs_eth_sq_init_attr);

/**
 * @brief Recover a DOCA Verbs Ethernet Send Queue from error state into RDY state.
 *
 * @param [in] verbs_eth_sq
 * Pointer to verbs_eth_sq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - Ethernet Send Queue is not in error state.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_recover_from_error(struct doca_verbs_eth_sq *verbs_eth_sq);

/**
 * @brief Get the hardware queue number of a DOCA Verbs Ethernet Send Queue instance.
 *
 * @param [in] verbs_eth_sq
 * Pointer to verbs_eth_sq instance.
 *
 * @return
 * The hardware queue number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_eth_sq_get_sq_hw_number(const struct doca_verbs_eth_sq *verbs_eth_sq);

/**
 * @brief Get a DPA handle for a DOCA Verbs Ethernet Send Queue instance.
 * The handle can be used to post send work requests in DPA kernel for this Ethernet Send Queue.
 *
 * @note To use this API, external datapath must be enabled.
 *
 * @param [in] verbs_eth_sq
 * Pointer to verbs_eth_sq instance.
 * @param [in] dpa_ctx
 * Pointer to DPA context instance.
 * @param [out] dpa_eth_sq_handle
 * Pointer to DPA ETH SQ handle to be set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_get_dpa_handle(struct doca_verbs_eth_sq *verbs_eth_sq,
					      struct doca_dpa *dpa_ctx,
					      doca_dpa_dev_verbs_eth_sq_t *dpa_eth_sq_handle);

/**
 * @brief Get the Work Queue attributes of a DOCA Verbs Ethernet Send Queue instance.
 *
 * @param [in] verbs_eth_sq
 * Pointer to verbs_eth_sq instance.
 * @param [out] sq_buf
 * Pointer to Send Queue buffer.
 * @param [out] sq_num_entries
 * The number of entries in Send Queue buffer.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_sq_get_wq(const struct doca_verbs_eth_sq *verbs_eth_sq,
				      void **sq_buf,
				      uint32_t *sq_num_entries);

/**
 * @brief Get the UAR address of a DOCA Verbs Ethernet Send Queue instance.
 *
 * @param [in] verbs_eth_sq
 * Pointer to verbs_eth_sq instance.
 *
 * @return
 * The UAR register address.
 */
DOCA_EXPERIMENTAL
void *doca_verbs_eth_sq_get_uar_addr(const struct doca_verbs_eth_sq *verbs_eth_sq);

/**
 * @brief Get the DBR address of a DOCA Verbs Ethernet Send Queue instance.
 *
 * @param [in] verbs_eth_sq
 * Pointer to verbs_eth_sq instance.
 *
 * @return
 * The DBR address.
 */
DOCA_EXPERIMENTAL
void *doca_verbs_eth_sq_get_dbr_addr(const struct doca_verbs_eth_sq *verbs_eth_sq);

/**
 * @brief Create a DOCA Verbs Ethernet RQ Attributes instance.
 *
 * @param [out] verbs_eth_rq_init_attr
 * Pointer to pointer to be set to point to the created verbs_eth_rq_init_attr instance.
 * User is expected to free this object with "doca_verbs_eth_rq_init_attr_destroy()".
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_create(struct doca_verbs_eth_rq_init_attr **verbs_eth_rq_init_attr);

/**
 * @brief Destroy a DOCA Verbs Ethernet RQ Attributes instance.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_destroy(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set PD attribute for verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] pd
 * PD attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_pd(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
						struct doca_verbs_pd *pd);

/**
 * @brief Get PD attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * PD attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_pd *doca_verbs_eth_rq_init_attr_get_pd(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set CQ attribute for verbs_eth_rq_init_attr.
 *
 * @note This API is mutually exclusive with doca_verbs_eth_rq_init_attr_set_dpa_completion().
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] cq
 * CQ attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_cq(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
						struct doca_verbs_cq *cq);

/**
 * @brief Get CQ attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * CQ attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_cq *doca_verbs_eth_rq_init_attr_get_cq(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set WR number (number of WRs that the queue can handle simultaneously) attribute for verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] wr_num
 * WR number attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_wr_num(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
						    uint32_t wr_num);

/**
 * @brief Get WR number attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * WR number attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_eth_rq_init_attr_get_wr_num(const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set max_sges attribute for verbs_eth_rq_init_attr.
 *
 * @details The max number of scatter-gather elements (specifically scatter) in receive work request.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] max_sges
 * max_sges attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_max_sges(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
						      uint32_t max_sges);

/**
 * @brief Get max_sges attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * max_sges attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_eth_rq_init_attr_get_max_sges(const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set queue_id attribute for verbs_eth_rq_init_attr.
 *
 * @note This logical queue ID is associated with Verbs' ethernet receive queue when referring to it in DOCA Flow.
 *	    This logical queue ID should be unique across all receive queues for a specific device (including other
 *DOCA libraries and DPDK queues).
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] queue_id
 * queue_id attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_queue_id(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
						      uint16_t queue_id);

/**
 * @brief Get queue_id attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * queue_id attribute.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_eth_rq_init_attr_get_queue_id(const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set user_index attribute for verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] user_index
 * user_index attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_user_index(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
							uint32_t user_index);

/**
 * @brief Get user_index attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * user_index attribute.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_eth_rq_init_attr_get_user_index(const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set user_data attribute for verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] user_data
 * user_data attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_user_data(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
						       union doca_data user_data);

/**
 * @brief Get user_data attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * user_data attribute.
 */
DOCA_EXPERIMENTAL
union doca_data doca_verbs_eth_rq_init_attr_get_user_data(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set external_datapath_en attribute for verbs_eth_rq_init_attr.
 *
 * @note User needs to set external_datapath_en to true if he wants to use DPA data-path.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] external_datapath_en
 * external_datapath_en attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_external_datapath_en(
	struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
	uint8_t external_datapath_en);

/**
 * @brief Get external_datapath_en attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * external_datapath_en attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_eth_rq_init_attr_get_external_datapath_en(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set external umem attributes for verbs_eth_rq_init_attr.
 *
 * Setting these attributes means that the user wants to create and provide the umem by himself,
 * in compare with the default mode where the umem is created internally.
 * In that case it is the user responsibility to allocate enough memory for the umem and to free it.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] external_umem
 * External umem instance.
 * @param [in] external_umem_offset
 * The offset in the external umem buffer to set the Work Queue
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_external_umem(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
							   struct doca_umem *external_umem,
							   uint64_t external_umem_offset);

/**
 * @brief Get external umem attributes from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [out] external_umem
 * External umem instance.
 * @param [out] external_umem_offset
 * The offset in the external umem buffer to set the Work Queue
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_get_external_umem(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
	struct doca_umem **external_umem,
	uint64_t *external_umem_offset);

/**
 * @brief Set external DBR umem attributes for verbs_eth_rq_init_attr.
 *
 * Setting these attributes means that the user wants to create and provide the dbr umem by himself,
 * in compare with the default mode where the dbr umem is created internally.
 * In that case it is the user responsibility to allocate enough memory for the umem and to free it.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] external_dbr_umem
 * External dbr umem instance.
 * @param [in] external_dbr_umem_offset
 * The offset in the external dbr umem buffer to set the DBR
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_external_dbr_umem(
	struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
	struct doca_umem *external_dbr_umem,
	uint64_t external_dbr_umem_offset);

/**
 * @brief Get external DBR umem attributes from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [out] external_dbr_umem
 * External dbr umem instance.
 * @param [out] external_dbr_umem_offset
 * The offset in the external dbr umem buffer to set the DBR
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_get_external_dbr_umem(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
	struct doca_umem **external_dbr_umem,
	uint64_t *external_dbr_umem_offset);

/**
 * @brief Set DPA context for verbs_eth_rq_init_attr.
 * This API binds the DOCA Verbs Ethernet Receive Queue to a DPA device, and datapath is expected to be executed on the
 * DPA.
 *
 * @note If used, the following is expected from the user:
 *       - Enable external datapath with doca_verbs_eth_rq_init_attr_set_external_datapath_en()
 *       - Create an Ethernet Receive Queue DPA handle with doca_verbs_eth_rq_get_dpa_handle()
 *
 *       The following APIs must not be used if a DPA context is set:
 *       - doca_verbs_eth_rq_init_attr_set_external_umem()
 *       - doca_verbs_eth_rq_init_attr_set_external_dbr_umem()
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] dpa_ctx
 * Pointer to DPA context instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_dpa(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
						 struct doca_dpa *dpa_ctx);

/**
 * @brief Get DPA context from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * Pointer to DPA context instance.
 */
DOCA_EXPERIMENTAL
struct doca_dpa *doca_verbs_eth_rq_init_attr_get_dpa(const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set dpa_completion attribute for verbs_eth_rq_init_attr.
 * This API is used to attach a DPA completion to the Ethernet Receive Queue.
 * This means that handling of completion will be executed on the DPA.
 *
 * @note This API is mutually exclusive with doca_verbs_eth_rq_init_attr_set_cq().
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] dpa_completion
 * dpa_completion attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_dpa_completion(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
							    struct doca_dpa_completion *dpa_completion);

/**
 * @brief Get dpa_completion attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * Pointer to dpa_completion instance.
 */
DOCA_EXPERIMENTAL
struct doca_dpa_completion *doca_verbs_eth_rq_init_attr_get_dpa_completion(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set flush_in_error_en attribute for verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] flush_in_error_en
 * flush_in_error_en attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_flush_in_error_en(
	struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
	uint8_t flush_in_error_en);

/**
 * @brief Get flush_in_error_en attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * flush_in_error_en attribute.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_eth_rq_init_attr_get_flush_in_error_en(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set SRQ attribute for verbs_eth_rq_init_attr.
 *
 * @note This API is mutually exclusive with:
 *       - doca_verbs_eth_rq_init_attr_set_wr()
 *       - doca_verbs_eth_rq_init_attr_set_max_sges()
 *       - doca_verbs_eth_rq_init_attr_set_external_umem()
 *       - doca_verbs_eth_rq_init_attr_set_external_dbr_umem()
 *       - doca_verbs_eth_rq_init_attr_set_flush_in_error_en()
 *       - doca_verbs_eth_rq_init_attr_set_pd()
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] srq
 * SRQ attribute.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_srq(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
						 struct doca_verbs_srq *srq);

/**
 * @brief Get SRQ attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * SRQ attribute.
 */
DOCA_EXPERIMENTAL
struct doca_verbs_srq *doca_verbs_eth_rq_init_attr_get_srq(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Set Time Stamp source attribute for verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [in] ts_source_type
 * ts_source_type attribute. (See DOCA_VERBS_TS_SOURCE_*)
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_init_attr_set_ts_source_type(struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
							    uint8_t ts_source_type);

/**
 * @brief Get Time Stamp source attribute from verbs_eth_rq_init_attr.
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * ts_source_type attribute. (See DOCA_VERBS_TS_SOURCE_*)
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_eth_rq_init_attr_get_ts_source_type(const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Get state attribute from verbs_eth_rq_init_attr. This API is used to get the state of the Ethernet Receive
 * Queue after calling doca_verbs_eth_rq_init_attr_query().
 *
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * state attribute.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_eth_rq_state doca_verbs_eth_rq_init_attr_get_state(
	const struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Create a DOCA Verbs Ethernet Receive Queue instance.
 *
 * @param [in] verbs_context
 * Pointer to verbs_context instance.
 * @param [in] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 * @param [out] verbs_eth_rq
 * Pointer to pointer to be set to point to the created verbs_eth_rq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_create(struct doca_verbs_context *verbs_context,
				      struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr,
				      struct doca_verbs_eth_rq **verbs_eth_rq);

/**
 * @brief Destroy a DOCA Verbs Ethernet Receive Queue instance.
 *
 * @param [in] verbs_eth_rq
 * Pointer to verbs_eth_rq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_destroy(struct doca_verbs_eth_rq *verbs_eth_rq);

/**
 * @brief Query the attributes of a DOCA Verbs Ethernet Receive Queue instance.
 *
 * @param [in] verbs_eth_rq
 * Pointer to verbs_eth_rq instance.
 * @param [out] verbs_eth_rq_init_attr
 * Pointer to verbs_eth_rq_init_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_query(struct doca_verbs_eth_rq *verbs_eth_rq,
				     struct doca_verbs_eth_rq_init_attr *verbs_eth_rq_init_attr);

/**
 * @brief Get the hardware queue number of a DOCA Verbs Ethernet Receive Queue instance.
 *
 * @param [in] verbs_eth_rq
 * Pointer to verbs_eth_rq instance.
 *
 * @return
 * The hardware queue number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_eth_rq_get_rq_hw_number(const struct doca_verbs_eth_rq *verbs_eth_rq);

/**
 * @brief Recover a DOCA Verbs Ethernet Receive Queue from error state into RDY state.
 *
 * @param [in] verbs_eth_rq
 * Pointer to verbs_eth_rq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - Ethernet Receive Queue is not in error state.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_recover_from_error(struct doca_verbs_eth_rq *verbs_eth_rq);

/**
 * @brief Get a DPA handle for a DOCA Verbs Ethernet Receive Queue instance.
 * The handle can be used to post receive work requests in DPA kernel for this Ethernet Receive Queue.
 *
 * @note To use this API, external datapath must be enabled.
 *
 * @param [in] verbs_eth_rq
 * Pointer to verbs_eth_rq instance.
 * @param [in] dpa_ctx
 * Pointer to DPA context instance.
 * @param [out] dpa_eth_rq_handle
 * Pointer to DPA ETH RQ handle to be set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_get_dpa_handle(struct doca_verbs_eth_rq *verbs_eth_rq,
					      struct doca_dpa *dpa_ctx,
					      doca_dpa_dev_verbs_eth_rq_t *dpa_eth_rq_handle);

/**
 * @brief Get the Work Queue attributes of a DOCA Verbs Ethernet Receive Queue instance.
 *
 * @note Valid only when RQ isn't connected to a SRQ.
 *
 * @param [in] verbs_eth_rq
 * Pointer to verbs_eth_rq instance.
 * @param [out] rq_buf
 * Pointer to Receive Queue buffer.
 * @param [out] rq_num_entries
 * The number of entries in Receive Queue buffer.
 * @param [out] rwqe_size_bytes
 * Receive WQE size in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - RQ is connected to a SRQ.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_eth_rq_get_wq(const struct doca_verbs_eth_rq *verbs_eth_rq,
				      void **rq_buf,
				      uint32_t *rq_num_entries,
				      uint32_t *rwqe_size_bytes);

/**
 * @brief Get the DBR address of a DOCA Verbs Ethernet Receive Queue instance.
 *
 * @note Valid only when RQ isn't connected to a SRQ.
 *
 * @param [in] verbs_eth_rq
 * Pointer to verbs_eth_rq instance.
 *
 * @return
 * The DBR address (NULL if RQ is connected to a SRQ).
 */
DOCA_EXPERIMENTAL
void *doca_verbs_eth_rq_get_dbr_addr(const struct doca_verbs_eth_rq *verbs_eth_rq);

/**********************************************************************************************************************
 * Capabilities functions
 *********************************************************************************************************************/

/**
 * @brief Query DOCA Verbs device attributes.
 *
 * @param [in] context
 * Pointer to doca_verbs_context instance.
 * @param [out] verbs_device_attr
 * Pointer to pointer to be set to point to the created verbs_device_attr instance.
 * User is expected to free this object with "doca_verbs_device_attr_free()".
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_NOT_DRIVER - low level layer failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_query_device(struct doca_verbs_context *context,
				     struct doca_verbs_device_attr **verbs_device_attr);

/**
 * @brief Free a DOCA Verbs Device Attributes instance.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_free(struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of QPs supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of QPs supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_qp(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of work requests on send/receive queue supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of work requests on send/receive queue supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_qp_wr(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of scatter/gather entries per send/receive work request in a QP other than RD supported
 * by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of scatter/gather entries per send/receive work request in a QP other than RD supported by the device.
 *
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_sge(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of CQs supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of CQs supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_cq(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of entries on CQ supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of entries on CQ supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_cqe(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of MRs supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of MRs supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_mr(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of PDs supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of MRs supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_pd(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the ability of the device to support atomic operations. The support level can be one of the following
 * enumerated values:
 * - DOCA_VERBS_ATOMIC_CAP_NONE - Atomic operations aren't supported at all
 * - DOCA_VERBS_ATOMIC_CAP_HCA - Atomicity is guaranteed between QPs on this device only
 * - DOCA_VERBS_ATOMIC_CAP_GLOB - Atomicity is guaranteed between this device and any other component, such as
 * CPUs, IO devices and other RDMA devices
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The ability of the device to support atomic operations.
 */
DOCA_EXPERIMENTAL
enum doca_verbs_atomic_cap doca_verbs_device_attr_get_atomic_cap(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of outstanding RDMA Read or Atomic requests that a single QP is allowed to initiate
 * concurrently, as supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The maximum number of outstanding RDMA Read or Atomic requests supported by the device.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_device_attr_get_max_qp_rd_atom(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of incoming RDMA Read or Atomic requests that a single QP can handle concurrently as a
 * responder, as supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The maximum number of incoming RDMA Read or Atomic requests supported by the device.
 */
DOCA_EXPERIMENTAL
uint8_t doca_verbs_device_attr_get_max_qp_init_rd_atom(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of AHs supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of AHs supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_ah(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of SRQs supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of SRQs supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_srq(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of work requests on SRQ supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of work requests on SRQ supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_srq_wr(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of scatter entries per receive work request in a SRQ supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of scatter entries per receive work request in a SRQ supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_srq_sge(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of partitions supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of partitions supported by the device.
 */
DOCA_EXPERIMENTAL
uint16_t doca_verbs_device_attr_get_max_pkeys(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if a given QP type is supported on this device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 * @param [in] qp_type
 * The QP type to check its support.
 *
 * @return
 * DOCA_SUCCESS - in case QP type is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if QP type is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_qp_type_supported(const struct doca_verbs_device_attr *verbs_device_attr,
							     uint32_t qp_type);

/**
 * @brief Check if DPA datapath is supported on this device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case DPA datapath is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if DPA datapath is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_dpa_external_datapath_supported(
	const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if GPU datapath is supported on this device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case GPU datapath is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if GPU datapath is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_gpu_external_datapath_supported(
	const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if ECE (enhanced connection establishment) is supported on this device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case ECE is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if ECE is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_ece_supported(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if CC group is supported on this device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case CC group is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if CC group is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_cc_group_supported(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of CC groups supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of CC groups supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_cc_group(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum CC group hint size supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max CC group hint size supported by the device.
 */
DOCA_EXPERIMENTAL
size_t doca_verbs_device_attr_get_max_cc_group_hint_max_size(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if the device support CQE inline for receive.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case CQE inline for receive is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if CQE inline for receive is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_cqe_inline_supported(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the maximum number of work requests on Ethernet Send Queue supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of work requests on Ethernet Send Queue supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_eth_sq_wr_num(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if the device supports allow_multi_pkt_send_wqe for Ethernet Send Queue.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case allow_multi_pkt_send_wqe for Ethernet Send Queue is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if allow_multi_pkt_send_wqe for Ethernet Send Queue is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_allow_multi_pkt_send_wqe_supported(
	const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if the device supports plane_index for Ethernet Send Queue.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case plane_index for Ethernet Send Queue is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if plane_index for Ethernet Send Queue is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_plane_index_supported(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Get the frequency of the free running clock for the device in kHz.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The frequency of the free running clock for the device in kHz.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_ts_free_running_clock_frequency(
	const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if the device supports ts_source_type for Ethernet Send Queue.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 * @param [in] ts_source_type
 * ts_source_type attribute to check its support. (See DOCA_VERBS_TS_SOURCE_*)
 *
 * @return
 * DOCA_SUCCESS - in case ts_source_type for Ethernet Send Queue is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if ts_source_type for Ethernet Send Queue is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_eth_sq_ts_source_type_supported(
	const struct doca_verbs_device_attr *verbs_device_attr,
	uint8_t ts_source_type);

/**
 * @brief Get the maximum number of work requests on Ethernet Receive Queue supported by the device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * The max number of work requests on Ethernet Receive Queue supported by the device.
 */
DOCA_EXPERIMENTAL
uint32_t doca_verbs_device_attr_get_max_eth_rq_wr_num(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if the device supports connecting SRQ to Ethernet Receive Queue.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 *
 * @return
 * DOCA_SUCCESS - in case connecting SRQ to Ethernet Receive Queue is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if connecting SRQ to Ethernet Receive Queue is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_eth_rq_srq_supported(const struct doca_verbs_device_attr *verbs_device_attr);

/**
 * @brief Check if the device supports ts_source_type for Ethernet Receive Queue.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 * @param [in] ts_source_type
 * ts_source_type attribute to check its support. (See DOCA_VERBS_TS_SOURCE_*)
 *
 * @return
 * DOCA_SUCCESS - in case ts_source_type for Ethernet Receive Queue is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if ts_source_type for Ethernet Receive Queue is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_eth_rq_ts_source_type_supported(
	const struct doca_verbs_device_attr *verbs_device_attr,
	uint8_t ts_source_type);

/**
 * @brief Check if the device supports a given send_dbr_mode input value.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 * @param [in] send_dbr_mode
 * The send dbr mode to check.
 *
 * @return
 * DOCA_SUCCESS - in case send_dbr_mode value is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if send_dbr_mode is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_send_dbr_mode_supported(
	const struct doca_verbs_device_attr *verbs_device_attr,
	enum doca_verbs_qp_send_dbr_mode send_dbr_mode);

/**
 * @brief Check if ordering semantic is supported for a specific QP type on this device.
 *
 * @param [in] verbs_device_attr
 * Pointer to doca_verbs_device_attr instance.
 * @param [in] qp_ordering_semantic
 * The QP ordering semantic.
 * @param [in] qp_type
 * The QP type.
 *
 * @return
 * DOCA_SUCCESS - in case ordering semantic is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_NOT_SUPPORTED - if ordering semantic is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_verbs_device_attr_get_is_ordering_semantic_supported(
	const struct doca_verbs_device_attr *verbs_device_attr,
	enum doca_verbs_qp_ordering_semantic qp_ordering_semantic,
	uint32_t qp_type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_VERBS_H_ */

/** @} */
