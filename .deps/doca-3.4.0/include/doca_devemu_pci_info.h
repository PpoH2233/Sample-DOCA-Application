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
 * @file doca_devemu_pci_info.h
 * @page doca_devemu_pci_info
 * @defgroup DOCA_DEVEMU_PCI_INFO DOCA Device Emulation - PCI Device information
 * @ingroup DOCA_DEVEMU_PCI
 *
 * DOCA PCI information for emulated pci devices
 *
 * @{
 */

#ifndef DOCA_DEVEMU_PCI_INFO_H_
#define DOCA_DEVEMU_PCI_INFO_H_

#include <stdint.h>
#include <sys/uio.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci_type.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing an emulated PCI BAR Info instance.
 * This structure is used to query information about the BAR configuration of a PCI device type.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_pci_bar_info;

/**
 * @brief Opaque structure representing an emulated PCI doorbell BAR region Info instance for doorbells identified
 * by their offset within the BAR.
 * This structure is used to query information about the doorbell BAR region configuration of a PCI device type.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_pci_db_region_by_offset_info;

/**
 * @brief Opaque structure representing an emulated PCI doorbell BAR region Info instance for doorbells identified
 * by the data written to the doorbell.
 * This structure is used to query information about the doorbell BAR region configuration of a PCI device type.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_pci_db_region_by_data_info;

/**
 * @brief Opaque structure representing an emulated PCI MSI-X table BAR region Info instance.
 * This structure is used to query information about the MSI-X table BAR region configuration of a PCI device type.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_pci_msix_table_region_info;

/**
 * @brief Opaque structure representing an emulated PCI MSI-X PBA BAR region Info instance.
 * This structure is used to query information about the MSI-X PBA BAR region configuration of a pci device type.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_pci_msix_pba_region_info;

/**
 * @brief Opaque structure representing an emulated PCI stateful BAR region Info instance.
 * This structure is used to query information about the stateful BAR region configuration of a pci device type.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_pci_stateful_region_info;

/**
 * @brief Opaque structure representing an emulated PCI transaction BAR region Info instance.
 * This structure is used to query information about the transaction BAR region configuration of a pci device type.
 * This structure is used by pci device emulation applications, libraries and services.
 */
struct doca_devemu_pci_transaction_region_info;

/*********************************************************************************************************************
 * DOCA devemu PCI info API
 *********************************************************************************************************************/

/**
 * @brief Creates a list of configured BARs for a given DOCA devemu PCI type that was created using
 * doca_devemu*_pci*_type_create().
 *
 * @param [in] pci_type
 * The DOCA devemu pci type. Must be started.
 * @param [out] bar_list
 * Pointer to an array of pointers. The output can then be accessed as follows (*bar_list)[idx].
 * @param [out] nb_bars
 * Number of configured BARs.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'bar_list' or 'nb_bars' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started.
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * @note The returned list must be destroyed using doca_devemu_pci_type_destroy_bar_info_list()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_bar_info_list(struct doca_devemu_pci_type *pci_type,
						       struct doca_devemu_pci_bar_info ***bar_list,
						       uint32_t *nb_bars);

/**
 * @brief Destroy list of BAR info structures.
 *
 * @param [in] bar_list
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'bar_list' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_destroy_bar_info_list(struct doca_devemu_pci_bar_info **bar_list);

/**
 * @brief Get the BAR identifier of a DOCA Devemu PCI BAR Info.
 *
 * @param [in] bar_info
 * The BAR Info instance to query.
 * @param [out] bar_id
 * The identifier of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'bar_info' or 'bar_id' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_bar_info_get_bar_id(const struct doca_devemu_pci_bar_info *bar_info, uint8_t *bar_id);

/**
 * @brief Get the BAR size, in Log (base 2) units, of a DOCA Devemu PCI BAR Info.
 *
 * @param [in] bar_info
 * The BAR Info instance to query.
 * @param [out] log_sz
 * The size, in Log (base 2) units of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'bar_info' or 'log_sz' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_bar_info_get_log_sz(const struct doca_devemu_pci_bar_info *bar_info, uint8_t *log_sz);

/**
 * @brief Get the memory type of a DOCA Devemu PCI BAR Info.
 *
 * @param [in] bar_info
 * The BAR Info instance to query.
 * @param [out] memory_type
 * The memory type value of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'bar_info' or 'memory_type' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_bar_info_get_mem_type(const struct doca_devemu_pci_bar_info *bar_info,
						   enum doca_devemu_pci_bar_mem_type *memory_type);

/**
 * @brief Get the Prefetchable bit value of a DOCA Devemu PCI BAR Info.
 *
 * @param [in] bar_info
 * The BAR Info instance to query.
 * @param [out] prefetchable
 * The Prefetchable bit value of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'bar_info' or 'prefetchable' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_bar_info_get_prefetchable(const struct doca_devemu_pci_bar_info *bar_info,
						       uint8_t *prefetchable);

/**
 * @brief Creates a list of configured doorbell BAR regions for a given DOCA devemu PCI type that was created using
 * doca_devemu*_pci*_type_create(). The doorbells associated with this type of BAR region are identified according to
 * their offset within the BAR.
 *
 * @param [in] pci_type
 * The DOCA devemu pci type. Must be started.
 * @param [out] region_list
 * Pointer to an array of pointers. The output can then be accessed as follows (*region_list)[idx].
 * @param [out] nb_regions
 * Number of configured regions.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'region_list' or 'nb_regions' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started.
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * @note The returned list must be destroyed using doca_devemu_pci_type_destroy_db_region_by_offset_info_list()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_db_region_by_offset_info_list(
	struct doca_devemu_pci_type *pci_type,
	struct doca_devemu_pci_db_region_by_offset_info ***region_list,
	uint32_t *nb_regions);

/**
 * @brief Destroy list of doorbell BAR regions info structures.
 *
 * @param [in] region_list
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'region_list' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_destroy_db_region_by_offset_info_list(
	struct doca_devemu_pci_db_region_by_offset_info **region_list);

/**
 * @brief Get the BAR identifier of the DOCA Devemu PCI doorbell BAR region Info for doorbells identified by their
 * offset within the BAR.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] bar_id
 * The identifier of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'bar_id' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_offset_info_get_bar_id(
	const struct doca_devemu_pci_db_region_by_offset_info *info,
	uint8_t *bar_id);

/**
 * @brief Get the start address of the DOCA Devemu PCI doorbell BAR region Info for doorbells identified by their
 * offset within the BAR.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] start_addr
 * The start address of the region within the BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'start_addr' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_offset_info_get_start_addr(
	const struct doca_devemu_pci_db_region_by_offset_info *info,
	uint64_t *start_addr);

/**
 * @brief Get the size of the DOCA Devemu PCI doorbell BAR region Info for doorbells identified by their offset within
 * the BAR.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] size
 * The size of the region in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_offset_info_get_size(
	const struct doca_devemu_pci_db_region_by_offset_info *info,
	uint64_t *size);

/**
 * @brief Get the size, in Log (base 2) units, of a single doorbell in the DOCA Devemu PCI doorbell BAR region Info for
 * doorbells identified by their offset within the BAR.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] log_db_size
 * The size, given in bytes, of single doorbell in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'log_db_size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_offset_info_get_log_db_size(
	const struct doca_devemu_pci_db_region_by_offset_info *info,
	uint8_t *log_db_size);

/**
 * @brief Get the size, in Log (base 2) units, of a single doorbell stride in the DOCA Devemu PCI doorbell BAR region
 * Info for doorbells identified by their offset within the BAR.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] log_db_stride_size
 * The size, given in bytes, of a single doorbell stride in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'log_db_stride_size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_offset_info_get_log_db_stride_size(
	const struct doca_devemu_pci_db_region_by_offset_info *info,
	uint8_t *log_db_stride_size);

/**
 * @brief Creates a list of configured doorbell BAR regions for a given DOCA devemu PCI type that was created using
 * doca_devemu*_pci*_type_create(). The doorbells associated with this type of BAR region are identified according to
 * the data written to the doorbell.
 *
 * @param [in] pci_type
 * The DOCA devemu pci type. Must be started.
 * @param [out] region_list
 * Pointer to an array of pointers. The output can then be accessed as follows (*region_list)[idx].
 * @param [out] nb_regions
 * Number of configured regions.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'region_list' or 'nb_regions' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started.
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * @note The returned list must be destroyed using doca_devemu_pci_type_destroy_db_region_by_data_info_list()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_db_region_by_data_info_list(
	struct doca_devemu_pci_type *pci_type,
	struct doca_devemu_pci_db_region_by_data_info ***region_list,
	uint32_t *nb_regions);

/**
 * @brief Destroy list of doorbell BAR regions info structures.
 *
 * @param [in] region_list
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'region_list' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_destroy_db_region_by_data_info_list(
	struct doca_devemu_pci_db_region_by_data_info **region_list);

/**
 * @brief Get the BAR identifier of the DOCA Devemu PCI doorbell BAR region Info for doorbells identified by the data
 * written to the doorbell.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] bar_id
 * The identifier of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'bar_id' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_data_info_get_bar_id(const struct doca_devemu_pci_db_region_by_data_info *info,
							       uint8_t *bar_id);

/**
 * @brief Get the start address of the DOCA Devemu PCI doorbell BAR region Info for doorbells identified by the data
 * written to the doorbell.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] start_addr
 * The start address of the region within the BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'start_addr' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_data_info_get_start_addr(
	const struct doca_devemu_pci_db_region_by_data_info *info,
	uint64_t *start_addr);

/**
 * @brief Get the size of the DOCA Devemu PCI doorbell BAR region Info for doorbells identified by the data written to
 * the doorbell.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] size
 * The size of the region in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_data_info_get_size(const struct doca_devemu_pci_db_region_by_data_info *info,
							     uint64_t *size);

/**
 * @brief Get the size, in Log (base 2) units, of a single doorbell in the DOCA Devemu PCI doorbell BAR region Info for
 * doorbells identified by the data written to the doorbell.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] log_db_size
 * The size, given in bytes, of single doorbell in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'log_db_size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_data_info_get_log_db_size(
	const struct doca_devemu_pci_db_region_by_data_info *info,
	uint8_t *log_db_size);

/**
 * @brief Get the start byte of the doorbell identifier within the doorbell data in the DOCA Devemu PCI doorbell BAR
 * region Info, for doorbells identified by the data written to the doorbell. If the db_id_msbyte > db_id_lsbyte for
 * the same doorbell BAR region, the doorbell identifier will be treated as Little-Endian.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] db_id_msbyte
 * The start byte of the doorbell identifier, within the doorbell data written by the driver.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'db_id_msbyte' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_data_info_get_db_id_msbyte(
	const struct doca_devemu_pci_db_region_by_data_info *info,
	uint16_t *db_id_msbyte);

/**
 * @brief Get the end byte of the doorbell identifier within the doorbell data in the DOCA Devemu PCI doorbell BAR
 * region Info, for doorbells identified by the data written to the doorbell. If the db_id_msbyte > db_id_lsbyte for
 * the same doorbell BAR region, the doorbell identifier will be treated as Little-Endian.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] db_id_lsbyte
 * The end byte of the doorbell identifier, within the doorbell data written by the driver.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'db_id_lsbyte' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_region_by_data_info_get_db_id_lsbyte(
	const struct doca_devemu_pci_db_region_by_data_info *info,
	uint16_t *db_id_lsbyte);

/**
 * @brief Creates a list of configured MSI-X table BAR regions for a given DOCA devemu PCI type that was created using
 * doca_devemu*_pci*_type_create().
 *
 * @param [in] pci_type
 * The DOCA devemu pci type. Must be started.
 * @param [out] region_list
 * Pointer to an array of pointers. The output can then be accessed as follows (*region_list)[idx].
 * @param [out] nb_regions
 * Number of configured regions.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'region_list' or 'nb_regions' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started.
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * @note The returned list must be destroyed using doca_devemu_pci_type_destroy_msix_table_region_info_list()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_msix_table_region_info_list(
	struct doca_devemu_pci_type *pci_type,
	struct doca_devemu_pci_msix_table_region_info ***region_list,
	uint32_t *nb_regions);

/**
 * @brief Destroy list of MSI-X table BAR regions info structures.
 *
 * @param [in] region_list
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'region_list' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_destroy_msix_table_region_info_list(
	struct doca_devemu_pci_msix_table_region_info **region_list);

/**
 * @brief Get the BAR identifier of the DOCA Devemu PCI MSI-X table BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] bar_id
 * The identifier of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'bar_id' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_msix_table_region_info_get_bar_id(const struct doca_devemu_pci_msix_table_region_info *info,
							       uint8_t *bar_id);

/**
 * @brief Get the start address of the DOCA Devemu PCI MSI-X table BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] start_addr
 * The start address of the region within the BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'start_addr' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_msix_table_region_info_get_start_addr(
	const struct doca_devemu_pci_msix_table_region_info *info,
	uint64_t *start_addr);

/**
 * @brief Get the size of the DOCA Devemu PCI MSI-X table BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] size
 * The size of the region in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_msix_table_region_info_get_size(const struct doca_devemu_pci_msix_table_region_info *info,
							     uint64_t *size);

/**
 * @brief Creates a list of configured MSI-X PBA BAR regions for a given DOCA devemu PCI type that was created using
 * doca_devemu*_pci*_type_create().
 *
 * @param [in] pci_type
 * The DOCA devemu pci type. Must be started.
 * @param [out] region_list
 * Pointer to an array of pointers. The output can then be accessed as follows (*region_list)[idx].
 * @param [out] nb_regions
 * Number of configured regions.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'region_list' or 'nb_regions' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started.
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * @note The returned list must be destroyed using doca_devemu_pci_type_destroy_msix_pba_region_info_list()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_msix_pba_region_info_list(
	struct doca_devemu_pci_type *pci_type,
	struct doca_devemu_pci_msix_pba_region_info ***region_list,
	uint32_t *nb_regions);

/**
 * @brief Destroy list of MSI-X PBA BAR regions info structures.
 *
 * @param [in] region_list
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'region_list' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_destroy_msix_pba_region_info_list(
	struct doca_devemu_pci_msix_pba_region_info **region_list);

/**
 * @brief Get the BAR identifier of the DOCA Devemu PCI MSI-X PBA BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] bar_id
 * The identifier of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'bar_id' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_msix_pba_region_info_get_bar_id(const struct doca_devemu_pci_msix_pba_region_info *info,
							     uint8_t *bar_id);

/**
 * @brief Get the start address of the DOCA Devemu PCI MSI-X PBA BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] start_addr
 * The start address of the region within the BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'start_addr' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_msix_pba_region_info_get_start_addr(const struct doca_devemu_pci_msix_pba_region_info *info,
								 uint64_t *start_addr);

/**
 * @brief Get the size of the DOCA Devemu PCI MSI-X PBA BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] size
 * The size of the region in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_msix_pba_region_info_get_size(const struct doca_devemu_pci_msix_pba_region_info *info,
							   uint64_t *size);

/**
 * @brief Creates a list of configured stateful BAR regions for a given DOCA devemu PCI type that was created using
 * doca_devemu*_pci*_type_create().
 *
 * @param [in] pci_type
 * The DOCA devemu pci type. Must be started.
 * @param [out] region_list
 * Pointer to an array of pointers. The output can then be accessed as follows (*region_list)[idx].
 * @param [out] nb_regions
 * Number of configured regions.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'region_list' or 'nb_regions' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started.
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * @note The returned list must be destroyed using doca_devemu_pci_type_destroy_stateful_region_info_list()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_stateful_region_info_list(
	struct doca_devemu_pci_type *pci_type,
	struct doca_devemu_pci_stateful_region_info ***region_list,
	uint32_t *nb_regions);

/**
 * @brief Destroy list of stateful BAR regions info structures.
 *
 * @param [in] region_list
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'region_list' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_destroy_stateful_region_info_list(
	struct doca_devemu_pci_stateful_region_info **region_list);

/**
 * @brief Get the BAR identifier of the DOCA Devemu PCI stateful BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] bar_id
 * The identifier of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'bar_id' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_stateful_region_info_get_bar_id(const struct doca_devemu_pci_stateful_region_info *info,
							     uint8_t *bar_id);

/**
 * @brief Get the start address of the DOCA Devemu PCI stateful BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] start_addr
 * The start address of the region within the BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'start_addr' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_stateful_region_info_get_start_addr(const struct doca_devemu_pci_stateful_region_info *info,
								 uint64_t *start_addr);

/**
 * @brief Get the size of the DOCA Devemu PCI stateful BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] size
 * The size of the region in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_stateful_region_info_get_size(const struct doca_devemu_pci_stateful_region_info *info,
							   uint64_t *size);

/**
 * @brief Creates a list of configured transaction BAR regions for a given DOCA devemu PCI type that was created using
 * doca_devemu*_pci*_type_create().
 *
 * @param [in] pci_type
 * The DOCA devemu pci type. Must be started.
 * @param [out] region_list
 * Pointer to an array of pointers. The output can then be accessed as follows (*region_list)[idx].
 * @param [out] nb_regions
 * Number of configured regions.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_type' or 'region_list' or 'nb_regions' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'pci_type' is not started.
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * @note The returned list must be destroyed using doca_devemu_pci_type_destroy_transaction_region_info_list()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_create_transaction_region_info_list(
	struct doca_devemu_pci_type *pci_type,
	struct doca_devemu_pci_transaction_region_info ***region_list,
	uint32_t *nb_regions);

/**
 * @brief Destroy list of transaction BAR regions info structures.
 *
 * @param [in] region_list
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'region_list' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_destroy_transaction_region_info_list(
	struct doca_devemu_pci_transaction_region_info **region_list);

/**
 * @brief Get the BAR identifier of the DOCA Devemu PCI transaction BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] bar_id
 * The identifier of the associated BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'bar_id' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_transaction_region_info_get_bar_id(
	const struct doca_devemu_pci_transaction_region_info *info,
	uint8_t *bar_id);

/**
 * @brief Get the start address of the DOCA Devemu PCI transaction BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] start_addr
 * The start address of the region within the BAR.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'start_addr' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_transaction_region_info_get_start_addr(
	const struct doca_devemu_pci_transaction_region_info *info,
	uint64_t *start_addr);

/**
 * @brief Get the size of the DOCA Devemu PCI transaction BAR region Info.
 *
 * @param [in] info
 * The region Info instance to query.
 * @param [out] size
 * The size of the region in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'info' or 'size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_transaction_region_info_get_size(const struct doca_devemu_pci_transaction_region_info *info,
							      uint64_t *size);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_PCI_INFO_H_ */
