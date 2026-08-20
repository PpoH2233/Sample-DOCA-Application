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
 * @file doca_uar.h
 * @page doca_uar
 * @defgroup DOCA_UAR DOCA uar
 * @ingroup DOCACore
 * The DOCA UAR represents a user mapped memory
 *
 * @{
 */

#ifndef DOCA_UAR_H_
#define DOCA_UAR_H_

#include <stddef.h>
#include <stdint.h>

#include <doca_compat.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * DOCA UAR
 ******************************************************************************/

struct doca_dev;
struct doca_dpa;

/**
 * @brief Opaque structure representing a doca uar.
 */
struct doca_uar;

/**
 * @brief UAR allocation type.
 */
enum doca_uar_allocation_type {
	DOCA_UAR_ALLOCATION_TYPE_BLUEFLAME = 0,		 /* Preferred for low latency */
	DOCA_UAR_ALLOCATION_TYPE_NONCACHE = 1,		 /* Preferred for high throughput */
	DOCA_UAR_ALLOCATION_TYPE_NONCACHE_DEDICATED = 2, /* Type NONCACHE_DEDICATED requires libibverbs version
							    >= 1.14.47 */
};

/**
 * @brief Creates a UAR object
 *
 * @param [in] dev
 * doca device
 * @param [in] allocation_type
 * doca_uar_allocation_type
 * @param [out] uar
 * uar object
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - in case of error in a DOCA driver call.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_uar_create(const struct doca_dev *dev,
			     enum doca_uar_allocation_type allocation_type,
			     struct doca_uar **uar);

/**
 * @brief Creates a DPA UAR object
 *
 * @param [in] dpa
 * doca dpa ctx to create UAR for
 * @param [out] uar
 * uar object
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - in case of error in a DOCA driver call.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_uar_dpa_create(struct doca_dpa *dpa, struct doca_uar **uar);

/**
 * @brief Destroy UAR object
 *
 * @param [in] uar
 * uar object
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_uar_destroy(struct doca_uar *uar);

/**
 * @brief Returns the uar id
 *
 * @param [in] uar
 * uar object
 * @param [out] id
 * uar object id
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_uar_id_get(const struct doca_uar *uar, uint32_t *id);

/**
 * @brief Returns the uar page address
 *
 * @note Not supported for DPA UAR
 *
 * @param [in] uar
 * uar object
 * @param [out] page
 * uar page address
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_uar_page_get(const struct doca_uar *uar, void **page);

/**
 * @brief Returns the uar register address
 *
 * @note Not supported for DPA UAR
 *
 * @param [in] uar
 * uar object
 * @param [out] reg
 * uar register address
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_uar_reg_addr_get(const struct doca_uar *uar, void **reg);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_UAR_H_ */
