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
 * @file doca_devemu_pci_type.h
 * @page doca_devemu_pci_type
 * @defgroup DOCA_DEVEMU_PCI_TYPE DOCA Device Emulation - PCI Device Types
 * @ingroup DOCA_DEVEMU_PCI
 *
 * DOCA PCI type for emulated pci devices
 *
 * @{
 */

#ifndef DOCA_DEVEMU_PCI_TYPE_H_
#define DOCA_DEVEMU_PCI_TYPE_H_

#include <stdint.h>
#include <sys/uio.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_ctx.h>
#include <doca_pe.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA devemu PCI type API
 *********************************************************************************************************************/

/**
 * @brief Opaque structure representing emulated pci device type.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_pci_type;

/**
 * @brief Opaque structure representing the initialization attributes for a pci device representor.
 * This structure is used by doca_devemu_pci_type_create_rep_ex() when initialization
 * attributes are needed.
 */
struct doca_devemu_pci_rep_init_attr;

/**
 * @brief Maximal length for the NULL terminated string that describe the name of the emulated PCI device type.
 */
#define DOCA_DEVEMU_PCI_TYPE_NAME_LEN 32

/**
 * @brief Get the maximum number of PCI types that can be created by the device.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_pci_types
 * Number of PCI types that can be created using doca_devemu_pci_type_create().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_pci_types' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_get_max_types(const struct doca_devinfo *devinfo, uint16_t *max_pci_types);

/**
 * @brief Get the minimal BAR size (in Log base 2) that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_bar_size
 * The minimal BAR size, given in bytes, of single BAR in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'log_bar_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_log_min_bar_size(const struct doca_devinfo *devinfo, uint8_t *log_bar_size);

/**
 * @brief Get the maximum BAR size (in Log base 2) that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_bar_size
 * The maximal BAR size, given in bytes, of single BAR in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'log_bar_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_log_max_bar_size(const struct doca_devinfo *devinfo, uint8_t *log_bar_size);

/**
 * @brief Get the maximal number of MSIXs that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create() and for any PCI device that is associated with a PCI type that was created using
 * doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] num_msix
 * The maximal number of MSIXs.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'num_msix' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_max_num_msix(const struct doca_devinfo *devinfo, uint16_t *num_msix);

/**
 * @brief Get the maximal number of doorbells that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create() and for any PCI device that is associated with a PCI type that was created using
 * doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] num_db
 * The maximal number of doorbells.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'num_db' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_max_num_db(const struct doca_devinfo *devinfo, uint16_t *num_db);

/**
 * @brief Get the minimal doorbell size (in Log base 2) that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_db_size
 * The minimal doorbell size, given in bytes, of single doorbell in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'log_db_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_log_min_db_size(const struct doca_devinfo *devinfo, uint8_t *log_db_size);

/**
 * @brief Get the maximum doorbell size (in Log base 2) that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_db_size
 * The maximal doorbell size, given in bytes, of single doorbell in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'log_db_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_log_max_db_size(const struct doca_devinfo *devinfo, uint8_t *log_db_size);

/**
 * @brief Get the minimal stride size (in Log base 2) of a single doorbell that can be configured for any PCI type
 * that was created using doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_stride_size
 * The minimal single doorbell stride size, given in bytes, of single doorbell in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'log_stride_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_log_min_db_stride_size(const struct doca_devinfo *devinfo,
								 uint8_t *log_stride_size);

/**
 * @brief Get the maximal stride size (in Log base 2) of a single doorbell that can be configured for any PCI type
 * that was created using doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_stride_size
 * The maximal single doorbell stride size, given in bytes, of single doorbell in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'log_stride_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_log_max_db_stride_size(const struct doca_devinfo *devinfo,
								 uint8_t *log_stride_size);

/**
 * @brief Get the maximum number of BARs that can be configured to an emulated PCI device, for any PCI type that was
 * created using doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_bars
 * Number of BARs that can be configured for any pci type using doca_devemu_pci_type_*_bar_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_bars' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_max_bars(const struct doca_devinfo *devinfo, uint8_t *max_bars);

/**
 * @brief Get the maximum number of BAR regions that can be configured to an emulated PCI device, for any PCI type that
 * was created using doca_devemu_pci_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_bar_regions
 * Number of BAR regions that can be configured per BAR, for any pci type, using
 * doca_devemu_pci_type_bar_*_region_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_bar_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_get_max_bar_regions(const struct doca_devinfo *devinfo, uint32_t *max_bar_regions);

/**
 * @brief Get the maximum number of BAR regions that can be configured to an emulated PCI device, for any PCI type that
 * was created using doca_devemu_pci_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_bar_regions
 * Number of BAR regions that can be configured, for any pci type, using doca_devemu_pci_type_bar_*_region_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_bar_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_max_bar_regions(const struct doca_devinfo *devinfo,
							  uint32_t *max_bar_regions);

/**
 * @brief Check if the device supports configuring doorbell regions that can identify doorbells by their offset in the
 * BAR. If supported, one can configure a doorbell region using
 * doca_devemu_pci_type_set_bar_db_region_by_offset_conf().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] supported
 * 1 if the doorbell region by offset supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'supported' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_db_region_is_identify_by_offset_supported(const struct doca_devinfo *devinfo,
									       uint8_t *supported);

/**
 * @brief Check if the device supports configuring doorbell regions that can identify doorbells by their data.
 * If supported, one can configure a doorbell region using doca_devemu_pci_type_set_bar_db_region_by_data_conf().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] supported
 * 1 if the doorbell region by data supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'supported' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_db_region_is_identify_by_data_supported(const struct doca_devinfo *devinfo,
									     uint8_t *supported);

/**
 * @brief Get the region block size of a doorbell BAR region that can be configured to an emulated PCI device, for any
 * PCI type that was created using doca_devemu_pci_type_create() or will be created by it. The region block size is the
 * smallest allocation data unit for a BAR region. For example, if the region block size is 64B then the bar region
 * size can be 64B/128B/192B/../N*64B (N = max num region blocks per doorbell BAR region).
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] block_size
 * Region block size, in bytes, of a doorbell BAR region that will be configured using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'block_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_db_region_get_region_block_size(const struct doca_devinfo *devinfo,
								     uint32_t *block_size);

/**
 * @brief Get the maximum number of region blocks of a single doorbell BAR region that can be configured for any PCI
 * type that was created using doca_devemu_pci_type_create() or will be created by it.
 * The maximal number of region blocks together with the region block size defines the maximal size of a single
 * doorbell region.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_blocks
 * Maximal number of region blocks for a single doorbell BAR region that will be configured using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_blocks' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_db_region_get_max_num_region_blocks(const struct doca_devinfo *devinfo,
									 uint32_t *max_blocks);

/**
 * @brief Get the maximum amount of doorbell BAR regions that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create() or will be created by it.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Maximal number of doorbell BAR regions that can be configured using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_max_bar_db_regions(const struct doca_devinfo *devinfo, uint32_t *max_regions);

/**
 * @brief Get the maximum number of BAR doorbell regions that can be configured to an emulated PCI device, for any PCI
 * type that was created using doca_devemu_pci_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Number of BAR doorbell regions that can be configured per BAR, for any pci type, using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_get_max_bar_db_regions(const struct doca_devinfo *devinfo, uint32_t *max_regions);

/**
 * @brief Get the doorbell BAR region start address alignment that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create() or will be created by it.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] alignment
 * The start address alignment, in bytes, of doorbell BAR regions that can be configured using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'alignment' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_db_region_get_start_addr_alignment(const struct doca_devinfo *devinfo,
									uint32_t *alignment);

/**
 * @brief Get the region block size of a stateful BAR region that can be configured to an emulated PCI device, for any
 * PCI type that was created using doca_devemu_pci_type_create() or will be created by it. The region block size is the
 * smallest allocation data unit for a BAR region. For example, if the region block size is 64B then the bar region
 * size can be 64B/128B/192B/../N*64B (N = max num region blocks per stateful BAR region).
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] block_size
 * Region block size, in bytes, of a stateful BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_stateful_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'block_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_stateful_region_get_region_block_size(const struct doca_devinfo *devinfo,
									   uint32_t *block_size);

/**
 * @brief Get the maximum number of region blocks of a single stateful BAR region that can be configured for any PCI
 * type that was created using doca_devemu_pci_type_create() or will be created by it.
 * The maximal number of region blocks together with the region block size defines the maximal size of a single
 * stateful region.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_blocks
 * Maximal number of region blocks for a single stateful BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_stateful_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_blocks' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_stateful_region_get_max_num_region_blocks(const struct doca_devinfo *devinfo,
									       uint32_t *max_blocks);

/**
 * @brief Get the maximum amount of stateful BAR regions that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create() or will be created by it.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Maximal number of stateful BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_stateful_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_max_bar_stateful_regions(const struct doca_devinfo *devinfo,
								   uint32_t *max_regions);

/**
 * @brief Get the maximum number of BAR stateful regions that can be configured to an emulated PCI device, for any PCI
 * type that was created using doca_devemu_pci_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Number of BAR stateful regions that can be configured per BAR, for any pci type, using
 * doca_devemu_pci_type_set_bar_stateful_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_get_max_bar_stateful_regions(const struct doca_devinfo *devinfo,
								  uint32_t *max_regions);

/**
 * @brief Get the stateful BAR region start address alignment that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create() or will be created by it.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] alignment
 * The start address alignment, in bytes, of stateful BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_stateful_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'alignment' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_stateful_region_get_start_addr_alignment(const struct doca_devinfo *devinfo,
									      uint32_t *alignment);

/**
 * @brief Get the region block size of a MSI-X table BAR region that can be configured to an emulated PCI device, for
 * any PCI type that was created using doca_devemu_pci_type_create() or will be created by it. The region block size is
 * the smallest allocation data unit for a BAR region. For example, if the region block size is 64B then the bar region
 * size can be 64B/128B/192B/../N*64B (N = max num region blocks per MSI-X table BAR region).
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] block_size
 * Region block size, in bytes, of a MSI-X table BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'block_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_msix_table_region_get_region_block_size(const struct doca_devinfo *devinfo,
									     uint32_t *block_size);

/**
 * @brief Get the maximum number of region blocks of a single MSI-X table BAR region that can be configured for any PCI
 * type that was created using doca_devemu_pci_type_create() or will be created by it.
 * The maximal number of region blocks together with the region block size defines the maximal size of a single
 * MSI-X table region.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_blocks
 * Maximal number of region blocks for a single MSI-X table BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_blocks' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_msix_table_region_get_max_num_region_blocks(const struct doca_devinfo *devinfo,
										 uint32_t *max_blocks);

/**
 * @brief Get the maximum amount of MSI-X table BAR regions that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create() or will be created by it.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Maximal number of MSI-X table BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_max_bar_msix_table_regions(const struct doca_devinfo *devinfo,
								     uint32_t *max_regions);

/**
 * @brief Get the maximum number of BAR MSI-X table regions that can be configured to an emulated PCI device, for any
 * PCI type that was created using doca_devemu_pci_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Number of BAR MSI-X table regions that can be configured per BAR, for any pci type, using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_get_max_bar_msix_table_regions(const struct doca_devinfo *devinfo,
								    uint32_t *max_regions);

/**
 * @brief Get the MSI-X table BAR region start address alignment that can be configured for any PCI type that was
 * created using doca_devemu_pci_type_create() or will be created by it.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] alignment
 * The start address alignment, in bytes, of MSI-X table BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'alignment' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_msix_table_region_get_start_addr_alignment(const struct doca_devinfo *devinfo,
										uint32_t *alignment);

/**
 * @brief Get the region block size of a MSI-X PBA BAR region that can be configured to an emulated PCI device, for any
 * PCI type that was created using doca_devemu_pci_type_create() or will be created by it. The region block size is the
 * smallest allocation data unit for a BAR region. For example, if the region block size is 64B then the bar region
 * size can be 64B/128B/192B/../N*64B (N = max num region blocks per MSI-X PBA BAR region).
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] block_size
 * Region block size, in bytes, of a MSI-X PBA BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'block_size' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_msix_pba_region_get_region_block_size(const struct doca_devinfo *devinfo,
									   uint32_t *block_size);

/**
 * @brief Get the maximum number of region blocks of a single MSI-X PBA BAR region that can be configured for any PCI
 * type that was created using doca_devemu_pci_type_create() or will be created by it.
 * The maximal number of region blocks together with the region block size defines the maximal size of a single
 * MSI-X PBA region.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_blocks
 * Maximal number of region blocks for a single MSI-X PBA BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_blocks' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_msix_pba_region_get_max_num_region_blocks(const struct doca_devinfo *devinfo,
									       uint32_t *max_blocks);

/**
 * @brief Get the maximum amount of MSI-X PBA BAR regions that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create() or will be created by it.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Maximal number of MSI-X PBA BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_get_max_bar_msix_pba_regions(const struct doca_devinfo *devinfo,
								   uint32_t *max_regions);

/**
 * @brief Get the maximum number of BAR MSI-X PBA regions that can be configured to an emulated PCI device, for any PCI
 * type that was created using doca_devemu_pci_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Number of BAR MSI-X PBA regions that can be configured per BAR, for any pci type, using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_regions' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_get_max_bar_msix_pba_regions(const struct doca_devinfo *devinfo,
								  uint32_t *max_regions);

/**
 * @brief Get the MSI-X PBA BAR region start address alignment that can be configured for any PCI type that was created
 * using doca_devemu_pci_type_create() or will be created by it.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] alignment
 * The start address alignment, in bytes, of MSI-X PBA BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf(), for any pci type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'alignment' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_bar_msix_pba_region_get_start_addr_alignment(const struct doca_devinfo *devinfo,
									      uint32_t *alignment);

/**
 * @brief According to the PCI specification a BAR that is mapped into Memory Space can define memory types.
 */
enum doca_devemu_pci_bar_mem_type {
	/**< Base register is 32 bits wide and can be mapped anywhere in the 32 address bit Memory Space. */
	DOCA_DEVEMU_PCI_BAR_MEM_TYPE_32_BIT = 0,
	/**< Base register support memory space below 1 MB */
	DOCA_DEVEMU_PCI_BAR_MEM_TYPE_1_MB,
	/**< Base register is 64 bits wide and can be mapped anywhere in the 64 address bit Memory Space. */
	DOCA_DEVEMU_PCI_BAR_MEM_TYPE_64_BIT,
};

/**
 * @brief Get the memory BAR types capability of the device. If supported, A BAR with that memory type can be
 * configured using doca_devemu_pci_type_set_memory_bar_conf() for any PCI type that was created using
 * doca_devemu_pci_type_create() or will be created by it.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [in] memory_type
 * The BAR memory type to query.
 * @param [out] supported
 * 1 if the BAR memory type is supported by the device, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'supported' is NULL, or 'memory_type' is invalid
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_is_bar_mem_type_supported(const struct doca_devinfo *devinfo,
								enum doca_devemu_pci_bar_mem_type memory_type,
								uint8_t *supported);

/**
 * @brief Get the minimum Expansion ROM BAR size supported by the device, in Log base 2.
 *
 * @details Retrieves the minimum Expansion ROM BAR size (as a base‑2 logarithm of
 * the size in bytes) that can be configured for a device created with
 * doca_devemu_pci_type_create_rep_ex(), for any PCI type that is created using
 * doca_devemu_pci_type_create(). A value of 0 indicates that Expansion
 * ROM BAR configuration is not supported for this device.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_exp_bar_size
 * The minimal configurable Expansion ROM BAR size, given in bytes, of single Expansion ROM in Log (base 2) units.
 * A value of 0 means Expansion ROM BAR configuration is not supported.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'log_exp_bar_size' is NULL.
 * - DOCA_ERROR_DRIVER - Internal DOCA driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_get_min_log_exp_bar_size(const struct doca_devinfo *devinfo,
							  uint8_t *log_exp_bar_size);

/**
 * @brief Get the maximum Expansion ROM BAR size supported by the device, in Log base 2.
 *
 * @details Retrieves the maximum Expansion ROM BAR size (as a base‑2 logarithm of
 * the size in bytes) that can be configured for a device created with
 * doca_devemu_pci_type_create_rep_ex(), for any PCI type that is created using
 * doca_devemu_pci_type_create(). A value of 0 indicates that Expansion
 * ROM BAR configuration is not supported for this device.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_exp_bar_size
 * The maximal configurable Expansion ROM BAR size, given in bytes, of single Expansion ROM in Log (base 2) units.
 * A value of 0 means Expansion ROM BAR configuration is not supported.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'log_exp_bar_size' is NULL.
 * - DOCA_ERROR_DRIVER - Internal DOCA driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_get_max_log_exp_bar_size(const struct doca_devinfo *devinfo,
							  uint8_t *log_exp_bar_size);

/**
 * @brief Create a stopped DOCA devemu PCI type.
 *
 * @param [in] name
 * The name to assign to the created DOCA devemu PCI type.
 * The NULL terminated string must not exceed DOCA_DEVEMU_PCI_TYPE_NAME_LEN.
 * @param [out] pci_type
 * The created and stopped DOCA devemu PCI type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'name' or 'pci_type' is NULL
 * - DOCA_ERROR_NO_MEMORY - allocation failure
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create(const char *name, struct doca_devemu_pci_type **pci_type);

/**
 * @brief Destroy a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to destroy. Must be stopped.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - type can't be destroyed while started. Need to stop it first - doca_devemu_pci_type_stop()
 * - DOCA_ERROR_NOT_PERMITTED - type was not created using doca_devemu_*_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_destroy(struct doca_devemu_pci_type *pci_type);

/**
 * @brief Start a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to start.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL or invalid PCI parameters were previously provided
 * - DOCA_ERROR_BAD_STATE - type is already started
 * - DOCA_ERROR_NOT_FOUND - device was not provided - use doca_devemu_pci_type_set_dev()
 * - DOCA_ERROR_NO_MEMORY - allocation failure
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * @note This method upon success disable the ability to configure the DOCA devemu pci type.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_start(struct doca_devemu_pci_type *pci_type);

/**
 * @brief Stop a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to stop. Must not be associated to any PCI device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - type is already stopped
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * @note This method upon success re-enable the ability to configure the DOCA devemu PCI type.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_stop(struct doca_devemu_pci_type *pci_type);

/**
 * @brief Check whether the DOCA devemu PCI type is started.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] started
 * 1 if the DOCA devemu PCI type is started, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'started' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_is_started(const struct doca_devemu_pci_type *pci_type, uint8_t *started);

/**
 * @brief Create list of available representor devices for a given DOCA devemu PCI type.
 *
 * @details Returns all representors that are associated with the provided DOCA devemu PCI type.
 * The provided DOCA devemu PCI type must be started.
 * A representor can either represent an emulated PCI function that is currently attached to the host PCI subsystem or
 * an emulated PCI function intended for hotplugging into the host PCI subsystem.
 *
 * @param [in] pci_type
 * The DOCA devemu pci type. Must be started.
 * @param [out] dev_list_rep
 * Pointer to array of pointers. Output can then be accessed as follows (*dev_list_rep)[idx].
 * @param [out] nb_devs_rep
 * Number of available representor devices.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'dev_list_rep' or 'nb_devs_rep' is NULL
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * @note Returned list must be destroyed using doca_devinfo_rep_destroy_list()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_rep_list(struct doca_devemu_pci_type *pci_type,
						  struct doca_devinfo_rep ***dev_list_rep,
						  uint32_t *nb_devs_rep);

/**
 * @brief Get the DOCA device associated with the DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] dev
 * The associated DOCA device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'dev' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_dev(const struct doca_devemu_pci_type *pci_type, struct doca_dev **dev);

/**
 * @brief Set the DOCA device for a specific DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] dev
 * The DOCA device to associate. This device must remain valid for the lifetime of the DOCA devemu PCI type, or until a
 * different device is assigned to this PCI type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'dev' is NULL
 * - DOCA_ERROR_BAD_STATE - can't configure PCI type after it has been started
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_*_type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_*_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_dev(struct doca_devemu_pci_type *pci_type, struct doca_dev *dev);

/**
 * @brief Get the hotplug capability of the device for a given DOCA devemu PCI type.
 *
 * @details Get a uint8_t value indicating whether the device can be used to hotplug devices for a specific PCI type.
 * The hotplug capability of a device implies its management capability.
 * If hotplug capability is supported, the following APIs can be used for associated devices and representors:
 *  - doca_devemu_pci_dev_get_hotplug_state()
 *  - doca_devemu_pci_rep_get_hotplug_state()
 *  - doca_devemu_pci_dev_hotplug()
 *  - doca_devemu_pci_dev_hotunplug()
 *  - doca_devemu_pci_dev_event_hotplug_state_change_register()
 *
 * @param [in] devinfo
 * The device to query.
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] supported
 * 1 if the hotplug capability is supported for this PCI type, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo', 'pci_type' or 'supported' is NULL or devinfo doesn't match associated
 *   doca_dev, in case doca_dev already was assigned to the pci_type
 * - DOCA_ERROR_DRIVER - internal doca driver error
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_type_is_hotplug_supported(const struct doca_devinfo *devinfo,
							   const struct doca_devemu_pci_type *pci_type,
							   uint8_t *supported);

/**
 * @brief Check whether the device supports PCI types created by doca_devemu_pci_type_create().
 *
 * @details If 'supported' is set to 1, the device can be associated with a type created by
 * doca_devemu_pci_type_create() using doca_devemu_pci_type_set_dev().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] supported
 * 1 if the capability is supported for types created by doca_devemu_pci_type_create(). Otherwise 0.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'supported' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * @note This capability does not imply hotplug capability.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_is_type_supported(const struct doca_devinfo *devinfo, uint8_t *supported);

/**
 * @brief Create a DOCA devemu PCI device representor Initialization Attributes instance.
 *
 * @param [out] init_attr
 * The DOCA devemu PCI device representor Initialization Attributes instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'init_attr' is NULL.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_rep_init_attr_create(struct doca_devemu_pci_rep_init_attr **init_attr);

/**
 * @brief Destroy a DOCA devemu PCI device representor Initialization Attributes instance.
 *
 * @param [in] init_attr
 * The DOCA devemu PCI device representor Initialization Attributes instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'init_attr' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_rep_init_attr_destroy(struct doca_devemu_pci_rep_init_attr *init_attr);

/**
 * @brief Set the log2 size of the expansion ROM BAR for doca_devemu_pci_rep_init_attr.
 *
 * @details This value will be interpreted as follows:
 * - If 'log_exp_bar_size' > 0: Configure an Expansion ROM BAR with the given size.
 *   Restrictions:
 *   - The used device must report non‑zero capabilities from both doca_devemu_*_cap_get_min_log_exp_bar_size() and
 *     doca_devemu_*_cap_get_max_log_exp_bar_size().
 *   - 'log_exp_bar_size' must fall within the inclusive range reported by these two capability queries for the
 *     associated device.
 *
 * - If 'log_exp_bar_size' == 0:
 *   - If the used device reports non-zero capabilities from both doca_devemu_*_cap_get_min_log_exp_bar_size() and
 *     doca_devemu_*_cap_get_max_log_exp_bar_size(): Disable the Expansion ROM BAR.
 *   - Otherwise: Use the device's default.
 *
 * If an invalid value is configured before calling doca_devemu_pci_type_create_rep_ex(), then
 * doca_devemu_pci_type_create_rep_ex() will fail and return DOCA_ERROR_INVALID_VALUE.
 *
 * @param [in] init_attr
 * The Initialization attributes to set.
 * @param [in] log_exp_bar_size
 * The log2 of the expansion ROM BAR size to set.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'init_attr' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_rep_init_attr_set_log_exp_bar_size(struct doca_devemu_pci_rep_init_attr *init_attr,
								uint8_t log_exp_bar_size);

/**
 * @brief Get the log2 size of the expansion ROM BAR from a doca_devemu_pci_rep_init_attr.
 *
 * @param [in] init_attr
 * The Initialization attributes to query.
 * @param [out] log_exp_bar_size
 * The log2 of the size of the expansion ROM BAR as was set in 'init_attr'.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'init_attr' or 'log_exp_bar_size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_rep_init_attr_get_log_exp_bar_size(const struct doca_devemu_pci_rep_init_attr *init_attr,
								uint8_t *log_exp_bar_size);

/**
 * @brief Create a new representor device for a given DOCA devemu PCI type.
 *
 * @details Upon success, the created representor device will be discoverable using the discovery mechanism for
 * representors. Hot-pluggable representors can be confirmed using doca_devinfo_rep_get_is_hotplug(). The state of
 * hot-pluggable representors, upon success, will be DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF, and the device will not be
 * visible to the host.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type. Must be started.
 * @param [out] dev_rep
 * Initialized representor DOCA device instance on success. Valid on success only.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'dev_rep' is NULL
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started
 * - DOCA_ERROR_DRIVER - internal doca driver error
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_rep(const struct doca_devemu_pci_type *pci_type,
					     struct doca_dev_rep **dev_rep);

/**
 * @brief Create a new representor device for a given DOCA devemu PCI type, using extended
 * attributes.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type. Must be started.
 * @param [in] init_attr
 * The Initialization attributes used for the representor creation.
 * @param [out] dev_rep
 * Initialized representor DOCA device instance on success. Valid on success only.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type', 'init_attr' or 'dev_rep' is NULL. Or if 'init_attr'
 *   was set with invalid values.
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started.
 * - DOCA_ERROR_DRIVER - Internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_rep_ex(const struct doca_devemu_pci_type *pci_type,
						const struct doca_devemu_pci_rep_init_attr *init_attr,
						struct doca_dev_rep **dev_rep);

/**
 * @brief Destroy a representor device created by doca_devemu_pci_type_create_rep().
 *
 * @details Upon success, the representor device will no longer be discoverable using the discovery mechanism for
 * representors. The representor device must not be associated with any existing doca_devemu_pci_ep. For
 * hot-pluggable representors, the hotplug state must be DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF (indicating the device is
 * not visible to the host). Use doca_devemu_pci_rep_get_hotplug_state() to verify the hotplug state.
 *
 * @param [in] rep_dev
 * Previously initialized representor DOCA device instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'rep_dev' is NULL or corrupted
 * - DOCA_ERROR_NOT_PERMITTED - representor was not created using doca_devemu_pci_type_create_rep() or it is associated
 *   with some doca_devemu_pci_ep
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_BAD_STATE - representor device is hot-pluggable and the hotplug state is not
 *   DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_destroy_rep(struct doca_dev_rep *rep_dev);

/**
 * @brief Get the name of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] name
 * The name of the PCI type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'name' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_name(const struct doca_devemu_pci_type *pci_type,
					   char (*name)[DOCA_DEVEMU_PCI_TYPE_NAME_LEN]);

/**
 * @brief Get the PCI Device ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] device_id
 * The PCI Device ID (DID) assigned by the vendor.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type' or 'device_id' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_device_id(const struct doca_devemu_pci_type *pci_type, uint16_t *device_id);

/**
 * @brief Set the PCI Device ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] device_id
 * The PCI Device ID (DID) to assign.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - can't configure PCI type after starting it
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_device_id(struct doca_devemu_pci_type *pci_type, uint16_t device_id);

/**
 * @brief Get the PCI Vendor ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] vendor_id
 * The PCI Vendor ID (VID) allocated by the PCI-SIG.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'vendor_id' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_vendor_id(const struct doca_devemu_pci_type *pci_type, uint16_t *vendor_id);

/**
 * @brief Set the PCI Vendor ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] vendor_id
 * The PCI Vendor ID (VID) allocated by the PCI-SIG.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - can't configure PCI type after starting it
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_vendor_id(struct doca_devemu_pci_type *pci_type, uint16_t vendor_id);

/**
 * @brief Get the PCI Subsystem ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] subsystem_id
 * The PCI Subsystem ID (SSID) assigned by the subsystem vendor.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'subsystem_id' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_subsystem_id(const struct doca_devemu_pci_type *pci_type, uint16_t *subsystem_id);

/**
 * @brief Set the PCI Subsystem ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] subsystem_id
 * The PCI Subsystem ID (SSID) to assign.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - can't configure PCI type after starting it
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_subsystem_id(struct doca_devemu_pci_type *pci_type, uint16_t subsystem_id);

/**
 * @brief Get the PCI Subsystem Vendor ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] subsystem_vid
 * The PCI Subsystem Vendor ID (SVID) allocated by the PCI-SIG.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'subsystem_vid' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_subsystem_vendor_id(const struct doca_devemu_pci_type *pci_type,
							  uint16_t *subsystem_vid);

/**
 * @brief Set the PCI Subsystem Vendor ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] subsystem_vid
 * The PCI Subsystem Vendor ID (SVID) allocated by the PCI-SIG.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - can't configure PCI type after starting it
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_subsystem_vendor_id(struct doca_devemu_pci_type *pci_type,
							  uint16_t subsystem_vid);

/**
 * @brief Get the PCI Revision ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] revision_id
 * The PCI Revision ID assigned by the vendor.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'revision_id' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_revision_id(const struct doca_devemu_pci_type *pci_type, uint8_t *revision_id);

/**
 * @brief Set the PCI Revision ID of a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] revision_id
 * The PCI Revision ID to assign.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - can't configure PCI type after starting it
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_revision_id(struct doca_devemu_pci_type *pci_type, uint8_t revision_id);

/**
 * @brief Get the PCI Class Code of a DOCA devemu PCI type to identify generic operation.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] class_code
 * The PCI Class Code to identify generic operation. Only 24 LSBits are valid.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'class_code' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_class_code(const struct doca_devemu_pci_type *pci_type, uint32_t *class_code);

/**
 * @brief Set the PCI Class Code of a DOCA devemu PCI type to identify generic operation.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] class_code
 * The PCI Class Code to identify generic operation. Only 24 LSBits are valid.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - can't configure PCI type after starting it
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_class_code(struct doca_devemu_pci_type *pci_type, uint32_t class_code);

/**
 * @brief Get the size of the MSI-X Table from MSI-X Capability Registers (1 based) of a DOCA devemu PCI type. This
 * value will be used as the default num_msix value for associated DOCA devemu pci devices, unless configured otherwise.
 *
 * @param [in] pci_type
 * The DOCA pci type to query.
 * @param [out] num_msix
 * The size of the MSI-X Table from MSI-X Capability Registers (1 based).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'num_msix' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_*_type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_*_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_num_msix(const struct doca_devemu_pci_type *pci_type, uint16_t *num_msix);

/**
 * @brief Set the size of the MSI-X Table from MSI-X Capability Registers (1 based) of a DOCA devemu PCI type. Must
 * conform with doca_devemu_pci_*cap_type_get_max_num_msix().
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] num_msix
 * The size of the MSI-X Table from MSI-X Capability Registers (1 based).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - can't configure PCI type after starting it
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_*_type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_*_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_num_msix(struct doca_devemu_pci_type *pci_type, uint16_t num_msix);

/**
 * @brief Get the number of doorbells assigned to a pci type. This value will be used as the default num_db value for
 * associated DOCA devemu pci devices, unless configured otherwise.
 *
 * @param [in] pci_type
 * The DOCA pci type to query.
 * @param [out] num_db
 * The number of doorbells assigned to the pci_type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'num_db' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_*_type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_*_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_get_num_db(const struct doca_devemu_pci_type *pci_type, uint16_t *num_db);

/**
 * @brief Set the number of doorbells of a PCI type. Must conform with doca_devemu_pci_*cap_type_get_max_num_db().
 *
 * @param [in] pci_type
 * The DOCA pci type to modify. Must not be started.
 * @param [in] num_db
 * The number of doorbells to assign for the pci_type.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL
 * - DOCA_ERROR_BAD_STATE - can't configure type after starting it
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_*_type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_*_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_num_db(struct doca_devemu_pci_type *pci_type, uint16_t num_db);

/**
 * @brief Set a memory BAR layout configuration for DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type. Must not be started.
 * @param [in] id
 * The BAR id. Must be at the range of [0, doca_devemu_pci_cap_type_get_max_bars() - 1].
 * @param [in] log_sz
 * The BAR size, in Log (base 2) units. Must be set to 0 if previous BAR requires an extension (e.g. for 64-bit BARs).
 * Otherwise, this value must conform with doca_devemu_pci_*cap_type_get_log_min_bar_size() and
 * doca_devemu_pci_*cap_type_get_log_max_bar_size() capabilities.
 * @param [in] memory_type
 * Memory type value to expose for this BAR.
 * @param [in] prefetchable
 * Prefetchable bit value to expose for this BAR. Set to 1 if the BAR does not contain locations with side effects on
 * reads. Set to 0 if the BAR contains locations with read side effects or locations in which the function does not
 * tolerate write merging.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL, 'memory_type' is invalid, or 'id' is invalid
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is started
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_pci_*type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_pci_*type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_memory_bar_conf(struct doca_devemu_pci_type *pci_type,
						      uint8_t id,
						      uint8_t log_sz,
						      enum doca_devemu_pci_bar_mem_type memory_type,
						      uint8_t prefetchable);

/**
 * @brief Set an IO BAR layout configuration for DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] id
 * The BAR id. Must be at the range of [0, doca_devemu_pci_cap_type_get_max_bars() - 1].
 * @param [in] log_sz
 * The BAR size, in Log (base 2) units. Must be set to 0 if previous BAR requires an extension (e.g. for 64-bit BARs).
 * Otherwise, this value must conform with doca_devemu_pci_*cap_type_get_log_min_bar_size() and
 * doca_devemu_pci_*cap_type_get_log_max_bar_size() capabilities.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL, or 'id' is invalid
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is started
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_pci_*type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_pci_*type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_io_bar_conf(struct doca_devemu_pci_type *pci_type, uint8_t id, uint8_t log_sz);

/**
 * @brief Set a doorbell BAR region configuration for a BAR layout in a DOCA devemu PCI type. The doorbells that will
 * be associated with this doorbell BAR region will be identified according to their offset within the BAR.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] id
 * The BAR id that will contain the new region.
 * Must be at the range of [0, doca_devemu_pci_cap_type_get_max_bars() - 1].
 * @param [in] start_addr
 * The start address of the region within the BAR. This value must conform with the start address alignment capability
 * from doca_devemu_pci_*cap_bar_db_region_get_start_addr_alignment().
 * @param [in] size
 * The size of the region in bytes. Must conform with doca_devemu_pci_*cap_bar_db_region_get_region_block_size() and
 * doca_devemu_pci_*cap_bar_db_region_get_max_num_region_blocks().
 * @param [in] log_db_size
 * The size, given in bytes, of single doorbell in Log (base 2) units. This value must conform with
 * doca_devemu_pci_*cap_type_get_log_min_db_size() and doca_devemu_pci_*cap_type_get_log_max_db_size().
 * @param [in] log_stride_size
 * The size, given in bytes, of a single doorbell stride in Log (base 2) units. This value must conform with
 * doca_devemu_pci_*cap_type_get_log_min_db_stride_size() and
 * doca_devemu_pci_*cap_type_get_log_max_db_stride_size().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL, 'id' is invalid, or size == 0,
				or values do not conform to one another (stride size < DB size, or stride size > size).
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is started
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_pci_*type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_pci_*type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_bar_db_region_by_offset_conf(struct doca_devemu_pci_type *pci_type,
								   uint8_t id,
								   uint64_t start_addr,
								   uint64_t size,
								   uint8_t log_db_size,
								   uint8_t log_stride_size);

/**
 * @brief Set a doorbell BAR region configuration for a BAR layout in a DOCA devemu PCI type. The doorbells that will
 * be associated with this doorbell BAR region will be identified according to the data written to the doorbell.
 * The doorbell identifier will be deduced from the written doorbell data by the db_id_lsbyte and db_id_msbyte
 * settings.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] id
 * The BAR id that will contain the new region.
 * Must be at the range of [0, doca_devemu_pci_cap_type_get_max_bars() - 1].
 * @param [in] start_addr
 * The start address of the region within the BAR. This value must conform with the start address alignment capability
 * from doca_devemu_pci_*cap_bar_db_region_get_start_addr_alignment().
 * @param [in] size
 * The size of the region in bytes. Must conform with doca_devemu_pci_*cap_bar_db_region_get_region_block_size() and
 * doca_devemu_pci_*cap_bar_db_region_get_max_num_region_blocks().
 * @param [in] log_db_size
 * The size, given in bytes, of single doorbell in Log (base 2) units. This value must conform with
 * doca_devemu_pci_*cap_type_get_log_min_db_size() and doca_devemu_pci_*cap_type_get_log_max_db_size().
 * @param [in] db_id_msbyte
 * The start byte of the doorbell identifier, within the doorbell data written by the driver. If the
 * db_id_msbyte > db_id_lsbyte then the doorbell identifier will be treated as Little-Endian.
 * @param [in] db_id_lsbyte
 * The end byte of the doorbell identifier, within the doorbell data written by the driver. If the
 * db_id_msbyte > db_id_lsbyte then the doorbell identifier will be treated as Little-Endian.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL, 'id' is invalid, or size == 0,
				or values do not conform to one another (db_id_lsbyte > DB size,  db_id_msbyte > DB size
				or DB size > size).
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is started
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_pci_*type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_pci_*type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_bar_db_region_by_data_conf(struct doca_devemu_pci_type *pci_type,
								 uint8_t id,
								 uint64_t start_addr,
								 uint64_t size,
								 uint8_t log_db_size,
								 uint16_t db_id_msbyte,
								 uint16_t db_id_lsbyte);

/**
 * @brief Set a MSI-X table BAR region configuration for a BAR layout in a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] id
 * The BAR id that will contain the new region.
 * Must be at the range of [0, doca_devemu_pci_cap_type_get_max_bars() - 1].
 * @param [in] start_addr
 * The start address of the region within the BAR. This value must conform with the start address alignment capability
 * from doca_devemu_pci_*cap_bar_msix_table_region_get_start_addr_alignment().
 * @param [in] size
 * The size of the region in bytes. Must correlate with the num_msix value of the pci type and conform with
 * doca_devemu_pci_*cap_bar_msix_table_region_get_region_block_size() and
 * doca_devemu_pci_*cap_bar_msix_table_region_get_max_num_region_blocks().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL, 'id' is invalid, or size == 0
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is started
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_pci_*type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_pci_*type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_bar_msix_table_region_conf(struct doca_devemu_pci_type *pci_type,
								 uint8_t id,
								 uint64_t start_addr,
								 uint64_t size);

/**
 * @brief Set a MSI-X PBA BAR region configuration for a BAR layout in a DOCA devemu PCI type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] id
 * The BAR id that will contain the new region.
 * Must be at the range of [0, doca_devemu_pci_cap_type_get_max_bars() - 1].
 * @param [in] start_addr
 * The start address of the region within the BAR. This value must conform with the start address alignment capability
 * from doca_devemu_pci_*cap_bar_msix_pba_region_get_start_addr_alignment().
 * @param [in] size
 * The size of the region in bytes. Must correlate with the num_msix value of the pci type and conform with
 * doca_devemu_pci_*cap_bar_msix_pba_region_get_region_block_size() and
 * doca_devemu_pci_*cap_bar_msix_pba_region_get_max_num_region_blocks().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL, 'id' is invalid, or size == 0
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is started
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_pci_*type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_pci_*type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_bar_msix_pba_region_conf(struct doca_devemu_pci_type *pci_type,
							       uint8_t id,
							       uint64_t start_addr,
							       uint64_t size);

/**
 * @brief Set a stateful BAR region configuration for a BAR layout in a DOCA devemu PCI type.
 *
 * @details The stateful BAR region will be used by the device to expose registers to the driver. This region will
 * be actively maintained by the device. PCI READ transactions for this region will be automatically answered by the
 * device. PCI WRITE transactions will be stored to this region by the device. The associated pci_dev will be notified
 * upon any PCI WRITE transaction to this region initiated by the device driver using the
 * doca_devemu_pci_dev_event_bar_stateful_region_driver_write event, if this event was register.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to modify. Must not be started.
 * @param [in] id
 * The BAR id that will contain the new region.
 * Must be at the range of [0, doca_devemu_pci_cap_type_get_max_bars() - 1].
 * @param [in] start_addr
 * The start address of the region within the BAR. This value must conform with the start address alignment capability
 * from doca_devemu_pci_cap_bar_stateful_region_get_start_addr_alignment().
 * @param [in] size
 * The size of the region in bytes. Must conform with doca_devemu_pci_cap_bar_stateful_region_get_region_block_size()
 * and doca_devemu_pci_cap_bar_stateful_region_get_max_num_region_blocks().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' is NULL, 'id' is invalid, or size == 0
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is started
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_pci_type_create()
 * @note This configuration is applicable only for PCI types created by doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_set_bar_stateful_region_conf(struct doca_devemu_pci_type *pci_type,
							       uint8_t id,
							       uint64_t start_addr,
							       uint64_t size);

/**
 * @brief Modify default registers values for a configured stateful region in a DOCA devemu PCI type.
 *
 * @details This method will modify the default values for the entire stateful region registers area in a PCI type BAR
 * (before the first modification, the initial default values of the stateful region registers are zeroed).
 * The doca_dev_rep that will be created using doca_devemu_pci_type_create_rep() from the associated pci_type will
 * inherit these default values upon creation.
 * Furthermore, each pci_dev that will be created from the associated doca_dev_rep will have a chance to update its own
 * default values by calling doca_devemu_pci_dev_modify_bar_stateful_region_default_values() and not effecting the
 * default values of the pci_type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type. Must be started and must not be associated to any pci_dev or doca_dev_rep.
 * @param [in] id
 * The BAR id that contains the stateful region. This value must conform with the id provided during
 * doca_devemu_pci_type_set_bar_stateful_region_conf().
 * @param [in] start_addr
 * The start address of the region within the BAR. This value must conform with the start address provided during
 * doca_devemu_pci_type_set_bar_stateful_region_conf().
 * @param [in] default_values
 * Input buffer that contain the default values data.
 * @param [in] size
 * The size of the default_values buffer in bytes. The size must not be smaller than the actual size of the stateful
 * bar region that was configured using doca_devemu_pci_type_set_bar_stateful_region_conf(). If size is bigger than the
 * actual size, the first relevant bytes will be used according to the actual size. The rest of the buffer will be
 * ignored.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'default_values' is NULL, or 'id' is invalid
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_pci_type_create()
 * - DOCA_ERROR_NOT_SUPPORTED - The PCI type does not support modifying stateful region default values
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * @note This configuration is applicable only for PCI types created by doca_devemu_pci_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_modify_bar_stateful_region_default_values(struct doca_devemu_pci_type *pci_type,
									    uint8_t id,
									    uint64_t start_addr,
									    void *default_values,
									    uint64_t size);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_PCI_TYPE_H_ */
