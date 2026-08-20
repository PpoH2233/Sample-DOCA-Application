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
 * @file doca_telemetry_phy.h
 * @page DOCA_TELEMETRY_PHY
 * @defgroup DOCA_TELEMETRY_PHY DOCA telemetry PHY
 * DOCA telemetry PHY library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_TELEMETRY_PHY_H_
#define DOCA_TELEMETRY_PHY_H_

#include <doca_error.h>
#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DOCA_TELEMETRY_PHY_MODULE_INFO_ASCII_STRING_SIZE \
	(17) /**< Max character count of the ASCII string in the module info vendor name, vendor part number, vendor \
		serial number, etc. */
#define DOCA_TELEMETRY_PHY_MAX_LANES 8		 /**< Maximum Number of module lanes supported. */
#define DOCA_TELEMETRY_PHY_FEC_MAX_BINS_RANGE 16 /**< Maximum Number of bins supported. */
#define DOCA_TELEMETRY_PHY_TS_INFO_STATUS_MSG_STRING_SIZE \
	(237) /**< Max character count of the ASCII string in the troubleshooting info status message. */
#define DOCA_TELEMETRY_PHY_MAX_PAGES 256 /**< Maximum Number of pages supported. */

/**********************************************************************************************************************
 * DOCA core opaque types
 *********************************************************************************************************************/
struct doca_dev;
struct doca_devinfo;

/*********************************************************************************************************************
 * DOCA Telemetry PHY Context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA Telemetry PHY instance.
 */
struct doca_telemetry_phy;

/**
 *@brief doca_telemetry_phy_protocol
 */
enum doca_telemetry_phy_protocol {
	DOCA_TELEMETRY_PHY_PROTOCOL_IB = 0x1,
	DOCA_TELEMETRY_PHY_PROTOCOL_ETH = 0x4
};

/**
 *@brief doca_telemetry_phy_mng_state
 */
enum doca_telemetry_phy_mng_state {
	DOCA_TELEMETRY_PHY_STATE_DISABLED = 0x0,
	DOCA_TELEMETRY_PHY_STATE_OPEN_PORT = 0x1,
	DOCA_TELEMETRY_PHY_STATE_POLLING = 0x2,
	DOCA_TELEMETRY_PHY_STATE_ACTIVE = 0x3,
	DOCA_TELEMETRY_PHY_STATE_CLOSE_PORT = 0x4,
	DOCA_TELEMETRY_PHY_STATE_PHY_UP = 0x5,
	DOCA_TELEMETRY_PHY_STATE_SLEEP = 0x6,
	DOCA_TELEMETRY_PHY_STATE_RX_DISABLE = 0x7,
	DOCA_TELEMETRY_PHY_STATE_SIGNAL_DETECT = 0x8,
	DOCA_TELEMETRY_PHY_STATE_RCVR_DETECT = 0x9,
	DOCA_TELEMETRY_PHY_STATE_SYNC_PEER = 0xa,
	DOCA_TELEMETRY_PHY_STATE_NEGOTIATION = 0xb,
	DOCA_TELEMETRY_PHY_STATE_TRAINING = 0xc,
	DOCA_TELEMETRY_PHY_STATE_SUBFSM_ACTIVE = 0xd,
	DOCA_TELEMETRY_PHY_STATE_PROTOCOL_DETECT = 0xe,
	DOCA_TELEMETRY_PHY_STATE_UNKNOWN = 0xf
};

/**
 *@brief doca_telemetry_phy_ib_phy_state
 */
enum doca_telemetry_phy_ib_phy_state {
	DOCA_TELEMETRY_PHY_IB_PHY_STATE_AN_FSM_DISABLED = 0x0,
	DOCA_TELEMETRY_PHY_IB_PHY_STATE_AN_FSM_INITIALIZING = 0x1,
	DOCA_TELEMETRY_PHY_IB_PHY_STATE_AN_FSM_RECOVER_CFG = 0x2,
	DOCA_TELEMETRY_PHY_IB_PHY_STATE_AN_FSM_CFG_TEST = 0x3,
	DOCA_TELEMETRY_PHY_IB_PHY_STATE_AN_FSM_WAIT_REMOTE_TEST = 0x4,
	DOCA_TELEMETRY_PHY_IB_PHY_STATE_AN_FSM_WAIT_CFG_ENHANCED = 0x5,
	DOCA_TELEMETRY_PHY_IB_PHY_STATE_AN_FSM_CFG_IDLE = 0x6,
	DOCA_TELEMETRY_PHY_IB_PHY_STATE_AN_FSM_LINK_UP = 0x7,
	DOCA_TELEMETRY_PHY_IB_PHY_STATE_AN_FSM_POLLING = 0x8
};

/**
 *@brief doca_telemetry_phy_eth_phy_state
 */
enum doca_telemetry_phy_eth_phy_state {
	DOCA_TELEMETRY_PHY_ETH_PHY_STATE_ENABLE = 0x0,
	DOCA_TELEMETRY_PHY_ETH_PHY_STATE_XMIT_DISABLE = 0x1,
	DOCA_TELEMETRY_PHY_ETH_PHY_STATE_ABILITY_DETECT = 0x2,
	DOCA_TELEMETRY_PHY_ETH_PHY_STATE_ACK_DETECT = 0x3,
	DOCA_TELEMETRY_PHY_ETH_PHY_STATE_COMPLETE_ACK = 0x4,
	DOCA_TELEMETRY_PHY_ETH_PHY_STATE_AN_GOOD_CHECK = 0x5,
	DOCA_TELEMETRY_PHY_ETH_PHY_STATE_LINK_UP = 0x6,
	DOCA_TELEMETRY_PHY_ETH_PHY_STATE_NEXT_PAGE_WAIT = 0x7
};

/**
 *@brief doca_telemetry_phy_state
 */
union doca_telemetry_phy_state {
	enum doca_telemetry_phy_ib_phy_state ib_phy_state;   /**< IB phy state. */
	enum doca_telemetry_phy_eth_phy_state eth_phy_state; /**< Eth phy state. */
};

/**
 *@brief doca_telemetry_phy_eth_link_speed
 */
enum doca_telemetry_phy_eth_link_speed {
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_UNKNOWN = 0x0,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_10M = 0x1,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_10M_BASE_T = 0x2,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_100M = 0x4,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_100M_BASE_TX = 0x8,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_1000M_BASE_T = 0x10,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_CX = 0x20,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_KX = 0x40,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_CX4 = 0x80,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_KX4 = 0x100,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_1G = 0x400,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_5G = 0x1000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_10G_BASE_T = 0x2000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_10G = 0x4000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_25G = 0x8000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_40G = 0x10000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_50G = 0x20000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_100G = 0x40000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_200G = 0x80000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_400G = 0x100000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_800G = 0x200000,
	DOCA_TELEMETRY_PHY_ETH_LINK_SPEED_1600G = 0x400000,
};

/**
 *@brief doca_telemetry_phy_ib_link_speed
 */
enum doca_telemetry_phy_ib_link_speed {
	DOCA_TELEMETRY_PHY_IB_LINK_SPEED_SDR = 0x1,
	DOCA_TELEMETRY_PHY_IB_LINK_SPEED_DDR = 0x2,
	DOCA_TELEMETRY_PHY_IB_LINK_SPEED_QDR = 0x4,
	DOCA_TELEMETRY_PHY_IB_LINK_SPEED_FDR10 = 0x8,
	DOCA_TELEMETRY_PHY_IB_LINK_SPEED_FDR = 0x10,
	DOCA_TELEMETRY_PHY_IB_LINK_SPEED_EDR = 0x20,
	DOCA_TELEMETRY_PHY_IB_LINK_SPEED_HDR = 0x40,
	DOCA_TELEMETRY_PHY_IB_LINK_SPEED_NDR = 0x80,
	DOCA_TELEMETRY_PHY_IB_LINK_SPEED_XDR = 0x100
};

/**
 *@brief doca_telemetry_phy_link_speed
 */
union doca_telemetry_phy_link_speed {
	enum doca_telemetry_phy_eth_link_speed link_speed_eth; /**< Link speed Eth. */
	enum doca_telemetry_phy_ib_link_speed link_speed_ib;   /**< Link speed IB. */
};

/**
 *@brief doca_telemetry_phy_fec_mode
 */
enum doca_telemetry_phy_fec_mode {
	DOCA_TELEMETRY_PHY_NO_FEC = 0x0,
	DOCA_TELEMETRY_PHY_FIRECODE_FEC = 0x1,
	DOCA_TELEMETRY_PHY_STANDARD_RS_FEC_528_514 = 0x2,
	DOCA_TELEMETRY_PHY_STANDARD_LL_RS_FEC_271_257 = 0x3,
	DOCA_TELEMETRY_PHY_INTERLEAVED_QUAD_RS_FEC_544_514 = 0x4,
	DOCA_TELEMETRY_PHY_INTERLEAVED_QUAD_RS_FEC_PLR_546_516 = 0x5,
	DOCA_TELEMETRY_PHY_INTERLEAVED_STANDARD_RS_544_514 = 0x6,
	DOCA_TELEMETRY_PHY_STANDARD_RS_FEC_544_514 = 0x7,
	DOCA_TELEMETRY_PHY_INTERLEAVED_OCTET_RS_FEC_PLR_546_516 = 0x8,
	DOCA_TELEMETRY_PHY_ETH_CONSORTIUM_LL_50G_RS_FEC_272_257_PLUS_1 = 0x9,
	DOCA_TELEMETRY_PHY_INTERLEAVED_ETH_CONSORTIUM_LL_50G_RS_FEC_272_257_PLUS_1 = 0xa,
	DOCA_TELEMETRY_PHY_INTERLEAVED_STANDARD_RS_FEC_PLR = 0xb
};

/**
 *@brief doca_telemetry_phy_loopback_mode
 */
enum doca_telemetry_phy_loopback_mode {
	DOCA_TELEMETRY_PHY_NO_LOOPBACK_ACTIVE = 0x0,
	DOCA_TELEMETRY_PHY_REMOTE_LOOPBACK = 0x1,
	DOCA_TELEMETRY_PHY_LOCAL_LOOPBACK = 0x2,
	DOCA_TELEMETRY_PHY_EXTERNAL_LOCAL_LOOPBACK = 0x4
};

/**
 *@brief doca_telemetry_phy_auto_negotiation
 */
enum doca_telemetry_phy_auto_negotiation {
	DOCA_TELEMETRY_PHY_AUTO_NEGOTIATION_ON = 0x0,
	DOCA_TELEMETRY_PHY_AUTO_NEGOTIATION_FORCE = 0x1
};

/**
 *@brief doca_telemetry_phy_operation_info
 */
struct doca_telemetry_phy_operation_info {
	enum doca_telemetry_phy_protocol active_protocol;	   /**< Active protocol. */
	enum doca_telemetry_phy_mng_state state;		   /**< FW Phy Manager FSM state. */
	union doca_telemetry_phy_state phy_state;		   /**< Physical state. */
	union doca_telemetry_phy_link_speed link_speed_active;	   /**< Link speed active. */
	uint8_t link_width;					   /**< link_width. */
	enum doca_telemetry_phy_fec_mode fec_mode_active;	   /**< FEC mode active. */
	enum doca_telemetry_phy_loopback_mode loopback_mode;	   /**< Loopback mode. */
	enum doca_telemetry_phy_auto_negotiation auto_negotiation; /**< Auto-negotiation. */
};

/**
 *@brief doca_telemetry_phy_supported_speed
 */
union doca_telemetry_phy_supported_speed {
	uint32_t speed_eth; /**< Bitmask for supported speed Eth. See enum doca_telemetry_phy_eth_link_speed.  */
	uint16_t speed_ib;  /**< Bitmask for supported speed IB. See enum doca_telemetry_phy_ib_link_speed.  */
};

/**
 *@brief doca_telemetry_phy_supported_info
 */
struct doca_telemetry_phy_supported_info {
	enum doca_telemetry_phy_protocol active_protocol;		/**< Active protocol. */
	union doca_telemetry_phy_supported_speed enabled_link_speed;	/**< Enabled link speed. */
	union doca_telemetry_phy_supported_speed supported_cable_speed; /**< Supported link speed. */
};

/**
 *@brief doca_telemetry_phy_group_opcode
 */
enum doca_telemetry_phy_status_opcode {
	DOCA_TELEMETRY_PHY_FW_NO_ISSUE_OBSERVED = 0x0,
	DOCA_TELEMETRY_PHY_FW_PORT_IS_CLOSE_BY_COMM = 0x1,
	DOCA_TELEMETRY_PHY_FW_AN_FAIL = 0x2,
	DOCA_TELEMETRY_PHY_FW_LINK_TRAINING_FAIL = 0x3,
	DOCA_TELEMETRY_PHY_FW_LOGICAL_MISMATCH_BETWEEN_LINK_PARTNERS = 0x4,
	DOCA_TELEMETRY_PHY_FW_REMOTE_FAULT_RECEIVED = 0x5,
	DOCA_TELEMETRY_PHY_FW_BAD_SIGNAL_INTEGRITY = 0x6,
	DOCA_TELEMETRY_PHY_FW_CABLE_CC_MISMATCH = 0x7,
	DOCA_TELEMETRY_PHY_FW_INTERNAL_ERROR = 0x8,
	DOCA_TELEMETRY_PHY_FW_SPEED_DEGRADATION = 0x9,
	DOCA_TELEMETRY_PHY_FW_MDL_LANES_FREQUENCY_NOT_SYNCED = 0xa,
	DOCA_TELEMETRY_PHY_FW_SIGNAL_NOT_DETECTED = 0xb,
	DOCA_TELEMETRY_PHY_FW_NO_PARTNER_DETECTED_FOR_LONG_TIME = 0xc,
	DOCA_TELEMETRY_PHY_FW_TROUBLESHOOTING_IN_PROCESS = 0xd,
	DOCA_TELEMETRY_PHY_FW_INFO_NOT_AVAILABLE = 0xe,
	DOCA_TELEMETRY_PHY_MNG_FW_CABLE_IS_UNPLUGGED = 0xf,
	DOCA_TELEMETRY_PHY_MNG_FW_LONG_RANGE_FOR_NON_MLX_CABLE_MDL = 0x10,
	DOCA_TELEMETRY_PHY_MNG_FW_BUS_STUCK_I2C_DATA_OR_CLOCK_SHORTED = 0x11,
	DOCA_TELEMETRY_PHY_MNG_FW_BAD_OR_UNSUPPORTED_EEPROM = 0x12,
	DOCA_TELEMETRY_PHY_MNG_FW_PART_NUMBER_LIST = 0x13,
	DOCA_TELEMETRY_PHY_MNG_FW_UNSUPPORTED_CABLE = 0x14,
	DOCA_TELEMETRY_PHY_MNG_FW_MDL_TEMPERATURE_SHUTDOWN = 0x15,
	DOCA_TELEMETRY_PHY_MNG_FW_SHORTED_CABLE = 0x16,
	DOCA_TELEMETRY_PHY_MNG_FW_PWD_BUDGET_EXCEEDED = 0x17,
	DOCA_TELEMETRY_PHY_MNG_FW_MANAGEMENT_FORCED_DOWN_THE_PORT = 0x18,
	DOCA_TELEMETRY_PHY_MNG_FW_MDL_IS_DISABLED_BY_COMM = 0x19,
	DOCA_TELEMETRY_PHY_MNG_FW_SYS_PWR_EXCEEDED_SO_MDL_PWD_OFF = 0x1a,
	DOCA_TELEMETRY_PHY_MNG_FW_MDLS_PMD_TYPE_IS_NOT_ENABLED = 0x1b,
	DOCA_TELEMETRY_PHY_MNG_FW_PCIE_SYS_PWD_SLOT_EXCEEDED = 0x1c,
	DOCA_TELEMETRY_PHY_MNG_FW_MDL_STATE_MACHINE_FAULT = 0x1d,
	DOCA_TELEMETRY_PHY_MNG_FW_MDLS_STAMPING_SPEED_DEGENERATION = 0x1e,
	DOCA_TELEMETRY_PHY_MNG_FW_MDLS_DATAPATH_FSM_FAULT = 0x1f,
	DOCA_TELEMETRY_PHY_MNG_FW_MDL_BOOT_ERROR = 0x20,
	DOCA_TELEMETRY_PHY_MNG_FW_MDL_FORCED_TO_LOW_PWD_BY_COM = 0x21,
	DOCA_TELEMETRY_PHY_OP_FW_NA = 0xffff
};

/**
 *@brief doca_telemetry_phy_group_opcode_alt
 */
enum doca_telemetry_phy_group_opcode {
	DOCA_TELEMETRY_PHY_FW_GOP = 0x0,
	DOCA_TELEMETRY_PHY_MNG_FW_GOP = 0x1,
	DOCA_TELEMETRY_PHY_GOP_NA = 0xf
};

/**
 *@brief doca_telemetry_phy_troubleshooting_info
 */
struct doca_telemetry_phy_troubleshooting_info {
	uint16_t status_opcode_raw;						/**< Status opcode raw value.  */
	enum doca_telemetry_phy_status_opcode status_opcode;			/**< Status opcode. */
	enum doca_telemetry_phy_group_opcode group_opcode;			/**< Group opcode. */
	char status_message[DOCA_TELEMETRY_PHY_TS_INFO_STATUS_MSG_STRING_SIZE]; /**< Status message. */
};

/**
 *@brief doca_telemetry_phy_error_code
 */
enum doca_telemetry_phy_error_code {
	DOCA_TELEMETRY_PHY_ERROR_CODE_CFG_UNDEFINED = 0x0,
	DOCA_TELEMETRY_PHY_ERROR_CODE_CFG_SUCCESS = 0x1,
	DOCA_TELEMETRY_PHY_ERROR_CODE_CFG_REJECTED = 0x2,
	DOCA_TELEMETRY_PHY_ERROR_CODE_CFG_REJECTED_INVALID_APP_SEL = 0x3,
	DOCA_TELEMETRY_PHY_ERROR_CODE_CFG_REJECTED_INVALID_DATA_PATH = 0x4,
	DOCA_TELEMETRY_PHY_ERROR_CODE_CFG_REJECTED_INVALID_SI = 0x5,
	DOCA_TELEMETRY_PHY_ERROR_CODE_CFG_REJECTED_LANES_IN_USE = 0x6,
	DOCA_TELEMETRY_PHY_ERROR_CODE_CFG_REJECTED_PARTIAL_DATA_PATH = 0x7,
	DOCA_TELEMETRY_PHY_ERROR_CODE_CFG_IN_PROGRESS = 0xc,
	DOCA_TELEMETRY_PHY_ERROR_CODE_NA = 0xff
};

/**
 *@brief doca_telemetry_phy_cable_vendor
 */
enum doca_telemetry_phy_cable_vendor {
	DOCA_TELEMETRY_PHY_CABLE_VENDOR_OTHER = 0x0,
	DOCA_TELEMETRY_PHY_CABLE_VENDOR_MELLANOX = 0x1,
	DOCA_TELEMETRY_PHY_CABLE_VENDOR_KNOWN_OUI = 0x2,
	DOCA_TELEMETRY_PHY_CABLE_VENDOR_NVIDIA = 0x3,
	DOCA_TELEMETRY_PHY_CABLE_VENDOR_NA = 0xff
};

/**
 *@brief doca_telemetry_phy_date
 */
struct doca_telemetry_phy_date {
	uint8_t year;  /**< Year. 00 = year 2000. */
	uint8_t month; /**< Month of the year. */
	uint8_t day;   /**< Day of the month. */
};

/**
 *@brief doca_telemetry_phy_manufacturing_date
 */
struct doca_telemetry_phy_manufacturing_date {
	struct doca_telemetry_phy_date date; /**< Date. */
	char lot[2];			     /**< LOT code. */
};

/**
 *@brief doca_telemetry_phy_cable_vendor_info
 */
struct doca_telemetry_phy_cable_vendor_info {
	enum doca_telemetry_phy_cable_vendor cable_vendor;			     /**< Cable vendor. */
	char vendor_name[DOCA_TELEMETRY_PHY_MODULE_INFO_ASCII_STRING_SIZE];	     /**< Vendor name. */
	char vendor_part_number[DOCA_TELEMETRY_PHY_MODULE_INFO_ASCII_STRING_SIZE];   /**< Vendor part number. */
	char vendor_serial_number[DOCA_TELEMETRY_PHY_MODULE_INFO_ASCII_STRING_SIZE]; /**< Vendor serial number. */
	char vendor_rev_number[DOCA_TELEMETRY_PHY_MODULE_INFO_ASCII_STRING_SIZE];    /**< Vendor revision number. */
	struct doca_telemetry_phy_manufacturing_date manufacturing_date;	     /**< Vendor manufacturing date. */
};

/**
 *@brief doca_telemetry_phy_ib_cable_width
 */
enum doca_telemetry_phy_ib_cable_width {
	DOCA_TELEMETRY_PHY_IB_CABLE_WIDTH_1X = 0x1,
	DOCA_TELEMETRY_PHY_IB_CABLE_WIDTH_2X = 0x2,
	DOCA_TELEMETRY_PHY_IB_CABLE_WIDTH_4X = 0x4,
	DOCA_TELEMETRY_PHY_IB_CABLE_WIDTH_8X = 0x8,
};

/**
 *@brief doca_telemetry_phy_firmware_version
 */
struct doca_telemetry_phy_firmware_version {
	uint8_t chip_id; /**< Chip ID. */
	uint8_t major;	 /**< Major version. */
	uint16_t minor;	 /**< Minor version. */
};

/**
 *@brief doca_telemetry_phy_QSFP_CMIS_cable_technology
 */
enum doca_telemetry_phy_QSFP_CMIS_cable_technology {
	DOCA_TELEMETRY_PHY_QSFP_CMIS_VCSEL_850NM = 0x0,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_VCSEL_1310NM = 0x1,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_VCSEL_1550NM = 0x2,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_FP_LASER_1310NM = 0x3,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_DFB_LASER_1310NM = 0x4,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_DFB_LASER_1550NM = 0x5,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_EML_1310NM = 0x6,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_EML_1550NM = 0x7,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_OTHERS = 0x8,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_DFB_LASER_1490NM = 0x9,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_PASSIVE_COPPER_CABLE_UNEQD = 0xa,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_PASSIVE_COPPER_CABLE_EQD = 0xb,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_COPPER_CABLE_NEAR_END_AND_FAR_END_LIMITING_ACT_EQ = 0xc,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_COPPER_CABLE_FAR_END_LIMITING_ACT_EQ = 0xd,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_COPPER_CABLE_NEAR_END_LIMITING_ACT_EQ = 0xe,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_COPPER_CABLE_LINEAR_ACT_EQS = 0xf,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_C_BAND_TUNABLE_LASER = 0x10,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_L_BAND_TUNABLE_LASER = 0x11,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_COPPER_CABLE_NEAR_END_AND_FAR_END_LINEAR_ACT_EQS = 0x12,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_COPPER_CABLE_FAR_END_LINEAR_ACT_EQS = 0x13,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_COPPER_CABLE_NEAR_END_LINEAR_ACT_EQS = 0x14,
	DOCA_TELEMETRY_PHY_QSFP_CMIS_NA = 0xffff
};

/**
 *@brief doca_telemetry_phy_SFP_cable_technology
 */
enum doca_telemetry_phy_SFP_cable_technology {
	DOCA_TELEMETRY_PHY_SFP_PASSIVE = 0x4,
	DOCA_TELEMETRY_PHY_SFP_ACTIVE = 0x8,
	DOCA_TELEMETRY_PHY_SFP_NA = 0xffff
};

/**
 *@brief doca_telemetry_phy_cable_technology
 */
union doca_telemetry_phy_cable_technology {
	enum doca_telemetry_phy_QSFP_CMIS_cable_technology QSFP_CMIS_cable_technology; /**< QSFP or CMIS cable
											  technology. */
	enum doca_telemetry_phy_SFP_cable_technology SFP_cable_technology;	       /**< SFP cable technology. */
};

/**
 *@brief doca_telemetry_phy_cable_identifier
 */
enum doca_telemetry_phy_cable_identifier {
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_QSFP28 = 0x0,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_QSFP_PLUS = 0x1,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_SFP28_OR_SFP_PLUS = 0x2,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_QSA_QSFP_SFP = 0x3,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_BLACKPLANE = 0x4,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_SFP_DD = 0x5,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_QSFP_DD = 0x6,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_QSFP_CMIS = 0x7,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_OSFP = 0x8,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_C2C = 0x9,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_DSFP = 0xa,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_QSFP_SPLIT_CABLE = 0xb,
	DOCA_TELEMETRY_PHY_CABLE_IDENTIFIER_NA = 0xffff
};

/**
 *@brief doca_telemetry_phy_cable_type
 */
enum doca_telemetry_phy_cable_type {
	DOCA_TELEMETRY_PHY_CABLE_TYPE_UNIDENTIFIED = 0x0,
	DOCA_TELEMETRY_PHY_CABLE_TYPE_ACTIVE_CABLE = 0x1,
	DOCA_TELEMETRY_PHY_CABLE_TYPE_OPTICAL_MODULE = 0x2,
	DOCA_TELEMETRY_PHY_CABLE_TYPE_PASSIVE_COPPER_CABLE_OR_LINEAR_COPPER = 0x3,
	DOCA_TELEMETRY_PHY_CABLE_TYPE_CABLE_UNPLUGGED = 0x4,
	DOCA_TELEMETRY_PHY_CABLE_TYPE_TWISTED_PAIR = 0x5,
};

/**
 *@brief doca_telemetry_phy_QSFP_cc
 */
enum doca_telemetry_phy_QSFP_cc {
	DOCA_TELEMETRY_PHY_QSFP_CC_UNSPECIFIED = 0,
	DOCA_TELEMETRY_PHY_QSFP_CC_40G_ACTIVE_CABLE_XLPPI = 0x1,
	DOCA_TELEMETRY_PHY_QSFP_CC_40GBASE_LR4 = 0x2,
	DOCA_TELEMETRY_PHY_QSFP_CC_40GBASE_SR4 = 0x4,
	DOCA_TELEMETRY_PHY_QSFP_CC_40GBASE_CR4 = 0x8,
	DOCA_TELEMETRY_PHY_QSFP_CC_10GBASE_SR = 0x10,
	DOCA_TELEMETRY_PHY_QSFP_CC_10GBASE_LR = 0x20,
	DOCA_TELEMETRY_PHY_QSFP_CC_10GBASE_LRM = 0x40,
	DOCA_TELEMETRY_PHY_QSFP_CC_EXT = 0x80,
};

/**
 *@brief doca_telemetry_phy_SFP_cc
 */
enum doca_telemetry_phy_SFP_cc {
	DOCA_TELEMETRY_PHY_SFP_CC_UNSPECIFIED = 0,
	DOCA_TELEMETRY_PHY_SFP_CC_10G_BASE_SR = 0x10,
	DOCA_TELEMETRY_PHY_SFP_CC_10G_BASE_LR = 0x20,
	DOCA_TELEMETRY_PHY_SFP_CC_10G_BASE_LRM = 0x40,
	DOCA_TELEMETRY_PHY_SFP_CC_10G_BASE_ER = 0x80,
};

/**
 *@brief doca_telemetry_phy_QSFP_SFP_common_cc
 */
enum doca_telemetry_phy_QSFP_SFP_common_cc {
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_UNSPECIFIED = 0,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_AOC_OR_25GAUI_C2M_AOC_WITH_FEC = 0x1,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_SR4_OR_25GBASE_SR = 0x2,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_LR4_OR_25GBASE_LR = 0x3,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_ER4_OR_25GBASE_ER = 0x4,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_SR10 = 0x5,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_CWDM4 = 0x6,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_PSM4_PARALLEL_SMF = 0x7,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_ACC_OR_25GAUI_C2M_ACC_WITH_FEC = 0x8,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_CR4_OR_25GBASE_CR_CA_25G_L_OR_50GBASE_CR2_WITH_RS_CLAUSE91_FEC = 0xB,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_25GBASE_CR_CA_S_OR_50GBASE_CR2_WITH_BASE_R_CLAUSE_74_FIRE_CODE_FEC = 0xC,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_25GBASE_CR_CA_N_OR_50GBASE_CR2_WITH_NO_FEC = 0xD,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_40GBASE_ER4 = 0x10,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_4_X_10GBASE_SR = 0x11,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_40G_PSM4_PARALLEL_SMF = 0x12,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_G959_1_P1I1_2D1_10709_MBD_2KM_1310NM_SM = 0x13,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_G959_1_P1S1_2D2_10709_MBD_40KM_1550NM_SM = 0x14,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_G959_1_P1L1_2D2_10709_MBD_80KM_1550NM_SM = 0x15,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_10GBASE_T_WITH_SFI_ELECTRICAL_INTERFACE = 0x16,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_CLR4 = 0x17,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_AOC_OR_25GAUI_C2M_AOC_WITH_NO_FEC = 0x18,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_ACC_OR_25GAUI_C2M_ACC_WITH_NO_FEC = 0x19,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_DWDM2 = 0X1A,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_1550NM_WDM = 0X1B,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_10GBASE_T_2 = 0X1C,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_5GBASE_T = 0X1D,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_2_5GBASE_T = 0X1E,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_40G_SWDM4 = 0X1F,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_SWDM4 = 0X20,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_PAM4_BIDI = 0X21,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_4WDM10_MSA = 0X22,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_4WDM20_MSA = 0X23,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_4WDM40_MSA = 0X24,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_DR_WITH_CAUI_4_WITHOUT_FEC = 0X25,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_FR_WITH_CAUI_4_WITHOUT_FEC = 0X26,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100G_LR_WITH_CAUI_4_WITHOUT_FEC = 0X27,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_SR1_OR_200GBASE_SR2_OR_400GBASE_SR4 = 0X29,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_FR1_OR_400GBASE_DR4_2 = 0X2A,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_LR1 = 0X2B,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_ACC_10_6_WITH_50GAUI_OR_100GAUI_2_OR_200GAUI_4_C2M = 0X30,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_AOC_10_6_WITH_50GAUI_OR_100GAUI_2_OR_200GAUI_4_C2M = 0X31,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_ACC_10_4_WITH_50GAUI_OR_100GAUI_2_OR_200GAUI_4_C2M = 0X32,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_AOC_10_4_WITH_50GAUI_OR_100GAUI_2_OR_200GAUI_4_C2M = 0X33,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_VR1_OR_200GBASE_VR2_OR_400GBASE_VR4 = 0X36,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_100GBASE_CR1_OR_200GBASE_CR2_OR_400GBASE_CR4 = 0X3f,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_50GBASE_CR_OR_100GBASE_CR2_OR_200GBASE_CR4 = 0X40,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_50GBASE_SR_OR_100GBASE_SR2_OR_200GBASE_SR4 = 0X41,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_50GBASE_FR_OR_200GBASE_DR4 = 0X42,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_200GBASE_FR4 = 0X43,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_200GBASE_1550NM_PSM4 = 0X44,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_50GBASE_LR = 0X45,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_200GBASE_LR4 = 0X46,
	DOCA_TELEMETRY_PHY_QSFP_SFP_CC_400GBASE_DR4_OR_400GAUI_4_C2M = 0X47,
};

/**
 *@brief doca_telemetry_phy_CMIS_common_cc
 */
enum doca_telemetry_phy_CMIS_common_cc {
	DOCA_TELEMETRY_PHY_CMIS_CC_UNSPECIFIED = 0x0,
	DOCA_TELEMETRY_PHY_CMIS_CC_1000_BASE_CX = 0x1,
	DOCA_TELEMETRY_PHY_CMIS_CC_XAUI = 0x2,
	DOCA_TELEMETRY_PHY_CMIS_CC_XFI = 0x3,
	DOCA_TELEMETRY_PHY_CMIS_CC_SFI = 0x4,
	DOCA_TELEMETRY_PHY_CMIS_CC_25G_AUI = 0x5,
	DOCA_TELEMETRY_PHY_CMIS_CC_XL_AUI = 0x6,
	DOCA_TELEMETRY_PHY_CMIS_CC_XL_PPI = 0x7,
	DOCA_TELEMETRY_PHY_CMIS_CC_L_AUI2 = 0x8,
	DOCA_TELEMETRY_PHY_CMIS_CC_50G_AUI2 = 0x9,
	DOCA_TELEMETRY_PHY_CMIS_CC_50G_AUI1 = 0xa,
	DOCA_TELEMETRY_PHY_CMIS_CC_C_AUI4 = 0xb,
	DOCA_TELEMETRY_PHY_CMIS_CC_100G_AUI4 = 0xc,
	DOCA_TELEMETRY_PHY_CMIS_CC_100G_AUI2 = 0xd,
	DOCA_TELEMETRY_PHY_CMIS_CC_200G_AUI8 = 0xe,
	DOCA_TELEMETRY_PHY_CMIS_CC_200G_AUI4 = 0xf,
	DOCA_TELEMETRY_PHY_CMIS_CC_400G_AUI16 = 0x10,
	DOCA_TELEMETRY_PHY_CMIS_CC_400G_AUI8 = 0x11,
	DOCA_TELEMETRY_PHY_CMIS_CC_10G_BASE_CX4 = 0x13,
	DOCA_TELEMETRY_PHY_CMIS_CC_25G_CR_L = 0x14,
	DOCA_TELEMETRY_PHY_CMIS_CC_25G_CR_S = 0x15,
	DOCA_TELEMETRY_PHY_CMIS_CC_25G_CR_N = 0x16,
	DOCA_TELEMETRY_PHY_CMIS_CC_40G_BASE_CR4 = 0x17,
	DOCA_TELEMETRY_PHY_CMIS_CC_50G_BASE_CR = 0x18,
	DOCA_TELEMETRY_PHY_CMIS_CC_100G_BASE_CR10 = 0x19,
	DOCA_TELEMETRY_PHY_CMIS_CC_100G_BASE_CR4 = 0x1a,
	DOCA_TELEMETRY_PHY_CMIS_CC_100G_BASE_CR2 = 0x1b,
	DOCA_TELEMETRY_PHY_CMIS_CC_200G_BASE_CR4 = 0x1c,
	DOCA_TELEMETRY_PHY_CMIS_CC_400G_CR8 = 0x1d,
	DOCA_TELEMETRY_PHY_CMIS_CC_1000_BASE_T = 0x1e,
	DOCA_TELEMETRY_PHY_CMIS_CC_2_5G_BASE_T = 0x1f,
	DOCA_TELEMETRY_PHY_CMIS_CC_5G_BASE_T = 0x20,
	DOCA_TELEMETRY_PHY_CMIS_CC_10G_BASE_T = 0x21,
	DOCA_TELEMETRY_PHY_CMIS_CC_25_BASE_T = 0x22,
	DOCA_TELEMETRY_PHY_CMIS_CC_40_BASE_T = 0x23,
	DOCA_TELEMETRY_PHY_CMIS_CC_50_BASE_T = 0x24,
	DOCA_TELEMETRY_PHY_CMIS_CC_SDR = 0x2c,
	DOCA_TELEMETRY_PHY_CMIS_CC_FDR = 0x2f,
	DOCA_TELEMETRY_PHY_CMIS_CC_EDR = 0x30,
	DOCA_TELEMETRY_PHY_CMIS_CC_HDR = 0x31,
	DOCA_TELEMETRY_PHY_CMIS_CC_NDR = 0x32,
	DOCA_TELEMETRY_PHY_CMIS_CC_XDR = 0xA0,
	DOCA_TELEMETRY_PHY_CMIS_CC_100G_BASE_R1 = 0x46,
	DOCA_TELEMETRY_PHY_CMIS_CC_200G_BASE_R2 = 0x47,
	DOCA_TELEMETRY_PHY_CMIS_CC_400G_BASE_R4 = 0x48,
	DOCA_TELEMETRY_PHY_CMIS_CC_800G_ETC_CR8_OR_800GBASE_CR8 = 0x49,
};

/**
 *@brief doca_telemetry_phy_CMIS_optical_mm
 */
enum doca_telemetry_phy_CMIS_optical_mm {
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_UNSPECIFIED = 0,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_10G_BASE_SW = 0x1,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_10G_BASE_SR = 0x2,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_25G_BASE_SR = 0x3,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_40G_BASE_SR4 = 0x4,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_40G_SWDM4 = 0x5,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_40G_BIDI = 0x6,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_50G_BASE_SR = 0x7,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_100G_BASE_SR10 = 0x8,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_100G_BASE_SR4 = 0x9,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_100G_SWDM4 = 0xa,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_100G_BIDI = 0xb,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_100G_SR2 = 0xc,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_100G_SR = 0xd,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_200G_BASE_SR4 = 0xe,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_400G_BASE_SR16 = 0xf,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_400G_BASE_SR8 = 0x10,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_400G_SR4 = 0x11,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_800G_SR8 = 0x12,
	DOCA_TELEMETRY_PHY_CMIS_MM_CC_400G_BIDI = 0x1a,
};

/**
 *@brief doca_telemetry_phy_CMIS_optical_sm
 */
enum doca_telemetry_phy_CMIS_optical_sm {
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_UNSPECIFIED = 0,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_10G_BASE_LW = 0x1,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_10G_BASE_EW = 0x2,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_10G_ZW = 0x3,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_10G_BASE_LR = 0x4,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_10G_BASE_ER = 0x5,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_10G_BASE_ZR = 0x6,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_25G_BASE_LR = 0x7,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_25G_BASE_ER = 0x8,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_40G_BASE_LR4 = 0x9,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_40G_BASE_FR = 0xa,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_50G_BASE_FR = 0xb,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_50G_BASE_LR = 0xc,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_BASE_LR4 = 0xd,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_BASE_ER4 = 0xe,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_PSM4 = 0xf,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_CWDM4_OCP = 0x34,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_CWDM4 = 0x10,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_4WDM_10 = 0x11,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_4WDM_20 = 0x12,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_4WDM_40 = 0x13,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_BASE_DR = 0x14,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_FR = 0x15,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_100G_LR = 0x16,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_200G_BASE_DR4 = 0x17,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_200G_BASE_FR4 = 0x18,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_200G_BASE_LR4 = 0x19,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_400G_BASE_FR8 = 0x1a,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_400G_BASE_LR8 = 0x1b,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_400G_BASE_DR4 = 0x1c,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_400G_FR4 = 0x1d,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_400G_LR4 = 0x1e,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_10G_SR = 0x38,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_10G_LR = 0x39,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_25G_SR = 0x3a,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_25G_LR = 0x3b,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_10G_LR_BIDI = 0x3c,
	DOCA_TELEMETRY_PHY_CMIS_SM_CC_25G_LR_BIDI = 0x3d,
};

/**
 *@brief doca_telemetry_phy_CMIS_copper
 */
enum doca_telemetry_phy_CMIS_copper {
	DOCA_TELEMETRY_PHY_CMIS_COPPER_CC_UNSPECIFIED = 0,
	DOCA_TELEMETRY_PHY_CMIS_COPPER_CC_ASSEMBLY_BER_LT_1E_12 = 0x1,
	DOCA_TELEMETRY_PHY_CMIS_COPPER_CC_ASSEMBLY_BER_LT_5E_5 = 0x2,
	DOCA_TELEMETRY_PHY_CMIS_COPPER_CC_ASSEMBLY_BER_LT_2_6E_4 = 0x3,
	DOCA_TELEMETRY_PHY_CMIS_COPPER_CC_ASSEMBLY_BER_LT_1E_6 = 0x4,
};

/**
 *@brief doca_telemetry_phy_CMIS_specific
 */
union doca_telemetry_phy_CMIS_specific {
	enum doca_telemetry_phy_CMIS_copper copper_cc; /**< CMIS copper cable compliance code. */
	enum doca_telemetry_phy_CMIS_optical_sm sm_cc; /**< CMIS single mode optical cable compliance
							  code. */
	enum doca_telemetry_phy_CMIS_optical_mm mm_cc; /**< CMIS multi mode optical cable compliance
							  code. */
};

/**
 *@brief doca_telemetry_phy_CMIS
 */
struct doca_telemetry_phy_CMIS {
	enum doca_telemetry_phy_CMIS_common_cc common_cc;   /**< CMIS common compliance code. */
	union doca_telemetry_phy_CMIS_specific specific_cc; /**< CMIS specific compliance code. */
};

/**
 *@brief doca_telemetry_phy_QSFP_SFP_specific
 */
union doca_telemetry_phy_QSFP_SFP_specific {
	uint8_t qsfp_cc; /**< Bitmask for QSFP compliance code. See enum doca_telemetry_phy_QSFP_cc. */
	uint8_t sfp_cc;	 /**< Bitmask for SFP compliance code. See enum doca_telemetry_phy_SFP_cc. */
};

/**
 *@brief doca_telemetry_phy_QSFP_SFP
 */
struct doca_telemetry_phy_QSFP_SFP {
	enum doca_telemetry_phy_QSFP_SFP_common_cc common_cc;	/**< QSFP and SFP common compliance code. */
	union doca_telemetry_phy_QSFP_SFP_specific specific_cc; /**< QSFP and SFP specific compliance code. */
};

/**
 *@brief doca_telemetry_phy_compliance_code
 */
union doca_telemetry_phy_compliance_code {
	struct doca_telemetry_phy_CMIS cmis_cc;		/**< CMIS cable compliance code. */
	struct doca_telemetry_phy_QSFP_SFP qsfp_sfp_cc; /**< QSFP and SFP cable compliance code. Only applicable to
							   Ethernet protocol. */
};

/**
 *@brief doca_telemetry_phy_QSFP_far_end
 */
enum doca_telemetry_phy_QSFP_far_end {
	DOCA_TELEMETRY_PHY_QSFP_FAR_END_UNSPECIFIED = 0,
	DOCA_TELEMETRY_PHY_QSFP_SINGLE_FAR_END_4_CHS_IMP_OR_SEPAR_MOD_4_CH_CONN = 0x1,
	DOCA_TELEMETRY_PHY_QSFP_SINGLE_FAR_END_2_CHS_IMP_OR_SEPAR_MOD_2_CH_CONN = 0x2,
	DOCA_TELEMETRY_PHY_QSFP_SINGLE_FAR_END_1_CHS_IMP_OR_SEPAR_MOD_1_CH_CONN = 0x3,
	DOCA_TELEMETRY_PHY_QSFP_4_FAR_ENDS_1_CHS_IMP_IN_EACH = 0x4,
	DOCA_TELEMETRY_PHY_QSFP_2_FAR_ENDS_2_CHS_IMP_IN_EACH = 0x5,
	DOCA_TELEMETRY_PHY_QSFP_2_FAR_ENDS_1_CHS_IMP_IN_EACH = 0x6,
	DOCA_TELEMETRY_PHY_QSFP_FAR_END_NA = 0xff,
};

/**
 *@brief doca_telemetry_phy_QSFP_near_end
 */
enum doca_telemetry_phy_QSFP_near_end {
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_IMP_CH1_IMP_CH2_IMP_CH3_IMP = 0,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_NOTIMP_CH1_IMP_CH2_IMP_CH3_IMP = 0x1,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_IMP_CH1_NOT_IMP_CH2_IMP_CH3_IMP = 0x2,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_NOTIMP_CH1_NOTIMP_CH2_IMP_CH3_IMP = 0x3,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_IMP_CH1_IMP_CH2_NOTIMP_CH3_IMP = 0x4,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_NOTIMP_CH1_IMP_CH2_NOTIMP_CH3_IMP = 0x5,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_IMP_CH1_NOTIMP_CH2_NOTIMP_CH3_IMP = 0x6,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_NOTIMP_CH1_NOTIMP_CH2_NOTIMP_CH3_IMP = 0x7,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_IMP_CH1_IMP_CH2_IMP_CH3_NOTIMP = 0x8,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_NOTIMP_CH1_IMP_CH2_IMP_CH3_NOTIMP = 0x9,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_IMP_CH1_NOTIMP_CH2_IMP_CH3_NOTIMP = 0xa,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_NOTIMP_CH1_NOTIMP_CH2_IMP_CH3_NOTIMP = 0xb,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_IMP_CH1_IMP_CH2_NOTIMP_CH3_NOTIMP = 0xc,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_NOTIMP_CH1_IMP_CH2_NOTIMP_CH3_NOTIMP = 0xd,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_IMP_CH1_NOTIMP_CH2_NOTIMP_CH3_NOTIMP = 0xe,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_CH0_NOTIMP_CH1_NOTIMP_CH2_NOTIMP_CH3_NOTIMP = 0xf,
	DOCA_TELEMETRY_PHY_QSFP_NEAR_END_NA = 0xff,
};

/**
 *@brief doca_telemetry_phy_QSFP_cable_breakout
 */
struct doca_telemetry_phy_QSFP_cable_breakout {
	enum doca_telemetry_phy_QSFP_far_end far_end;	/**< Far end cable breakout. */
	enum doca_telemetry_phy_QSFP_near_end near_end; /**< Near end cable breakout. */
};

/**
 *@brief doca_telemetry_phy_CMIS_cable_breakout
 * X is a wildcard according to cable identifier
 * e.g. if cable identifier is 'OSFP', then 'X = (OSFP)'
 * DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_(OSFP)_TO_(OSFP) = 0x1
 */
enum doca_telemetry_phy_CMIS_cable_breakout {
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_UNSPECIFIED = 0,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_TO_X = 0x1,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_TO_2QSFP_OR_2X_4LANES = 0x2,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_TO_4DSFP_OR_4QSFP_2LANES = 0x3,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_TO_8SFP = 0x4,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_4LANES_TO_QSFP_OR_X_4LANES = 0x5,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_4LANES_TO_2X_2LANES_OR_2SFPDD = 0x6,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_4LANES_to_4SFP = 0x7,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_2LANES_to_X = 0x8,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_X_2LANES_to_2SFP = 0x9,
	DOCA_TELEMETRY_PHY_CMIS_CABLE_BREAKOUT_NA = 0xffff,
};

/**
 *@brief doca_telemetry_phy_cable_breakout
 */
union doca_telemetry_phy_cable_breakout {
	enum doca_telemetry_phy_CMIS_cable_breakout cmis_cable_breakout;   /**< CMIS cable breakout. */
	struct doca_telemetry_phy_QSFP_cable_breakout qsfp_cable_breakout; /**< QSFP cable breakout. */
};

/**
 *@brief doca_telemetry_phy_cable_general_properties_info
 */
struct doca_telemetry_phy_cable_general_properties_info {
	uint8_t memory_map_rev; /**< The cable management module memory map revision. */
	uint8_t ib_cable_width; /**< Bitmask of width of IB Protocols. See enum doca_telemetry_phy_ib_cable_width. */
	uint8_t linear_direct_drive; /**< Linear Direct Drive Optical Modules supported. 0 means not supported. */
	struct doca_telemetry_phy_firmware_version firmware_version; /**< Firmware version. Valid only if cable type is
								not passive. "{0,0,0}" means not supported. */
	union doca_telemetry_phy_cable_technology cable_technology;  /**< Cable_technology. */
	enum doca_telemetry_phy_cable_identifier cable_identifier;   /**< Cable identifier. */
	enum doca_telemetry_phy_cable_type cable_type;		     /**< Cable type. */
	float transfer_distance;				     /**< Transfer distance (cable length) in meters. */
	union doca_telemetry_phy_compliance_code compliance_code;    /**< Cable compliance code. Compliance code for
									QSFP_SFP cable is only applicable to Ethernet
									protocol. */
	union doca_telemetry_phy_cable_breakout cable_breakout;	     /**< Cable breakout. Not valid for SFP. */
	uint32_t smf_length;					     /**< SMF length in meters. */
	enum doca_telemetry_phy_CMIS_common_cc active_set_host_compliance_code;	 /**< Compliance code according to
										    current Active set, value of Host
										    Electrical Interface byte. Valid for
										    CMIS modules only. */
	union doca_telemetry_phy_CMIS_specific active_set_media_compliance_code; /**< Compliance code according to
										    current Active set, value of Module
										    Media Interface byte. Valid for CMIS
										    modules only. */
};

/**
 *@brief doca_telemetry_phy_temperature
 */
struct doca_telemetry_phy_temperature {
	int16_t temperature;	  /**< Module temperature in Celsius. */
	int16_t temperature_low;  /**< Alarm low temperature threshold in Celsius.  */
	int16_t temperature_high; /**< Alarm high temperature threshold in Celsius. */
};

/**
 *@brief doca_telemetry_phy_voltage
 */
struct doca_telemetry_phy_voltage {
	uint16_t voltage;      /**< Module voltage in mV. */
	uint16_t voltage_low;  /**< Alarm low voltage threshold in mV. */
	uint16_t voltage_high; /**< Alarm high voltage threshold in mV. */
};

/**
 *@brief doca_telemetry_phy_tx_bias_current
 */
struct doca_telemetry_phy_tx_bias_current {
	float tx_bias_current_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< TX bias current in mA unit. Each element X
									 correspond to lane X. Array of up to
									 'number_of_lanes' elements. */
	float tx_bias_current_low;				      /**< Alarm low tx bias current threshold in mA. */
	float tx_bias_current_high; /**< Alarm high tx bias current threshold in mA. */
};

/**
 *@brief doca_telemetry_phy_rx_power
 */
struct doca_telemetry_phy_rx_power {
	float rx_power_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< RX power in dBm unit. Each element X correspond to
								  lane X. Array of up to 'number_of_lanes' elements. */
	float rx_power_low;				       /**< Alarm low RX power threshold in dBm unit. */
	float rx_power_high;				       /**< Alarm high RX power threshold in dBm unit. */
};

/**
 *@brief doca_telemetry_phy_tx_power
 */
struct doca_telemetry_phy_tx_power {
	float tx_power_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< TX power in dBm unit. Each element X correspond to
								  lane X. Array of up to 'number_of_lanes' elements. */
	float tx_power_low;				       /**< Alarm low TX power threshold in dBm unit. */
	float tx_power_high;				       /**< Alarm high TX power threshold in dBm unit. */
};

/**
 *@brief doca_telemetry_phy_rx_power_type
 */
enum doca_telemetry_phy_rx_power_type {
	DOCA_TELEMETRY_PHY_RX_POWER_TYPE_OMA = 0x0, /**< Optical Modulation Amplitude */
	DOCA_TELEMETRY_PHY_RX_POWER_TYPE_AP = 0x1,  /**< Average Power */
	DOCA_TELEMETRY_PHY_RX_POWER_TYPE_NA = 0xf
};

/**
 * @brief doca_telemetry_phy_sfp_qsfp
 */
enum doca_telemetry_phy_sfp_qsfp {				      /**< Power class for SFP and QSFP cable. */
	DOCA_TELEMETRY_PHY_SFP_QSFP_1_0_W_MAX = 0x0,		      /**< 1.0 W max */
	DOCA_TELEMETRY_PHY_SFP_QSFP_1_5_W_MAX = 0x1,		      /**< 1.5 W max */
	DOCA_TELEMETRY_PHY_SFP_QSFP_2_0_W_MAX = 0x2,		      /**< 2.0 W max */
	DOCA_TELEMETRY_PHY_SFP_QSFP_2_5_W_MAX = 0x3,		      /**< 2.5 W max */
	DOCA_TELEMETRY_PHY_SFP_QSFP_3_5_W_MAX = 0x4,		      /**< 3.5 W max */
	DOCA_TELEMETRY_PHY_SFP_QSFP_4_0_W_MAX = 0x5,		      /**< 4.0 W max */
	DOCA_TELEMETRY_PHY_SFP_QSFP_4_5_W_MAX = 0x6,		      /**< 4.5 W max */
	DOCA_TELEMETRY_PHY_SFP_QSFP_5_0_W_MAX = 0x7,		      /**< 5.0 W max */
	DOCA_TELEMETRY_PHY_SFP_QSFP_POWER_FROM_MAX_POWER_FIELD = 0x8, /**< Power from max power field */
	DOCA_TELEMETRY_PHY_SFP_QSFP_NA = 0xff
};

/**
 * @brief doca_telemetry_phy_sfpdd
 */
enum doca_telemetry_phy_sfpdd {					   /**< Power class for SFP-DD cable. */
	DOCA_TELEMETRY_PHY_SFPDD_0_5_W_MAX = 0x0,		   /**< 0.5 W max */
	DOCA_TELEMETRY_PHY_SFPDD_1_0_W_MAX = 0x1,		   /**< 1.0 W max */
	DOCA_TELEMETRY_PHY_SFPDD_1_5_W_MAX = 0x2,		   /**< 1.5 W max */
	DOCA_TELEMETRY_PHY_SFPDD_2_0_W_MAX = 0x3,		   /**< 2.0 W max */
	DOCA_TELEMETRY_PHY_SFPDD_3_5_W_MAX = 0x4,		   /**< 3.5 W max */
	DOCA_TELEMETRY_PHY_SFPDD_5_0_W_MAX = 0x5,		   /**< 5.0 W max */
	DOCA_TELEMETRY_PHY_SFPDD_POWER_FROM_MAX_POWER_FIELD = 0x8, /**< Power from max power field */
	DOCA_TELEMETRY_PHY_SFPDD_NA = 0xff
};

/**
 * @brief doca_telemetry_phy_qsfpdd_osfp
 */
enum doca_telemetry_phy_qsfpdd_osfp {					 /**< Power class for QSFP-DD and OSFP cable. */
	DOCA_TELEMETRY_PHY_QSFPDD_OSFP_1_5_W_MAX = 0x1,			 /**< 1.5 W max */
	DOCA_TELEMETRY_PHY_QSFPDD_OSFP_3_5_W_MAX = 0x2,			 /**< 3.5 W max */
	DOCA_TELEMETRY_PHY_QSFPDD_OSFP_7_0_W_MAX = 0x3,			 /**< 7.0 W max */
	DOCA_TELEMETRY_PHY_QSFPDD_OSFP_8_0_W_MAX = 0x4,			 /**< 8.0 W max */
	DOCA_TELEMETRY_PHY_QSFPDD_OSFP_10_W_MAX = 0x5,			 /**< 10 W max */
	DOCA_TELEMETRY_PHY_QSFPDD_OSFP_12_W_MAX = 0x6,			 /**< 12 W max */
	DOCA_TELEMETRY_PHY_QSFPDD_OSFP_14_W_MAX = 0x7,			 /**< 14 W max */
	DOCA_TELEMETRY_PHY_QSFPDD_OSFP_POWER_FROM_MAX_POWER_FIELD = 0x8, /**< Power from max power field */
	DOCA_TELEMETRY_PHY_QSFPDD_OSFP_NA = 0xff
};

/**
 * @brief doca_telemetry_phy_power_class
 */
union doca_telemetry_phy_power_class {
	enum doca_telemetry_phy_sfp_qsfp sfp_qsfp;	 /**< Power class for SFP and QSFP cable. */
	enum doca_telemetry_phy_sfpdd sfpdd;		 /**< Power class for SFP-DD cable. */
	enum doca_telemetry_phy_qsfpdd_osfp qsfpdd_osfp; /**< Power class for QSFP-DD and OSFP
										    cable. */
};

/**
 *@brief doca_telemetry_phy_cable_power_and_temp_info
 */
struct doca_telemetry_phy_cable_power_and_temp_info {
	uint8_t ddm_supported;					  /**< Digital Diagnostic Monitoring is supported. */
	struct doca_telemetry_phy_temperature module_temperature; /**< Module temperature parameters. Valid only if
								     Digital Diagnostic Monitoring is supported and low
								     and high thresholds are not 0. */
	struct doca_telemetry_phy_voltage module_voltage;	  /**< Module voltage parameters. Valid only if Digital
								     Diagnostic Monitoring is supported and low and high
								     thresholds are not 0. */
	enum doca_telemetry_phy_rx_power_type rx_power_type;	  /**< RX power measurement type. */
	float max_power; /**< Max power in Watts. Valid only if cable type is not passive. 0 means not supported. */
	struct doca_telemetry_phy_tx_bias_current tx_bias_current; /**< TX bias current. Valid only if Digital
							      Diagnostic Monitoring is supported and low and high
							      thresholds are not 0. */
	struct doca_telemetry_phy_rx_power rx_power;	  /**< RX power. Valid only if Digital Diagnostic Monitoring is
							     supported and low and high thresholds are not 0. */
	struct doca_telemetry_phy_tx_power tx_power;	  /**< TX power. Valid only if Digital Diagnostic Monitoring is
							     supported and low and high thresholds are not 0. */
	union doca_telemetry_phy_power_class power_class; /**< Module maximum power consumption. */
};

/**
 *@brief doca_telemetry_phy_cable_attenuation
 */
struct doca_telemetry_phy_cable_attenuation {
	uint8_t ca_5g;	/**< Cable attenuation at 5GHz in dB. 0 means not supported. */
	uint8_t ca_7g;	/**< Cable attenuation at 7GHz in dB. 0 means not supported. */
	uint8_t ca_12g; /**< Cable attenuation at 12GHz in dB. 0 means not supported. */
	uint8_t ca_25g; /**< Cable attenuation at 25GHz in dB, valid only for CMIS cables. 0 means not supported. */
	uint8_t ca_53g; /**< Cable attenuation at 53GHz in dB. 0 means not supported. */
};

/**
 *@brief doca_telemetry_phy_active_cable_info
 */
struct doca_telemetry_phy_active_cable_info {
	uint8_t cable_rx_amp;	   /**< Cable Rx Output Amplitude indicator. */
	uint8_t cable_rx_emphasis; /**< Cable RX Emphasis. For CMIS (QSFP-DD/ SFP-DD/ OSFP) cables, it will represent Rx
				      pre-emphasis. */
	uint8_t cable_rx_post_emphasis; /**< Cable RX post emphasis. Valid only for CMIS (QSFP-DD/ SFP-DD/ OSFP) cables.
					 */
	uint8_t cable_tx_equalization;	/**< Cable TX Equalization. */
	uint16_t wavelength;		/**< Nominal laser wavelength in nanometers. */
	float wavelength_tolerance;	/**< Nominal laser wavelength tolerance in nanometers. */
	struct doca_telemetry_phy_cable_attenuation cable_attenuation; /**< Cable attenuation. Valid only for non
								   passive copper based cables. */
	float snr_media_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< SNR (signal to noise ratio) value on the media
								   optical lanes in dB. Each element X correspond to
								   lane X. Array of up to 'number_of_lanes' elements. 0
								   means not supported. */
	float snr_host_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES];	/**< SNR (signal to noise ratio) value on the host
								   optical lanes in dB. Each element X correspond to lane
								   X. Array of up to 'number_of_lanes' elements. 0 means
								   not supported. */
};

/**
 *@brief doca_telemetry_phy_error_counter_info
 */
struct doca_telemetry_phy_error_counter_info {
	uint8_t tx_fault; /**< Bitmask for latched Tx fault flag per lane. Bit X correspond to lane X. */
	uint8_t tx_los; /**< Bitmask for latched Tx loss of signal flag per lane, Bit X correspond to lane X. Valid only
			   if cable identifier is neither SFP28_OR_SFP_PLUS nor QSA_QSFP_SFP. */
	uint8_t tx_cdr_lol; /**< Bitmask for latched Tx CDR loss of lock flag per lane. Bit X correspond to lane X. */
	uint8_t rx_los;	    /**< Bitmask for latched Rx loss of signal flag per lane. Bit X correspond to lane X. */
	uint8_t rx_cdr_lol; /**< Bitmask for latched Rx CDR loss of lock flag per lane. Bit X correspond to lane X. */
	uint8_t tx_ad_eq_fault; /**< Bitmask for latched Tx adaptive equalization fault flag per lane. Bit X correspond
				   to lane X. Valid only if cable identifier is neither SFP28_OR_SFP_PLUS nor
				   QSA_QSFP_SFP. */
};

/**
 *@brief doca_telemetry_phy_cmis_state
 */
enum doca_telemetry_phy_cmis_state {
	DOCA_TELEMETRY_PHY_CMIS_ST_RESERVED = 0,
	DOCA_TELEMETRY_PHY_CMIS_ST_LOWPWR = 0x1,
	DOCA_TELEMETRY_PHY_CMIS_ST_PWRUP = 0x2,
	DOCA_TELEMETRY_PHY_CMIS_ST_READY = 0x3,
	DOCA_TELEMETRY_PHY_CMIS_ST_PWRDN = 0x4,
	DOCA_TELEMETRY_PHY_CMIS_ST_FAULT = 0x5,
	DOCA_TELEMETRY_PHY_CMIS_ST_NA = 0xff
};

/**
 *@brief doca_telemetry_phy_cpo_state
 */
enum doca_telemetry_phy_cpo_state {
	DOCA_TELEMETRY_PHY_CPO_ST_LASER_INIT = 0,
	DOCA_TELEMETRY_PHY_CPO_ST_LASER_ACTIVE = 1,
	DOCA_TELEMETRY_PHY_CPO_ST_LASER_ACTIVE_WITH_FAULT = 2,
	DOCA_TELEMETRY_PHY_CPO_ST_LASER_DOWN = 3,
	DOCA_TELEMETRY_PHY_CPO_ST_LASER_DOWN_WITH_FAULT = 4,
	DOCA_TELEMETRY_PHY_CPO_ST_NA = 0xff
};

/**
 *@brief doca_telemetry_phy_module_state
 */
struct doca_telemetry_phy_module_state {
	uint8_t is_cpo; /**< Indicates if the module is a CMIS or CPO module. */
	union {
		enum doca_telemetry_phy_cmis_state cmis_state; /**< CMIS module state. */
		enum doca_telemetry_phy_cpo_state cpo_state;   /**< CPO module state. */
	} state;					       /**< Module state. */
};

/**
 *@brief doca_telemetry_phy_data_path_state
 */
enum doca_telemetry_phy_data_path_state {
	DOCA_TELEMETRY_PHY_DP_ST_RES = 0,
	DOCA_TELEMETRY_PHY_DP_ST_DEACTIVATED = 0x1,
	DOCA_TELEMETRY_PHY_DP_ST_INIT = 0x2,
	DOCA_TELEMETRY_PHY_DP_ST_DEINIT = 0x3,
	DOCA_TELEMETRY_PHY_DP_ST_ACTIVATED = 0x4,
	DOCA_TELEMETRY_PHY_DP_ST_TX_TURN_ON = 0x5,
	DOCA_TELEMETRY_PHY_DP_ST_TX_TURN_OFF = 0x6,
	DOCA_TELEMETRY_PHY_DP_ST_INITIALIZED = 0x7,
	DOCA_TELEMETRY_PHY_DP_ST_NA = 0xff
};

/**
 *@brief doca_telemetry_phy_cdr_cap
 */
enum doca_telemetry_phy_cdr_cap {
	DOCA_TELEMETRY_PHY_CDR_UNSUPPORTED = 0x0,
	DOCA_TELEMETRY_PHY_CDR_ON_OFF_CONTROL_ENABLED = 0x1,
	DOCA_TELEMETRY_PHY_CDR_ON_OFF_CONTROL_DISABLED = 0x2,
};

/**
 *@brief doca_telemetry_phy_cdr_info
 */
struct doca_telemetry_phy_cdr_info {
	enum doca_telemetry_phy_cdr_cap rx_cdr_cap; /**< RX CDR cap. */
	enum doca_telemetry_phy_cdr_cap tx_cdr_cap; /**< TX CDR cap. */
	uint8_t rx_cdr_state; /**< Bitmask for RX CDR state per lane. Bit X correspond to lane X, up to
	'number_of_lanes' elements. Valid only if cable identifier is neither SFP28_OR_SFP_PLUS nor QSA_QSFP_SFP. */
	uint8_t tx_cdr_state; /**< Bitmask for TX CDR state per lane. Bit X correspond to lane X, up to
	'number_of_lanes' elements. Valid only if cable identifier is neither SFP28_OR_SFP_PLUS nor QSA_QSFP_SFP. */
};

/**
 *@brief doca_telemetry_phy_status_info
 */
struct doca_telemetry_phy_status_info {
	struct doca_telemetry_phy_cdr_info cdr_info;	     /**< CDR info. */
	struct doca_telemetry_phy_module_state module_state; /**< Module state. Valid for CMIS and CPO modules. */
	enum doca_telemetry_phy_data_path_state dp_st_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< Data path state per
												 lane. Each element X
												 correspond to lane X.
												 Array of up to
												 'number_of_lanes'
												 elements. */
	uint8_t rx_output_valid;   /**< Bitmask for rx output status indication per lane. Bit X correspond to lane X. */
	uint8_t module_fw_fault;   /**< Latched module fw fault flag. Valid for CMIS based modules only. */
	uint8_t datapath_fw_fault; /**< Latched module datapath fw fault flag. Valid for CMIS based modules only. */
};

/**
 *@brief doca_telemetry_phy_latency_info
 */
struct doca_telemetry_phy_latency_info {
	uint16_t intra_asic_latency; /**< Intra-ASIC pipeline latency of the physical layer in nanoseconds. 0 means not
					supported */
	uint16_t module_datapath_latency; /**< Module’s Datapath pipeline latency in nanoseconds. 0 means not supported.
					   */
	float round_trip_latency; /**< Round-trip latency of the link attached to this port in nanoseconds. 0 means not
				     supported. */
};

/**
 *@brief doca_telemetry_phy_module_info
 */
struct doca_telemetry_phy_module_info {
	uint8_t number_of_lanes;			  /**< Number of module lanes supported. */
	float nominal_bit_rate;				  /**< Nominal bit rate in Gb/s. */
	enum doca_telemetry_phy_protocol active_protocol; /**< Active protocol. */
	enum doca_telemetry_phy_error_code error_code;	  /**< Error Code response for Control or Set configuration of
						      DataPath. Relevant for CMIS modules only. */
	struct doca_telemetry_phy_cable_vendor_info cable_vendor_info; /**< Cable vendor info. */
	struct doca_telemetry_phy_cable_general_properties_info cable_general_properties_info; /**< Cable general
												   properties info. */
	struct doca_telemetry_phy_cable_power_and_temp_info cable_power_and_temp_info; /**< Cable power and temperature
											  info. */
	struct doca_telemetry_phy_active_cable_info active_cable_info;		       /**< Active cable info. */
	struct doca_telemetry_phy_error_counter_info error_counter_info;	       /**< Error counter info. */
	struct doca_telemetry_phy_status_info status_info;			       /**< Status info. */
	struct doca_telemetry_phy_latency_info latency_info;			       /**< Latency info. */
};

/**
 *@brief doca_telemetry_phy_ber
 */
struct doca_telemetry_phy_ber {
	uint8_t ber_coef;      /**< BER coefficient to show the Bit Error Rate. */
	uint8_t ber_magnitude; /**< BER magnitude to show the Bit Error Rate. */
};

/**
 *@brief doca_telemetry_phy_symbol_error
 */
struct doca_telemetry_phy_symbol_error {
	uint64_t symbol_errors_counter;		  /**< Error bits counter. */
	struct doca_telemetry_phy_ber symbol_ber; /**< Symbol BER (Bit Error Rate = ber_coef*10^(-ber_magnitude)). Valid
						     only for devices with active protocol infiniband and if ber_coef
						     and ber_magnitude are not 0. */
};

/**
 *@brief doca_telemetry_phy_effective_error
 */
struct doca_telemetry_phy_effective_error {
	uint64_t effective_errors_counter;	     /**< Error bits counter. */
	struct doca_telemetry_phy_ber effective_ber; /**< Effective FEC BER (Bit Error Rate =
							ber_coef*10^(-ber_magnitude)). Valid only if ber_coef and
							ber_magnitude are not 0. */
};

/**
 *@brief doca_telemetry_phy_raw_error
 */
struct doca_telemetry_phy_raw_error {
	uint64_t raw_errors_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< Each element X provides information on error
								       bits that were identified on lane X. Array of up
								       to 'number_of_lanes' elements. */
	struct doca_telemetry_phy_ber raw_ber_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< Each element X provides
											 information on raw BER (Bit
											 Error Rate =
											 ber_coef*10^(-ber_magnitude))
											 in lane X. Valid only if
											 ber_coef and ber_magnitude are
											 not 0. Array of up to
											 'number_of_lanes' elements. */
	struct doca_telemetry_phy_ber raw_ber; /**< Raw BER (Bit Error Rate = ber_coef*10^(-ber_magnitude)). Valid only
						  if ber_coef and ber_magnitude are not 0. */
};

/**
 *@brief doca_telemetry_phy_counter_and_ber_info
 */
struct doca_telemetry_phy_counter_and_ber_info {
	uint8_t number_of_lanes;	     /**< Number of module lanes supported. */
	uint32_t link_down_counter;	     /**< Unintentional link drops counter (no remote consideration). */
	uint32_t link_down_recovery_counter; /**< Successful recovery events per active link counter. */
	uint64_t time_since_last_clear; /**< The time passed since the last counters clear event in msec. 0 means not
					   supported. */
	enum doca_telemetry_phy_protocol active_protocol;     /**< Active protocol. */
	struct doca_telemetry_phy_symbol_error symbol_errors; /**< Error counter caused by invalid bits received that
								 were not corrected by phy correction mechanisms. Only
								 valid for devices with active protocol infiniband. */
	struct doca_telemetry_phy_effective_error effective_errors; /**< Error counter caused by invalid bits received
								       that were not corrected by FEC (forward error
								       correct) algorithm. */
	struct doca_telemetry_phy_raw_error raw_errors; /**< Raw error counter caused by invalid bits received. */
};

/**
 *@brief doca_telemetry_phy_bin_range
 */
struct doca_telemetry_phy_bin_range_info {
	uint8_t high_val; /**< High range of bin's measurement. */
	uint8_t low_val;  /**< Low range of bin's measurement. */
};

/**
 *@brief doca_telemetry_phy_fec_histogram_info
 */
struct doca_telemetry_phy_fec_histogram_info {
	uint8_t num_of_bins; /**< Available number of bins. */
	struct doca_telemetry_phy_bin_range_info bin_range[DOCA_TELEMETRY_PHY_FEC_MAX_BINS_RANGE]; /**< Range of bin
									 errors distribution. Array of up to
									 'num_of_bins' elements. */
	uint64_t bin_errors[DOCA_TELEMETRY_PHY_FEC_MAX_BINS_RANGE]; /**< Each element X correspond to a value of FEC
						histogram bin X errors. Bin errors distribution according to bin_range.
						Array of up to 'num_of_bins' elements. */
};

/**
 *@brief doca_telemetry_phy_page_info
 */
struct doca_telemetry_phy_page_info {
	uint8_t page_id;		     /**< ID of page returned. */
	uint8_t data_size_lower_offset;	     /**< Number of bytes read from page offset of 0 bytes */
	uint8_t data_size_upper_offset;	     /**< Number of bytes read from page offset of 128 bytes */
	uint8_t page_lower_offset_data[128]; /**< Bytes read from page offset of 0 bytes.*/
	uint8_t page_upper_offset_data[128]; /**< Bytes read from page offset of 128 bytes.*/
};

/**
 *@brief doca_telemetry_phy_management_cable_raw_info
 */
struct doca_telemetry_phy_management_cable_raw_info {
	uint16_t num_of_pages;							     /**< Number of pages returned. */
	struct doca_telemetry_phy_page_info page_info[DOCA_TELEMETRY_PHY_MAX_PAGES]; /**< Retrieved page information.
											Each element correspond to a
											retrieved page. Array of
											'num_of_pages' elements. */
};

/**
 *@brief doca_telemetry_phy_ddm_temperature
 */
struct doca_telemetry_phy_ddm_temperature {
	int16_t temperature;		      /**< Module temperature in Celsius. */
	int16_t temperature_high_warning;     /**< Warning high temperature threshold in Celsius. */
	int16_t temperature_low_warning;      /**< Warning low temperature threshold in Celsius.  */
	int16_t temperature_high_alarm;	      /**< Alarm high temperature threshold in Celsius. */
	int16_t temperature_low_alarm;	      /**< Alarm low temperature threshold in Celsius.  */
	int8_t temperature_high_warning_flag; /**< Warning high temperature flag. */
	int8_t temperature_low_warning_flag;  /**< Warning low temperature flag. */
	int8_t temperature_high_alarm_flag;   /**< Alarm high temperature flag. */
	int8_t temperature_low_alarm_flag;    /**< Alarm low temperature flag. */
};

/**
 *@brief doca_telemetry_phy_ddm_voltage
 */
struct doca_telemetry_phy_ddm_voltage {
	uint16_t voltage;		  /**< Module voltage in mV. */
	uint16_t voltage_high_warning;	  /**< Warning high voltage threshold in mV. */
	uint16_t voltage_low_warning;	  /**< Warning low voltage threshold in mV.  */
	uint16_t voltage_high_alarm;	  /**< Alarm high voltage threshold in mV. */
	uint16_t voltage_low_alarm;	  /**< Alarm low voltage threshold in mV.  */
	int8_t voltage_high_warning_flag; /**< Warning high voltage flag. */
	int8_t voltage_low_warning_flag;  /**< Warning low voltage flag. */
	int8_t voltage_high_alarm_flag;	  /**< Alarm high voltage flag. */
	int8_t voltage_low_alarm_flag;	  /**< Alarm low voltage flag. */
};

/**
 *@brief doca_telemetry_phy_ddm_rx_power
 */
struct doca_telemetry_phy_ddm_rx_power {
	float rx_power_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< RX power in mA unit. Each element X correspond to
								  lane X. Array of up to 'number_of_lanes' elements. */
	float rx_power_high_warning;			       /**< Warning high RX power threshold in mA. */
	float rx_power_low_warning;			       /**< Warning low RX power threshold in mA.  */
	float rx_power_high_alarm;			       /**< Alarm high RX power threshold in mA. */
	float rx_power_low_alarm;			       /**< Alarm low RX power threshold in mA.  */
	int8_t rx_power_high_warning_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< Warning high RX power flag. Each
										     element X correspond to lane X.
										     Array of up to 'number_of_lanes'
										     elements. */
	int8_t rx_power_low_warning_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES];  /**< Warning low RX power flag. Each
										     element X correspond to lane X.
										     Array of up to 'number_of_lanes'
										     elements. */
	int8_t rx_power_high_alarm_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES];	  /**< Alarm high RX power flag. Each
										     element X correspond to lane X. Array
										     of up to 'number_of_lanes' elements.
										   */
	int8_t rx_power_low_alarm_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES];	  /**< Alarm low RX power flag. Each
										     element X correspond to lane X. Array
										     of up to 'number_of_lanes' elements.
										   */
};

/**
 *@brief doca_telemetry_phy_ddm_tx_power
 */
struct doca_telemetry_phy_ddm_tx_power {
	float tx_power_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< TX power in mA unit. Each element X correspond to
								  lane X. Array of up to 'number_of_lanes' elements. */
	float tx_power_high_warning;			       /**< Warning high TX power threshold in mA. */
	float tx_power_low_warning;			       /**< Warning low TX power threshold in mA.  */
	float tx_power_high_alarm;			       /**< Alarm high TX power threshold in mA. */
	float tx_power_low_alarm;			       /**< Alarm low TX power threshold in mA.  */
	int8_t tx_power_high_warning_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< Warning high TX power flag. Each
										     element X correspond to lane X.
										     Array of up to 'number_of_lanes'
										     elements. */
	int8_t tx_power_low_warning_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES];  /**< Warning low TX power flag. Each
										     element X correspond to lane X.
										     Array of up to 'number_of_lanes'
										     elements. */
	int8_t tx_power_high_alarm_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES];	  /**< Alarm high TX power flag. Each
										     element X correspond to lane X. Array
										     of up to 'number_of_lanes' elements.
										   */
	int8_t tx_power_low_alarm_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES];	  /**< Alarm low TX power flag. Each
										     element X correspond to lane X. Array
										     of up to 'number_of_lanes' elements.
										   */
};

/**
 *@brief doca_telemetry_phy_ddm_tx_bias
 */
struct doca_telemetry_phy_ddm_tx_bias {
	float tx_bias_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< TX bias in mA unit. Each element X correspond to lane
								 X. Array of up to 'number_of_lanes' elements. */
	float tx_bias_high_warning;			      /**< Warning high TX bias threshold in mA. */
	float tx_bias_low_warning;			      /**< Warning low TX bias threshold in mA.  */
	float tx_bias_high_alarm;			      /**< Alarm high TX bias threshold. */
	float tx_bias_low_alarm;			      /**< Alarm low TX bias threshold.  */
	int8_t tx_bias_high_warning_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< Warning high TX bias flag. Each
										    element X correspond to lane X.
										    Array of up to 'number_of_lanes'
										    elements. */
	int8_t tx_bias_low_warning_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES];	 /**< Warning low TX bias flag. Each
										    element X correspond to lane X.  Array
										    of up to 'number_of_lanes' elements.
										  */
	int8_t tx_bias_high_alarm_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES];	 /**< Alarm high TX bias flag. Each
										    element X correspond to lane X. Array
										    of up to 'number_of_lanes' elements.
										  */
	int8_t tx_bias_low_alarm_flag_per_lane[DOCA_TELEMETRY_PHY_MAX_LANES]; /**< Alarm low TX bias flag. Each element
										 X correspond to lane X. Array of up to
										 'number_of_lanes' elements. */
};

/**
 *@brief doca_telemetry_phy_management_cable_ddm_info
 */
struct doca_telemetry_phy_management_cable_ddm_info {
	uint8_t number_of_lanes;				      /**< Number of lanes supported. */
	struct doca_telemetry_phy_ddm_temperature module_temperature; /**< Temperature parameters. Valid only if low and
								  high alarm thresholds are not 0.*/
	struct doca_telemetry_phy_ddm_voltage module_voltage; /**< Module voltage parameters. Valid only if low and high
							  alarm thresholds are not 0. */
	struct doca_telemetry_phy_ddm_rx_power rx_power;      /**< RX power parameters. Valid only if low and high alarm
								 thresholds are not 0. */
	struct doca_telemetry_phy_ddm_tx_power tx_power;      /**< TX power parameters. Valid only if low and high alarm
								 thresholds are not 0. */
	struct doca_telemetry_phy_ddm_tx_bias tx_bias;	      /**< TX bias parameters. Valid only if low and high alarm
								 thresholds are not 0. */
};

/**
 * @brief Check if given device is capable of executing telemetry PHY operations.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry PHY.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PHY.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_cap_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of retrieving telemetry PHY operation info.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry PHY operation info.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PHY operation info.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_cap_operation_info_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of retrieving telemetry PHY supported info.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry PHY supported info.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PHY supported info.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_cap_supported_info_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of retrieving telemetry PHY troubleshooting info.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry PHY troubleshooting info.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PHY troubleshooting info.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_cap_troubleshooting_info_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of retrieving telemetry PHY module info.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry PHY module info.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PHY module info.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_cap_module_info_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of retrieving telemetry PHY counter and BER info.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry PHY Counter and BER info.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PHY counter and BER info.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_cap_counter_and_ber_info_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of retrieving telemetry PHY FEC histogram info.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry PHY FEC Histogram.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PHY FEC histogram info.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_cap_fec_histogram_info_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Check if given device is capable of retrieving telemetry PHY management cable info.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @return
 * DOCA_SUCCESS - in case device supports telemetry PHY management cable info.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query the device for its capabilities.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support telemetry PHY management cable info.
 * - DOCA_ERROR_OPERATING_SYSTEM - user does not have the required permissions to access the device.
 * - DOCA_ERROR_NOT_FOUND - could not find FWCTL FD associated with devinfo.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_cap_management_cable_is_supported(const struct doca_devinfo *devinfo);

/**
 * @brief Create a DOCA telemetry PHY instance.
 *
 * @param [in] dev
 * The device to attach to the telemetry PHY instance.
 * @param [out] phy
 * Pointer to pointer to be set to point to the created doca_telemetry_phy instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate resources.
 * - DOCA_ERROR_DRIVER - internal doca driver error.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_create(struct doca_dev *dev, struct doca_telemetry_phy **phy);

/**
 * @brief Destroy doca_telemetry_phy previously created by doca_telemetry_phy_create().
 *
 * @param [in] phy
 * Pointer to instance to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - context is not stopped.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_destroy(struct doca_telemetry_phy *phy);

/**
 * @brief Start context for telemetry PHY.
 *
 * @param [in] phy
 * Pointer to phy instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - context is already started.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_start(struct doca_telemetry_phy *phy);

/**
 * @brief Stop telemetry PHY context.
 *
 * @param [in] phy
 * Pointer to phy instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - NULL parameter.
 * - DOCA_ERROR_BAD_STATE - phy instance doesn't require stopping.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_stop(struct doca_telemetry_phy *phy);

/**
 * @brief Get DOCA Telemetry PHY operation info.
 *
 * @param [in] phy
 * Pointer to phy instance.
 * @param [out] operation_info_struct
 * Pointer to operation info structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received or retrieved invalid value.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY operation info.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_get_operation_info(struct doca_telemetry_phy *phy,
						   struct doca_telemetry_phy_operation_info *operation_info_struct);

/**
 * @brief Get DOCA Telemetry PHY supported info.
 *
 * @param [in] phy
 * Pointer to phy instance.
 * @param [out] supported_info_struct
 * Pointer to supported info structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received or retrieved invalid value.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY supported info.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_get_supported_info(struct doca_telemetry_phy *phy,
						   struct doca_telemetry_phy_supported_info *supported_info_struct);

/**
 * @brief Get DOCA Telemetry PHY troubleshooting info.
 *
 * @param [in] phy
 * Pointer to phy instance.
 * @param [out] troubleshooting_info_struct
 * Pointer to troubleshooting info structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received or retrieved invalid value.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY troubleshooting info.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_get_troubleshooting_info(
	struct doca_telemetry_phy *phy,
	struct doca_telemetry_phy_troubleshooting_info *troubleshooting_info_struct);

/**
 * @brief Get DOCA Telemetry PHY module info.
 *
 * @param [in] phy
 * Pointer to phy instance.
 * @param [out] module_info_struct
 * Pointer to monitor info structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received or retrieved invalid value.
 * - DOCA_ERROR_NOT_CONNECTED - no plugged cable detected to retrieve telemetry PHY module info.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY module info or unknown cable type.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_get_module_info(struct doca_telemetry_phy *phy,
						struct doca_telemetry_phy_module_info *module_info_struct);

/**
 * @brief Get DOCA Telemetry PHY counter and BER info.
 *
 * @param [in] phy
 * Pointer to phy instance.
 * @param [out] counter_and_ber_info_struct
 * Pointer to counter and BER info structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received or retrieved invalid value.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY counter and BER info.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_get_counter_and_ber_info(
	struct doca_telemetry_phy *phy,
	struct doca_telemetry_phy_counter_and_ber_info *counter_and_ber_info_struct);

/**
 * @brief Get DOCA Telemetry PHY FEC histogram info.
 *
 * @param [in] phy
 * Pointer to phy instance.
 * @param [out] fec_histogram_info_struct
 * Pointer to FEC histogram info structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received or retrieved invalid value.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY FEC histogram info.
 * - DOCA_ERROR_NOT_CONNECTED - no active link or FEC detected to retrieve valid telemetry PHY FEC histogram info.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_get_fec_histogram_info(
	struct doca_telemetry_phy *phy,
	struct doca_telemetry_phy_fec_histogram_info *fec_histogram_info_struct);

/**
 * @brief Get DOCA Telemetry PHY management cable single page info.
 *
 * @param [in] phy
 * Pointer to phy instance.
 * @param [in] page_number
 * Pointer to management cable page number to retrieve.
 * @param [out] management_cable_raw_info_struct
 * Pointer to management cable raw info structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_IO_FAILED - error while accessing EEPROM module.
 * - DOCA_ERROR_INVALID_VALUE - received or retrieved invalid value.
 * - DOCA_ERROR_NOT_CONNECTED - no plugged cable or EEPROM module detected.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY management cable single page info.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_get_management_cable_single_page_info(
	struct doca_telemetry_phy *phy,
	uint8_t page_number,
	struct doca_telemetry_phy_management_cable_raw_info *management_cable_raw_info_struct);

/**
 * @brief Get DOCA Telemetry PHY management cable dump info.
 *
 * @param [in] phy
 * Pointer to phy instance.
 * @param [out] management_cable_raw_info_struct
 * Pointer to management cable raw info structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_IO_FAILED - error while accessing EEPROM module.
 * - DOCA_ERROR_INVALID_VALUE - received or retrieved invalid value.
 * - DOCA_ERROR_NOT_CONNECTED - no plugged cable or EEPROM module detected.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY management cable single page info.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_get_management_cable_dump_info(
	struct doca_telemetry_phy *phy,
	struct doca_telemetry_phy_management_cable_raw_info *management_cable_raw_info_struct);

/**
 * @brief Get DOCA Telemetry PHY management cable Digital Diagnostic Monitoring (DDM) info.
 *
 * @param [in] phy
 * Pointer to phy instance.
 * @param [out] management_cable_ddm_info_struct
 * Pointer to management cable DDM info structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_IO_FAILED - error while accessing EEPROM module.
 * - DOCA_ERROR_INVALID_VALUE - received or retrieved invalid value.
 * - DOCA_ERROR_NOT_CONNECTED - no plugged cable or EEPROM module detected.
 * - DOCA_ERROR_NOT_SUPPORTED - provided instance does not support telemetry PHY Digital Diagnostic Monitoring info.
 */
DOCA_EXPERIMENTAL
doca_error_t doca_telemetry_phy_get_management_cable_ddm_info(
	struct doca_telemetry_phy *phy,
	struct doca_telemetry_phy_management_cable_ddm_info *management_cable_ddm_info_struct);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_TELEMETRY_PHY_H_ */

/** @} */
