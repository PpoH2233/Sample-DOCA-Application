/*
 * Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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

#ifndef DOCA_STA_HANDLE_H_
#define DOCA_STA_HANDLE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DOCA STA completion size.
 */

#define DOCA_STA_COMPLETION_SIZE (16)

typedef uint8_t doca_sta_nvme_completion_t[DOCA_STA_COMPLETION_SIZE];

/**
 * @brief DOCA STA queue pair (QP) handle type definition.
 */
struct doca_sta_qp_handle;

/**
 * @brief DOCA STA subsystem handle type definition.
 */
struct doca_sta_subs_handle;

/**
 * @brief DOCA STA namespace handle type definition.
 */
struct doca_sta_ns_handle;

/**
 * @brief DOCA STA execution unit (EU) handle type definition.
 */
struct doca_sta_eu_handle;

/**
 * @brief DOCA STA backend (BE) handle type definition.
 */
struct doca_sta_be_handle;

/**
 * @brief DOCA STA backend queue handle type definition.
 */
struct doca_sta_be_q_handle;

#ifdef __cplusplus
}
#endif

#endif /* DOCA_STA_HANDLE_H_ */
