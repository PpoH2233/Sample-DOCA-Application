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
 * @file doca_eth_rxq_gpu_data_path.h
 * @page DOCA_ETH_RXQ_GPU_DATA_PATH
 * @defgroup DOCA_ETH_RXQ_GPU_DATA_PATH DOCA ETH RXQ GPU Data Path
 * @ingroup DOCA_ETH_RXQ
 * DOCA ETH RXQ library.
 *
 * @{
 */
#ifndef DOCA_ETH_RXQ_GPU_DATA_PATH_H_
#define DOCA_ETH_RXQ_GPU_DATA_PATH_H_

#include <doca_compat.h>
#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque structure representing a DOCA ETH RXQ instance.
 */
struct doca_eth_rxq;

/**
 * @brief By default, the Eth Rxq UAR is on the GPU.
 * With this method set the Eth Rxq UAR on the CPU.
 *
 * @note Supported for DOCA ETH TXQ instance for GPU only.
 *
 * @param [in] eth_rxq
 * Pointer to doca_eth_rxq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 * - DOCA_ERROR_BAD_STATE - internal error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_rxq_gpu_set_uar_on_cpu(struct doca_eth_rxq *eth_rxq);

/**
 * @brief By default, the Eth Rxq dump QP is disabled.
 * With this method enable the Eth Rxq dump QP.
 *
 * @note Supported for DOCA ETH RXQ instance for GPU only.
 *
 * @param [in] eth_rxq
 * Pointer to doca_eth_rxq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case one of the arguments is NULL.
 * - DOCA_ERROR_BAD_STATE - internal error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_rxq_gpu_enable_mcst_qp(struct doca_eth_rxq *eth_rxq);

/**
 * @brief Set the GPU memory type for the ETH's receive queue.
 * can only be called before calling doca_ctx_start().
 *
 * @note Supported for DOCA ETH RXQ instance for GPU only.
 * @note The default GPU memory type is DOCA_GPU_MEM_TYPE_GPU.
 * @note This also sets the GPU memory type for the dump QP (if exists) and CQ.
 *
 * @param [in] eth_rxq
 * Pointer to doca_eth_rxq instance.
 * @param [in] rq_mem_type
 * The GPU memory type to be used for ETH's receive queue.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid parameter was given.
 * - DOCA_ERROR_BAD_STATE - if context was not started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_rxq_gpu_set_rq_mem_type(struct doca_eth_rxq *eth_rxq, enum doca_gpu_mem_type rq_mem_type);

/**
 * Progress Rxq memory consistency queue (mcst qp) with GPUNetIO data path via CPU proxy.
 * This function is useful only when mcst qp is present (enabled via doca_eth_rxq_gpu_enable_mcst_qp()).
 * This function should be called in a continuous loop from a CPU thread.
 *
 * @param [in] rxq
 * Pointer to doca_eth_rxq instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NOT_SUPPORTED - if rxq doorbell must be rang from GPU.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_eth_rxq_gpu_cpu_proxy_progress(struct doca_eth_rxq *rxq);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_ETH_RXQ_GPU_DATA_PATH_H_ */

/** @} */
