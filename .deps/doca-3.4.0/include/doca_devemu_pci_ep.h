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
 * @file doca_devemu_pci_ep.h
 * @page doca_devemu_pci_ep
 * @defgroup DOCA_DEVEMU DOCA Device Emulation
 * @defgroup DOCA_DEVEMU_PCI_EP DOCA Device Emulation - PCI endpoint emulation
 * @ingroup DOCA_DEVEMU_PCI
 *
 * DOCA PCI EP emulation
 *
 * @{
 */

#ifndef DOCA_DEVEMU_PCI_EP_H_
#define DOCA_DEVEMU_PCI_EP_H_

#include <stdint.h>
#include <stdbool.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_mmap.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing an emulated PCI endpoint.
 * It serves as the base for all emulated PCI devices.
 * This structure is used by PCI device emulation applications, libraries and services.
 */
struct doca_devemu_pci_ep;

/**
 * @brief Opaque structure representing emulated PCI device doorbell.
 * This structure is used by PCI device emulation applications, libraries and services.
 */
struct doca_devemu_pci_db;

/**
 * @brief DPA handle for emulated PCI device doorbell.
 */
typedef uint64_t doca_dpa_dev_devemu_pci_db_t;

/**
 * @brief Opaque structure representing emulated PCI device doorbell completion context.
 * This structure is used by PCI device emulation applications, libraries and services.
 */
struct doca_devemu_pci_db_completion;

/**
 * @brief DPA handle for emulated PCI device doorbell completion context.
 */
typedef uint64_t doca_dpa_dev_devemu_pci_db_completion_t;

/**
 * @brief Opaque structure representing emulated PCI device MSI-X.
 * This structure is used by PCI device emulation applications, libraries and services.
 */
struct doca_devemu_pci_msix;

/**
 * @brief DPA handle for emulated PCI device MSI-X.
 */
typedef uint64_t doca_dpa_dev_devemu_pci_msix_t;

/*********************************************************************************************************************
 * DOCA libraries opaque structures
 *********************************************************************************************************************/
struct doca_dpa;

/*********************************************************************************************************************
 * DOCA devemu PCI endpoint API
 *********************************************************************************************************************/

/**
 * @brief Binds the DOCA devemu PCI endpoint to a DPA device.
 *
 * @details The data path will be executed on the device and not on the CPU.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint instance to bind. Must not be started.
 * @param [in] dpa_dev
 * A pointer to a doca_dpa device.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - on failure:
 * - DOCA_ERROR_INVALID_VALUE - 'ep' or 'dpa_dev' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'ep' is started.
 * - DOCA_ERROR_NOT_PERMITTED - 'ep' is already bound to a dpa device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_ep_set_datapath_on_dpa(struct doca_devemu_pci_ep *ep, struct doca_dpa *dpa_dev);

/**
 * @brief Get the number of MSI-X vectors configured to a DOCA devemu PCI endpoint.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint instance to query.
 * @param [out] num_msix
 * The number of MSI-X vectors configured for the endpoint.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'ep' or 'num_msix' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_ep_get_num_msix(const struct doca_devemu_pci_ep *ep, uint16_t *num_msix);

/**
 * @brief Set the number of MSI-X vectors of a specific DOCA devemu PCI endpoint.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint instance to modify. Must not be started.
 * @param [in] num_msix
 * The number of MSI-X vectors to be configured for the endpoint. This value must conform with device capabilities.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'ep' is NULL
 * - DOCA_ERROR_BAD_STATE - endpoint is started
 * - DOCA_ERROR_NOT_PERMITTED - device is not a Virtual Function (VF) and is not a Physical Function (PF) associated
 * with a representor that was created by doca_devemu_pci_type_create_rep().
 * - DOCA_ERROR_NOT_SUPPORTED - If the num_msix value doesn't conform with device capabilities.
 * @note The device should be a Virtual Function (VF), or a Physical Function (PF) associated with a representor that
 * was created by doca_devemu_pci_type_create_rep().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_ep_set_num_msix(struct doca_devemu_pci_ep *ep, uint16_t num_msix);

/**
 * @brief Get the number of doorbells configured to DOCA devemu PCI endpoint.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint instance to query. Must be created using doca_devemu_pci_*dev_create().
 * @param [out] num_db
 * The number of doorbells configured for the endpoint.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'ep' or 'num_db' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - endpoint was not created using doca_devemu_pci_*dev_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_ep_get_num_db(const struct doca_devemu_pci_ep *ep, uint16_t *num_db);

/**
 * @brief Set the number of doorbells of a specific DOCA devemu PCI endpoint.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint instance to modify. Must not be started.
 * @param [in] num_db
 * The number of doorbells to be configured for the endpoint. This value must conform with device capabilities.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'ep' is NULL
 * - DOCA_ERROR_BAD_STATE - device is started
 * - DOCA_ERROR_NOT_PERMITTED - device is not a Virtual Function (VF) and is not a Physical Function (PF) created using
 * doca_devemu_pci_*dev_create().
 * - DOCA_ERROR_NOT_SUPPORTED - If the num_db value doesn't conform with device capabilities.
 * @note The device should be a Virtual Function (VF) or a Physical Function (PF) created by
 * doca_devemu_pci_*dev_create().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_ep_set_num_db(struct doca_devemu_pci_ep *ep, uint16_t num_db);

/*********************************************************************************************************************
 * DOCA devemu PCI Doorbell
 *********************************************************************************************************************/

/**
 * @brief Allocate a doorbell on the DPA for a DOCA devemu PCI endpoint. The created doorbell will be associated with a
 * single completion context that was also created on DPA.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint to be associated with the doorbell. Must be started.
 * @param [in] db_comp
 * The DOCA devemu PCI doorbell completion context to be associated with the doorbell. Must be started.
 * @param [in] bar_id
 * The identifier of the BAR that contains the associated doorbell region for the created doorbell.
 * @param [in] bar_start_addr
 * The start address of the associated doorbell region within the BAR. This value must conform with the start
 * address that was configured to the doorbell region during the configuration cycle of the PCI type that is associated
 * with the given PCI endpoint.
 * @param [in] db_id
 * The doorbell identifier that will be used to map the doorbell to its handler. This value must be in the range of
 * [0, num_db - 1] when num_db is the number of doorbells configured to the associated DOCA devemu PCI endpoint.
 * @param [in] user_data_on_dpa
 * The user data that is associated with and can be retrieved by the DOCA devemu PCI doorbell DPA handle.
 * @param [out] db
 * The newly created DOCA devemu PCI doorbell.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'ep' or 'db_comp' or 'db' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'ep' or 'db_comp' are not started.
 * - DOCA_ERROR_NOT_PERMITTED - device was not created using doca_devemu_pci_*dev_create().
 * - DOCA_ERROR_FULL - The maximum number of doorbells that can be associated with the same DB completion object has
 * been reached.
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * @note There is no need to use this API if the datapath is implemented using dedicated offload engines.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_ep_create_db_on_dpa(struct doca_devemu_pci_ep *ep,
						 struct doca_devemu_pci_db_completion *db_comp,
						 uint8_t bar_id,
						 uint64_t bar_start_addr,
						 uint32_t db_id,
						 uint64_t user_data_on_dpa,
						 struct doca_devemu_pci_db **db);

/*********************************************************************************************************************
 * DOCA devemu pci MSIX
 *********************************************************************************************************************/

/**
 * @brief Allocate MSI-X for a DOCA devemu PCI endpoint.
 *
 * @details Only a single MSI-X instance for a given msix_idx is allowed per endpoint. Creating a second MSI-X with the
 * same msix_idx for the same ep is not permitted.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint to be associated with the MSI-X. Must be started.
 * @param [in] bar_id
 * The identifier of the BAR that contains the associated MSI-X table region for the created MSI-X. This value must
 * conform with the identifier that was configured to the MSI-X table region during the configuration cycle of the PCI
 * type that is associated with the given PCI endpoint.
 * @param [in] bar_start_addr
 * The start address of the associated MSI-X table region within the BAR. This value must conform with the start
 * address that was configured to the MSI-X table region during the configuration cycle of the PCI type that is
 * associated with the given PCI endpoint.
 * @param [in] msix_idx
 * The associated MSI-X table entry index.
 * @param [out] msix
 * The newly created DOCA devemu PCI device MSI-X.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'ep' or 'msix' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'ep' is not started.
 * - DOCA_ERROR_NOT_PERMITTED - 'ep' was not created using doca_devemu_pci_*dev_create().
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * - DOCA_ERROR_ALREADY_EXIST - MSI-X instance for the specified msix_idx already exists for the given endpoint.
 * @note There is no need to use this API if this MSI-X vector is handled by dedicated offload engines.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_ep_create_msix(struct doca_devemu_pci_ep *ep,
					    uint8_t bar_id,
					    uint64_t bar_start_addr,
					    uint16_t msix_idx,
					    struct doca_devemu_pci_msix **msix);

/**
 * @brief Raise MSI-X notification towards the driver.
 *
 * @param [in] msix
 * The DOCA devemu PCI MSI-X instance to be raised. Must not be NULL and must have been created using
 * doca_devemu_pci_ep_create_msix().
 */
DOCA_EXPERIMENTAL
void doca_devemu_pci_msix_raise(struct doca_devemu_pci_msix *msix);

/**
 * @brief Allocate MSI-X on the DPA for a DOCA devemu PCI endpoint.
 *
 * @details Only a single MSI-X instance for a given msix_idx is allowed per endpoint. Creating a second MSI-X with the
 * same msix_idx for the same ep is not permitted.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint to be associated with the MSI-X. Must be started.
 * @param [in] bar_id
 * The identifier of the BAR that contains the associated MSI-X table region for the created MSI-X. This value must
 * conform with the identifier that was configured to the MSI-X table region during the configuration cycle of the PCI
 * type that is associated with the given PCI endpoint.
 * @param [in] bar_start_addr
 * The start address of the associated MSI-X table region within the BAR. This value must conform with the start
 * address that was configured to the MSI-X table region during the configuration cycle of the PCI type that is
 * associated with the given PCI endpoint.
 * @param [in] msix_idx
 * The associated MSI-X table entry index.
 * @param [out] msix
 * The newly created DOCA devemu PCI device MSI-X.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'ep' or 'msix' is NULL, or 'ep' datapath was not set on DPA.
 * - DOCA_ERROR_BAD_STATE - 'ep' is not started.
 * - DOCA_ERROR_NOT_PERMITTED - 'ep' was not created using doca_devemu_pci_*dev_create().
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 * - DOCA_ERROR_ALREADY_EXIST - MSI-X instance for the specified msix_idx already exists for the given endpoint.
 * @note There is no need to use this API if this MSI-X vector is handled by dedicated offload engines.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_ep_create_msix_on_dpa(struct doca_devemu_pci_ep *ep,
						   uint8_t bar_id,
						   uint64_t bar_start_addr,
						   uint16_t msix_idx,
						   struct doca_devemu_pci_msix **msix);

/*********************************************************************************************************************
 * DOCA devemu MMAP
 *********************************************************************************************************************/

/**
 * @brief Allocates zero size memory map object with default/unset attributes associated with a DOCA devemu PCI
 * endpoint.
 *
 * @details The returned memory map object can be manipulated with common doca_mmap APIs.
 *
 * The created memory map object will cover a memory range in the domain that hosts the DOCA devemu PCI endpoint.
 *
 * @param [in] ep
 * The DOCA devemu PCI endpoint to be associated with the doca_mmap. Must be started.
 * @param [out] mmap
 * DOCA memory map structure with default/unset attributes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'ep' or 'mmap' is NULL.
 * - DOCA_ERROR_BAD_STATE - 'ep' is not started.
 * - DOCA_ERROR_NO_MEMORY - allocation failure.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_ep_mmap_create(struct doca_devemu_pci_ep *ep, struct doca_mmap **mmap);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_PCI_EP_H_ */
