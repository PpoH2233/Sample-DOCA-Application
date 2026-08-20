/*
 * Copyright (c) 2022-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_devemu_pci.h
 * @page doca_devemu_pci
 * @defgroup DOCA_DEVEMU DOCA Device Emulation
 * @defgroup DOCA_DEVEMU_PCI DOCA Device Emulation - PCI Devices
 * @ingroup DOCA_DEVEMU
 *
 * DOCA library for emulated PCI devices
 *
 * @{
 */

#ifndef DOCA_DEVEMU_PCI_H_
#define DOCA_DEVEMU_PCI_H_

#include <stdint.h>
#include <sys/uio.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_mmap.h>
#include <doca_ctx.h>
#include <doca_pe.h>
#include <doca_devemu_pci_type.h>
#include <doca_devemu_pci_ep.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing emulated PCI device.
 * This structure extends the functionality of doca_devemu_pci_ep.
 * This structure is used by PCI device emulation applications, libraries and services.
 */
struct doca_devemu_pci_dev;

/**
 * @brief Opaque structure representing emulated PCI resources.
 * This structure is used by PCI device emulation applications, libraries and services.
 */
struct doca_devemu_pci_resources;

/*********************************************************************************************************************
 * DOCA devemu pci Device Info Properties
 *********************************************************************************************************************/

/**
 * @brief Get the maximum number of PCI devices, across all PCI types, that can be hot-plugged by the device.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_hotplug_devices
 * Number of PCI devices that can be hot plugged by the device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_get_max_hotplug_devices(const struct doca_devinfo *devinfo,
							 uint32_t *max_hotplug_devices);

/**
 * @brief Check if adding the device to a DOCA mmap associated with a DOCA devemu PCI device is supported.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] supported
 * 1 if adding the device to a DOCA devemu PCI device associated DOCA mmap is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_cap_is_mmap_add_dev_supported(const struct doca_devinfo *devinfo, uint8_t *supported);

/*********************************************************************************************************************
 * DOCA devemu PCI device API
 *********************************************************************************************************************/

/**
 * @brief Allocate DOCA devemu PCI device.
 *
 * @param [in] pci_type
 * The DOCA PCI type to be associated with the device. Must be started and remain started throughout the lifetime
 * of the newly created 'pci_dev'.
 * @param [in] dev_rep
 * Representor DOCA device.
 * @param [in] pe
 * The progress engine that will be used to receive events and task completions.
 * @param [out] pci_dev
 * The newly created DOCA devemu PCI device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type', 'dev_rep', 'pe' or 'pci_dev' is NULL or representor type
 * does not match the PCI type.
 * - DOCA_ERROR_BAD_STATE - If 'pci_type' is not started. Use doca_devemu_pci_type_start() to start the 'pci_type'.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_*pci_type_create().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_create(struct doca_devemu_pci_type *pci_type,
					struct doca_dev_rep *dev_rep,
					struct doca_pe *pe,
					struct doca_devemu_pci_dev **pci_dev);

/**
 * @brief Allocate a started DOCA devemu PCI device.
 *
 * @details The returned device is started and uses default properties. This function is typically used in handover and
 * takeover scenarios, where the device properties were configured in a different (or crashed) process.
 *
 * @param [in] pci_type
 * The DOCA PCI type to be associated with the device. Must be started and remain started throughout the lifetime
 * of the newly created 'pci_dev'.
 * @param [in] dev_rep
 * Representor DOCA device.
 * @param [in] pe
 * The progress engine that will be used to receive events and task completions.
 * @param [in] user_data
 * User data to be passed to the state changed callback of the underlying doca_ctx.
 * @param [in] cb
 * Callback invoked when the underlying doca_ctx state changes.
 * @param [out] pci_dev
 * The newly created and started DOCA devemu PCI device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type', 'dev_rep', 'pe', 'cb' or 'pci_dev' is NULL or representor
 * type does not match the PCI type.
 * - DOCA_ERROR_BAD_STATE - If 'pci_type' is not started. Use doca_devemu_pci_type_start() to start the 'pci_type'.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_*pci_type_create().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_create_started(struct doca_devemu_pci_type *pci_type,
						struct doca_dev_rep *dev_rep,
						struct doca_pe *pe,
						union doca_data user_data,
						doca_ctx_state_changed_callback_t cb,
						struct doca_devemu_pci_dev **pci_dev);

/**
 * @brief Allocate a started DOCA devemu PCI device binded to DPA device.
 *
 * @details The returned device is started and uses default properties and its datapath bound to the specified
 * DPA device. This function is typically used in handover and takeover scenarios, where the device properties were
 * configured in a different (or crashed) process.
 *
 * @param [in] pci_type
 * The DOCA PCI type to be associated with the device. Must be started and remain started throughout the lifetime
 * of the newly created 'pci_dev'.
 * @param [in] dev_rep
 * Representor DOCA device.
 * @param [in] pe
 * The progress engine that will be used to receive events and task completions.
 * @param [in] dpa_dev
 * A pointer to a doca_dpa device. The datapath will be executed on this device.
 * @param [in] user_data
 * User data to be passed to the state changed callback of the underlying doca_ctx.
 * @param [in] cb
 * Callback invoked when the underlying doca_ctx state changes.
 * @param [out] pci_dev
 * The newly created and started DOCA devemu PCI device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type', 'dev_rep', 'pe', 'dpa_dev', 'cb' or 'pci_dev' is NULL or
 * representor type does not match the PCI type.
 * - DOCA_ERROR_BAD_STATE - If 'pci_type' is not started. Use doca_devemu_pci_type_start() to start the 'pci_type'.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_*pci_type_create().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_create_started_on_dpa(struct doca_devemu_pci_type *pci_type,
						       struct doca_dev_rep *dev_rep,
						       struct doca_pe *pe,
						       struct doca_dpa *dpa_dev,
						       union doca_data user_data,
						       doca_ctx_state_changed_callback_t cb,
						       struct doca_devemu_pci_dev **pci_dev);

/**
 * @brief Free a DOCA devemu PCI device created by doca_devemu_pci_dev_create*().
 *
 * @param [in] pci_dev
 * The previously created DOCA devemu PCI device. Must be idle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_dev' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'pci_dev' is not idle. Use doca_ctx_stop() to stop it.
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_dev' was not created using doca_devemu_pci_dev_create*().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_destroy(struct doca_devemu_pci_dev *pci_dev);

/**
 * @brief Get the PCI Device ID configured to DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query.
 * @param [out] device_id
 * The PCI Device ID (DID) assigned by the vendor.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'device_id' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_device_id(const struct doca_devemu_pci_dev *pci_dev, uint16_t *device_id);

/**
 * @brief Set the PCI Device ID of a specific DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to modify. Must be idle and not a static Physical Function (PF).
 * @param [in] device_id
 * The PCI Device ID (DID) to assign.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - pci_dev is not idle.
 * - DOCA_ERROR_NOT_PERMITTED - device is a static Physical Function (PF).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_set_device_id(struct doca_devemu_pci_dev *pci_dev, uint16_t device_id);

/**
 * @brief Get the PCI Vendor ID configured to DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query.
 * @param [out] vendor_id
 * The PCI Vendor ID (VID) assigned by the vendor.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'vendor_id' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_vendor_id(const struct doca_devemu_pci_dev *pci_dev, uint16_t *vendor_id);

/**
 * @brief Set the PCI Vendor ID of a specific DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to modify. Must be idle and not a static Physical Function (PF).
 * @param [in] vendor_id
 * The PCI Vendor ID (VID) to assign.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - pci_dev is not idle.
 * - DOCA_ERROR_NOT_PERMITTED - device is a static Physical Function (PF).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_set_vendor_id(struct doca_devemu_pci_dev *pci_dev, uint16_t vendor_id);

/**
 * @brief Get the PCI Subsystem ID configured to DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query.
 * @param [out] subsystem_id
 * The PCI Subsystem ID (SSID) assigned by the subsystem vendor.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'subsystem_id' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_subsystem_id(const struct doca_devemu_pci_dev *pci_dev, uint16_t *subsystem_id);

/**
 * @brief Set the PCI Subsystem ID of a specific DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to modify. Must be idle and not a static Physical Function (PF).
 * @param [in] subsystem_id
 * The PCI Subsystem ID (SSID) to assign.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - pci_dev is not idle.
 * - DOCA_ERROR_NOT_PERMITTED - device is a static Physical Function (PF).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_set_subsystem_id(struct doca_devemu_pci_dev *pci_dev, uint16_t subsystem_id);

/**
 * @brief Get the PCI Subsystem Vendor ID configured to DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query.
 * @param [out] subsystem_vid
 * The PCI Subsystem Vendor ID (SVID) assigned by the subsystem vendor.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'subsystem_vid' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_subsystem_vendor_id(const struct doca_devemu_pci_dev *pci_dev,
							 uint16_t *subsystem_vid);

/**
 * @brief Set the PCI Subsystem Vendor ID of a specific DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to modify. Must be idle and not a static Physical Function (PF).
 * @param [in] subsystem_vid
 * The PCI Subsystem Vendor ID (SVID) to assign.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - pci_dev is not idle.
 * - DOCA_ERROR_NOT_PERMITTED - device is a static Physical Function (PF).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_set_subsystem_vendor_id(struct doca_devemu_pci_dev *pci_dev, uint16_t subsystem_vid);

/**
 * @brief Get the PCI Revision ID configured to DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query.
 * @param [out] revision_id
 * The PCI Revision ID assigned by the vendor.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'revision_id' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_revision_id(const struct doca_devemu_pci_dev *pci_dev, uint8_t *revision_id);

/**
 * @brief Set the PCI Revision ID of a specific DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to modify. Must be idle and not a static Physical Function (PF).
 * @param [in] revision_id
 * The PCI Revision ID to assign.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - pci_dev is not idle.
 * - DOCA_ERROR_NOT_PERMITTED - device is a static Physical Function (PF).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_set_revision_id(struct doca_devemu_pci_dev *pci_dev, uint8_t revision_id);

/**
 * @brief Get the PCI Class Code configured to DOCA devemu PCI device to identify generic operation.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query.
 * @param [out] class_code
 * The PCI Class Code to identify generic operation. Only 24 LSBits are valid.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'class_code' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_class_code(const struct doca_devemu_pci_dev *pci_dev, uint32_t *class_code);

/**
 * @brief Set the PCI Class Code of a specific DOCA devemu PCI device to identify generic operation.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to modify. Must be idle and not a static Physical Function (PF).
 * @param [in] class_code
 * The PCI Class Code to identify generic operation. Only 24 LSBits are valid.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - pci_dev is not idle.
 * - DOCA_ERROR_NOT_PERMITTED - device is a static Physical Function (PF).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_set_class_code(struct doca_devemu_pci_dev *pci_dev, uint32_t class_code);

/**
 * @brief Configure if a DOCA devemu PCI device will assign MSI-X resources to it's VFs or not.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to modify. Must be idle, and a static Physical Function (PF).
 * @param [in] reset
 * If set to true, VFs will be assigned 0 MSI-X vectors on creation.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - device is not idle
 * - DOCA_ERROR_NOT_PERMITTED - device is not a static Physical Function (PF).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_reset_msix(struct doca_devemu_pci_dev *pci_dev, uint8_t reset);

/**
 * @brief Configure if a DOCA devemu PCI device will assign DB resources to it's VFs or not.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to modify. Must be idle, and a static Physical Function (PF).
 * @param [in] reset
 * If set to true, VFs will be assigned 0 DBs on creation.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - device is not idle
 * - DOCA_ERROR_NOT_PERMITTED - device is not a static Physical Function (PF).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_reset_db(struct doca_devemu_pci_dev *pci_dev, uint8_t reset);

/**
 * @brief Modify default registers values for stateful region in a DOCA devemu PCI device.
 *
 * @details This method will modify the default values for the entire stateful region registers area in a PCI device
 * BAR (before the first modification, the initial default values of the stateful region registers are taken from the
 * associated PCI type).
 * These values will override the previous default values and will become valid during the next exposure/hotplug of the
 * associated PCI device to the host or during the next FLR.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device. Must be started.
 * @param [in] id
 * The BAR id that contains the stateful region.
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
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_dev' or 'default_values' is NULL or 'id' >= max number of BARs according to the
 * PCI specification.
 * - DOCA_ERROR_BAD_STATE - If 'pci_dev' is not started.
 * - DOCA_ERROR_NOT_PERMITTED - If the PCI device was not created using doca_devemu_pci_dev_create*().
 * - DOCA_ERROR_NOT_SUPPORTED - If the PCI device does not support modifying stateful region default values.
 * - DOCA_ERROR_NOT_FOUND - If the stateful region was not found.
 * - DOCA_ERROR_DRIVER - Internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_modify_bar_stateful_region_default_values(struct doca_devemu_pci_dev *pci_dev,
									   uint8_t id,
									   uint64_t start_addr,
									   void *default_values,
									   uint64_t size);

/**
 * @brief Modify registers values for stateful region in a DOCA devemu PCI device.
 *
 * @details This method will modify the values of the stateful region registers in a PCI device BAR. These values will
 * override the existing values of the stateful region of the associated PCI device. Modifying registers by calling
 * this method will not trigger the registered event handler of the
 * doca_devemu_pci_dev_event_bar_stateful_region_driver_write event.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device. Must be started.
 * @param [in] id
 * The BAR id that contains the stateful region.
 * @param [in] offset
 * The offset of the registers region to modify within the BAR. Must be located within the stateful BAR region.
 * @param [in] values
 * Input buffer that contain the values data.
 * @param [in] size
 * The size of the values buffer in bytes. The (offset + size) must be located within the stateful BAR region.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_dev' or 'values' is NULL or 'id' >= max number of BARs according to the PCI
 * specification.
 * - DOCA_ERROR_BAD_STATE - If 'pci_dev' is not started.
 * - DOCA_ERROR_NOT_PERMITTED - If the PCI device was not created using doca_devemu_pci_dev_create*().
 * - DOCA_ERROR_NOT_SUPPORTED - If the PCI device does not support modifying stateful region values.
 * - DOCA_ERROR_NOT_FOUND - If the stateful region was not found.
 * - DOCA_ERROR_DRIVER - Internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_modify_bar_stateful_region_values(struct doca_devemu_pci_dev *pci_dev,
								   uint8_t id,
								   uint64_t offset,
								   void *values,
								   uint64_t size);

/**
 * @brief Query registers values of the stateful region in a DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device. Must be started.
 * @param [in] id
 * The BAR id that contains the stateful region.
 * @param [in] offset
 * The offset of the registers region to query within the BAR. Must be located within the stateful BAR region.
 * @param [out] out_values
 * Output buffer that will contain the values data upon success.
 * @param [in] size
 * The size of the out_values buffer in bytes. The (offset + size) must be located within the stateful BAR region.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_dev' or 'out_values' is NULL or 'id' >= max number of BARs according to the PCI
 * specification.
 * - DOCA_ERROR_BAD_STATE - If 'pci_dev' is not started.
 * - DOCA_ERROR_NOT_PERMITTED - If the PCI device was not created using doca_devemu_pci_dev_create*().
 * - DOCA_ERROR_NOT_SUPPORTED - If the PCI device does not support querying stateful region values.
 * - DOCA_ERROR_NOT_FOUND - If the stateful region was not found.
 * - DOCA_ERROR_DRIVER - Internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_query_bar_stateful_region_values(struct doca_devemu_pci_dev *pci_dev,
								  uint8_t id,
								  uint64_t offset,
								  void *out_values,
								  uint64_t size);

/**
 * @brief Convert DOCA devemu PCI device instance into DOCA context.
 *
 * @param [in] pci_dev
 * DOCA devemu PCI device. The device must remain valid until after the returned DOCA context is no longer required.
 *
 * @return
 * DOCA context upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_devemu_pci_dev_as_ctx(struct doca_devemu_pci_dev *pci_dev);

/**
 * @brief Query whether the DOCA devemu PCI device is having FLR (Function Level Reset).
 *
 * If true, all PCI I/O transactions to/from the host memory are disabled and the user should re-configure the
 * emulated PCI device. This re-configuration requires destruction of all the associated resources (e.g. DBs, MSIXs,
 * MMAPs), resetting the associated emulated PCI device (perform stop() and start() operations) and re-creating all the
 * needed resources.

 * @param [in] pci_dev
 * The DOCA devemu PCI device to query. Must be started.
 * @param [out] flr
 * 1 if the DOCA devemu PCI device is having FLR, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'flr' is NULL.
 * - DOCA_ERROR_BAD_STATE - PCI device is not started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_is_flr(const struct doca_devemu_pci_dev *pci_dev, uint8_t *flr);

/**
 * @brief Convert DOCA devemu PCI device instance into DOCA devemu PCI endpoint.
 *
 * @param [in] pci_dev
 * DOCA devemu PCI device. The device must remain valid until after the returned endpoint is no longer required.
 *
 * @return
 * DOCA devemu PCI endpoint upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_pci_ep *doca_devemu_pci_dev_as_ep(struct doca_devemu_pci_dev *pci_dev);

/**
 * @brief Get the configured Expansion ROM BAR size for the device.
 *
 * @details On success, returns the base‑2 logarithm of the Expansion ROM BAR size
 * in bytes. A value of 0x0 indicates that no Expansion ROM BAR is exposed for
 * this device.
 *
 * @param [in] devinfo_rep
 * The device to query. Must be a PF representor.
 * @param [out] log_exp_bar_size
 * The log2 size, in bytes, of the Expansion ROM BAR, or 0xFF if no such BAR is exposed.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo_rep' or 'log_exp_bar_size' is NULL.
 * - DOCA_ERROR_NOT_PERMITTED - If 'devinfo_rep' is not a PF representor.
 * - DOCA_ERROR_NOT_SUPPORTED - If querying the Expansion ROM BAR size is not supported
 *   for the provided device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_rep_get_log_exp_bar_size(const struct doca_devinfo_rep *devinfo_rep,
						      uint8_t *log_exp_bar_size);

/*********************************************************************************************************************
 * DOCA devemu PCI device hotplug API
 *********************************************************************************************************************/

/**
 * @brief DOCA devemu pci hotplug state.
 *
 * The steps for hotplug a device are:
 * 1. check the current hotplug state
 *     1.1 if state == DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF then issue hotplug operation
 *         (call doca_devemu_pci_dev_hotplug()) and wait for transition to DOCA_DEVEMU_PCI_HP_STATE_POWER_ON.
 *     1.2 if state == DOCA_DEVEMU_PCI_HP_STATE_PLUG_IN_PROGRESS then wait for transition to
 *         DOCA_DEVEMU_PCI_HP_STATE_POWER_ON.
 *     1.3 if state == DOCA_DEVEMU_PCI_HP_STATE_UNPLUG_IN_PROGRESS then wait for transition to
 *         DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF and go to step 1.1.
 *     1.4 if state == DOCA_DEVEMU_PCI_HP_STATE_POWER_ON then do nothing --> device is plugged.
 *
 * The steps for hot unplug a device are:
 * 1. check the current hotplug state
 *     1.1 if state == DOCA_DEVEMU_PCI_HP_STATE_POWER_ON then issue hot unplug operation
 *         (call doca_devemu_pci_dev_hotunplug()) and wait for transition to DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF.
 *     1.2 if state == DOCA_DEVEMU_PCI_HP_STATE_UNPLUG_IN_PROGRESS then wait for transition to
 *         DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF.
 *     1.3 if state == DOCA_DEVEMU_PCI_HP_STATE_PLUG_IN_PROGRESS then wait for transition to
 *         DOCA_DEVEMU_PCI_HP_STATE_POWER_ON and go to step 1.1 or issue hot unplug operation
 *         (call doca_devemu_pci_dev_hotunplug()) then wait for DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF.
 *     1.4 if state == DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF then do nothing --> device is un-plugged.
 *
 * @note It is recommended to use doca_devemu_pci_dev_event_hotplug_state_change mechanism to get notifications on
 * hotplug state changes.
 */
enum doca_devemu_pci_hotplug_state {
	/**< Device is powered off and not visible by the host. */
	DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF = 0,
	/**< Device is in transitional state to become un-plugged from host. */
	DOCA_DEVEMU_PCI_HP_STATE_UNPLUG_IN_PROGRESS,
	/**< Device is in transitional state to become plugged to host. */
	DOCA_DEVEMU_PCI_HP_STATE_PLUG_IN_PROGRESS,
	/**< Device is powered on and visible by the host. */
	DOCA_DEVEMU_PCI_HP_STATE_POWER_ON,
};

/**
 * @brief Get the hotplug state of the DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device. Must be started.
 * @param [out] state
 * The hotplug state of the given DOCA devemu PCI device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'state' is NULL
 * - DOCA_ERROR_BAD_STATE - PCI device is not started
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_hotplug_state(struct doca_devemu_pci_dev *pci_dev,
						   enum doca_devemu_pci_hotplug_state *state);

/**
 * @brief Get the total number of VFs that can be created for a PF.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query. Must be started.
 * @param [out] total_vfs
 * The number of VF the device can support.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'total_vfs' is NULL, pci_dev doesn't represent a PF device.
 * - DOCA_ERROR_BAD_STATE - PCI device is not started
 * - DOCA_ERROR_NOT_PERMITTED - device is NOT a static Physical Function (PF)
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_total_vfs(const struct doca_devemu_pci_dev *pci_dev, uint16_t *total_vfs);

/**
 * @brief Get the number of VFs configured by the host (sriov_numvfs).
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query.
 * @param [out] num_vfs
 * The number of VFs.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'num_vfs' is NULL.
 * - DOCA_ERROR_NOT_PERMITTED - device is NOT a static Physical Function (PF)
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_num_vfs(const struct doca_devemu_pci_dev *pci_dev, uint16_t *num_vfs);

/**
 * @brief Get the number of MSI-X vectors a PF can assign to its VFs.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query. Must be started.
 * @param [out] num_free_msix
 * The number of MSI-X vectors available for the device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'num_free_msix' is NULL.
 * - DOCA_ERROR_BAD_STATE - PCI device is not started.
 * - DOCA_ERROR_NOT_PERMITTED - device is NOT a static Physical Function (PF).
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_num_free_dynamic_vf_msix(const struct doca_devemu_pci_dev *pci_dev,
							      uint16_t *num_free_msix);

/**
 * @brief Get the number of DBs a PF can assign to its VFs.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device instance to query. Must be started.
 * @param [out] num_free_db
 * The number of doorbells available for the device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'num_free_db' is NULL.
 * - DOCA_ERROR_BAD_STATE - PCI device is not started.
 * - DOCA_ERROR_NOT_PERMITTED - device is NOT a static Physical Function (PF)
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_get_num_free_dynamic_vf_db(const struct doca_devemu_pci_dev *pci_dev,
							    uint16_t *num_free_db);

/**
 * @brief Get the hotplug state of a DOCA representor device.
 *
 * @details The specified representor device should be a hotplug device, which can be verified using
 * doca_devinfo_rep_get_is_hotplug(). Upon success, the current hotplug state is returned.
 *
 * @param [in] rep_dev
 * DOCA representor device instance.
 * @param [out] state
 * The hotplug state of the given DOCA representor device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'rep_dev' or 'state' is NULL
 * - DOCA_ERROR_NOT_PERMITTED - 'rep_dev' is not a hotplug device
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_rep_get_hotplug_state(struct doca_dev_rep *rep_dev,
						   enum doca_devemu_pci_hotplug_state *state);

/**
 * @brief Issue hotplug procedure of the DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device to hotplug. Must be started.
 *
 * @return
 * DOCA_SUCCESS - in case of success. On success, pci_dev is at DOCA_DEVEMU_PCI_HP_STATE_PLUG_IN_PROGRESS state.
 *                Event will not be raised in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - PCI device is not started, or hotplug state is not DOCA_DEVEMU_PCI_HP_STATE_POWER_OFF
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - The DOCA device that was set does not support hotplug, use
 *	doca_devemu_pci_cap_type_is_hotplug_supported() to find device that supports it
 * - DOCA_ERROR_AGAIN - host system is not ready, try again later
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_hotplug(struct doca_devemu_pci_dev *pci_dev);

/**
 * @brief Issue hot unplug procedure of the DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device to hot unplug. Must be started.
 *
 * @return
 * DOCA_SUCCESS - in case of success. On success, pci_dev is at DOCA_DEVEMU_PCI_HP_STATE_UNPLUG_IN_PROGRESS state.
 *                Event will not be raised in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' is NULL
 * - DOCA_ERROR_BAD_STATE - PCI device is not started, or current hotplug state is not one of
 *      DOCA_DEVEMU_PCI_HP_STATE_POWER_ON or DOCA_DEVEMU_PCI_HP_STATE_PLUG_IN_PROGRESS
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * - DOCA_ERROR_NOT_SUPPORTED - The DOCA device that was set does not support hot unplug, use
 *	doca_devemu_pci_cap_type_is_hotplug_supported() to find device that supports it
 * - DOCA_ERROR_AGAIN - host system is not ready, try again later
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_hotunplug(struct doca_devemu_pci_dev *pci_dev);

/*********************************************************************************************************************
 * DOCA devemu PCI device events
 *********************************************************************************************************************/

/**
 * @brief Function to be executed on hotplug state change event occurrence.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device that is associated with the event.
 * @param [in] user_data
 * Same user data that was provided in doca_devemu_pci_dev_event_hotplug_state_change_register().
 */
typedef void (*doca_devemu_pci_dev_event_hotplug_state_change_handler_cb_t)(struct doca_devemu_pci_dev *pci_dev,
									    union doca_data user_data);

/**
 * @brief Register to hotplug state changes.
 *
 * Registration can be done only while DOCA devemu PCI device is idle. If called multiple times then only the last call
 * will take effect.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 * @param [in] user_data
 * User data that will be provided to the handler once invoked.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'handler' is NULL
 * - DOCA_ERROR_BAD_STATE - PCI device is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_event_hotplug_state_change_register(
	struct doca_devemu_pci_dev *pci_dev,
	doca_devemu_pci_dev_event_hotplug_state_change_handler_cb_t handler,
	union doca_data user_data);

/**
 * @brief Opaque structure representing an asynchronous event for notifying upon PCI write transaction made by the
 * driver to a register located in the BAR stateful region.
 * Upon event registration the event is armed. Upon calling the registered event handler, the event becomes fired.
 * The event handler will be called to signal PCI writes for a single stateful region. The actual PCI write transaction
 * for the stateful region should be recognized by the service/application according to the device specification. For
 * example, by keeping the entire BAR stateful region copy and comparing it with the current BAR stateful region.
 * One can use doca_devemu_pci_dev_query_bar_stateful_region_values() to get the new values of the relevant stateful
 * region registers. One can update relevant stateful region registers by calling
 * doca_devemu_pci_dev_modify_bar_stateful_region_values().
 */
struct doca_devemu_pci_dev_event_bar_stateful_region_driver_write;

/**
 * @brief Function to be executed on PCI write transactions to BAR stateful region.
 *
 * @param [in] event
 * The BAR stateful region driver write event.
 * @param [in] user_data
 * Same user data that was provided in doca_devemu_pci_dev_event_bar_stateful_region_driver_write_register().
 */
typedef void (*doca_devemu_pci_dev_event_bar_stateful_region_driver_write_handler_cb_t)(
	struct doca_devemu_pci_dev_event_bar_stateful_region_driver_write *event,
	union doca_data user_data);

/**
 * @brief Register to BAR stateful region driver write event.
 *
 * Registration can be done only while DOCA devemu PCI device is idle. If called multiple times, for the same {pci_dev,
 * bar_id, bar_region_start_addr} tuple, then only the last call will take effect. The registration will be valid for
 * the entire stateful region that was configured for the associated DOCA devemu PCI device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 * @param [in] bar_id
 * The BAR id to be associated with the event. Must conform with the BAR stateful region configuration that was done
 * using doca_devemu_pci_type_set_bar_stateful_region_conf().
 * @param [in] bar_region_start_addr
 * The start address of the BAR stateful region to be associated with the event. Must conform with the BAR stateful
 * region configuration that was done using doca_devemu_pci_type_set_bar_stateful_region_conf().
 * @param [in] user_data
 * User data that will be provided to the handler once invoked.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'handler' is NULL
 * - DOCA_ERROR_BAD_STATE - PCI device is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_event_bar_stateful_region_driver_write_register(
	struct doca_devemu_pci_dev *pci_dev,
	doca_devemu_pci_dev_event_bar_stateful_region_driver_write_handler_cb_t handler,
	uint8_t bar_id,
	uint64_t bar_region_start_addr,
	union doca_data user_data);

/**
 * @brief Get DOCA devemu PCI device from BAR stateful region driver write event.
 *
 * @param [in] event
 * The registered BAR stateful region driver write event. Must not be NULL.
 *
 * @return
 * The DOCA devemu PCI device associated with the event.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_pci_dev *doca_devemu_pci_dev_event_bar_stateful_region_driver_write_get_pci_dev(
	struct doca_devemu_pci_dev_event_bar_stateful_region_driver_write *event);

/**
 * @brief Get the BAR id from BAR stateful region driver write event.
 *
 * @param [in] event
 * The registered BAR stateful region driver write event. Must not be NULL.
 *
 * @return
 * The BAR id that is associated with the event.
 */
DOCA_EXPERIMENTAL
uint8_t doca_devemu_pci_dev_event_bar_stateful_region_driver_write_get_bar_id(
	struct doca_devemu_pci_dev_event_bar_stateful_region_driver_write *event);

/**
 * @brief Get the BAR region start address that is associated with BAR stateful region driver write event.
 *
 * @param [in] event
 * The registered BAR stateful region driver write event. Must not be NULL.
 *
 * @return
 * The start address of the BAR stateful region that is associated with the event.
 */
DOCA_EXPERIMENTAL
uint64_t doca_devemu_pci_dev_event_bar_stateful_region_driver_write_get_bar_region_start_addr(
	struct doca_devemu_pci_dev_event_bar_stateful_region_driver_write *event);

/**
 * @brief Function to be executed on PCI FLR (Function Level Reset). The event handler will enable users to quiesce,
 * flush and reset the necessary resources associated with the emulated PCI device.
 * Upon event, all PCI I/O transactions to/from the host memory are disabled.
 * Additionally, the user should re-configure the emulated PCI device. This re-configuration requires flushing of all
 * the outstanding resources associated with the emulated PCI device, which were initially owned by the PCI device and
 * moved the ownership of the user. The re-configuration also requires destruction of all the associated resources
 * (e.g. DBs, MSIXs, MMAPs), resetting the associated emulated PCI device (perform stop() and start() operations) and
 * re-creating all the needed resources.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device that is associated with the event.
 * @param [in] user_data
 * Same user data that was provided in doca_devemu_pci_dev_event_flr_register().
 */
typedef void (*doca_devemu_pci_dev_event_flr_handler_cb_t)(struct doca_devemu_pci_dev *pci_dev,
							   union doca_data user_data);

/**
 * @brief Register to PCI FLR (Function Level Reset) event.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device to be associated with the event. Must be idle.
 * @param [in] handler
 * Method that is invoked once event is triggered.
 * @param [in] user_data
 * User data that will be provided to the handler once invoked.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'handler' is NULL
 * - DOCA_ERROR_BAD_STATE - PCI device is not idle
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_event_flr_register(struct doca_devemu_pci_dev *pci_dev,
						    doca_devemu_pci_dev_event_flr_handler_cb_t handler,
						    union doca_data user_data);

/**
 * @brief Function to be executed when SR-IOV is enabled or disabled by the host.
 *
 * When SR-IOV is disabled, the host writes 0 to the NumVFs register, resulting in
 * curr_numvfs being 0.
 *
 * When SR-IOV is enabled, the host writes the desired number of VFs to the NumVFs register.
 * This value is reflected in curr_numvfs, and prev_numvfs will be 0.
 *
 * The event handler allows the user to retrieve both the current and the previous values
 * of the number of VFs visible on the PCIe bus.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device associated with the event.
 * @param [in] prev_numvfs
 * The previous value of the NumVFs register.
 * @param [in] curr_numvfs
 * The current value of the NumVFs register.
 * @param [in] user_data
 * The same user data provided in doca_devemu_pci_dev_event_sriov_register().
 */
typedef void (*doca_devemu_pci_dev_event_sriov_handler_cb_t)(struct doca_devemu_pci_dev *pci_dev,
							     uint16_t prev_numvfs,
							     uint16_t curr_numvfs,
							     union doca_data user_data);

/**
 * @brief Register to an SR-IOV event.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI device to be associated with the event. Must be idle and a static Physical Function (PF).
 * @param [in] handler
 * Method that is invoked once event is triggered.
 * @param [in] user_data
 * User data that will be provided to the handler once invoked.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_dev' or 'handler' is NULL
 * - DOCA_ERROR_BAD_STATE - PCI device is not idle
 * - DOCA_ERROR_NOT_PERMITTED - PCI device is NOT a static Physical Function (PF)
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_dev_event_sriov_register(struct doca_devemu_pci_dev *pci_dev,
						      doca_devemu_pci_dev_event_sriov_handler_cb_t handler,
						      union doca_data user_data);

/*********************************************************************************************************************
 * DOCA devemu pci Doorbell
 *********************************************************************************************************************/

struct doca_dpa_thread;

/**
 * @brief Allocate DOCA devemu PCI device doorbell completion context on DPA. The created completion context will be
 * associated with a single dpa thread.
 *
 * @param [in] th
 * The DOCA dpa thread to be associated with the completion context.
 * @param [out] db_comp
 * The newly created DOCA devemu PCI device doorbell completion context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'th' or 'db_comp' is NULL
 * - DOCA_ERROR_NO_MEMORY - allocation failure
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_completion_create(struct doca_dpa_thread *th,
						  struct doca_devemu_pci_db_completion **db_comp);

/**
 * @brief Destroy the DOCA devemu PCI device doorbell completion context.
 *
 * The associated dpa handle will be destroyed as well.
 *
 * @param [in] db_comp
 * The DOCA devemu PCI device doorbell completion context to destroy. Must be stopped.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db_comp' is NULL
 * - DOCA_ERROR_BAD_STATE - 'db_comp' is not stopped
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_completion_destroy(struct doca_devemu_pci_db_completion *db_comp);

/**
 * @brief Start DOCA devemu PCI device doorbell completion context.
 *
 * @details On start verifies and finalizes the completion context configuration.
 *
 * The following is possible for started completion context:
 * - Associating DOCA devemu PCI device doorbells with the completion context.
 *
 * The following is NOT possible while completion context is started:
 * - Setting the properties of the completion context
 *
 * @param [in] db_comp
 * The DOCA devemu PCI device doorbell completion context to start.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db_comp' is NULL
 * - DOCA_ERROR_BAD_STATE - 'db_comp' is already started
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_completion_start(struct doca_devemu_pci_db_completion *db_comp);

/**
 * @brief Stop DOCA devemu PCI device doorbell completion context.
 *
 * @details On stop prevents execution of different operations and allows operations that were available before start.
 * For details, see doca_devemu_pci_db_completion_start(). Completion context can't be stopped while there are
 * DOCA devemu PCI device doorbells associated with it.
 *
 * The following is possible for stopped completion context:
 * - Setting the properties of the completion context
 *
 * The following is NOT possible while completion context is stopped:
 * - Associating DOCA devemu PCI device doorbells with the completion context.
 *
 * @param [in] db_comp
 * The DOCA devemu PCI device doorbell completion context to stop.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db_comp' is NULL
 * - DOCA_ERROR_BAD_STATE - 'db_comp' is already stopped
 * - DOCA_ERROR_IN_USE - 'db_comp' is still associated with one or more doorbells
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_completion_stop(struct doca_devemu_pci_db_completion *db_comp);

/**
 * @brief Get the DPA handle for the DOCA devemu PCI device doorbell completion context.
 *
 * @param [in] db_comp
 * The DOCA devemu PCI device doorbell completion context previously created on DPA.
 * @param [out] db_comp_handle
 * A pointer to the associated DPA handle in the dpa memory space.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db_comp' or 'db_comp_handle' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_completion_get_dpa_handle(struct doca_devemu_pci_db_completion *db_comp,
							  doca_dpa_dev_devemu_pci_db_completion_t *db_comp_handle);

/**
 * @brief Set the maximal number of doorbells that can be associated with the completion context.
 *
 * @param [in] db_comp
 * The DOCA devemu PCI device doorbell completion context to modify. Must be stopped.
 * @param [in] num_dbs
 * The maximal number of doorbells that can be associated with the context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db_comp' is NULL or 'num_dbs' is 0
 * - DOCA_ERROR_BAD_STATE - 'db_comp' is started
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_completion_set_max_num_dbs(struct doca_devemu_pci_db_completion *db_comp,
							   uint32_t num_dbs);

/**
 * @brief Get the maximal number of doorbells that can be associated with the completion context.
 *
 * @param [in] db_comp
 * The DOCA devemu PCI device doorbell completion context to query.
 * @param [out] num_dbs
 * The maximal number of doorbells that can be associated with the context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db_comp' or 'num_dbs' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_completion_get_max_num_dbs(struct doca_devemu_pci_db_completion *db_comp,
							   uint32_t *num_dbs);

/**
 * @brief Get the current number of doorbells that are associated with the completion context.
 *
 * @param [in] db_comp
 * The DOCA devemu PCI device doorbell completion context to query.
 * @param [out] num_dbs
 * The current number of doorbells that are associated with the context.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db_comp' or 'num_dbs' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_completion_get_curr_num_dbs(struct doca_devemu_pci_db_completion *db_comp,
							    uint32_t *num_dbs);

/**
 * @brief Destroy the DOCA devemu PCI device doorbell.
 *
 * If the doorbell was created on dpa, the associated dpa handle will be destroyed as well.
 *
 * @param [in] db
 * The DOCA devemu PCI device doorbell to destroy. Must be stopped.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db' is NULL
 * - DOCA_ERROR_BAD_STATE - 'db' is not stopped
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_destroy(struct doca_devemu_pci_db *db);

/**
 * @brief Get the DPA handle for the DOCA devemu PCI device doorbell.
 *
 * @param [in] db
 * The DOCA devemu PCI device doorbell previously created on DPA.
 * @param [out] db_handle
 * A pointer to the associated DPA handle in the dpa memory space.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db' or 'db_handle' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_get_dpa_handle(struct doca_devemu_pci_db *db, doca_dpa_dev_devemu_pci_db_t *db_handle);

/**
 * @brief Start DOCA devemu PCI device doorbell. A started doorbell will be able to trigger completions on the
 * associated doorbell completion context. Therefore, in case the doorbell was created on DPA, one should bind
 * the associated doorbell handle to its doorbell completion context before starting the doorbell.
 *
 * @param [in] db
 * The DOCA devemu PCI device doorbell to start.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db' is NULL
 * - DOCA_ERROR_BAD_STATE - 'db' is already started
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_start(struct doca_devemu_pci_db *db);

/**
 * @brief Stop DOCA devemu PCI device doorbell. A stopped doorbell will not trigger completions on the associated
 * doorbell completion context. Therefore, in case the doorbell was created on DPA, one should stop the doorbell before
 * un-binding the associated doorbell handle from its doorbell completion context.
 *
 * @param [in] db
 * The DOCA devemu PCI device doorbell to stop.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db' is NULL
 * - DOCA_ERROR_BAD_STATE - 'db' is already stopped
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_stop(struct doca_devemu_pci_db *db);

/**
 * @brief Modify the current value of DOCA devemu PCI device doorbell. If the doorbell is started, this setting will
 * behave as if the doorbell value was modified by the PCI device driver. If doorbell value will not be modified before
 * starting the DOCA devemu PCI device doorbell, the device will keep the current value of the doorbell.
 *
 * @param [in] db
 * The DOCA devemu PCI device doorbell to modify.
 * @param [in] db_value
 * The new value of the DOCA devemu PCI device doorbell.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_modify_value(struct doca_devemu_pci_db *db, uint32_t db_value);

/**
 * @brief Query the current value of DOCA devemu PCI device doorbell.
 *
 * @param [in] db
 * The DOCA devemu PCI device doorbell to query.
 * @param [out] db_value
 * The current value of the DOCA devemu PCI device doorbell.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'db' or 'db_value' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_db_query_value(struct doca_devemu_pci_db *db, uint32_t *db_value);

/*********************************************************************************************************************
 * DOCA devemu pci MSIX
 *********************************************************************************************************************/

/**
 * @brief Destroy the DOCA devemu PCI device MSI-X.
 *
 * If the MSI-X was created on dpa, the associated dpa handle will be destroyed as well.
 *
 * @param [in] msix
 * The DOCA devemu PCI device MSI-X context to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'msix' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_msix_destroy(struct doca_devemu_pci_msix *msix);

/**
 * @brief Get the DPA handle for the DOCA devemu PCI device MSI-X.
 *
 * @param [in] msix
 * The DOCA devemu PCI device MSI-X previously created on DPA.
 * @param [out] msix_handle
 * A pointer to the associated DPA handle in the dpa memory space.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'msix' or 'msix_handle' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_msix_get_dpa_handle(struct doca_devemu_pci_msix *msix,
						 doca_dpa_dev_devemu_pci_msix_t *msix_handle);

/*********************************************************************************************************************
 * DOCA devemu PCI resources
 *********************************************************************************************************************/

/**
 * @brief Retrieve available PCI resources for device usage.
 *
 * @details The returned structure represents the PCI resources that can be utilized by all DOCA devemu PCI devices and
 * PCI types associated with the specified device. These resources are global and shared across all supported devices.
 * The distribution of the available resources should comply with the relevant capabilities.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] pci_resources
 * The newly created DOCA devemu PCI resources structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'pci_resources' is NULL
 * - DOCA_ERROR_NO_MEMORY - allocation failure
 * - DOCA_ERROR_DRIVER - internal doca driver error
 * @note The returned structure must be deallocated using doca_devemu_pci_release_resources() to prevent memory leaks.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_get_available_resources(const struct doca_devinfo *devinfo,
						     struct doca_devemu_pci_resources **pci_resources);

/**
 * @brief Release the DOCA devemu PCI resources structure.
 *
 *
 * @param [in] pci_resources
 * PCI resources structure to be released.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_resources' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_release_resources(struct doca_devemu_pci_resources *pci_resources);

/**
 * @brief Retrieve the number of MSI-X vectors from a PCI resources structure.
 *
 * @param [in] pci_resources
 * The DOCA devemu PCI resources instance to query.
 * @param [out] num_msix
 * The number of MSI-X vectors associated with PCI resources structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_resources' or 'num_msix' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_resources_get_num_msix(const struct doca_devemu_pci_resources *pci_resources,
						    uint32_t *num_msix);

/**
 * @brief Retrieve the number of Doorbells from a PCI resources structure.
 *
 * @param [in] pci_resources
 * The DOCA devemu PCI resources instance to query.
 * @param [out] num_db
 * The number of Doorbells associated with PCI resources structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'pci_resources' or 'num_db' is NULL
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_resources_get_num_db(const struct doca_devemu_pci_resources *pci_resources,
						  uint32_t *num_db);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_PCI_H_ */
