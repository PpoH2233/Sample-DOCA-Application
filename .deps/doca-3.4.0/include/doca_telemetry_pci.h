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
 * @file doca_telemetry_pci.h
 * @page DOCA_TELEMETRY_PCI
 * @defgroup DOCA_TELEMETRY_PCI DOCA Telemetry pci
 * DOCA Telemetry PCI library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_TELEMETRY_PCI_H_
#define DOCA_TELEMETRY_PCI_H_

#include <stdint.h>

#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 *
 * DOCA core opaque types
 *
 *********************************************************************************************************************/
struct doca_dev;
struct doca_devinfo;

/*********************************************************************************************************************
 *
 * DOCA Telemetry PCI context
 *
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA Telemetry PCI instance.
 */
struct doca_telemetry_pci;

/**
 * DPN value to specify the PCI device to query.
 *
 * mlxlink can be used to determine the values to use. (replace mlx5_0 as required)
 * `mlxlink -d mlx5_0 --port_type PCIE --show_links`
 */
struct doca_telemetry_pci_dpn {
	uint8_t depth;	   /**< PCI device depth */
	uint8_t pci_index; /**< PCI device index */
	uint8_t node;	   /**< PCI device node */
};

/**
 * @brief Create a DOCA Telemetry PCI instance.
 *
 * @param [in] dev
 * The device to attach to the telemetry PCI instance.
 * @param [out] telemetry_pci
 * Pointer to pointer to be set to point to the created doca_telemetry_pci instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_create(struct doca_dev *dev, struct doca_telemetry_pci **telemetry_pci);

/**
 * @brief Destroy doca_telemetry_pci previously created by doca_telemetry_pci_create().
 *
 * @param [in] telemetry_pci
 * Pointer to instance to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - PCI needs to be stopped before destroy.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_destroy(struct doca_telemetry_pci *telemetry_pci);

/**
 * @brief Start the telemetry context.
 *
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 *
 * @return
 * DOCA_SUCCESS - in case of context started successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - Context is already running.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_start(struct doca_telemetry_pci *telemetry_pci);

/**
 * @brief Stop the telemetry context.
 *
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 *
 * @return
 * DOCA_SUCCESS - in case of context stopped successfully.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - Context is already stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_stop(struct doca_telemetry_pci *telemetry_pci);

/*********************************************************************************************************************
 *
 * PCI Management Info
 *
 *********************************************************************************************************************/

/**
 * PCI link width options
 */
enum doca_telemetry_pci_link_width {
	DOCA_TELEMETRY_PCI_LINK_WIDTH_X1 = 1,	/**< 1 PCI lane */
	DOCA_TELEMETRY_PCI_LINK_WIDTH_X2 = 2,	/**< 2 PCI lanes */
	DOCA_TELEMETRY_PCI_LINK_WIDTH_X4 = 4,	/**< 4 PCI lanes */
	DOCA_TELEMETRY_PCI_LINK_WIDTH_X8 = 8,	/**< 8 PCI lanes */
	DOCA_TELEMETRY_PCI_LINK_WIDTH_X16 = 16, /**< 16 PCI lanes */
};

/**
 * PCI link speed options
 */
enum doca_telemetry_pci_link_speed {
	DOCA_TELEMETRY_PCI_LINK_SPEED_2_5_G = 1,      /**< 2.5G link speed (Gen 1) */
	DOCA_TELEMETRY_PCI_LINK_SPEED_5_G = 2,	      /**< 5G link speed (Gen 2) */
	DOCA_TELEMETRY_PCI_LINK_SPEED_8_G = 3,	      /**< 8G link speed (Gen 3) */
	DOCA_TELEMETRY_PCI_LINK_SPEED_16_G = 4,	      /**< 16G link speed (Gen 4) */
	DOCA_TELEMETRY_PCI_LINK_SPEED_32_G = 5,	      /**< 32G link speed (Gen 5) */
	DOCA_TELEMETRY_PCI_LINK_SPEED_32_G_PAM_4 = 6, /**< 32G PAM-4 link speed (Gen 6) */
};

/**
 * PCI data size options
 */
enum doca_telemetry_pci_data_size {
	DOCA_TELEMETRY_PCI_DATA_SIZE_128B = 128,   /**< 128 byte size */
	DOCA_TELEMETRY_PCI_DATA_SIZE_256B = 256,   /**< 256 byte size */
	DOCA_TELEMETRY_PCI_DATA_SIZE_512B = 512,   /**< 512 byte size */
	DOCA_TELEMETRY_PCI_DATA_SIZE_1024B = 1024, /**< 1024 byte size */
	DOCA_TELEMETRY_PCI_DATA_SIZE_2048B = 2048, /**< 2048 byte size */
	DOCA_TELEMETRY_PCI_DATA_SIZE_4096B = 4096, /**< 4096 byte size */
};

/**
 * PCI power status options
 */
enum doca_telemetry_pci_power_status {
	DOCA_TELEMETRY_PCI_POWER_STATUS_NO_VALUE = 0,  /**< Power status value could not be read */
	DOCA_TELEMETRY_PCI_POWER_STATUS_OK = 1,	       /**< Sufficient power available */
	DOCA_TELEMETRY_PCI_POWER_STATUS_LOW_POWER = 2, /**< Insufficient power available */
};

/**
 * PCI port type options
 */
enum doca_telemetry_pci_port_type {
	DOCA_TELEMETRY_PCI_PORT_TYPE_PCIE_ENDPOINT = 0,	  /**< PCI Express endpoint port */
	DOCA_TELEMETRY_PCI_PORT_TYPE_PCIE_ROOT_PORT = 4,  /**< Root Port of PCI Express Root Complex */
	DOCA_TELEMETRY_PCI_PORT_TYPE_PCIE_UPSTREAM = 5,	  /**< PCI Express Upstream port */
	DOCA_TELEMETRY_PCI_PORT_TYPE_PCIE_DOWNSTREAM = 6, /**< PCI Express Downstream port */
};

/**
 * PCI lane reversal options
 */
enum doca_telemetry_pci_lane_reversal_mode {
	DOCA_TELEMETRY_PCI_LANE_REVERSAL_MODE_STRAIGHT = 0, /**< Straight (no reversal) */
	DOCA_TELEMETRY_PCI_LANE_REVERSAL_MODE_REVERSAL = 1, /**< Reversal (reversed) */
};

/**
 * PCI status information
 */
struct doca_telemetry_pci_management_info {
	enum doca_telemetry_pci_link_width link_width_enabled;	 /**< Maximum link width enabled */
	enum doca_telemetry_pci_link_speed link_speed_enabled;	 /**< Maximum link speed enabled */
	enum doca_telemetry_pci_link_width link_width_active;	 /**< Active link width */
	enum doca_telemetry_pci_link_speed link_speed_active;	 /**< Active link speed */
	enum doca_telemetry_pci_data_size max_read_request_size; /**< Max read request size in bytes */
	enum doca_telemetry_pci_data_size max_payload_size;	 /**< Max payload size in bytes */
	enum doca_telemetry_pci_power_status
		pwr_status;			     /**< Indicates the status of PCI power consumption limitations.
							  Only valid when
							  doca_telemetry_pci_cap_management_info_power_reporting_is_supported()
							  returns DOCA_SUCCESS. */
	enum doca_telemetry_pci_port_type port_type; /**< Indicates the specific type of this PCI Express Function.
							 Note that different Functions in a multi-Function device
							 can generally be of different types */
	enum doca_telemetry_pci_lane_reversal_mode lane_reversal; /**< Reversal mode of the link, together with
								     lane0_physical_position provide the physical lane
								   */
	enum doca_telemetry_pci_link_speed
		link_peer_max_speed; /**< Peer Max Link Speed. Only valid when
					  doca_telemetry_pci_cap_management_info_link_peer_max_speed_is_supported()
					  returns DOCA_SUCCESS. */
	uint16_t num_of_pfs;	     /**< Number of Physical Functions (PFs) */
	uint16_t num_of_vfs;	     /**< Number of Virtual Functions (for all PFs) */
	uint16_t bdf0;		     /**< Bus Device Function - only for function0 */
	uint16_t pci_power; /**< Power reported by the PCI device. The units are in Watts. 0: Power is unknown. Only
				 valid when doca_telemetry_pci_cap_management_info_power_reporting_is_supported()
			       returns DOCA_SUCCESS. */
	uint8_t lane0_physical_position;      /**< The physical lane position of logical lane0 */
	uint8_t precode_sup;		      /**< 1 when precoding is supported for the current speed, 0 otherwise */
	uint8_t precode_active;		      /**< 1 when precoding is active for the current speed, 0 otherwise */
	uint8_t flit_sup;		      /**< 1 when flit is supported for the current speed, 0 otherwise */
	uint8_t flit_active;		      /**< 1 when flit is active for the current speed, 0 otherwise */
	uint8_t correctable_error_detected;   /**< 1 when a correctable error has been detected, 0 otherwise */
	uint8_t non_fatal_error_detected;     /**< 1 when a non-fatal error has been detected, 0 otherwise */
	uint8_t fatal_error_detected;	      /**< 1 when a fatal error has been detected, 0 otherwise */
	uint8_t unsupported_request_detected; /**< 1 when an unsupported request has been detected, 0 otherwise */
	uint8_t aux_power_detected;	      /**< 1 when aux power has been detected, 0 otherwise */
	uint8_t transaction_pending;	      /**< 1 when a transaction is pending, 0 otherwise */
};

/**
 * @brief Check if given device is capable of reading PCI management info
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports PCI management info
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI management info.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_cap_management_info_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of reading PCI management info power fields:
 *  - pwr_status
 *  - pci_power
 *
 * @note when not supported the values will contain the value 0
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports PCI management info power fields.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI management info power fields.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_cap_management_info_power_reporting_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of reading PCI management info link peer max speed:
 *  - link_peer_max_speed
 *
 * @note when not supported the values will contain the value 0
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports PCI management info link peer max speed.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI management info link peer max speed.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_cap_management_info_link_peer_max_speed_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Read current PCI management info values.
 *
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 * @param [in] dpn
 * DPN of the PCI link to get values for.
 * @param [out] management_info
 * Pointer to doca_telemetry_pci_management_info structure to populate.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI management info.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_read_management_info(struct doca_telemetry_pci *telemetry_pci,
						     struct doca_telemetry_pci_dpn dpn,
						     struct doca_telemetry_pci_management_info *management_info);

/**
 * @brief Read current PCI management info values.
 *
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 * @param [in] pci_addr
 * PCI addr of the PCI link to get values for. Should be as one of the following formats:
 * - "Domain:Bus:Device.Function", e.g., "0000:3a:00.0" (size DOCA_DEVINFO_PCI_ADDR_SIZE including a null terminator).
 * - "Bus:Device.Function", e.g., "3a:00.0" (size DOCA_DEVINFO_PCI_BDF_SIZE including a null terminator), Domain is
 *   assumed to be "0000" (i.e. "0000:<pci_addr_str>").
 * @param [out] management_info
 * Pointer to doca_telemetry_pci_management_info structure to populate.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI management info.
 * - DOCA_ERROR_NOT_FOUND - the provided PCI addr did not map to a PCI link that can be queried.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_read_management_info_by_pci_addr(
	struct doca_telemetry_pci *telemetry_pci,
	const char *pci_addr,
	struct doca_telemetry_pci_management_info *management_info);

/*********************************************************************************************************************
 *
 * PCI perf counters group 1
 *
 *********************************************************************************************************************/

/**
 * PCI perf counters group 1
 */
struct doca_telemetry_pci_perf_counters_1 {
	uint64_t tx_overflow_buffer_pkt; /**< The number of packets dropped due to lack of PCIe buffers or receive path
					    from NIC port toward the hosts. Only valid when
					    doca_telemetry_pci_cap_perf_counters_1_tx_overflow_is_supported() returns
					    DOCA_SUCCESS. */
	uint64_t tx_overflow_buffer_marked_pkt; /**< The number of packets marked due to lack of PCIe buffers or receive
						   path from NIC port toward the hosts. Only valid when
						   doca_telemetry_pci_cap_perf_counters_1_tx_overflow_is_supported()
						   returns DOCA_SUCCESS. */
	uint32_t rx_errors; /**< Number of transitions to recovery due to Framing errors and CRC (dlp and tlp) errors.
			     */
	uint32_t tx_errors; /**< Number of transitions to recovery due to EIEOS and TS errors. */
	uint32_t crc_error_dllp;	  /**< Number of transitions to recovery due to identifying CRC DLLP errors. */
	uint32_t crc_error_tlp;		  /**< Number of transitions to recovery due to identifying CRC TLP errors. */
	uint32_t outbound_stalled_reads;  /**< The percentage of time within the last second that the NIC had outbound
					     non-posted read requests but could not perform the operation due to
					     insufficient non-posted credits. Only valid when
					     doca_telemetry_pci_cap_perf_counters_1_outbound_stalled_is_supported()
					     returns DOCA_SUCCESS. */
	uint32_t outbound_stalled_writes; /**< The percentage of time within the last second that the NIC had outbound
					     posted writes requests but could not perform the operation due to
					     insufficient posted credits. Only valid when
					     doca_telemetry_pci_cap_perf_counters_1_outbound_stalled_is_supported()
					     returns DOCA_SUCCESS. */
	uint32_t outbound_stalled_reads_events;	  /**< The number of events where outbound_stalled_reads was above a
						     threshold. Only valid when
						     doca_telemetry_pci_cap_perf_counters_1_outbound_stalled_is_supported()
						     returns DOCA_SUCCESS. */
	uint32_t outbound_stalled_writes_events;  /**< The number of events where outbound_stalled_writes was above a
						     threshold. Only valid when
						     doca_telemetry_pci_cap_perf_counters_1_outbound_stalled_is_supported()
						     returns DOCA_SUCCESS. */
	uint32_t fec_correctable_error_counter;	  /**< FEC correctable error counter. Only valid when
						     doca_telemetry_pci_cap_perf_counters_1_fec_error_counters_is_supported()
						     returns DOCA_SUCCESS. */
	uint32_t fec_uncorrectable_error_counter; /**< FEC uncorrectable error counter. Only valid when
						     doca_telemetry_pci_cap_perf_counters_1_fec_error_counters_is_supported()
						     returns DOCA_SUCCESS.*/
	uint32_t l0_to_recovery; /**< total l0 to recovery - this is a sum of all the l0 to recovery specific causes-
				    l0_to_recovery_eieos, l0_to_recovery_ts, l0_to_recovery_framing,
				    l0_to_recovery_retrain. */
	uint8_t effective_ber_magnitude; /**< Effective_BER = effective_ber_coef*10^(-effective_ber_magnitude) */
	uint8_t effective_ber_coef;	 /**< Effective_BER = effective_ber_coef*10^(-effective_ber_magnitude) */
	uint8_t fber_magnitude;		 /**< FBER = fber_coef*10^(-fber_magnitude). Only valid when
						     doca_telemetry_pci_cap_perf_counters_1_fber_counter_is_supported()
						     returns DOCA_SUCCESS. */
	uint8_t fber_coef;		 /**< FBER = fber_coef*10^(-fber_magnitude). Only valid when
						     doca_telemetry_pci_cap_perf_counters_1_fber_counter_is_supported()
						     returns DOCA_SUCCESS. */
};

/**
 * @brief Check if given device is capable of reading PCI performance counters 1
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports PCI performance counters 1.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI performance counters 1.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_cap_perf_counters_1_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of reading PCI performance counters 1: tx overflow counters:
 *  - tx_overflow_buffer_pkt
 *  - tx_overflow_buffer_marked_pkt
 *
 * @note when not supported the values will contain the value 0
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports PCI performance counters 1: tx overflow counters.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI performance counters 1: tx overflow counters.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_cap_perf_counters_1_tx_overflow_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of reading PCI performance counters 1: outbound stalled counters:
 *  - outbound_stalled_reads
 *  - outbound_stalled_writes
 *  - outbound_stalled_reads_events
 *  - outbound_stalled_writes_events
 *
 * @note when not supported the values will contain the value 0
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports PCI performance counters 1: outbound stalled counters.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI performance counters 1: outbound stalled counters.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_cap_perf_counters_1_outbound_stalled_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of reading PCI performance counters 1: FEC error counters:
 *  - fec_correctable_error_counter
 *  - fec_uncorrectable_error_counter
 *
 * @note when not supported the values will contain the value 0
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports PCI performance counters 1: FEC error counters.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI performance counters 1: FEC error counters.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_cap_perf_counters_1_fec_error_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of reading PCI performance counters 1: FBER counter:
 *  - fber_magnitude
 *  - fber_coef
 *
 * @note when not supported the values will contain the value 0
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports PCI performance counters 1: FBER counter.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI performance counters 1: FBER counter.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_cap_perf_counters_1_fber_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Read PCI performance counters 1.
 *
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 * @param [in] dpn
 * DPN of the PCI link to get values for.
 * @param [out] counters
 * Pointer to doca_telemetry_pci_perf_counters_1 structure to populate.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI performance counters 1.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_read_perf_counters_1(struct doca_telemetry_pci *telemetry_pci,
						     struct doca_telemetry_pci_dpn dpn,
						     struct doca_telemetry_pci_perf_counters_1 *counters);

/**
 * @brief Read PCI performance counters 1.
 *
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 * @param [in] pci_addr
 * PCI addr of the PCI link to get values for. Should be as one of the following formats:
 * - "Domain:Bus:Device.Function", e.g., "0000:3a:00.0" (size DOCA_DEVINFO_PCI_ADDR_SIZE including a null terminator).
 * - "Bus:Device.Function", e.g., "3a:00.0" (size DOCA_DEVINFO_PCI_BDF_SIZE including a null terminator), Domain is
 *   assumed to be "0000" (i.e. "0000:<pci_addr_str>").
 * @param [out] counters
 * Pointer to doca_telemetry_pci_perf_counters_1 structure to populate.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI performance counters 1.
 * - DOCA_ERROR_NOT_FOUND - the provided PCI addr did not map to a PCI link that can be queried.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_read_perf_counters_1_by_pci_addr(struct doca_telemetry_pci *telemetry_pci,
								 const char *pci_addr,
								 struct doca_telemetry_pci_perf_counters_1 *counters);

/*********************************************************************************************************************
 *
 * PCI latency histogram
 *
 *********************************************************************************************************************/

/**
 * @brief Check if given device is capable of reading PCI latency histogram
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports PCI latency histogram.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI latency histogram.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_cap_latency_histogram_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Get the PCI latency histogram dimensions
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 * @param [in] dpn
 * DPN of the PCI link to get values for.
 * @param [out] bucket_count
 * Number of buckets in the histogram
 * @param [out] bucket_width_ns
 * Width of each bucket (in nanoseconds)
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI latency histogram.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_get_latency_histogram_dimensions(struct doca_telemetry_pci *telemetry_pci,
								 struct doca_telemetry_pci_dpn dpn,
								 uint32_t *bucket_count,
								 uint32_t *bucket_width_ns);

/**
 * @brief Get the PCI latency histogram dimensions
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 * @param [in] pci_addr
 * PCI addr of the PCI link to get values for. Should be as one of the following formats:
 * - "Domain:Bus:Device.Function", e.g., "0000:3a:00.0" (size DOCA_DEVINFO_PCI_ADDR_SIZE including a null terminator).
 * - "Bus:Device.Function", e.g., "3a:00.0" (size DOCA_DEVINFO_PCI_BDF_SIZE including a null terminator), Domain is
 *   assumed to be "0000" (i.e. "0000:<pci_addr_str>").
 * @param [out] bucket_count
 * Number of buckets in the histogram
 * @param [out] bucket_width_ns
 * Width of each bucket (in nanoseconds)
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI latency histogram.
 * - DOCA_ERROR_NOT_FOUND - the provided PCI addr did not map to a PCI link that can be queried.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_get_latency_histogram_dimensions_by_pci_addr(struct doca_telemetry_pci *telemetry_pci,
									     const char *pci_addr,
									     uint32_t *bucket_count,
									     uint32_t *bucket_width_ns);

/**
 * @brief Read the PCI latency histogram.
 *
 * The PCI latency histogram defines a number of time-bound buckets into which the number of PCI read operations
 * that completed within the timeframe of a given bucket are recorded.
 *
 * For example if the latency histogram was 4 buckets of 500 nanoseconds each:
 *  +-------------------+-------------------+-------------------+-------------------+
 *  | bucket_arr[0]     | bucket_arr[1]     | bucket_arr[2]     | bucket_arr[3]     |
 *  | [0-499]ns         | [500-999]ns       | [1000-1499]ns     | [1500-inf)ns      |
 *  +-------------------+-------------------+-------------------+-------------------+
 *
 * When each PCI read operation completes; its latency is compared to the bucket dimensions and the appropriate counter
 * is incremented. For example if a PCI read operation completed in 502ns then bucket_arr[1] would be incremented. If
 * another request took 3000ns then bucket_arr[3] would be incremented.
 *
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 * @param [in] dpn
 * DPN of the PCI link to get values for.
 * @param [out] bucket_arr
 * Array of buckets to populate. MUST be a pointer to an array of bucket_count uint64_t elements.
 * (See: doca_telemetry_pci_get_latency_histogram_dimensions)
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI latency histogram.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_read_latency_histogram(struct doca_telemetry_pci *telemetry_pci,
						       struct doca_telemetry_pci_dpn dpn,
						       uint64_t *bucket_arr);

/**
 * @brief Read the PCI latency histogram.
 *
 * The PCI latency histogram defines a number of time-bound buckets into which the number of PCI read operations
 * that completed within the timeframe of a given bucket are recorded.
 *
 * For example if the latency histogram was 4 buckets of 500 nanoseconds each:
 *  +-------------------+-------------------+-------------------+-------------------+
 *  | bucket_arr[0]     | bucket_arr[1]     | bucket_arr[2]     | bucket_arr[3]     |
 *  | [0-499]ns         | [500-999]ns       | [1000-1499]ns     | [1500-inf)ns      |
 *  +-------------------+-------------------+-------------------+-------------------+
 *
 * When each PCI read operation completes; its latency is compared to the bucket dimensions and the appropriate counter
 * is incremented. For example if a PCI read operation completed in 502ns then bucket_arr[1] would be incremented. If
 * another request took 3000ns then bucket_arr[3] would be incremented.
 *
 * @param [in] telemetry_pci
 * Pointer to PCI telemetry instance.
 * @param [in] pci_addr
 * PCI addr of the PCI link to get values for. Should be as one of the following formats:
 * - "Domain:Bus:Device.Function", e.g., "0000:3a:00.0" (size DOCA_DEVINFO_PCI_ADDR_SIZE including a null terminator).
 * - "Bus:Device.Function", e.g., "3a:00.0" (size DOCA_DEVINFO_PCI_BDF_SIZE including a null terminator), Domain is
 *   assumed to be "0000" (i.e. "0000:<pci_addr_str>").
 * @param [out] bucket_arr
 * Array of buckets to populate. MUST be a pointer to an array of bucket_count uint64_t elements.
 * (See: doca_telemetry_pci_get_latency_histogram_dimensions)
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_DRIVER - internal driver error.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support PCI latency histogram.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_pci_read_latency_histogram_by_pci_addr(struct doca_telemetry_pci *telemetry_pci,
								   const char *pci_addr,
								   uint64_t *bucket_arr);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_TELEMETRY_PCI_H_ */

/** @} */
