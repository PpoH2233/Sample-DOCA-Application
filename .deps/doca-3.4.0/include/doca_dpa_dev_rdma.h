/*
 * Copyright (c) 2023-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_dpa_dev_rdma.h
 * @page doca_dpa_rdma
 * @defgroup DOCA_DPA_DEVICE_RDMA DOCA DPA Device - RDMA
 * @ingroup DOCA_DPA_DEVICE
 * DOCA DPA Device - RDMA
 * @{
 */

#ifndef DOCA_DPA_DEV_RDMA_H_
#define DOCA_DPA_DEV_RDMA_H_

#include <doca_dpa_dev.h>
#include <doca_dpa_dev_buf.h>
#include <doca_dpa_dev_sync_event.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DPA RDMA handle type definition
 */
__dpa_global__ typedef uint64_t doca_dpa_dev_rdma_t;

/**
 * @brief Send an RDMA read operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] dst_mmap_handle - destination DOCA Mmap handle
 * @param[in] dst_addr - address of destination buffer
 * @param[in] src_mmap_handle - source DOCA Mmap handle
 * @param[in] src_addr - address of source buffer
 * @param[in] length - length of buffer
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_read(doca_dpa_dev_rdma_t rdma,
				 uint32_t connection_id,
				 doca_dpa_dev_mmap_t dst_mmap_handle,
				 uint64_t dst_addr,
				 doca_dpa_dev_mmap_t src_mmap_handle,
				 uint64_t src_addr,
				 size_t length,
				 uint32_t flags);

/**
 * @brief Post an RDMA read operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] dst_buf_handle - destination DOCA buffer DPA handle
 * @param[in] src_buf_handle - source DOCA buffer DPA handle
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_buf_read(doca_dpa_dev_rdma_t rdma,
				     uint32_t connection_id,
				     doca_dpa_dev_buf_t dst_buf_handle,
				     doca_dpa_dev_buf_t src_buf_handle,
				     uint32_t flags);

/**
 * @brief Post an RDMA write operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] dst_mmap_handle - destination DOCA Mmap handle
 * @param[in] dst_addr - address of destination buffer
 * @param[in] src_mmap_handle - source DOCA Mmap handle
 * @param[in] src_addr - address of source buffer
 * @param[in] length - length of buffer
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_write(doca_dpa_dev_rdma_t rdma,
				  uint32_t connection_id,
				  doca_dpa_dev_mmap_t dst_mmap_handle,
				  uint64_t dst_addr,
				  doca_dpa_dev_mmap_t src_mmap_handle,
				  uint64_t src_addr,
				  size_t length,
				  uint32_t flags);

/**
 * @brief Post an RDMA write operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] dst_buf_handle - destination DOCA buffer DPA handle
 * @param[in] src_buf_handle - source DOCA buffer DPA handle
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_buf_write(doca_dpa_dev_rdma_t rdma,
				      uint32_t connection_id,
				      doca_dpa_dev_buf_t dst_buf_handle,
				      doca_dpa_dev_buf_t src_buf_handle,
				      uint32_t flags);

/**
 * @brief Post an RDMA write with immediate operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] dst_mmap_handle - destination DOCA Mmap handle
 * @param[in] dst_addr - address of destination buffer
 * @param[in] src_mmap_handle - source DOCA Mmap handle
 * @param[in] src_addr - address of source buffer
 * @param[in] length - length of buffer
 * @param[in] immediate - immediate data
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_write_imm(doca_dpa_dev_rdma_t rdma,
				      uint32_t connection_id,
				      doca_dpa_dev_mmap_t dst_mmap_handle,
				      uint64_t dst_addr,
				      doca_dpa_dev_mmap_t src_mmap_handle,
				      uint64_t src_addr,
				      size_t length,
				      uint32_t immediate,
				      uint32_t flags);

/**
 * @brief Post an RDMA write with immediate operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] dst_buf_handle - destination DOCA buffer DPA handle
 * @param[in] src_buf_handle - source DOCA buffer DPA handle
 * @param[in] immediate - immediate data
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_buf_write_imm(doca_dpa_dev_rdma_t rdma,
					  uint32_t connection_id,
					  doca_dpa_dev_buf_t dst_buf_handle,
					  doca_dpa_dev_buf_t src_buf_handle,
					  uint32_t immediate,
					  uint32_t flags);

/**
 * @brief Post an RDMA send operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] mmap_handle - send DOCA Mmap handle
 * @param[in] addr - address of send buffer
 * @param[in] length - length of send buffer
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_send(doca_dpa_dev_rdma_t rdma,
				 uint32_t connection_id,
				 doca_dpa_dev_mmap_t mmap_handle,
				 uint64_t addr,
				 size_t length,
				 uint32_t flags);

/**
 * @brief Post an RDMA send operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] send_buf_handle - send DOCA buffer DPA handle
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_buf_send(doca_dpa_dev_rdma_t rdma,
				     uint32_t connection_id,
				     doca_dpa_dev_buf_t send_buf_handle,
				     uint32_t flags);

/**
 * @brief Post an RDMA send with immediate operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] mmap_handle - send DOCA Mmap handle
 * @param[in] addr - address of send buffer
 * @param[in] length - length of send buffer
 * @param[in] immediate - immediate data
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_send_imm(doca_dpa_dev_rdma_t rdma,
				     uint32_t connection_id,
				     doca_dpa_dev_mmap_t mmap_handle,
				     uint64_t addr,
				     size_t length,
				     uint32_t immediate,
				     uint32_t flags);

/**
 * @brief Post an RDMA send with immediate operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] send_buf_handle - send DOCA buffer DPA handle
 * @param[in] immediate - immediate data
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_buf_send_imm(doca_dpa_dev_rdma_t rdma,
					 uint32_t connection_id,
					 doca_dpa_dev_buf_t send_buf_handle,
					 uint32_t immediate,
					 uint32_t flags);

/**
 * @brief Post an RDMA receive operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] mmap_handle - received DOCA Mmap handle
 * @param[in] addr - address of received buffer
 * @param[in] length - length of received buffer
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_receive(doca_dpa_dev_rdma_t rdma,
				    doca_dpa_dev_mmap_t mmap_handle,
				    uint64_t addr,
				    size_t length);

/**
 * @brief Post an RDMA receive operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] receive_buf_handle - received DOCA buffer DPA handle
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_buf_receive(doca_dpa_dev_rdma_t rdma, doca_dpa_dev_buf_t receive_buf_handle);

/**
 * @brief Ack an RDMA receive operations to enable reposting the buffers
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] num_acked - Number of receives to ack
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_receive_ack(doca_dpa_dev_rdma_t rdma, uint32_t num_acked);

/**
 * @brief Get completion work request index
 *
 * @param[in] comp_element - DPA completion element
 *
 * @return The completion work request index.
 */
DOCA_EXPERIMENTAL
uint32_t doca_dpa_dev_rdma_completion_get_wr_index(doca_dpa_dev_completion_element_t comp_element);

/**
 * @brief Post an RDMA atomic fetch and add operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] dst_mmap_handle - destination DOCA Mmap handle
 * @param[in] dst_addr - address of destination buffer
 * @param[in] value - value to add to the destination buffer
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_atomic_fetch_add(doca_dpa_dev_rdma_t rdma,
					     uint32_t connection_id,
					     doca_dpa_dev_mmap_t dst_mmap_handle,
					     uint64_t dst_addr,
					     uint64_t value,
					     uint32_t flags);

/**
 * @brief Post an RDMA atomic fetch and add operation
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] dst_buf_handle - destination buffer DPA handle
 * @param[in] value - value to add to the destination buffer
 * @param[in] flags - bitwise or of enum doca_dpa_dev_submit_flag (see enum doca_dpa_dev_submit_flag for more details)
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_post_buf_atomic_fetch_add(doca_dpa_dev_rdma_t rdma,
						 uint32_t connection_id,
						 doca_dpa_dev_buf_t dst_buf_handle,
						 uint64_t value,
						 uint32_t flags);

/**
 * @brief Signal to set a remote sync event count
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] remote_sync_event - remote sync event DPA handle
 * @param[in] count - count to set
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_signal_set(doca_dpa_dev_rdma_t rdma,
				  uint32_t connection_id,
				  doca_dpa_dev_sync_event_remote_net_t remote_sync_event,
				  uint64_t count);

/**
 * @brief Signal to atomically add to a remote sync event count
 *
 * @param[in] rdma - RDMA DPA handle
 * @param[in] connection_id - RDMA connection ID
 * @param[in] remote_sync_event - remote sync event DPA handle
 * @param[in] count - count to add
 */
DOCA_EXPERIMENTAL
void doca_dpa_dev_rdma_signal_add(doca_dpa_dev_rdma_t rdma,
				  uint32_t connection_id,
				  doca_dpa_dev_sync_event_remote_net_t remote_sync_event,
				  uint64_t count);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DPA_DEV_RDMA_H_ */
