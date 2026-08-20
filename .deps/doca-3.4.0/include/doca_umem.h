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

/**
 * @file doca_umem.h
 * @page doca_umem
 * @defgroup DOCA_UMEM DOCA umem
 * @ingroup DOCACore
 * The DOCA UMEM represents a user mapped memory
 *
 * @{
 */

#ifndef DOCA_UMEM_H_
#define DOCA_UMEM_H_

#include <stddef.h>
#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * DOCA UMEM
 ******************************************************************************/

struct doca_dev;
struct doca_dpa;
struct doca_gpu;

/**
 * @brief Opaque structure representing a doca umem.
 */
struct doca_umem;

/**
 * @brief creates a doca umem
 *
 * @param [in] dev DOCA device to create umem on
 * @param [in] address host address
 * @param [in] size The size of the UMEM
 * @param [in] access_flags flags (see doca_access_flag in doca_types.h)
 * @param [out] umem_obj The UMEM object
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NO_MEMORY - failed to alloc doca_mmap.
 * - DOCA_ERROR_DRIVER - in case of error in a DOCA driver call.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_umem_create(const struct doca_dev *dev,
			      void *address,
			      size_t size,
			      uint32_t access_flags,
			      struct doca_umem **umem_obj);

/**
 * @brief creates a DPA doca umem
 *
 * @param [in] dpa DOCA DPA ctx to create UMEM for
 * @param [in] address DPA heap address, acquired by DPA library
 * @param [in] size The size of the UMEM
 * @param [in] access_flags flags (see doca_access_flag in doca_types.h)
 * @param [out] umem_obj The UMEM object
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NO_MEMORY - failed to alloc doca_mmap.
 * - DOCA_ERROR_DRIVER - in case of error in a DOCA driver call.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_umem_dpa_create(struct doca_dpa *dpa,
				  uint64_t address,
				  size_t size,
				  uint32_t access_flags,
				  struct doca_umem **umem_obj);

/**
 * @brief creates a GPU doca umem
 *
 * @param [in] gpu DOCA GPU dev to create UMEM for
 * @param [in] dev DOCA dev to create UMEM for
 * @param [in] address GPU UMEM address
 * @param [in] size The size of the UMEM
 * @param [in] access_flags flags (see doca_access_flag in doca_types.h)
 * @param [out] umem_obj The UMEM object
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NO_MEMORY - failed to alloc doca_mmap.
 * - DOCA_ERROR_DRIVER - in case of error in a DOCA driver call.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_umem_gpu_create(struct doca_gpu *gpu,
				  struct doca_dev *dev,
				  void *address,
				  size_t size,
				  uint32_t access_flags,
				  struct doca_umem **umem_obj);

/**
 * @brief destroys doca umem
 *
 * @param [in] umem_obj The UMEM object
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_DRIVER - in case of error in a DOCA driver call.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_umem_destroy(struct doca_umem *umem_obj);

/**
 * @brief This method retrieves the umem id from the umem object
 *
 * @param [in] umem_obj The UMEM object
 * @param [out] umem_id id
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_umem_get_id(const struct doca_umem *umem_obj, uint32_t *umem_id);

/**
 * @brief This method retrieves the umem size from the umem object
 *
 * @param [in] umem_obj object
 * @param [out] umem_size size
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_umem_get_size(const struct doca_umem *umem_obj, uint32_t *umem_size);

/**
 * @brief This method retrieves the umem address from the umem object
 *
 * @param [in] umem_obj object
 * @param [out] umem_addr address
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_umem_get_address(const struct doca_umem *umem_obj, void **umem_addr);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_UMEM_H_ */
