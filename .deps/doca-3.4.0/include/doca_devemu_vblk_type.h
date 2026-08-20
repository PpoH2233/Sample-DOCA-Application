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
 * @file doca_devemu_vblk_type.h
 * @page doca_devemu_vblk_type
 * @defgroup DOCA_DEVEMU_VBLK_TYPES DOCA Device Emulation - Virtio Block Device Types
 * @ingroup DOCA_DEVEMU_VBLK
 *
 * DOCA Virtio Block type
 *
 * @{
 */

#ifndef DOCA_DEVEMU_VBLK_TYPE_H_
#define DOCA_DEVEMU_VBLK_TYPE_H_

#include <stdint.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_devemu_pci.h>
#include <doca_devemu_virtio.h>
#include <doca_devemu_virtio_type.h>
#include <doca_devemu_vblk.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA devemu Virtio Block type API
 *********************************************************************************************************************/

/**
 * @brief Check if the DOCA devemu PCI TLP type for Virtio Block emulation is supported by the device.
 *
 * @details Get uint8_t value defining if the device can be used to manage DOCA Virtio Block PCI TLP emulation.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] supported
 * 1 if the Virtio Block TLP type is supported by the device, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'supported' are NULL.
 * - DOCA_ERROR_DRIVER - internal doca driver error
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_cap_is_pci_tlp_type_supported(const struct doca_devinfo *devinfo,
							    uint8_t *supported);
/**
 * @brief Create a stopped DOCA devemu PCI TLP type for Virtio Block emulation.
 *
 * @details The following setter are allowed for Virtio Block PCI TLP emulation type:
 *  - doca_devemu_pci_type_set_dev()
 *  - doca_devemu_pci_type_set_num_msix()
 *  - doca_devemu_pci_type_set_num_db()
 *  - doca_devemu_pci_tlp_type_set_pci_cap_conf()
 *  - doca_devemu_pci_tlp_type_set_pcie_cap_conf()
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
 * - DOCA_ERROR_INVALID_VALUE - 'name' or 'pci_type' are NULL
 * - DOCA_ERROR_NO_MEMORY - allocation failure
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_vblk_pci_tlp_type_create(const char *name, struct doca_devemu_pci_type **pci_type);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_VBLK_TYPE_H_ */
