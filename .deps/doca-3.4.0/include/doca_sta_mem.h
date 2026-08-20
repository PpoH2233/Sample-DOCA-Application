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

#ifndef DOCA_STA_MEM_H_
#define DOCA_STA_MEM_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_sta;

/**
 * @brief Allocate a pinned memory buffer with the given size and alignment.
 * The buffer will be zeroed.
 *
 * @param [in] size
 * Size in bytes
 * @param [in] align
 * If non-zero, the allocated buffer is aligned to a multiple of align.
 * In this case, it must be a power of two. The returned buffer is always
 * aligned to at least cache line size
 * @param [in] phys_addr
 * Pointer to a variable that will hold the physical address of the allocated buffer.
 * If NULL, the physical address is not returned
 *
 * @return
 * Pointer to the allocated memory buffer
 */
typedef void *(*doca_sta_zmalloc_cb_t)(size_t size, size_t align, uint64_t *phys_addr);

/**
 * @brief Function to free a memory buffer previously allocated by doca_sta_zmalloc_cb_t().
 *
 * @param [in] buf
 * Buffer to free
 */
typedef void (*doca_sta_free_cb_t)(void *buf);

#define DOCA_STA_VTOPHYS_ERROR (0xFFFFFFFFFFFFFFFFULL)

/**
 * @brief Get the physical address of a buffer previously allocated by doca_sta_zmalloc_cb_t().
 *
 * @param [in] buf
 * Pointer to a buffer
 * @param [in] size
 * Size of the memory region pointed to by buf
 *
 * @return
 * Physical address of the buffer on success, or DOCA_STA_VTOPHYS_ERROR on failure
 */
typedef uint64_t (*doca_sta_vtophys_cb_t)(const void *buf, uint32_t size);

/**
 * @brief Configure the pinned memory management callbacks.
 * The callbacks will be used for allocation, freeing, and virtual to physical
 * address translation.
 *
 * @param [in] sta
 * STA context
 * @param [in] alloc_cb
 * Callback function to allocate a pinned memory buffer
 * @param [in] free_cb
 * Callback function to free a previously allocated memory buffer
 * @param [in] vtophys_cb
 * Callback function to get the physical address of a buffer
 *
 * @return
 * DOCA_SUCCESS - in case of success
 * Error code - in case of failure, see doca_error_t
 */
DOCA_EXPERIMENTAL
doca_error_t doca_sta_mem_allocator_register(const struct doca_sta *sta,
					     doca_sta_zmalloc_cb_t alloc_cb,
					     doca_sta_free_cb_t free_cb,
					     doca_sta_vtophys_cb_t vtophys_cb);

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_MEM_H_ */
