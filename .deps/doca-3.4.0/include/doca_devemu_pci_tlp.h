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
 * @file doca_devemu_pci_tlp.h
 * @page doca_devemu_pci_tlp
 * @defgroup DOCA_DEVEMU DOCA Device Emulation
 * @defgroup DOCA_DEVEMU_PCI_TLP DOCA Device Emulation - PCI TLP emulation
 * @ingroup DOCA_DEVEMU_PCI
 *
 * DOCA PCI TLP emulation
 *
 * @{
 */

#ifndef DOCA_DEVEMU_PCI_TLP_H_
#define DOCA_DEVEMU_PCI_TLP_H_

#include <stdint.h>

#include <doca_error.h>
#include <doca_dev.h>
#include <doca_ctx.h>
#include <doca_pe.h>
#include <doca_devemu_pci_type.h>
#include <doca_devemu_pci_ep.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing emulated PCI TLP channel.
 * This structure represents a logical path between a single PCI link on the host and the emulation software.
 * When the physical association between the device and the host PCI link is via a PCI switch, the TLP channel
 * includes all the downstream ports (DSPs) of that switch that are allocated for PCI TLP emulation.
 * This structure is used by PCI device emulation applications, libraries and services.
 */
struct doca_devemu_pci_tlp_channel;

/**
 * @brief Opaque structure representing emulated PCI TLP device.
 * This structure extends the functionality of doca_devemu_pci_ep.
 * This structure is used by PCI device emulation applications, libraries and services.
 */
struct doca_devemu_pci_tlp_dev;

/**
 * @brief Opaque structure representing emulated PCI TLP channel request.
 * This structure is used by PCI device emulation applications, libraries and services.
 */
struct doca_devemu_pci_tlp_channel_req;

/**
 * @brief DOCA devemu PCI TLP channel request opcodes.
 */
enum doca_devemu_pci_tlp_channel_req_opcode {
	/**< Incoming TLP request opcode. */
	DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP = 0x00,
	/**< Asynchronous Credit Grant (ACG) request opcode. */
	DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_ACG = 0x01,
	/**< PCI event request opcode. */
	DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_PCI_EVENT = 0x02,
	/**< INVALID request opcode. */
	DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_INVALID = 0x10000,
};

/**
 * @brief DOCA devemu PCI TLP PCI_EVENT request operation modes.
 */
enum doca_devemu_pci_tlp_channel_req_pci_event_opmode {
	/**< PERST# is asserted (enters reset). */
	DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_PCI_EVENT_OPMODE_PERST_ASSERT = 0x00,
	/**< PERST# is deasserted (released from reset). */
	DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_PCI_EVENT_OPMODE_PERST_DEASSERT = 0x01,
	/**< INVALID PCI_EVENT request operation mode. */
	DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_PCI_EVENT_OPMODE_INVALID = 0x10000,
};

/**
 * @brief DOCA devemu PCI TLP Asynchronous Credit Grant (ACG) completion operation modes.
 */
enum doca_devemu_pci_tlp_channel_req_acg_comp_opmode {
	/**< Flush ACG completion. */
	DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_ACG_COMP_OPMODE_FLUSH = 0x00,
	/**< MMIO WRITE ACG completion. */
	DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_ACG_COMP_OPMODE_MMIO_WRITE = 0x01,
};

/*********************************************************************************************************************
 * DOCA devemu PCI TLP Capabilities
 *********************************************************************************************************************/

/**
 * @brief Get the maximum number of PCI TLP emulation types that can be created by the device.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_tlp_types
 * Number of PCI types that can be created using doca_devemu_pci_tlp_type_create().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_tlp_types' is NULL
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 * - DOCA_ERROR_DRIVER - internal doca driver error
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_get_max_types(const struct doca_devinfo *devinfo, uint16_t *max_tlp_types);

/**
 * @brief Get the maximum number of PCI TLP PF (physical functions) devices supported by the device.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_devices
 * Number of PCI TLP PF devices that can be supported by the device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'max_devices' is NULL
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 * - DOCA_ERROR_DRIVER - internal doca driver error
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_get_max_pf_devices(const struct doca_devinfo *devinfo, uint32_t *max_devices);

/**
 * @brief Get the maximum allowed payload size (header + data), in bytes, that user can submit through a TLP channel
 * associated with the specified device for a completion operation.
 *
 * @details The returned size includes both header and data, allowing the user to construct a completion payload up to
 * this maximum size.
 *
 * For opcode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP, the total payload size that can be placed in the output of
 * doca_devemu_pci_tlp_channel_req_get_tlp_cpl_header() and doca_devemu_pci_tlp_channel_req_get_tlp_cpl_data() is
 * limited by this value.
 *
 * For opcode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_ACG, the total payload size that can be placed in the output of
 * doca_devemu_pci_tlp_channel_req_get_acg_buf() is limited by this value.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] size
 * The maximum allowed payload size, in bytes, for each completion operation (header + data).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'size' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 * - DOCA_ERROR_DRIVER - internal doca driver error
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_get_max_payload_size_per_tlp_channel_comp(const struct doca_devinfo *devinfo,
									       uint16_t *size);

/**
 * @brief Get the maximum number of outstanding Asynchronous Credit Grant (ACG) requests supported by the TLP channel
 * for the specified device.
 *
 * @details The Asynchronous Credit Grant (ACG) mechanism allows the device to proactively send credits through the
 * TLP emulation channel to the user.
 * The pre-registered request handler (set via doca_devemu_pci_tlp_channel_event_req_register()) will be invoked for
 * requests with opcode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_ACG, as retrieved by
 * doca_devemu_pci_tlp_channel_req_get_opcode().
 * The user may consume these credits at any later time as needed, by doca_devemu_pci_tlp_channel_req_complete_acg().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_acg
 * The maximum number of outstanding ACG requests that are supported for TLP channels associated with the specified
 * device. A value greater than 0 indicates that TLP channel requests with opcode
 * DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_ACG are supported. If supported, user can enable or disable ACG requests
 * using doca_devemu_pci_tlp_channel_set_acg_enabled(). The user should be prepared to handle 'max_acg' outstanding
 * ACGs from a TLP channel if ACG requests are enabled for that channel.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_acg' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported
 * - DOCA_ERROR_DRIVER - internal doca driver error
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_get_max_acg(const struct doca_devinfo *devinfo, uint16_t *max_acg);

/**
 * @brief Get the minimum Expansion ROM BAR size supported by the device, in Log base 2.
 *
 * @details Retrieves the minimum Expansion ROM BAR size (as a base‑2 logarithm of
 * the size in bytes) that can be configured for a device created with
 * doca_devemu_pci_type_create_rep_ex(), for any PCI type that is created using
 * doca_devemu_pci_tlp_type_create(). A value of 0 indicates that Expansion
 * ROM BAR configuration is not supported for this device.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_exp_bar_size
 * The minimal configurable Expansion ROM BAR size, given in bytes, of single Expansion ROM in Log (base 2) units.
 * A value of 0 means Expansion ROM configuration is not supported.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'log_exp_bar_size' is NULL.
 * - DOCA_ERROR_DRIVER - Internal DOCA driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_get_min_log_exp_bar_size(const struct doca_devinfo *devinfo,
							      uint8_t *log_exp_bar_size);

/**
 * @brief Get the maximum Expansion ROM BAR size supported by the device, in Log base 2.
 *
 * @details Retrieves the maximum Expansion ROM BAR size (as a base‑2 logarithm of
 * the size in bytes) that can be configured for a device created with
 * doca_devemu_pci_type_create_rep_ex(), for any PCI type that is created using
 * doca_devemu_pci_tlp_type_create(). A value of 0 indicates that Expansion
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
doca_error_t doca_devemu_pci_tlp_cap_get_max_log_exp_bar_size(const struct doca_devinfo *devinfo,
							      uint8_t *log_exp_bar_size);

/**
 * @brief MMIO TLP header fields according to PCI specification.
 */
enum doca_devemu_pci_tlp_mmio_hdr_f {
	/* Format of TLP */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_FMT = 0,
	/* Type of TLP */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_TYPE,
	/* Tag[9] (T9) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_T9,
	/* Traffic Class (TC) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_TC,
	/* Tag[8] (T8) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_T8,
	/* Attr[2] - Attribute bit */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_ATTR_2,
	/* Lightweight Notification (LN) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_LN,
	/* TLP Hints (TH) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_TH,
	/* TLP Digest (TD) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_TD,
	/* Error Poisoned (EP) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_EP,
	/* Attr[1:0] - Attributes */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_ATTR_1_0,
	/* Address Type (AT) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_AT,
	/* Length (data payload size) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_LENGTH,
	/* Requester ID */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_REQ_ID,
	/* Tag[7:0], can be in extended tag mode (paired with T8/T9) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_TAG,
	/* Last DW BE */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_LAST_DW_BE,
	/* First DW BE */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_FIRST_DW_BE,
	/* Address[31:2] */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_ADDRESS_31_2,
	/* Address[63:32] */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_ADDRESS_63_32,
	/* PH (Processing Hint) */
	DOCA_DEVEMU_PCI_TLP_MMIO_HDR_F_TLP_PH,
};

/**
 * @brief Check if a specific ingress MMIO READ TLP header field is valid.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [in] field
 * The TLP MMIO header field to check.
 * @param [out] valid
 * 1 if the specified field as part of an ingress MMIO READ TLP header is valid, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'valid' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * @note Invalid fields will be set to 0 before transferring ownership of the TLP request to the user.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_is_ingress_mmio_read_hdr_field_valid(const struct doca_devinfo *devinfo,
									  enum doca_devemu_pci_tlp_mmio_hdr_f field,
									  uint8_t *valid);

/**
 * @brief Check if a specific ingress MMIO WRITE TLP header field is valid.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [in] field
 * The TLP WRITE header field to check.
 * @param [out] valid
 * 1 if the specified field as part of an ingress MMIO WRITE TLP header is valid, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'valid' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 * @note Invalid fields will be set to 0 before transferring ownership of the TLP request to the user.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_is_ingress_mmio_write_hdr_field_valid(const struct doca_devinfo *devinfo,
									   enum doca_devemu_pci_tlp_mmio_hdr_f field,
									   uint8_t *valid);

/**
 * @brief Check if exposing a specific PCI capability in the PCI configuration space is supported for devices created
 * with doca_devemu_pci_tlp_dev_create*().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [in] cap_id
 * The PCI capability ID to check.
 * @param [out] supported
 * 1 if exposing the specified PCI capability in the PCI configuration space is supported by the device, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'supported' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_is_pci_cap_supported(const struct doca_devinfo *devinfo,
							  uint8_t cap_id,
							  uint8_t *supported);

/**
 * @brief Check if exposing a specific PCIe capability in the PCIe configuration space is supported for devices created
 * with doca_devemu_pci_tlp_dev_create*().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [in] cap_id
 * The PCIe capability ID to check.
 * @param [out] supported
 * 1 if exposing the specified PCIe capability in the PCIe configuration space is supported by the device, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'supported' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_is_pcie_cap_supported(const struct doca_devinfo *devinfo,
							   uint16_t cap_id,
							   uint8_t *supported);

/**
 * @brief Get the maximum number of BARs that can be configured to an emulated PCI TLP device, for any PCI type that
 * is created using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_bars
 * Number of BARs that can be configured for any PCI TLP type using doca_devemu_pci_type_*_bar_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_bars' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_max_bars(const struct doca_devinfo *devinfo, uint8_t *max_bars);

/**
 * @brief Get the maximum number of BAR regions that can be configured to an emulated PCI TLP device, for any PCI type
 * that is created using doca_devemu_pci_tlp_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_bar_regions
 * Number of BAR regions that can be configured per BAR using doca_devemu_pci_type_bar_*_region_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_bar_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_get_max_bar_regions(const struct doca_devinfo *devinfo,
							     uint32_t *max_bar_regions);

/**
 * @brief Get the maximum number of BAR regions that can be configured to an emulated PCI TLP device, for any PCI type
 * that is created using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_bar_regions
 * Number of BAR regions that can be configured using doca_devemu_pci_type_bar_*_region_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_bar_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_max_bar_regions(const struct doca_devinfo *devinfo,
							      uint32_t *max_bar_regions);

/**
 * @brief Get the minimal BAR size (in Log base 2) that can be configured for any PCI TLP type that is created
 * using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_bar_size
 * The minimal BAR size, given in bytes, of single BAR in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'log_bar_size' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_log_min_bar_size(const struct doca_devinfo *devinfo,
							       uint8_t *log_bar_size);

/**
 * @brief Get the maximum BAR size (in Log base 2) that can be configured for any PCI TLP type that is created
 * using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] log_bar_size
 * The maximal BAR size, given in bytes, of single BAR in Log (base 2) units.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'log_bar_size' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_log_max_bar_size(const struct doca_devinfo *devinfo,
							       uint8_t *log_bar_size);

/**
 * @brief Get the maximal number of MSIXs that can be configured for any PCI TLP device that is associated with a PCI
 * TLP type that is created using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] num_msix
 * The maximal number of MSIXs.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'num_msix' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_max_num_msix(const struct doca_devinfo *devinfo, uint16_t *num_msix);

/**
 * @brief Get the maximal number of doorbells that can be configured for any PCI TLP device that is associated with a
 * PCI TLP type that is created using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] num_db
 * The maximal number of doorbells.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'num_db' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_max_num_db(const struct doca_devinfo *devinfo, uint16_t *num_db);

/**
 * @brief Get the region block size of a doorbell BAR region that can be configured to an emulated PCI device, for any
 * PCI type that is created using doca_devemu_pci_tlp_type_create().
 * The region block size is the smallest allocation data unit for a BAR region. For example, if the region block size
 * is 64B then the bar region  size can be 64B/128B/192B/../N*64B (N = max num region blocks per doorbell BAR region).
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] block_size
 * Region block size, in bytes, of a doorbell BAR region that will be configured using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'block_size' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_db_region_get_region_block_size(const struct doca_devinfo *devinfo,
									 uint32_t *block_size);

/**
 * @brief Get the maximum number of region blocks of a single doorbell BAR region that can be configured for any PCI
 * type that is created using doca_devemu_pci_tlp_type_create().
 * The maximal number of region blocks together with the region block size defines the maximal size of a single
 * doorbell region.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_blocks
 * Maximal number of region blocks for a single doorbell BAR region that will be configured using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_blocks' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_db_region_get_max_num_region_blocks(const struct doca_devinfo *devinfo,
									     uint32_t *max_blocks);

/**
 * @brief Get the maximum amount of doorbell BAR regions that can be configured for any PCI type that is created
 * using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Maximal number of doorbell BAR regions that can be configured using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_max_bar_db_regions(const struct doca_devinfo *devinfo,
								 uint32_t *max_regions);

/**
 * @brief Get the maximum number of BAR doorbell regions that can be configured to an emulated PCI device, for any PCI
 * type that is created using doca_devemu_pci_tlp_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Number of BAR doorbell regions that can be configured per BAR using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_get_max_bar_db_regions(const struct doca_devinfo *devinfo,
								uint32_t *max_regions);

/**
 * @brief Get the doorbell BAR region start address alignment that can be configured for any PCI type that is created
 * using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] alignment
 * The start address alignment, in bytes, of doorbell BAR regions that can be configured using
 * doca_devemu_pci_type_bar_db_region_<*>_conf_set().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'alignment' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_db_region_get_start_addr_alignment(const struct doca_devinfo *devinfo,
									    uint32_t *alignment);

/**
 * @brief Get the region block size of a MSI-X table BAR region that can be configured to an emulated PCI device, for
 * any PCI type that is created using doca_devemu_pci_tlp_type_create().
 * The region block size is the smallest allocation data unit for a BAR region. For example, if the region block size
 * is 64B then the bar region size can be 64B/128B/192B/../N*64B (N = max num region blocks per MSI-X table BAR
 * region).
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] block_size
 * Region block size, in bytes, of a MSI-X table BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'block_size' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_msix_table_region_get_region_block_size(const struct doca_devinfo *devinfo,
										 uint32_t *block_size);

/**
 * @brief Get the maximum number of region blocks of a single MSI-X table BAR region that can be configured for any PCI
 * type that is created using doca_devemu_pci_tlp_type_create().
 * The maximal number of region blocks together with the region block size defines the maximal size of a single
 * MSI-X table region.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_blocks
 * Maximal number of region blocks for a single MSI-X table BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_blocks' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_msix_table_region_get_max_num_region_blocks(const struct doca_devinfo *devinfo,
										     uint32_t *max_blocks);

/**
 * @brief Get the maximum amount of MSI-X table BAR regions that can be configured for any PCI type that is created
 * using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Maximal number of MSI-X table BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_max_bar_msix_table_regions(const struct doca_devinfo *devinfo,
									 uint32_t *max_regions);

/**
 * @brief Get the maximum number of BAR MSI-X table regions that can be configured to an emulated PCI device, for any
 * PCI type that is created using doca_devemu_pci_tlp_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Number of BAR MSI-X table regions that can be configured per BAR using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_get_max_bar_msix_table_regions(const struct doca_devinfo *devinfo,
									uint32_t *max_regions);

/**
 * @brief Get the MSI-X table BAR region start address alignment that can be configured for any PCI type that is
 * created using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] alignment
 * The start address alignment, in bytes, of MSI-X table BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_msix_table_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'alignment' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_msix_table_region_get_start_addr_alignment(const struct doca_devinfo *devinfo,
										    uint32_t *alignment);

/**
 * @brief Get the region block size of a MSI-X PBA BAR region that can be configured to an emulated PCI device, for any
 * PCI type that is created using doca_devemu_pci_tlp_type_create().
 * The region block size is the smallest allocation data unit for a BAR region. For example, if the region block size
 * is 64B then the bar region size can be 64B/128B/192B/../N*64B (N = max num region blocks per MSI-X PBA BAR region).
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] block_size
 * Region block size, in bytes, of a MSI-X PBA BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'block_size' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_msix_pba_region_get_region_block_size(const struct doca_devinfo *devinfo,
									       uint32_t *block_size);

/**
 * @brief Get the maximum number of region blocks of a single MSI-X PBA BAR region that can be configured for any PCI
 * type that is created using doca_devemu_pci_tlp_type_create().
 * The maximal number of region blocks together with the region block size defines the maximal size of a single
 * MSI-X PBA region.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_blocks
 * Maximal number of region blocks for a single MSI-X PBA BAR region that will be configured using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_blocks' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_msix_pba_region_get_max_num_region_blocks(const struct doca_devinfo *devinfo,
										   uint32_t *max_blocks);

/**
 * @brief Get the maximum amount of MSI-X PBA BAR regions that can be configured for any PCI type that is created
 * using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Maximal number of MSI-X PBA BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_max_bar_msix_pba_regions(const struct doca_devinfo *devinfo,
								       uint32_t *max_regions);

/**
 * @brief Get the maximum number of BAR MSI-X PBA regions that can be configured to an emulated PCI device, for any PCI
 * type that is created using doca_devemu_pci_tlp_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Number of BAR MSI-X PBA regions that can be configured per BAR using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_get_max_bar_msix_pba_regions(const struct doca_devinfo *devinfo,
								      uint32_t *max_regions);

/**
 * @brief Get the MSI-X PBA BAR region start address alignment that can be configured for any PCI type that is created
 * using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] alignment
 * The start address alignment, in bytes, of MSI-X PBA BAR regions that can be configured using
 * doca_devemu_pci_type_set_bar_msix_pba_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'alignment' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_msix_pba_region_get_start_addr_alignment(const struct doca_devinfo *devinfo,
										  uint32_t *alignment);

/**
 * @brief Get the region block size of a transaction BAR region that can be configured to an emulated PCI device, for
 * any PCI type that is created using doca_devemu_pci_tlp_type_create(). The region block size is the smallest
 * allocation data unit for a BAR region. For example, if the region block size is 64B then the bar region size can be
 * 64B/128B/192B/../N*64B (N = max num region blocks per transaction BAR region).
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] block_size
 * Region block size, in bytes, of a transaction BAR region that will be configured using
 * doca_devemu_pci_tlp_type_set_bar_transaction_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'block_size' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_transaction_region_get_region_block_size(const struct doca_devinfo *devinfo,
										  uint32_t *block_size);

/**
 * @brief Get the maximum number of region blocks of a single transaction BAR region that can be configured for any PCI
 * type that is created using doca_devemu_pci_tlp_type_create(). The maximal number of region blocks together with the
 * region block size defines the maximal size of a single transaction region.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_blocks
 * Maximal number of region blocks for a single transaction BAR region that will be configured using
 * doca_devemu_pci_tlp_type_set_bar_transaction_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_blocks' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_transaction_region_get_max_num_region_blocks(const struct doca_devinfo *devinfo,
										      uint32_t *max_blocks);

/**
 * @brief Get the maximum amount of transaction BAR regions that can be configured for any PCI type that is created
 * using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Maximal number of transaction BAR regions that can be configured using
 * doca_devemu_pci_tlp_type_set_bar_transaction_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_get_max_bar_transaction_regions(const struct doca_devinfo *devinfo,
									  uint32_t *max_regions);

/**
 * @brief Get the maximum number of BAR transaction regions that can be configured to an emulated PCI device, for any
 * PCI type that is created using doca_devemu_pci_tlp_type_create(), per BAR.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] max_regions
 * Number of BAR transaction regions that can be configured per BAR using
 * doca_devemu_pci_tlp_type_set_bar_transaction_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'max_regions' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_get_max_bar_transaction_regions(const struct doca_devinfo *devinfo,
									 uint32_t *max_regions);

/**
 * @brief Get the transaction BAR region start address alignment that can be configured for any PCI type that is created
 * using doca_devemu_pci_tlp_type_create().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] alignment
 * The start address alignment, in bytes, of transaction BAR regions that can be configured using
 * doca_devemu_pci_tlp_type_set_bar_transaction_region_conf().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'alignment' is NULL.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_bar_transaction_region_get_start_addr_alignment(const struct doca_devinfo *devinfo,
										     uint32_t *alignment);

/**
 * @brief Get the BAR memory types capability of the device. If supported, A BAR with that memory type can be
 * configured using doca_devemu_pci_type_set_memory_bar_conf() for any PCI type that is created using
 * doca_devemu_pci_tlp_type_create().
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
 * - DOCA_ERROR_INVALID_VALUE - If 'devinfo' or 'supported' is NULL, or 'memory_type' is invalid.
 * - DOCA_ERROR_NOT_SUPPORTED - query the capability for the provided device is not supported.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_type_is_bar_mem_type_supported(const struct doca_devinfo *devinfo,
								    enum doca_devemu_pci_bar_mem_type memory_type,
								    uint8_t *supported);

/**
 * @brief Check whether the device supports PCI types created by doca_devemu_pci_tlp_type_create().
 *
 * @details If 'supported' is set to 1, the device can be associated with a type created by
 * doca_devemu_pci_tlp_type_create() using doca_devemu_pci_type_set_dev().
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] supported
 * 1 if the capability is supported for types created by doca_demu_pci_tlp_type_create(). Otherwise 0.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'devinfo' or 'supported' is NULL
 * - DOCA_ERROR_DRIVER - internal doca driver error
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_cap_is_type_supported(const struct doca_devinfo *devinfo, uint8_t *supported);

/*********************************************************************************************************************
 * DOCA devemu PCI TLP type API
 *********************************************************************************************************************/

/**
 * @brief Create a stopped DOCA devemu PCI type for TLP emulation.
 *
 * @details The following setters are allowed for PCI TLP emulation type:
 *  - doca_devemu_pci_type_set_dev()
 *  - doca_devemu_pci_type_set_num_msix()
 *  - doca_devemu_pci_type_set_num_db()
 *  - doca_devemu_pci_type_set_memory_bar_conf()
 *  - doca_devemu_pci_type_set_io_bar_conf()
 *  - doca_devemu_pci_type_set_bar_db_region_by_offset_conf()
 *  - doca_devemu_pci_type_set_bar_db_region_by_data_conf()
 *  - doca_devemu_pci_type_set_bar_msix_table_region_conf()
 *  - doca_devemu_pci_type_set_bar_msix_pba_region_conf()
 *  - doca_devemu_pci_tlp_type_set_bar_transaction_region_conf()
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
 * - DOCA_ERROR_INVALID_VALUE - 'name' or 'pci_type' is NULL
 * - DOCA_ERROR_NO_MEMORY - allocation failure
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_type_create(const char *name, struct doca_devemu_pci_type **pci_type);

/**
 * @brief Query whether the DOCA devemu PCI type is a TLP emulation type, created using
 * doca_devemu_pci_tlp_type_create().
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type to query.
 * @param [out] tlp
 * 1 if the DOCA devemu PCI type is a TLP emulation type, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if 'pci_type' or 'tlp' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_type_is_tlp(const struct doca_devemu_pci_type *pci_type, uint8_t *tlp);

/**
 * @brief Set a PCI capability for a DOCA devemu PCI TLP type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type. Must not be started.
 * @param [in] id
 * The PCI capability ID.
 * @param [in] offset
 * The offset within the PCI configuration space where this capability should be exposed for TLP devices that will be
 * associated with the pci_type.
 * @param [in] length
 * The size, in bytes, of the capability structure located in the PCI configuration space.
 * The capability structure spans from the given 'offset' to 'offset' + 'length' - 1.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type' is NULL, or at least one of the following applies:
 *   - 'id' is invalid according to the PCI specification.
 *   - 'offset' does not fall within the capabilities range according to the PCI specification.
 *   - 'offset' + 'length' exceeds the capabilities range according to the PCI specification.
 * - DOCA_ERROR_BAD_STATE - If 'pci_type' is started.
 * @note This configuration is applicable only for type created by doca_devemu_pci_tlp_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_type_set_pci_cap_conf(struct doca_devemu_pci_type *pci_type,
						       uint8_t id,
						       uint16_t offset,
						       uint16_t length);

/**
 * @brief Set a PCIe capability for a DOCA devemu PCI TLP type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI type. Must not be started.
 * @param [in] id
 * The PCIe capability ID.
 * @param [in] offset
 * The offset within the PCI configuration space where this capability should be exposed for TLP devices that will be
 * associated with the pci_type.
 * @param [in] length
 * The size, in bytes, of the capability structure located in the PCI configuration space.
 * The capability structure spans from the given 'offset' to 'offset' + 'length' - 1.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type' is NULL, or at least one of the following applies:
 *   - 'id' is invalid according to the PCI specification.
 *   - 'offset' does not fall within the capabilities range according to the PCIe specification.
 *   - 'offset' + 'length' exceeds the capabilities range according to the PCIe specification.
 * - DOCA_ERROR_BAD_STATE - If 'pci_type' is started.
 * @note This configuration is applicable only for type created by doca_devemu_pci_tlp_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_type_set_pcie_cap_conf(struct doca_devemu_pci_type *pci_type,
							uint16_t id,
							uint16_t offset,
							uint16_t length);

/**
 * @brief Set a transaction BAR region configuration for a BAR layout in a DOCA devemu PCI TLP type.
 *
 * @param [in] pci_type
 * The DOCA devemu PCI TLP type to modify. Must not be started.
 * @param [in] id
 * The BAR id that will contain the new region.
 * @param [in] start_addr
 * The start address of the region within the BAR. This value must conform with the start address alignment capability
 * from doca_devemu_pci_*cap_bar_transaction_region_get_start_addr_alignment().
 * @param [in] size
 * The size of the region in bytes. Must conform with
 * doca_devemu_pci_*cap_bar_transaction_region_get_region_block_size() and
 * doca_devemu_pci_*cap_bar_transaction_region_get_max_num_region_blocks().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type' is NULL, 'id' >= max number of BARs according to the PCI specification.
 * - DOCA_ERROR_BAD_STATE - If 'pci_type' is started.
 * - DOCA_ERROR_NOT_PERMITTED - The PCI type was not created using doca_devemu_pci_tlp_type_create().
 * @note This configuration is applicable only for PCI types created by doca_devemu_pci_tlp_type_create()
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_type_set_bar_transaction_region_conf(struct doca_devemu_pci_type *pci_type,
								      uint8_t id,
								      uint64_t start_addr,
								      uint64_t size);

/*********************************************************************************************************************
 * DOCA devemu PCI TLP channel API
 *********************************************************************************************************************/

/**
 * @brief Allocate a DOCA devemu PCI TLP channel.
 *
 * @details The DOCA devemu PCI TLP channel extends the DOCA Context base class and follows the standard DOCA Context
 * state machine. When created, the TLP channel is in the DOCA_CTX_STATE_IDLE state. After completing all configuration
 * for the TLP channel, the user should call doca_ctx_start() to move it towards the fully operational
 * DOCA_CTX_STATE_RUNNING state.
 *
 * If a non-empty shared memory directory path has been set with doca_devemu_pci_tlp_channel_set_shm_dir_path(), once
 * doca_ctx_start() has been called for the first time after setting this path, the path cannot be changed for the
 * remainder of the channel's lifetime.
 *
 * Before destroying a TLP channel that is in the DOCA_CTX_STATE_RUNNING state, the user must first call
 * doca_ctx_stop(). The destruction flow depends on the return value of doca_ctx_stop():
 * - If doca_ctx_stop() returns DOCA_ERROR_IN_PROGRESS (the TLP channel moved to DOCA_CTX_STATE_STOPPING state):
 *   1. Flush all pending requests from the TLP channel by calling doca_pe_progress() on the associated doca_pe until
 *      the call return 0.
 *   2. Complete all outstanding requests that are owned by the user by calling
 *      doca_devemu_pci_tlp_channel_req_complete_*().
 *   3. Progress the associated doca_pe by calling doca_pe_progress() until the TLP channel state transitions to
 *      DOCA_CTX_STATE_IDLE.
 *   4. Call doca_devemu_pci_tlp_channel_destroy().
 *
 * - If doca_ctx_stop() returns DOCA_SUCCESS (the TLP channel moved to DOCA_CTX_STATE_IDLE state):
 *   1. Call doca_devemu_pci_tlp_channel_destroy().
 *
 * @param [in] dev
 * The DOCA device to be associated with the TLP channel. This must remain valid for the lifetime of the PCI TLP
 * channel.
 * @param [out] channel
 * The newly created DOCA devemu PCI TLP channel.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'dev' or 'channel' is NULL.
 * - DOCA_ERROR_NO_MEMORY - allocation failure
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_create(struct doca_dev *dev, struct doca_devemu_pci_tlp_channel **channel);

/**
 * @brief Free a DOCA devemu PCI TLP channel.
 *
 * @param [in] channel
 * The previously created DOCA devemu PCI TLP channel. Must be idle.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'channel' is not idle. Use doca_ctx_stop() to stop it.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_destroy(struct doca_devemu_pci_tlp_channel *channel);

/**
 * @brief Export the runtime state representation of a TLP channel.
 *
 * @details The produced export descriptor is an opaque blob that contains the runtime state representation of the
 * TLP channel. It can be transferred to another process and used there with
 * doca_devemu_pci_tlp_channel_create_from_export() to obtain a TLP channel that reflects the same runtime state as
 * the exported channel. Typical use cases include handover scenarios where the destination process needs to access the
 * runtime state of the TLP channel originally created by the source process.
 *
 * Must be called on a started TLP channel that was configured with non-empty shared memory directory path.
 *
 * The returned export descriptor pointer must not be used after the TLP channel is stopped. Once the TLP channel is
 * stopped, the export descriptor memory is invalidated. If the export descriptor pointer is used after the TLP channel
 * is stopped, the behavior is undefined.
 * If the TLP channel is started again, the export descriptor should be re-created by calling this function again.
 *
 * The caller must not release the returned export descriptor pointer.
 *
 * @param [in] channel
 * The DOCA devemu PCI TLP channel to export. Must be started and configured with non-empty shared memory directory
 * path.
 * @param [out] export_desc
 * A blob containing the runtime state representation of the TLP channel. Valid only upon success.
 * @param [out] export_desc_len
 * Length in bytes of export_desc. Valid only upon success.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel', 'export_desc', or 'export_desc_len' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'channel' is not started.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_NOT_PERMITTED - If 'channel' was not configured with non-empty shared memory directory path.
 * @note The exported data contains sensitive information. Pass the export descriptor through a secure channel.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_export(struct doca_devemu_pci_tlp_channel *channel,
						const void **export_desc,
						size_t *export_desc_len);

/**
 * @brief Create a DOCA devemu PCI TLP channel from an export descriptor produced by
 * doca_devemu_pci_tlp_channel_export().
 *
 * @details This function is typically used on the destination side during a handover. The export_desc parameter
 * contains the runtime state representation of the exported channel. The created channel, if configured correctly,
 * will reflect the same runtime state as the exported channel, once become primary.
 *
 * The shm_dir_path must match the path configured on the exported channel, otherwise the behavior is undefined.
 * This path is fixed for the lifetime of the created channel.
 *
 * A TLP channel successfully created from an export descriptor can be exported again using
 * doca_devemu_pci_tlp_channel_export().
 *
 * The following are NOT possible for a channel created from export:
 * - Setting a shared memory directory path using doca_devemu_pci_tlp_channel_set_shm_dir_path().
 * - Enable/Disable ACG using doca_devemu_pci_tlp_channel_set_acg_enabled(). The ACG configuration is derived from the
 *   exported channel.
 *
 * @param [in] export_desc
 * Export descriptor produced by doca_devemu_pci_tlp_channel_export(), typically from the source channel.
 * @param [in] export_desc_len
 * Length in bytes of export_desc.
 * @param [in] shm_dir_path
 * Absolute directory path for shared memory (i.e., must start with '/'). Length must not exceed the value returned by
 * doca_devemu_pci_tlp_channel_cap_get_max_shm_dir_path_len(). Must match the path configured on the exported channel.
 * @param [in] dev
 * The DOCA device to be associated with the TLP channel. This must remain valid for the lifetime of the PCI TLP
 * channel.
 * @param [out] channel
 * The newly created idle DOCA devemu PCI TLP channel.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'export_desc', 'shm_dir_path', 'dev', or 'channel' is NULL, or 'export_desc_len' is
 *   0, or 'shm_dir_path' exceeds max length, or 'shm_dir_path' is not absolute.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_NOT_FOUND - If 'export_desc' or 'export_desc_len' does not correspond to an exported channel.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_create_from_export(const void *export_desc,
							    size_t export_desc_len,
							    const char *shm_dir_path,
							    struct doca_dev *dev,
							    struct doca_devemu_pci_tlp_channel **channel);

/**
 * @brief Convert DOCA devemu PCI TLP channel instance into DOCA context.
 *
 * @details After configuring the properties of a TLP channel context, the user should start it to make it operational.
 * An operational (started) channel context can be associated with PCI TLP devices.
 * When operational, the channel context will invoke pre-registered callback handlers upon receiving incoming TLPs.
 * A channel context can only be stopped if there are no associated TLP PCI devices.
 *
 * @param [in] channel
 * The DOCA devemu PCI TLP channel. The channel must remain valid until the returned context is no longer required.
 *
 * @return
 * DOCA context upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_ctx *doca_devemu_pci_tlp_channel_as_ctx(struct doca_devemu_pci_tlp_channel *channel);

/**
 * @brief Get the configured size of the user data buffer that will be allocated for each
 * doca_devemu_pci_tlp_channel_req on behalf of the user. This buffer will be valid and can used by the user upon
 * receiving new doca_devemu_pci_tlp_channel_req. The buffer will become invalid after doca_devemu_pci_tlp_channel_req
 * completion.
 *
 * @param [in] channel
 * The DOCA devemu PCI TLP channel instance to query.
 * @param [out] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each
 * doca_devemu_pci_tlp_channel_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - 'channel' or 'req_user_data_size' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_get_req_user_data_size(const struct doca_devemu_pci_tlp_channel *channel,
								uint32_t *req_user_data_size);

/**
 * @brief Set the size of the user data buffer that will be allocated for each doca_devemu_pci_tlp_channel_req on
 * behalf of the user. This buffer will be valid and can used by the user upon receiving new
 * doca_devemu_pci_tlp_channel_req. The buffer will become invalid after doca_devemu_pci_tlp_channel_req completion.
 *
 * @param [in] channel
 * The DOCA devemu PCI TLP channel instance to modify. Must be idle.
 * @param [in] req_user_data_size
 * Size, in bytes, of the user data buffer to be allocated on behalf of the user for each
 * doca_devemu_pci_tlp_channel_req.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'channel' is not idle. Use doca_ctx_stop() to stop it.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_set_req_user_data_size(struct doca_devemu_pci_tlp_channel *channel,
								uint32_t req_user_data_size);

/**
 * @brief Get the maximum length (in bytes, including null terminator) for the shared memory directory path.
 *
 * @details Use this cap to size buffers before calling doca_devemu_pci_tlp_channel_set_shm_dir_path(),
 * doca_devemu_pci_tlp_channel_create_from_export(), or doca_devemu_pci_tlp_channel_get_shm_dir_path(). The path
 * string may contain up to max_shm_dir_path_len - 1 characters plus the null terminator.
 *
 * @param [out] max_shm_dir_path_len
 * The maximum length in bytes for the shm_dir_path string (including null terminator).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'max_shm_dir_path_len' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_cap_get_max_shm_dir_path_len(uint32_t *max_shm_dir_path_len);

/**
 * @brief Set the directory path for TLP channel shared memory.
 *
 * @details Sets the shared memory directory path. The path is used to create shared memory files for internal usage.
 * A TLP channel with non-empty shm_dir_path set can be exported using doca_devemu_pci_tlp_channel_export().
 * Passing an empty string will disable previous configuration and the channel will not be able to export.
 *
 * This function can't be called for a channel created with doca_devemu_pci_tlp_channel_create_from_export().
 *
 * For a channel created with doca_devemu_pci_tlp_channel_create(), once a non-empty path has been set by a previous
 * call to this function followed by a call to doca_ctx_start(), it must not be changed for the remainder of the
 * channel's lifetime.
 *
 * The directory referenced by shm_dir_path may be used for datapath control shared memory and therefore it is
 * recommended to reside in a memory-backed storage (for example, a tmpfs-based directory such as /dev/shm) to ensure
 * low-latency access.
 *
 * @param [in] channel
 * The TLP channel to modify. Must be idle.
 * @param [in] shm_dir_path
 * Absolute directory path for shared memory (i.e., must start with '/'). Length (including null terminator) must not
 * exceed the value returned by doca_devemu_pci_tlp_channel_cap_get_max_shm_dir_path_len().
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' or 'shm_dir_path' is NULL, or 'shm_dir_path' exceeds max length, or
 *   'shm_dir_path' is not absolute.
 * - DOCA_ERROR_BAD_STATE - If 'channel' is not idle. Use doca_ctx_stop() to stop it.
 * - DOCA_ERROR_NOT_SUPPORTED - If the channel was created with doca_devemu_pci_tlp_channel_create_from_export().
 * - DOCA_ERROR_NOT_PERMITTED - If a non-empty path has already been used by a previous call to
 *   doca_devemu_pci_tlp_channel_set_shm_dir_path() followed by a call to doca_ctx_start().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_set_shm_dir_path(struct doca_devemu_pci_tlp_channel *channel,
							  const char *shm_dir_path);

/**
 * @brief Get the directory path for TLP channel shared memory.
 *
 * @details Copies the shared memory directory path configured to this channel into the user-allocated buffer.
 * The implementation validates the buffer size before copying to prevent overflow.
 *
 * @param [in] channel
 * The TLP channel to query.
 * @param [out] shm_dir_path
 * User-allocated buffer to receive the configured shared memory directory path (null-terminated).
 * @param [in] shm_dir_path_len
 * Length in bytes of the shm_dir_path buffer (including null terminator).
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' or 'shm_dir_path' is NULL, or 'shm_dir_path_len' is smaller than the
 * configured path length for this channel.
 * - DOCA_ERROR_NOT_FOUND - If a non-empty shared memory directory path was not configured for this channel.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_get_shm_dir_path(const struct doca_devemu_pci_tlp_channel *channel,
							  char *shm_dir_path,
							  uint32_t shm_dir_path_len);

/**
 * @brief Get the configured (enabled/disabled) state of Asynchronous Credit Grant (ACG) request support for the
 * specified TLP channel.
 *
 * @param [in] channel
 * The DOCA devemu PCI TLP channel instance to query. Must be started.
 * @param [out] acg_enabled
 * 1 if ACG request support is enabled, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' or 'acg_enabled' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'channel' is not started. Use doca_ctx_start() to start it.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_get_acg_enabled(const struct doca_devemu_pci_tlp_channel *channel,
							 uint8_t *acg_enabled);

/**
 * @brief Enables or disables Asynchronous Credit Grant (ACG) request support for the specified TLP channel.
 *
 * @details If set to 1, ACG requests will be sent through a running TLP emulation channel to the user. The user
 * must be prepared to handle doca_devemu_pci_tlp_channel_req instances with opcode
 * DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_ACG according to the value returned by doca_devemu_pci_tlp_cap_get_max_acg().
 * If set to 0, ACG requests will not be sent through the TLP emulation channel.
 *
 * This function can't be called for a channel created with doca_devemu_pci_tlp_channel_create_from_export().
 *
 * For a channel created with doca_devemu_pci_tlp_channel_create(), once doca_ctx_start() is called, this configuration
 * must not be changed for the remainder of the channel's lifetime.
 *
 * @param [in] channel
 * The DOCA devemu PCI TLP channel instance to modify. Must be idle.
 * @param [in] acg_enabled
 * Set to 1 to enable sending ACG requests through the TLP emulation channel to the user. Otherwise, set to 0.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'channel' is not idle. Use doca_ctx_stop() to stop it.
 * - DOCA_ERROR_NOT_SUPPORTED - If the 'acg_enabled' value does not conform to the value reported by
 *   doca_devemu_pci_tlp_cap_get_max_acg(), or if the channel was created with
 *   doca_devemu_pci_tlp_channel_create_from_export().
 * - DOCA_ERROR_NOT_PERMITTED - If the channel was already started once using doca_ctx_start().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_set_acg_enabled(struct doca_devemu_pci_tlp_channel *channel,
							 uint8_t acg_enabled);

/**
 * @brief Get whether the specified TLP channel is currently in the primary role.
 *
 * @param [in] channel
 * The DOCA devemu PCI TLP channel instance to query.
 * @param [out] is_primary
 * 1 if the channel is currently in the primary role, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' or 'is_primary' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_get_primary(const struct doca_devemu_pci_tlp_channel *channel,
						     uint8_t *is_primary);

/**
 * @brief Set whether the specified TLP channel acts as a primary channel.
 *
 * @details Configures the role of the given TLP channel. When set to 1, the channel is configured as the primary
 * channel. When set to 0, the channel is configured as a secondary channel. Only a primary channel is allowed to
 * forward TLP channel requests to the application using the pre-registered callback
 * doca_devemu_pci_tlp_channel_event_req_handler_cb_t.
 *
 * Typically, during a handover, for TLP channels created in the destination application from an exported descriptor,
 * this value should be set to 0. For those channels, this value should be set to 1 only after the peer source
 * channel is in idle state and in secondary role.
 *
 * @param [in] channel
 * The DOCA devemu PCI TLP channel instance to modify. Must be at DOCA_CTX_STATE_IDLE or DOCA_CTX_STATE_RUNNING state.
 * @param [in] is_primary
 * Set to 1 to configure the channel as a primary channel. Otherwise, set to 0.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'channel' is not at DOCA_CTX_STATE_IDLE or DOCA_CTX_STATE_RUNNING state.
 * @note Applications must coordinate the roles configured on the source and destination channels. For each handover
 * pair, at most one channel can be configured as primary. It is allowed for both channels to be configured as
 * secondary. If the roles are not aligned correctly between source and destination, the behavior is undefined.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_set_primary(struct doca_devemu_pci_tlp_channel *channel, uint8_t is_primary);

/**
 * @brief Get the number of downstream ports associated with a TLP channel.
 *
 * @details When the physical association between the device and the host PCI link is via a PCI switch, the TLP channel
 * represents all downstream ports of that switch that are assigned for TLP emulation. Upon success, 'num_dsp' holds
 * the number of downstream ports associated with the channel.
 *
 * The value of 'num_dsp' is guaranteed to be less than the smallest reserved special value, so valid downstream port
 * indices in the range of [0, num_dsp - 1] never collide with any reserved special value.
 *
 * @param [in] channel
 * The DOCA devemu PCI TLP channel instance to query. Must be started.
 * @param [out] num_dsp
 * The number of downstream ports associated with the specified TLP channel.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' or 'num_dsp' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'channel' is not started. Use doca_ctx_start() to start it.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_get_num_dsp(const struct doca_devemu_pci_tlp_channel *channel,
						     uint8_t *num_dsp);

/*********************************************************************************************************************
 * DOCA devemu PCI TLP channel events API
 *********************************************************************************************************************/

/**
 * @brief Callback function executed when a TLP channel request arrives.
 *
 * @details Ownership of the doca_devemu_pci_tlp_channel_req (including all associated fields) is transferred from the
 * doca_devemu_pci_tlp_channel context to the user. Upon calling doca_devemu_pci_tlp_channel_req_complete_*(),
 * ownership (including all associated fields) returns to the corresponding doca_devemu_pci_tlp_channel context.
 *
 * Upon invocation, the user should retrieve the request opcode using doca_devemu_pci_tlp_channel_req_get_opcode().
 *
 * For opcode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP:
 *     1. Retrieve the TLP header using doca_devemu_pci_tlp_channel_req_get_tlp_header()
 *     2. Decode and process the TLP header
 *     3. If needed, retrieve TLP data using doca_devemu_pci_tlp_channel_req_get_tlp_data() and process it
 *     4. If needed, retrieve and populate the completion header using
 * doca_devemu_pci_tlp_channel_req_get_tlp_cpl_header()
 *     5. If needed, retrieve and populate completion data using doca_devemu_pci_tlp_channel_req_get_tlp_cpl_data()
 *     6. Finalize processing with one of the following:
 *         - doca_devemu_pci_tlp_channel_req_complete_tlp()
 *         - doca_devemu_pci_tlp_channel_req_complete_config_write()
 *         - doca_devemu_pci_tlp_channel_req_complete_config_read()
 *
 * For opcode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_ACG (the user can hold on to the request and process it
 * whenever needed before completing it):
 *     1. The user should choose the operational mode for the ACG request completion (enum
 *        doca_devemu_pci_tlp_channel_req_acg_comp_opmode enum):
 *        1.1. For opmode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_ACG_COMP_OPMODE_FLUSH:
 *            - Finalize operation by calling doca_devemu_pci_tlp_channel_req_complete_acg()
 *        1.2 For opmode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_ACG_COMP_OPMODE_MMIO_WRITE:
 *            - Retrieve the ACG buffer using doca_devemu_pci_tlp_channel_req_get_acg_buf().
 *            - Populate the ACG buffer according to the PCI specification with a valid MMIO WRITE TLP header and data
 *            - Finalize operation by calling doca_devemu_pci_tlp_channel_req_complete_acg()
 *
 * For opcode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_PCI_EVENT:
 *     1. Retrieve the operation mode using doca_devemu_pci_tlp_channel_req_get_pci_event_opmode():
 *        1.1 For opmode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_PCI_EVENT_OPMODE_PERST_ASSERT:
 *            - Put the relevant PCI entities (endpoints/switches) into reset (assert PERST#), as defined by the PCI
 *              specification.
 *        1.2 For opmode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_PCI_EVENT_OPMODE_PERST_DEASSERT:
 *            - Release the relevant PCI entities (endpoints/switches) from reset (deassert PERST#), as defined by the
 *              PCI specification.
 *     2. Finalize the operation by calling doca_devemu_pci_tlp_channel_req_complete_pci_event().
 *
 * @param [in] channel
 * The PCI TLP channel associated with the arrived request.
 * @param [in] req
 * The arrived TLP channel request.
 * @param [in] req_user_data
 * The user data associated to the request.
 */
typedef void (*doca_devemu_pci_tlp_channel_event_req_handler_cb_t)(struct doca_devemu_pci_tlp_channel *channel,
								   struct doca_devemu_pci_tlp_channel_req *req,
								   void *req_user_data);

/**
 * @brief Register for TLP channel requests notifications.
 *
 * @details If called multiple times, only the last registration will take effect.
 *
 * @param [in] channel
 * The DOCA devemu PCI channel. Must be idle.
 * @param [in] handler
 * Callback function invoked when a TLP channel request arrives.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'channel' or 'handler' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'channel' is not idle. Use doca_ctx_stop() to stop it.
 * @note TLP channel requests targeting MSI-X or DB regions will not trigger notifications via this mechanism.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_channel_event_req_register(struct doca_devemu_pci_tlp_channel *channel,
							    doca_devemu_pci_tlp_channel_event_req_handler_cb_t handler);

/**
 * @brief Retrieve the opcode of the received TLP channel request.
 *
 * @details This function should only be called when the user has ownership of the TLP channel request.
 *
 * @param [in] req
 * The TLP channel request to query. Must be in the ownership of the user.
 *
 * @return
 * TLP channel request opcode on success. DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_INVALID otherwise.
 */
DOCA_EXPERIMENTAL
enum doca_devemu_pci_tlp_channel_req_opcode doca_devemu_pci_tlp_channel_req_get_opcode(
	const struct doca_devemu_pci_tlp_channel_req *req);

/**
 * @brief The specific downstream port is implicitly determined from the request data.
 */
#define DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_DSP_ID_IMPLICIT 0xFD
/**
 * @brief The request is not associated with any specific downstream port.
 */
#define DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_DSP_ID_NONE 0xFE
/**
 * @brief The request is associated with all downstream ports associated with the corresponding TLP channel.
 */
#define DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_DSP_ID_ALL 0xFF

/**
 * @brief Retrieve the downstream port identifier associated with a received request.
 *
 * @details This function must only be called when the user has ownership of the TLP channel request.
 *
 * The returned dsp_id is either a normal downstream port index in the range of [0, num_dsp - 1], where num_dsp is
 * retrieved by doca_devemu_pci_tlp_channel_get_num_dsp(), or one of the following special values (for more details see
 * documentation above):
 *  - DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_DSP_ID_IMPLICIT
 *  - DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_DSP_ID_NONE
 *  - DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_DSP_ID_ALL
 *
 * @param [in] req
 * The TLP channel request to query. Must be owned by the user.
 *
 * @return
 * The downstream port identifier associated with the request.
 * If 'req' is NULL, returns DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_DSP_ID_NONE.
 */
DOCA_EXPERIMENTAL
uint8_t doca_devemu_pci_tlp_channel_req_get_dsp_id(const struct doca_devemu_pci_tlp_channel_req *req);

/**
 * @brief Retrieve the TLP header associated with a received TLP channel request.
 *
 * @details This function should only be called when the user has ownership of the TLP channel request and if the
 * request's opcode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP.
 *
 * @param [in] req
 * The TLP channel request to query. Must be in the ownership of the user.
 *
 * @return
 * A pointer to the TLP header associated with the received request on success. NULL otherwise.
 * @note For incoming MMIO TLPs, not all header fields are valid. Use
 * doca_devemu_pci_tlp_cap_is_ingress_mmio_read_hdr_field_valid() and
 * doca_devemu_pci_tlp_cap_is_ingress_mmio_write_hdr_field_valid() to verify which fields are valid.
 */
DOCA_EXPERIMENTAL
const void *doca_devemu_pci_tlp_channel_req_get_tlp_header(const struct doca_devemu_pci_tlp_channel_req *req);

/**
 * @brief Retrieve the TLP data associated with a received TLP channel request.
 *
 * @details This function should only be called when the user has ownership of the TLP channel request and if the
 * request's opcode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP.
 *
 * @param [in] req
 * The TLP channel request to query. Must be in the ownership of the user.
 *
 * @return
 * A pointer to the data associated with the received request on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
const void *doca_devemu_pci_tlp_channel_req_get_tlp_data(const struct doca_devemu_pci_tlp_channel_req *req);

/**
 * @brief Retrieve a pointer to the TLP completion header associated with the TLP channel request.
 *
 * @details This function should only be called when the user has ownership of the TLP channel request and if the
 * request's opcode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP. The user is expected to populate the header according
 * to the PCI specification before completing the request.
 *
 * @param [in] req
 * The TLP channel request to query. Must be in the ownership of the user.
 *
 * @return
 * A pointer to the header associated with the TLP completion on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
void *doca_devemu_pci_tlp_channel_req_get_tlp_cpl_header(struct doca_devemu_pci_tlp_channel_req *req);

/**
 * @brief Retrieve a pointer to the TLP completion data associated with the TLP channel request.
 *
 * @details This function should only be called when the user has ownership of the TLP channel request and if the
 * request's opcode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP. The user is expected to populate the data according
 * to the PCI specification before completing the request.
 *
 * @param [in] req
 * The TLP channel request to query. Must be in the ownership of the user.
 *
 * @return
 * A pointer to the data associated with the TLP completion on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
void *doca_devemu_pci_tlp_channel_req_get_tlp_cpl_data(struct doca_devemu_pci_tlp_channel_req *req);

/**
 * @brief Complete the handling of a TLP channel request with opcode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP of any
 * type.
 *
 * @details Ownership of the request (including its associated fields) is transferred from the user back to the
 * associated doca_devemu_pci_tlp_channel context. The channel context will submit a completion TLP to the sender, if
 * required, according to the PCI specification.
 * It is assumed that the user has populated the completion header and data, if required, according to the PCI
 * specification, and that the populated payload size conforms to the
 * doca_devemu_pci_tlp_cap_get_max_payload_size_per_tlp_channel_comp() capability. Otherwise, behavior is undefined.
 *
 * If the completed request opcode is not DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP, the TLP channel will silently
 * drop the request.
 *
 * This function must be invoked in the same context where the request handler was called.
 *
 * @param [in] req
 * The TLP channel request to complete. Must be in the ownership of the user.
 * @param [in] has_cpl
 * 1 if the associated TLP requires sending a completion, 0 otherwise. If 1, it is assumed that the completion
 * header and data have been populated according to the PCI specification.
 * @param [in] pci_dev
 * The DOCA devemu PCI TLP device associated with the TLP request, or NULL in case the TLP request is not targeting
 * a DOCA devemu PCI TLP device.
 * @note To optimize the handling speed of configuration write and read TLP requests, it is recommended to use
 * doca_devemu_pci_tlp_channel_req_complete_config_write() and doca_devemu_pci_tlp_channel_req_complete_config_read().
 */
DOCA_EXPERIMENTAL
void doca_devemu_pci_tlp_channel_req_complete_tlp(struct doca_devemu_pci_tlp_channel_req *req,
						  uint8_t has_cpl,
						  struct doca_devemu_pci_tlp_dev *pci_dev);

/**
 * @brief Complete the handling of a TLP channel request with opcode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP for a
 * configuration write.
 *
 * @details Ownership of the request (including its associated fields) is transferred from the user back to the
 * associated doca_devemu_pci_tlp_channel context. The channel context will submit a completion TLP to the sender
 * according to the PCI specification.
 * It is assumed that the user has populated the completion header and data according to the PCI specification, and
 * that the populated payload size conforms to the doca_devemu_pci_tlp_cap_get_max_payload_size_per_tlp_channel_comp()
 * capability. Otherwise, behavior is undefined.
 *
 * If the completed request opcode is not DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP, the TLP channel will silently
 * drop the request.
 *
 * This function must be invoked in the same context where the request handler was called.
 *
 * @param [in] req
 * The TLP channel request to complete. Must be in the ownership of the user.
 * @param [in] pci_dev
 * The DOCA devemu PCI TLP device associated with the TLP request, or NULL in case the TLP request is not targeting
 * a DOCA devemu PCI TLP device.
 * @param [in] is_cap_id_valid
 * Indicates whether the cap_id and is_pcie_cap parameters are valid, meaning the configuration write targeted a
 * PCI/PCIe capability. This field is valid only if pci_dev is not NULL.
 * @param [in] cap_id
 * The PCI/PCIe capability ID. Valid only if is_cap_id_valid parameter is 1.
 * @param [in] is_pcie_cap
 * 1 if the cap_id parameter refers to a PCIe capability. 0 otherwise. Valid only if is_cap_id_valid parameter
 * is 1.
 * @note This API can be used to accelerate handling of the PCI TLP request completion compared to
 * doca_devemu_pci_tlp_channel_req_complete_tlp().
 */
DOCA_EXPERIMENTAL
void doca_devemu_pci_tlp_channel_req_complete_config_write(struct doca_devemu_pci_tlp_channel_req *req,
							   struct doca_devemu_pci_tlp_dev *pci_dev,
							   uint8_t is_cap_id_valid,
							   uint16_t cap_id,
							   uint8_t is_pcie_cap);

/**
 * @brief Complete the handling of a TLP channel request with opcode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP for a
 * configuration read.
 *
 * @details Ownership of the request (including its associated fields) is transferred from the user back to the
 * associated doca_devemu_pci_tlp_channel context. The channel context will submit a completion TLP to the sender
 * according to the PCI specification.
 * It is assumed that the user has populated the completion header and data according to the PCI specification, and
 * that the populated payload size conforms to the doca_devemu_pci_tlp_cap_get_max_payload_size_per_tlp_channel_comp()
 * capability. Otherwise, behavior is undefined.
 *
 * If the completed request opcode is not DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_TLP, the TLP channel will silently
 * drop the request.
 *
 * This function must be invoked in the same context where the request handler was called.
 *
 * @param [in] req
 * The TLP channel request to complete. Must be in the ownership of the user.
 * @param [in] pci_dev
 * The DOCA devemu PCI TLP device associated with the TLP request, or NULL in case the TLP request is not targeting
 * a DOCA devemu PCI TLP device.
 * @param [in] is_cap_id_valid
 * Indicates whether the cap_id and is_pcie_cap parameters are valid, meaning the configuration read targeted a
 * PCI/PCIe capability.
 * @param [in] cap_id
 * The PCI/PCIe capability ID. Valid only if is_cap_id_valid parameter is 1.
 * @param [in] is_pcie_cap
 * 1 if the cap_id parameter refers to a PCIe capability. 0 otherwise. Valid only if is_cap_id_valid parameter
 * is 1.
 * @note This API can be used to accelerate handling of the PCI TLP request completion compared to
 * doca_devemu_pci_tlp_channel_req_complete_tlp().
 */
DOCA_EXPERIMENTAL
void doca_devemu_pci_tlp_channel_req_complete_config_read(struct doca_devemu_pci_tlp_channel_req *req,
							  struct doca_devemu_pci_tlp_dev *pci_dev,
							  uint8_t is_cap_id_valid,
							  uint16_t cap_id,
							  uint8_t is_pcie_cap);

/**
 * @brief Retrieve a pointer to the ACG buffer associated with the TLP channel request.
 *
 * @details This function should only be called when the user has ownership of the TLP channel request and if the
 * request's opcode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_ACG. The user is expected to populate the buffer, if
 * needed, before completing the request.
 *
 * @param [in] req
 * The TLP channel request to query. Must be in the ownership of the user.
 *
 * @return
 * A pointer to the ACG buffer on success. NULL otherwise.
 */
DOCA_EXPERIMENTAL
void *doca_devemu_pci_tlp_channel_req_get_acg_buf(struct doca_devemu_pci_tlp_channel_req *req);

/**
 * @brief Complete the handling of a TLP channel request with opcode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_ACG.
 *
 * @details Ownership of the request (including its associated fields) is transferred from the user back to the
 * associated doca_devemu_pci_tlp_channel context. The 'acg_buf_len' parameter specifies the number of bytes populated
 * in the ACG buffer. For any opmode, the populated payload and 'acg_buf_len' should conform to the
 * doca_devemu_pci_tlp_cap_get_max_payload_size_per_tlp_channel_comp() capability. Otherwise, behavior is undefined.
 *
 * If the completed request opcode is not DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_ACG, the TLP channel will silently
 * drop the request.
 *
 * This function must be invoked in the same context where the request handler was called.
 *
 * For opmode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_ACG_COMP_OPMODE_FLUSH, no payload should be populated in the ACG buffer
 * and 'acg_buf_len' should be set to 0. Otherwise, behavior is undefined.
 *
 * For opmode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_ACG_COMP_OPMODE_MMIO_WRITE, it is assumed that the user has populated the
 * MMIO WRITE TLP header and data according to the PCI specification. Otherwise, behavior is undefined.
 *
 * @param [in] req
 * The TLP channel request to complete. Must be in the ownership of the user.
 * @param [in] acg_buf_len
 * Length, in bytes, of the data populated into the ACG buffer.
 * @param [in] opmode
 * The operational mode of the ACG request completion.
 */
DOCA_EXPERIMENTAL
void doca_devemu_pci_tlp_channel_req_complete_acg(struct doca_devemu_pci_tlp_channel_req *req,
						  uint16_t acg_buf_len,
						  enum doca_devemu_pci_tlp_channel_req_acg_comp_opmode opmode);

/**
 * @brief Retrieve the operation mode of a received TLP channel PCI_EVENT request.
 *
 * @details This function should only be called when the user has ownership of the TLP channel request and if the
 * request's opcode == DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_PCI_EVENT.
 *
 * @param [in] req
 * The TLP channel request to query. Must be in the ownership of the user.
 *
 * @return
 * The PCI_EVENT request operation mode on success. DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_PCI_EVENT_OPMODE_INVALID otherwise.
 */
DOCA_EXPERIMENTAL
enum doca_devemu_pci_tlp_channel_req_pci_event_opmode doca_devemu_pci_tlp_channel_req_get_pci_event_opmode(
	const struct doca_devemu_pci_tlp_channel_req *req);

/**
 * @brief Complete the handling of a TLP channel request with opcode DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_PCI_EVENT.
 *
 * @details Ownership of the request (including its associated fields) is transferred from the user back to the
 * associated doca_devemu_pci_tlp_channel context.
 *
 * If the completed request opcode is not DOCA_DEVEMU_PCI_TLP_CHANNEL_REQ_OPCODE_PCI_EVENT, the TLP channel will
 * silently drop the request.
 *
 * This function must be invoked in the same context where the request handler was called.
 *
 * @param [in] req
 * The TLP channel request to complete. Must be in the ownership of the user.
 */
DOCA_EXPERIMENTAL
void doca_devemu_pci_tlp_channel_req_complete_pci_event(struct doca_devemu_pci_tlp_channel_req *req);

/*********************************************************************************************************************
 * DOCA devemu PCI TLP device API
 *********************************************************************************************************************/

/**
 * @brief Allocate DOCA devemu PCI TLP device.
 *
 * @param [in] pci_type
 * The DOCA PCI TLP type to be associated with the device. Must be started and remain started throughout the lifetime
 * of the newly created 'pci_dev'.
 * @param [in] dev_rep
 * Representor DOCA device.
 * @param [out] pci_dev
 * The newly created DOCA devemu PCI TLP device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type', 'dev_rep' or 'pci_dev' is NULL or representor type does not match the
 * PCI type.
 * - DOCA_ERROR_BAD_STATE - If 'pci_type' is not started. Use doca_devemu_pci_type_start() to start the 'pci_type'.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_*pci_tlp_type_create().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_dev_create(struct doca_devemu_pci_type *pci_type,
					    struct doca_dev_rep *dev_rep,
					    struct doca_devemu_pci_tlp_dev **pci_dev);

/**
 * @brief Allocate a started DOCA devemu PCI TLP device.
 *
 * @details The returned device is started and uses default properties. This function is typically used in
 * handover and takeover scenarios, where the device properties were configured in a different (or crashed) process.
 *
 * @param [in] pci_type
 * The DOCA PCI TLP type to be associated with the device. Must be started and remain started throughout the lifetime
 * of the newly created 'pci_dev'.
 * @param [in] dev_rep
 * Representor DOCA device.
 * @param [out] pci_dev
 * The newly created and started DOCA devemu PCI TLP device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type', 'dev_rep' or 'pci_dev' is NULL or representor type does not match the
 * PCI type.
 * - DOCA_ERROR_BAD_STATE - If 'pci_type' is not started. Use doca_devemu_pci_type_start() to start the 'pci_type'.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_*pci_tlp_type_create().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_dev_create_started(struct doca_devemu_pci_type *pci_type,
						    struct doca_dev_rep *dev_rep,
						    struct doca_devemu_pci_tlp_dev **pci_dev);

/**
 * @brief Allocate a started DOCA devemu PCI TLP device binded to DPA device.
 *
 * @details The returned device is started and uses default properties and its datapath bound to the specified
 * DPA device. This function is typically used in handover and takeover scenarios, where the device properties were
 * configured in a different (or crashed) process.
 *
 * @param [in] pci_type
 * The DOCA PCI TLP type to be associated with the device. Must be started and remain started throughout the lifetime
 * of the newly created 'pci_dev'.
 * @param [in] dev_rep
 * Representor DOCA device.
 * @param [in] dpa_dev
 * A pointer to a doca_dpa device.
 * @param [out] pci_dev
 * The newly created and started DOCA devemu PCI TLP device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_type', 'dev_rep', 'dpa_dev' or 'pci_dev' is NULL or representor type does not
 * match the PCI type.
 * - DOCA_ERROR_BAD_STATE - If 'pci_type' is not started. Use doca_devemu_pci_type_start() to start the 'pci_type'.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_type' was not created using doca_devemu_*pci_tlp_type_create().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_dev_create_started_on_dpa(struct doca_devemu_pci_type *pci_type,
							   struct doca_dev_rep *dev_rep,
							   struct doca_dpa *dpa_dev,
							   struct doca_devemu_pci_tlp_dev **pci_dev);

/**
 * @brief Free a DOCA devemu PCI TLP device created by doca_devemu_pci_tlp_dev_create*().
 *
 * @param [in] pci_dev
 * The previously created DOCA devemu PCI TLP device. Must not be started.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_dev' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'pci_dev' is started. Use doca_devemu_pci_tlp_dev_stop() to stop it.
 * - DOCA_ERROR_NOT_PERMITTED - If 'pci_dev' was not created using doca_devemu_pci_tlp_dev_create*().
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_dev_destroy(struct doca_devemu_pci_tlp_dev *pci_dev);

/**
 * @brief Start a DOCA devemu PCI TLP device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI TLP device to start. Must not be started.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_dev' is NULL or invalid PCI parameters were provided.
 * - DOCA_ERROR_BAD_STATE - If 'pci_dev' is already started.
 * - DOCA_ERROR_NO_MEMORY - Allocation failure.
 * - DOCA_ERROR_DRIVER - Internal doca driver error.
 * @note This method upon success disable the ability to configure the DOCA devemu PCI TLP device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_dev_start(struct doca_devemu_pci_tlp_dev *pci_dev);

/**
 * @brief Stop a DOCA devemu PCI TLP device.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI TLP device to stop. Must be started.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_dev' is NULL.
 * - DOCA_ERROR_BAD_STATE - If 'pci_dev' is not started.
 * - DOCA_ERROR_DRIVER - Internal doca driver error.
 * @note This method upon success re-enable the ability to configure the DOCA devemu PCI TLP device.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_dev_stop(struct doca_devemu_pci_tlp_dev *pci_dev);

/**
 * @brief Check whether the DOCA devemu PCI TLP device is started.
 *
 * @param [in] pci_dev
 * The DOCA devemu PCI TLP device to query.
 * @param [out] started
 * 1 if the DOCA devemu PCI TLP device is started, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - If 'pci_dev' or 'started' is NULL.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_devemu_pci_tlp_dev_is_started(const struct doca_devemu_pci_tlp_dev *pci_dev, uint8_t *started);

/**
 * @brief Convert DOCA devemu PCI TLP device instance into DOCA devemu PCI endpoint.
 *
 * @param [in] pci_dev
 * DOCA devemu PCI TLP device. The device must remain valid until after the returned endpoint is no longer required.
 *
 * @return
 * DOCA devemu PCI endpoint upon success, NULL otherwise.
 */
DOCA_EXPERIMENTAL
struct doca_devemu_pci_ep *doca_devemu_pci_tlp_dev_as_ep(struct doca_devemu_pci_tlp_dev *pci_dev);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEVEMU_PCI_TLP_H_ */
