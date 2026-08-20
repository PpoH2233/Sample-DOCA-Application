/*
 * Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_dpa_dev_verbs.h
 * @page doca_dpa_verbs
 * @defgroup DOCA_DPA_DEVICE_VERBS DOCA DPA Device - Verbs
 * @ingroup DOCA_DPA_DEVICE
 * @brief DOCA DPA Device Verbs library provides RDMA operations support for DPA applications.
 *
 * This library enables RDMA operations (Send, Receive, Read, Write, Atomic) in DPA applications
 * using a verbs-like interface. It provides functionality for posting work requests,
 * managing scatter-gather elements, and handling RDMA operations.
 * @{
 */

#ifndef DOCA_DPA_DEV_VERBS_H_
#define DOCA_DPA_DEV_VERBS_H_

#include <doca_dpa_dev.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DPA Verbs QP handle type definition
 *
 * This type represents a handle to a Verbs Queue Pair (QP) in the DPA context.
 * It is used to identify and manage RDMA operations on a specific QP.
 */
__dpa_global__ typedef uint64_t doca_dpa_dev_verbs_qp_t;

/**
 * @brief DPA Verbs SRQ handle type definition
 *
 * This type represents a handle to a Shared Receive Queue (SRQ) in the DPA context.
 * An SRQ is a queue that can be shared among multiple Queue Pairs (QPs) to reduce
 * the number of receive buffers needed. It is used to manage and post receive work
 * requests that can be consumed by any QP associated with the SRQ.
 */
__dpa_global__ typedef uint64_t doca_dpa_dev_verbs_srq_t;

/**
 * @brief DPA Verbs ETH SQ handle type definition
 *
 * This type represents a handle to a Ethernet Send Queue (ETH SQ) in the DPA context.
 * It is used to manage and post send work requests (ethernet related) for the ETH SQ.
 */
__dpa_global__ typedef uint64_t doca_dpa_dev_verbs_eth_sq_t;

/**
 * @brief DPA Verbs ETH RQ handle type definition
 *
 * This type represents a handle to a Ethernet Receive Queue (ETH RQ) in the DPA context.
 * It is used to manage and post receive work requests (ethernet related) for the ETH RQ.
 */
__dpa_global__ typedef uint64_t doca_dpa_dev_verbs_eth_rq_t;

/**
 * @brief DPA Verbs send WR struct definition
 *
 * This structure represents a send work request for RDMA operations.
 * This structure is packed and aligned to 64 bytes for optimal performance.
 * @note This is an opaque structure - users should not access its fields directly.
 */
struct doca_dpa_dev_verbs_send_wr {
	uint8_t rsvd[64]; /**< Reserved */
} __attribute__((__packed__, aligned(8)));

/**
 * @brief DPA Verbs recv WR struct definition
 *
 * This structure represents a receive work request for RDMA operations.
 * This structure is packed and aligned to 8 bytes for optimal performance.
 * @note This is an opaque structure - users should not access its fields directly.
 */
struct doca_dpa_dev_verbs_recv_wr {
	uint8_t rsvd[12]; /**< Reserved */
} __attribute__((__packed__, aligned(8)));

/**
 * @brief DOCA DPA device Verbs send work request opcodes
 *
 * These opcodes define the type of RDMA operation to be performed in a send work request.
 */
enum doca_dpa_dev_verbs_send_wr_opcode {
	DOCA_DPA_DEV_VERBS_SEND_WR_OPCODE_WRITE = 0x8,		   /**< RDMA Write operation */
	DOCA_DPA_DEV_VERBS_SEND_WR_OPCODE_WRITE_WITH_IMM = 0x9,	   /**< RDMA Write with Immediate operation */
	DOCA_DPA_DEV_VERBS_SEND_WR_OPCODE_SEND = 0xA,		   /**< Send operation */
	DOCA_DPA_DEV_VERBS_SEND_WR_OPCODE_SEND_WITH_IMM = 0xB,	   /**< Send with Immediate operation */
	DOCA_DPA_DEV_VERBS_SEND_WR_OPCODE_READ = 0x10,		   /**< RDMA Read operation */
	DOCA_DPA_DEV_VERBS_SEND_WR_OPCODE_ATOMIC_FETCH_ADD = 0x12, /**< Atomic Fetch and Add operation */
};

/**
 * @brief DOCA DPA device Verbs send work request flags
 *
 * These flags control the behavior of send work requests.
 */
enum doca_dpa_dev_verbs_send_wr_flags {
	DOCA_DPA_DEV_VERBS_SEND_WR_FLAGS_SIGNALED = 1 << 1,  /**< Completion notification indicator. Generate a
							  completion  event for this work request */
	DOCA_DPA_DEV_VERBS_SEND_WR_FLAGS_SOLICITED = 1 << 2, /**< Solicited event indicator. It signals to the
							  receiver that the message is a response to a previous request
							  or event, and it should be processed with higher priority.
							  Valid only for Send and RDMA Write_with_imm */
};

/**
 * @brief Verbs SRQ type
 *
 * This enum defines the different types of Shared Receive Queues (SRQ)
 */
enum doca_dpa_dev_verbs_srq_type {
	DOCA_DPA_DEV_VERBS_SRQ_TYPE_LINKED_LIST = 0x0, /**< Linked list based SRQ. In this type, receive work
							 requests are stored in a linked list structure */
	DOCA_DPA_DEV_VERBS_SRQ_TYPE_CONTIGUOUS = 0x1,  /**< Contiguous memory based SRQ. In this type, receive work
							 requests are stored in a contiguous memory region */
};

/**
 * @brief DOCA DPA device Verbs send work request fence modes
 *
 * These fence modes control the behavior of send work requests.
 */
enum doca_dpa_dev_verbs_send_wr_fm {
	DOCA_DPA_DEV_VERBS_SEND_WR_FM_NO_FENCE = 0x0, /**< This mode does not enforce any specific order of WRs
							 execution. WRs can be executed as they are ready, without
							 waiting for the completion of previous WRs */
	DOCA_DPA_DEV_VERBS_SEND_WR_FM_INITIATOR_SMALL_FENCE = 0x1, /**< In this mode, the WR will wait for all
								      currently executing WRs that are performing local
								      operations (like gather or memory operations) to
								      complete before it starts execution. Ensures that
								      certain local operations are completed before
								      proceeding
								    */
	DOCA_DPA_DEV_VERBS_SEND_WR_FM_FENCE = 0x2, /**< This mode enforces that the WR will only start execution
						      after all previous Read or Atomic WRs have completed. It ensures
						      that all prior read or atomic operations are finished before the
						      current WR begins, maintaining a strict order for these types of
						      operations */
	DOCA_DPA_DEV_VERBS_SEND_WR_FM_STRONG_ORDERING_FENCE = 0x3, /**< This is the most stringent mode, where the
								      WR will be executed only after all previous WRs
								      have been executed. It ensures a strict sequential
								      order of execution for all WRs, not just Read or
								      Atomic ones. This mode is applicable only for
								      Reliable Connection (RC) QP */
	DOCA_DPA_DEV_VERBS_SEND_WR_FM_FENCE_AND_INITIATOR_SMALL_FENCE = 0x4, /**< This mode combines the Fence and
								      Initiator Small Fence modes. It ensures that the
								      WR will only start execution after all currently
								      executing WRs that are performing local operations
								      (like gather or memory operations) have completed,
								      and all previous Read or Atomic WRs have
								      completed. It ensures that certain local
								      operations are completed before proceeding, and
								      that all prior read or atomic operations are
								      finished before the current WR begins, maintaining
								      a strict order for these types of operations */
};

/**
 * @brief DOCA DPA device Verbs scatter-gather element terminating lkey
 *
 * This macro defines the terminating lkey for a scatter-gather element.
 * It is used to indicate that the scatter-gather element is the last element in the list.
 *
 * @note User must set this value to the last SGE's lkey in case of not occupying the entire receive WR buffer.
 */
#ifndef DOCA_DPA_DEV_VERBS_SGE_TERMINATING_LKEY
#define DOCA_DPA_DEV_VERBS_SGE_TERMINATING_LKEY (0x100)
#endif /* DOCA_DPA_DEV_VERBS_SGE_TERMINATING_LKEY */

/**
 * @brief DOCA DPA device Verbs scatter-gather element structure
 *
 * This structure represents a scatter-gather element for RDMA operations.
 * It contains the address, length, and local key for a memory region.
 */
struct doca_dpa_dev_verbs_sge {
	uint64_t addr;	 /**< Virtual address of the memory region */
	uint32_t length; /**< Length of the memory region in bytes */
	uint32_t lkey;	 /**< Local key for the memory region */
};

/**
 * @brief Set the scatter-gather list for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] sg_list - Pointer to the scatter-gather list
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_sg_list(struct doca_dpa_dev_verbs_send_wr *send_wr,
									      struct doca_dpa_dev_verbs_sge *sg_list);

/**
 * @brief Get the scatter-gather list from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Pointer to the scatter-gather list
 */
DOCA_EXPERIMENTAL __always_inline struct doca_dpa_dev_verbs_sge *doca_dpa_dev_verbs_send_wr_get_sg_list(
	struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the number of scatter-gather elements for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] num_sge - Number of scatter-gather elements
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_sg_num_sge(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	uint32_t num_sge);

/**
 * @brief Get the number of scatter-gather elements from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Number of scatter-gather elements
 */
DOCA_EXPERIMENTAL __always_inline uint32_t
doca_dpa_dev_verbs_send_wr_get_sg_num_sge(struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the opcode for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] opcode - Operation code to set
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_opcode(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	enum doca_dpa_dev_verbs_send_wr_opcode opcode);

/**
 * @brief Get the opcode from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Operation code
 */
DOCA_EXPERIMENTAL __always_inline enum doca_dpa_dev_verbs_send_wr_opcode doca_dpa_dev_verbs_send_wr_get_opcode(
	struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the send flags for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] send_flags - Flags to set
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_send_flags(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	unsigned int send_flags);

/**
 * @brief Get the send flags from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Send flags
 */
DOCA_EXPERIMENTAL __always_inline unsigned int doca_dpa_dev_verbs_send_wr_get_send_flags(
	struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the fence mode for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] fence_mode - Fence mode to set
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_fence_mode(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	enum doca_dpa_dev_verbs_send_wr_fm fence_mode);

/**
 * @brief Get the fence mode from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Fence mode
 */
DOCA_EXPERIMENTAL __always_inline enum doca_dpa_dev_verbs_send_wr_fm doca_dpa_dev_verbs_send_wr_get_fence_mode(
	struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the immediate data for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] imm_data - Immediate data to set
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_imm_data(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	uint32_t imm_data);

/**
 * @brief Get the immediate data from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Immediate data
 */
DOCA_EXPERIMENTAL __always_inline uint32_t
doca_dpa_dev_verbs_send_wr_get_imm_data(struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the invalidate remote key for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] invalidate_rkey - Remote key to invalidate
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_invalidate_rkey(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	uint32_t invalidate_rkey);

/**
 * @brief Get the invalidate remote key from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Remote key to invalidate
 */
DOCA_EXPERIMENTAL __always_inline uint32_t
doca_dpa_dev_verbs_send_wr_get_invalidate_rkey(struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the RDMA remote address for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] remote_addr - Remote address to set
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_rdma_remote_addr(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	uint64_t remote_addr);

/**
 * @brief Get the RDMA remote address from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Remote address
 */
DOCA_EXPERIMENTAL __always_inline uint64_t
doca_dpa_dev_verbs_send_wr_get_rdma_remote_addr(struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the RDMA remote key for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] rkey - Remote key to set
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_rdma_rkey(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	uint32_t rkey);

/**
 * @brief Get the RDMA remote key from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Remote key
 */
DOCA_EXPERIMENTAL __always_inline uint32_t
doca_dpa_dev_verbs_send_wr_get_rdma_rkey(struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the atomic remote address for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] remote_addr - Remote address for atomic operation
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_atomic_remote_addr(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	uint64_t remote_addr);

/**
 * @brief Get the atomic remote address from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Remote address for atomic operation
 */
DOCA_EXPERIMENTAL __always_inline uint64_t
doca_dpa_dev_verbs_send_wr_get_atomic_remote_addr(struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the atomic compare and add value for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] compare_add - Value to add in atomic operation
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_atomic_compare_add(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	uint64_t compare_add);

/**
 * @brief Get the atomic compare and add value from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Value to add in atomic operation
 */
DOCA_EXPERIMENTAL __always_inline uint64_t
doca_dpa_dev_verbs_send_wr_get_atomic_compare_add(struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the atomic swap value for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] swap - Value to swap in atomic operation
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_atomic_swap(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	uint64_t swap);

/**
 * @brief Get the atomic swap value from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Value to swap in atomic operation
 */
DOCA_EXPERIMENTAL __always_inline uint64_t
doca_dpa_dev_verbs_send_wr_get_atomic_swap(struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the atomic remote key for a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @param[in] rkey - Remote key for atomic operation
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_send_wr_set_atomic_rkey(
	struct doca_dpa_dev_verbs_send_wr *send_wr,
	uint32_t rkey);

/**
 * @brief Get the atomic remote key from a send work request
 *
 * @param[in] send_wr - Pointer to the send work request
 * @return Remote key for atomic operation
 */
DOCA_EXPERIMENTAL __always_inline uint32_t
doca_dpa_dev_verbs_send_wr_get_atomic_rkey(struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Set the scatter-gather list for a receive work request
 *
 * @param[in] recv_wr - Pointer to the receive work request
 * @param[in] sg_list - Pointer to the scatter-gather list
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_recv_wr_set_sg_list(struct doca_dpa_dev_verbs_recv_wr *recv_wr,
									      struct doca_dpa_dev_verbs_sge *sg_list);

/**
 * @brief Get the scatter-gather list from a receive work request
 *
 * @param[in] recv_wr - Pointer to the receive work request
 * @return Pointer to the scatter-gather list
 */
DOCA_EXPERIMENTAL __always_inline struct doca_dpa_dev_verbs_sge *doca_dpa_dev_verbs_recv_wr_get_sg_list(
	struct doca_dpa_dev_verbs_recv_wr *recv_wr);

/**
 * @brief Set the number of scatter-gather elements for a receive work request
 *
 * @param[in] recv_wr - Pointer to the receive work request
 * @param[in] num_sge - Number of scatter-gather elements
 */
DOCA_EXPERIMENTAL __always_inline void doca_dpa_dev_verbs_recv_wr_set_sg_num_sge(
	struct doca_dpa_dev_verbs_recv_wr *recv_wr,
	uint32_t num_sge);

/**
 * @brief Get the number of scatter-gather elements from a receive work request
 *
 * @param[in] recv_wr - Pointer to the receive work request
 * @return Number of scatter-gather elements
 */
DOCA_EXPERIMENTAL __always_inline uint32_t
doca_dpa_dev_verbs_recv_wr_get_sg_num_sge(struct doca_dpa_dev_verbs_recv_wr *recv_wr);

/**
 * @brief Get the Work Queue attributes of a DOCA Verbs Queue Pair instance.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 * @param[out] sq_buf - Pointer to the SQ buffer
 * @param[out] sq_num_entries - Pointer to the SQ number of entries
 * @param[out] rq_buf - Pointer to the RQ buffer
 * @param[out] rq_num_entries - Pointer to the RQ number of entries
 * @param[out] rwqe_size_bytes - Pointer to the receive WQE size in bytes
 */
DOCA_EXPERIMENTAL void doca_dpa_dev_verbs_qp_get_wq(doca_dpa_dev_verbs_qp_t verbs_qp,
						    uintptr_t *sq_buf,
						    uint32_t *sq_num_entries,
						    uintptr_t *rq_buf,
						    uint32_t *rq_num_entries,
						    uint32_t *rwqe_size_bytes);

/**
 * @brief Get the DBR address of a Verbs Queue Pair instance.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 * @return The DBR address.
 */
DOCA_EXPERIMENTAL
uint32_t *doca_dpa_dev_verbs_qp_get_dbr_addr(doca_dpa_dev_verbs_qp_t verbs_qp);

/**
 * @brief Get the QP number of a Verbs Queue Pair instance.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 * @return The QP number.
 */
uint32_t doca_dpa_dev_verbs_qp_get_qpn(doca_dpa_dev_verbs_qp_t verbs_qp);

/**
 * @brief Get the QP user index of a Verbs Queue Pair instance.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 * @return User index
 */
DOCA_EXPERIMENTAL uint32_t doca_dpa_dev_verbs_qp_get_user_index(doca_dpa_dev_verbs_qp_t verbs_qp);

/**
 * @brief Post a send work request to a Verbs Queue Pair
 *
 * @note The return value can be compared later on with
 *       doca_dpa_dev_completion_element_get_wqe_counter() which returns the
 *       send work request count which triggered that completion event.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 * @param[in] send_wr - Pointer to the send work request
 * @return Send work request count on success, zero on failure
 */
DOCA_EXPERIMENTAL __always_inline uint32_t
doca_dpa_dev_verbs_qp_post_send_wr(doca_dpa_dev_verbs_qp_t verbs_qp, struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Post a raw send WQE to a Verbs Queue Pair
 *
 * This function allows posting a custom-built WQE directly to the QP.
 * @note The caller is responsible for setting all WQE details except:
 * - ctrl.pi: Set by the driver
 *
 * @note The return value can be compared later on with
 *       doca_dpa_dev_completion_element_get_wqe_counter() which returns the
 *       send work request count which triggered that completion event.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 * @param[in] wqe - Pointer to the raw send WQE
 * @return Send work request count on success, zero on failure
 */
DOCA_EXPERIMENTAL uint32_t doca_dpa_dev_verbs_qp_post_send_raw_wqe(doca_dpa_dev_verbs_qp_t verbs_qp, void *wqe);

/**
 * @brief Ack (commit) existing receive WQEs to a Verbs Queue Pair
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 * @param[in] num_wr - Number of receive work requests to ack
 * @return Current receive work request count, zero on failure
 */
DOCA_EXPERIMENTAL uint32_t doca_dpa_dev_verbs_qp_post_recv_raw_ack(doca_dpa_dev_verbs_qp_t verbs_qp, uint64_t num_wr);

/**
 * @brief Post a receive work request to a Verbs Queue Pair
 *
 * @note The return value can be compared later on with
 *       doca_dpa_dev_completion_element_get_wqe_counter() which returns the
 *       receive work request count which triggered that completion event.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 * @param[in] recv_wr - Pointer to the receive work request
 * @return Receive work request count on success, zero on failure
 */
DOCA_EXPERIMENTAL __always_inline uint32_t
doca_dpa_dev_verbs_qp_post_recv_wr(doca_dpa_dev_verbs_qp_t verbs_qp, struct doca_dpa_dev_verbs_recv_wr *recv_wr);

/**
 * @brief Post a raw receive WQE to a Verbs Queue Pair
 *
 * This function allows posting a custom-built receive WQE directly to the QP.
 *
 * @note The return value can be compared later on with
 *       doca_dpa_dev_completion_element_get_wqe_counter() which returns the
 *       receive work request count which triggered that completion event.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 * @param[in] wqe - Pointer to the raw receive WQE
 * @param[in] sz - Size of the WQE in bytes
 * @return Receive work request count on success, zero on failure
 */
DOCA_EXPERIMENTAL
uint32_t doca_dpa_dev_verbs_qp_post_recv_raw_wqe(doca_dpa_dev_verbs_qp_t verbs_qp, void *wqe, uint64_t sz);

/**
 * @brief Commit pending send work requests to a Verbs Queue Pair
 *
 * This function flushes all available send work requests (since the last commit) to HW
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 */
DOCA_EXPERIMENTAL void doca_dpa_dev_verbs_qp_commit_send(doca_dpa_dev_verbs_qp_t verbs_qp);

/**
 * @brief Lightweight commit pending send work requests to a Verbs Queue Pair
 *
 * Lightweight commit variant, stands for commit w/o internal memory fence operation.
 * This function flushes all available send work requests (since the last commit) to HW
 *
 * @note It's user responsibility to call this function after performing the relevant memory fence operation.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 */
DOCA_EXPERIMENTAL void doca_dpa_dev_verbs_qp_lw_commit_send(doca_dpa_dev_verbs_qp_t verbs_qp);

/**
 * @brief Commit pending receive work requests to a Verbs Queue Pair
 *
 * This function flushes all available receive work requests (since the last commit) to HW
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 */
DOCA_EXPERIMENTAL void doca_dpa_dev_verbs_qp_commit_recv(doca_dpa_dev_verbs_qp_t verbs_qp);

/**
 * @brief Lightweight commit pending receive work requests to a Verbs Queue Pair
 *
 * Lightweight commit variant, stands for commit w/o internal memory fence operation.
 * This function flushes all available receive work requests (since the last commit) to HW
 *
 * @note It's user responsibility to call this function after performing the relevant memory fence operation.
 *
 * @param[in] verbs_qp - Verbs Queue Pair handle
 */
DOCA_EXPERIMENTAL void doca_dpa_dev_verbs_qp_lw_commit_recv(doca_dpa_dev_verbs_qp_t verbs_qp);

/**
 * @brief Post a receive work request to a Verbs Shared Receive Queue
 *
 * @note The return value can be compared later on with
 *       doca_dpa_dev_completion_element_get_wqe_counter() which returns the
 *       receive work request count which triggered that completion event.
 *
 * @param [in] verbs_srq - Verbs SRQ handle
 * @param [in] srq_type - SRQ type
 * @param [in] recv_wr - Pointer to the receive work request
 * @return Receive work request count on success, zero on failure
 */
DOCA_EXPERIMENTAL
__always_inline uint32_t doca_dpa_dev_verbs_srq_post_recv_wr(doca_dpa_dev_verbs_srq_t verbs_srq,
							     enum doca_dpa_dev_verbs_srq_type srq_type,
							     struct doca_dpa_dev_verbs_recv_wr *recv_wr);

/**
 * @brief Post a raw receive WQE to a Verbs Shared Receive Queue
 *
 * @note The return value can be compared later on with
 *       doca_dpa_dev_completion_element_get_wqe_counter() which returns the
 *       receive work request count which triggered that completion event.
 *
 * @param [in] verbs_srq - Verbs SRQ handle
 * @param [in] srq_type - SRQ type
 * @param [in] wqe - Pointer to the raw receive WQE
 * @param [in] sz - Size of the WQE in bytes
 * @return Receive work request count on success, zero on failure
 */
DOCA_EXPERIMENTAL
__always_inline uint32_t doca_dpa_dev_verbs_srq_post_recv_raw_wqe(doca_dpa_dev_verbs_srq_t verbs_srq,
								  enum doca_dpa_dev_verbs_srq_type srq_type,
								  void *wqe,
								  uint64_t sz);

/**
 * @brief Commit pending receive work requests to a Verbs Shared Receive Queue
 *
 * This function flushes all available receive work requests (since the last commit) to HW
 *
 * @param [in] verbs_srq - Verbs SRQ handle
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_srq_commit_recv(doca_dpa_dev_verbs_srq_t verbs_srq);

/**
 * @brief Lightweight commit pending receive work requests to a Verbs Shared Receive Queue
 *
 * Lightweight commit variant, stands for commit w/o internal memory fence operation.
 * This function flushes all available receive work requests (since the last commit) to HW
 *
 * @note It's user responsibility to call this function after performing the relevant memory fence operation.
 *
 * @param [in] verbs_srq - Verbs SRQ handle
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_srq_lw_commit_recv(doca_dpa_dev_verbs_srq_t verbs_srq);

/**
 * @brief Acknowledge a Linked List Verbs Shared Receive Queue receive work request
 *
 * This function frees the relevant receive work request resources from the SRQ.
 * It's user responsibility to call this function after receiving the work request completion event.
 *
 * @note This function is only applicable for linked list based SRQ.
 *
 * @param [in] verbs_srq - Verbs SRQ handle
 * @param [in] wr_count - Work Request count
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_srq_linked_list_ack_wr(doca_dpa_dev_verbs_srq_t verbs_srq, uint32_t wr_count);

/**
 * @brief Get the SRQ number of a Verbs Shared Receive Queue instance.
 *
 * @param[in] verbs_srq - Verbs SRQ handle
 * @return The SRQ number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_dpa_dev_verbs_srq_get_srqn(doca_dpa_dev_verbs_srq_t verbs_srq);

/**
 * @brief Get the Work Queue attributes of a Verbs Shared Receive Queue instance.
 *
 * @param[in] verbs_srq - Verbs SRQ handle
 * @param[out] srq_buf - Pointer to the SRQ buffer
 * @param[out] srq_num_entries - Pointer to the SRQ number of entries
 * @param[out] rwqe_size_bytes - Pointer to the receive WQE size in bytes
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_srq_get_wq(doca_dpa_dev_verbs_srq_t verbs_srq,
				   uintptr_t *srq_buf,
				   uint32_t *srq_num_entries,
				   uint32_t *rwqe_size_bytes);

/**
 * @brief Get the DBR address of a Verbs Shared Receive Queue instance.
 *
 * @param[in] verbs_srq - Verbs SRQ handle
 * @return The DBR address.
 */
DOCA_EXPERIMENTAL
uint32_t *doca_dpa_dev_verbs_srq_get_dbr_addr(doca_dpa_dev_verbs_srq_t verbs_srq);

/**
 * @brief Post a send work request to a Verbs Ethernet Send Queue
 *
 * @note The return value can be compared later on with
 *       doca_dpa_dev_completion_element_get_wqe_counter() which returns the
 *       send work request count which triggered that completion event.
 *       The only supported opcode for ETH SQ WR is DOCA_DPA_DEV_VERBS_SEND_WR_OPCODE_SEND.
 *
 * @param [in] verbs_eth_sq - Verbs ETH SQ handle
 * @param [in] send_wr - Pointer to the send work request
 * @return Send work request count on success, zero on failure
 */
DOCA_EXPERIMENTAL
__always_inline uint32_t doca_dpa_dev_verbs_eth_sq_post_send_wr(doca_dpa_dev_verbs_eth_sq_t verbs_eth_sq,
								struct doca_dpa_dev_verbs_send_wr *send_wr);

/**
 * @brief Commit pending send work requests to a Verbs Ethernet Send Queue
 *
 * This function flushes all available send work requests (since the last commit) to HW
 *
 * @param [in] verbs_eth_sq - Verbs ETH SQ handle
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_eth_sq_commit_send(doca_dpa_dev_verbs_eth_sq_t verbs_eth_sq);

/**
 * @brief Lightweight commit pending send work requests to a Verbs Ethernet Send Queue
 *
 * Lightweight commit variant, stands for commit w/o internal memory fence operation.
 * This function flushes all available send work requests (since the last commit) to HW
 *
 * @note It's user responsibility to call this function after performing the relevant memory fence operation.
 *
 * @param [in] verbs_eth_sq - Verbs ETH SQ handle
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_eth_sq_lw_commit_send(doca_dpa_dev_verbs_eth_sq_t verbs_eth_sq);

/**
 * @brief Get the ETH SQ number of a Verbs Ethernet Send Queue instance.
 *
 * @param[in] verbs_eth_sq - Verbs ETH SQ handle
 * @return The ETH SQ number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_dpa_dev_verbs_eth_sq_get_sqn(doca_dpa_dev_verbs_eth_sq_t verbs_eth_sq);

/**
 * @brief Get the user index of a Verbs Ethernet Send Queue instance.
 *
 * @param[in] verbs_eth_sq - Verbs ETH SQ handle
 * @return The user index.
 */
DOCA_EXPERIMENTAL
uint32_t doca_dpa_dev_verbs_eth_sq_get_user_index(doca_dpa_dev_verbs_eth_sq_t verbs_eth_sq);

/**
 * @brief Get the Work Queue attributes of a Verbs Ethernet Send Queue instance.
 *
 * @param[in] verbs_eth_sq - Verbs ETH SQ handle
 * @param[out] eth_sq_buf - Pointer to the ETH SQ buffer
 * @param[out] eth_sq_num_entries - Pointer to the ETH SQ number of entries
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_eth_sq_get_wq(doca_dpa_dev_verbs_eth_sq_t verbs_eth_sq,
				      uintptr_t *eth_sq_buf,
				      uint32_t *eth_sq_num_entries);

/**
 * @brief Post a receive work request to a Verbs Ethernet Receive Queue
 *
 * @note The return value can be compared later on with
 *       doca_dpa_dev_completion_element_get_wqe_counter() which returns the
 *       receive work request count which triggered that completion event.
 *
 * @param [in] verbs_eth_rq - Verbs ETH RQ handle
 * @param [in] recv_wr - Pointer to the receive work request
 * @return Receive work request count on success, zero on failure
 */
DOCA_EXPERIMENTAL
__always_inline uint32_t doca_dpa_dev_verbs_eth_rq_post_recv_wr(doca_dpa_dev_verbs_eth_rq_t verbs_eth_rq,
								struct doca_dpa_dev_verbs_recv_wr *recv_wr);

/**
 * @brief Ack (commit) existing receive WQEs to a Verbs Ethernet Receive Queue
 *
 * @param[in] verbs_eth_rq - Verbs ETH RQ handle
 * @param[in] num_wr - Number of receive work requests to ack
 * @return Current receive work request count, zero on failure
 */
DOCA_EXPERIMENTAL
uint32_t doca_dpa_dev_verbs_eth_rq_post_recv_raw_ack(doca_dpa_dev_verbs_eth_rq_t verbs_eth_rq, uint64_t num_wr);

/**
 * @brief Commit pending receive work requests to a Verbs Ethernet Receive Queue
 *
 * This function flushes all available receive work requests (since the last commit) to HW
 *
 * @param [in] verbs_eth_rq - Verbs ETH RQ handle
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_eth_rq_commit_recv(doca_dpa_dev_verbs_eth_rq_t verbs_eth_rq);

/**
 * @brief Lightweight commit pending receive work requests to a Verbs Ethernet Receive Queue
 *
 * Lightweight commit variant, stands for commit w/o internal memory fence operation.
 * This function flushes all available receive work requests (since the last commit) to HW
 *
 * @note It's user responsibility to call this function after performing the relevant memory fence operation.
 *
 * @param [in] verbs_eth_rq - Verbs ETH RQ handle
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_eth_rq_lw_commit_recv(doca_dpa_dev_verbs_eth_rq_t verbs_eth_rq);

/**
 * @brief Get the ETH RQ number of a Verbs Ethernet Receive Queue instance.
 *
 * @param[in] verbs_eth_rq - Verbs ETH RQ handle
 * @return The ETH RQ number.
 */
DOCA_EXPERIMENTAL
uint32_t doca_dpa_dev_verbs_eth_rq_get_rqn(doca_dpa_dev_verbs_eth_rq_t verbs_eth_rq);

/**
 * @brief Get the user index of a Verbs Ethernet Receive Queue instance.
 *
 * @param[in] verbs_eth_rq - Verbs ETH RQ handle
 * @return The user index.
 */
DOCA_EXPERIMENTAL
uint32_t doca_dpa_dev_verbs_eth_rq_get_user_index(doca_dpa_dev_verbs_eth_rq_t verbs_eth_rq);

/**
 * @brief Get the Work Queue attributes of a Verbs Ethernet Receive Queue instance.
 *
 * @param[in] verbs_eth_rq - Verbs ETH RQ handle
 * @param[out] eth_rq_buf - Pointer to the ETH RQ buffer
 * @param[out] eth_rq_num_entries - Pointer to the ETH RQ number of entries
 * @param[out] rwqe_size_bytes - Pointer to the receive WQE size in bytes
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_verbs_eth_rq_get_wq(doca_dpa_dev_verbs_eth_rq_t verbs_eth_rq,
				      uintptr_t *eth_rq_buf,
				      uint32_t *eth_rq_num_entries,
				      uint32_t *rwqe_size_bytes);

/**
 * @brief Get the DBR address of a Verbs Ethernet Receive Queue instance.
 *
 * @param[in] verbs_eth_rq - Verbs ETH RQ handle
 * @return The DBR address.
 */
DOCA_EXPERIMENTAL
uintptr_t doca_dpa_dev_verbs_eth_rq_get_dbr_addr(doca_dpa_dev_verbs_eth_rq_t verbs_eth_rq);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DPA_DEV_VERBS_H_ */
