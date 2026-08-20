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
 * @file doca_dpn.h
 * @page doca_dpn
 * @defgroup DOCA_DPN DOCA DPN
 * @ingroup DOCACore
 * DOCA DPN utilities to allow a user to translate a PCI BDF to a DPN
 *
 * @{
 */

#ifndef DOCA_DPN_H_
#define DOCA_DPN_H_

#include <stdint.h>

#include <doca_compat.h>
#include <doca_dev.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DPN identifier.
 */
struct doca_dpn_id {
	uint8_t depth;	   /**< DPN depth */
	uint8_t pci_index; /**< DPN pci index */
	uint8_t node;	   /**< DPN node */
};

/**
 * @brief Check if PCI address to DPN mapping feature is available
 *
 * @note This function is only supported on Linux
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - DPN mapping feature is not supported by the device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_dpn_cap_is_map_pci_addr_to_dpn_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Get the DPN equivalent to a given PCI address
 *
 * @note This function is only supported on Linux
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [in] pci_addr
 * The PCI address to check, should be as one of the following formats:
 * - "Domain:Bus:Device.Function", e.g., "0000:3a:00.0" (size DOCA_DEVINFO_PCI_ADDR_SIZE including a null terminator).
 * - "Bus:Device.Function", e.g., "3a:00.0" (size DOCA_DEVINFO_PCI_BDF_SIZE including a null terminator), Domain is
 *   assumed to be "0000" (i.e. "0000:<pci_addr_str>").
 * @param [out] dpn_id_out
 * The corresponding DPN.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - Mapping feature is not supported by the device.
 * - DOCA_ERROR_NOT_FOUND - No DPN was found that matched the supplied PCI BDF.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_dpn_map_pci_addr_to_dpn_id(const struct doca_devinfo *devinfo,
					     const char *pci_addr,
					     struct doca_dpn_id *dpn_id_out);

#ifdef __cplusplus
} /* extern "C" { */
#endif

/** @} */

#endif /* DOCA_DPN_H_ */
