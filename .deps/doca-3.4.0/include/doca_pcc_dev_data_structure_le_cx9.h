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


 #ifndef DOCA_PCC_DEV_DATA_STRUCTURE_LE_CX9_H_
 #define DOCA_PCC_DEV_DATA_STRUCTURE_LE_CX9_H_

#include <stdint.h>


 struct mlnx_cc_rx_sack_attr_3_t {       /* Little Endian */
	uint32_t                  total_rx_128bytes:24;                   /* Cumulative counter of bytes received in the NP (128B units) */
	uint32_t                  rx_cwnd_penalty:7;                      /* Receiver C-window penalty from receiver congestion calculation */
	uint32_t                  restore_cwnd:1;                 /* Set to 1 (true) if sender should restore cwnd after flow control */
/* --------------------------------------------------------- */
};

struct mlnx_cc_rx_sack_attr_2_t {       /* Little Endian */
	uint32_t                  ooo_count:15;                   /* Out of order count */
	uint32_t                  reserved_at_10:1;
	uint32_t                  trigger_packet_tx_128ns_timestamp:16;                   /* time the packet that triggered the SACK event sent (in 16ns resolution) */
/* --------------------------------------------------------- */
};

struct mlnx_cc_rx_sack_attr_1_t {       /* Little Endian */
	uint32_t                  cc_flags:4;                     /* TBD */
	uint32_t                  cc_type:4;                      /* Defines the meaning of the CC_STATE field in SACK packet. */
	uint32_t                  reserved_at_16:2;
	uint32_t                  reliability_probe_response:1;                   /* Sack sent triggered by reliability probe */
	uint32_t                  reserved_at_12:3;
	uint32_t                  seth_m_bits:2;                  /* 0b00:none;0b0:ECN marked;0b10:bad path;0b11:reserved */
	uint32_t                  reserved_at_0:16;
/* --------------------------------------------------------- */
};

struct mlnx_cc_rx_trimm_nack_attr_3_t { /* Little Endian */
	uint32_t                  reserved_at_10:16;
	uint32_t                  trigger_packet_tx_128ns_timestamp:16;                   /* Time the packet that triggered the NACK was sent. (in 128ns resolution) */
/* --------------------------------------------------------- */
};

struct mlnx_cc_rx_trimm_nack_attr_2_t { /* Little Endian */
	uint32_t                  nack_psn:24;                    /* PSN of the packet that triggered the NACK */
	uint32_t                  nack_reason:8;                  /* Code indicating what event triggered the NACK packet.;0x00 - no reason;0x01 - TRIM - packet was trimmed;0x02 - PSNE - PSN error (Out-of-PSN range, etc);0x03 - NBM - No receiver bitmnap resource;0x04 - NRB - No receiver buffer resource;0x05 - last hop trim;0x06-FE - reserved;0xFF - GERR - general receiver error */
/* --------------------------------------------------------- */
};

struct mlnx_cc_rx_trimm_nack_attr_1_t { /* Little Endian */
	uint32_t                  cc_flags:4;                     /* TBD */
	uint32_t                  cc_type:4;                      /* Defines the meaning of the CC_STATE field. */
	uint32_t                  seth_m_bits:2;                  /* 0b00 - none;0b01 - ECN marked;0b10 - bad path;0b11 - reserved */
	uint32_t                  reserved_at_0:22;
/* --------------------------------------------------------- */
};

struct mlnx_cc_no_credits_attr_2_t {    /* Little Endian */
	uint32_t                  num_of_coalesced_events:16;                     /* how many no credit events were sent since first timestamp */
	uint32_t                  credit_finished_usec_timestamp:12;                      /* time the event was sent */
	uint32_t                  reserved_at_0:4;
/* --------------------------------------------------------- */
};

struct mlnx_cc_rtt_spec_data3_t {       /* Little Endian */
	uint32_t                  version:4;                      /* version */
	uint32_t                  reserved_at_0:28;
/* --------------------------------------------------------- */
};

struct mlnx_cc_rtt_spec_data2_t {       /* Little Endian */
	uint32_t                  reserved_at_1c:4;
	uint32_t                  resp_send_timestamp:28;                 /* response send timestamp */
/* --------------------------------------------------------- */
};

struct mlnx_cc_rtt_spec_data1_t {       /* Little Endian */
	uint32_t                  port_type:4;                    /* port_type */
	uint32_t                  req_recv_timestamp:28;                  /* request receive timestamp */
/* --------------------------------------------------------- */
};

struct mlnx_cc_rtt_spec_data0_t {       /* Little Endian */
	uint32_t                  reserved_at_1e:2;
	uint32_t                  send_plane:2;                   /* send_plane */
	uint32_t                  req_send_timestamp:28;                  /* request send timestamp */
/* --------------------------------------------------------- */
};

struct mlnx_cc_ack_nack_cnp_extra_t {   /* Little Endian */
	uint32_t                  num_coalesced:16;                       /* number of coalesced events, incremented on each coalesced event */
	uint32_t                  reserved_at_0:16;
/* --------------------------------------------------------- */
};

struct mlnx_cc_roce_tx_credits_left_t { /* Little Endian */
	uint32_t                  credits_left:24;                        /* number of remaining window credits for the QP */
	uint32_t                  reserved_at_0:8;
/* --------------------------------------------------------- */
};

struct mlnx_cc_roce_tx_cntrs_t {        /* Little Endian */
	uint32_t                  sent_32bytes:16;                        /* sent 32 bytes amount, additive increase on each event */
	uint32_t                  sent_pkts:16;                   /* sent packets amount, additive increase on each event */
/* --------------------------------------------------------- */
};

struct mlnx_cc_rx_sack_t {      /* Little Endian */
	uint32_t                  orig_path_entropy_value;                        /* indicate the path of the packet that triggered the SACK */
	/*----------------------------------------------------------*/
	struct mlnx_cc_rx_sack_attr_1_t                 attr_1;
	/*----------------------------------------------------------*/
	struct mlnx_cc_rx_sack_attr_2_t                 attr_2;
	/*----------------------------------------------------------*/
	struct mlnx_cc_rx_sack_attr_3_t                 attr_3;
/* --------------------------------------------------------- */
};

struct mlnx_cc_rx_trimm_nack_t {        /* Little Endian */
	uint32_t                  orig_path_entropy_value;                        /* indicate the path of the packet that triggered the NACK */
	/*----------------------------------------------------------*/
	struct mlnx_cc_rx_trimm_nack_attr_1_t                   attr_1;
	/*----------------------------------------------------------*/
	struct mlnx_cc_rx_trimm_nack_attr_2_t                   attr_2;
	/*----------------------------------------------------------*/
	struct mlnx_cc_rx_trimm_nack_attr_3_t                   attr_3;
/* --------------------------------------------------------- */
};

struct mlnx_cc_no_credits_t {   /* Little Endian */
	uint32_t                  first_coalesced_timestamp;                      /* first time qp ran out of credits. */
	/*----------------------------------------------------------*/
	unsigned char                   reserved_at_20[4];
	/*----------------------------------------------------------*/
	struct mlnx_cc_no_credits_attr_2_t                      attr_2;
	/*----------------------------------------------------------*/
	unsigned char                   reserved_at_60[4];
/* --------------------------------------------------------- */
};

struct mlnx_cc_fw_data_t {      /* Little Endian */
	uint32_t                  data[3];                        /* 3 dword fw data */
/* --------------------------------------------------------- */
};

struct mlnx_cc_rtt_tstamp_t {   /* Little Endian */
	struct mlnx_cc_rtt_spec_data0_t                 data0;                  /* rtt event data */
	/*----------------------------------------------------------*/
	struct mlnx_cc_rtt_spec_data1_t                 data1;                  /* rtt event data */
	/*----------------------------------------------------------*/
	struct mlnx_cc_rtt_spec_data2_t                 data2;                  /* rtt event data */
	/*----------------------------------------------------------*/
	struct mlnx_cc_rtt_spec_data3_t                 data3;                  /* rtt event data */
/* --------------------------------------------------------- */
};

struct mlnx_cc_ack_nack_cnp_t { /* Little Endian */
	uint32_t                  first_timestamp;                        /* first coalesced event timestamp */
	/*----------------------------------------------------------*/
	uint32_t	                  first_sn;                       /* first coalesced event serial number */
	/*----------------------------------------------------------*/
	struct mlnx_cc_ack_nack_cnp_extra_t                     extra;                  /* extra attributes */
	/*----------------------------------------------------------*/
	unsigned char                   reserved_at_60[4];
/* --------------------------------------------------------- */
};

struct mlnx_cc_roce_tx_t {      /* Little Endian */
	uint32_t                  first_timestamp;                        /* first coalesced event timestamp */
	/*----------------------------------------------------------*/
	struct mlnx_cc_roce_tx_cntrs_t                  cntrs;                  /* tx counters */
	/*----------------------------------------------------------*/
	struct mlnx_cc_roce_tx_credits_left_t                   credits;                        /* number of remaining window credits for the QP */
	/*----------------------------------------------------------*/
	unsigned char                   reserved_at_60[4];
/* --------------------------------------------------------- */
};

union mlnx_cc_event_spec_attr_t {       /* Little Endian */
	struct mlnx_cc_roce_tx_t                        roce_tx;                        /* tx attributes */
	/*----------------------------------------------------------*/
	struct mlnx_cc_ack_nack_cnp_t                   ack_nack_cnp;                   /* ack/nack/cnp attributes */
	/*----------------------------------------------------------*/
	struct mlnx_cc_rtt_tstamp_t                     rtt_tstamp;                     /* rtt timestamp */
	/*----------------------------------------------------------*/
	struct mlnx_cc_fw_data_t                        fw_data;                        /* fw data */
	/*----------------------------------------------------------*/
	struct mlnx_cc_no_credits_t                     no_credits;                     /* no credits event */
	/*----------------------------------------------------------*/
	struct mlnx_cc_rx_trimm_nack_t                  rx_trimm_nack;                  /* trimm nack event */
	/*----------------------------------------------------------*/
	struct mlnx_cc_rx_sack_t                        rx_sack;                        /* sack event */
	/*----------------------------------------------------------*/
	unsigned char                   reserved_at_0[16];
/* --------------------------------------------------------- */
};

struct mlnx_cc_event_general_attr_t {   /* Little Endian */
	uint32_t                  ev_type:8;                      /* event type */
	uint32_t                  ev_subtype:8;                   /* event subtype */
	uint32_t                  port_num:3;                     /* port id */
	uint32_t                  plane_num:5;                    /* plane num */
	uint32_t                  flags:8;                        /* event flags */
/* --------------------------------------------------------- */
};

struct mlnx_cc_event_general_dword2_t { /* Little Endian */
	uint32_t                  flow_qpn:24;                    /* flow qp number */
	uint32_t                  ttl_hoplimit:8;                 /* TTL/hop limit */
/* --------------------------------------------------------- */
};

struct mlnx_cc_algo_ctxt_t {    /* Little Endian */
	uint32_t                  data[12];                       /* 12 dword algorithm context */
/* --------------------------------------------------------- */
};

struct val_t {  /* Little Endian */
	uint32_t                  val;                    /* uint32 value */
/* --------------------------------------------------------- */
};

struct mlnx_cc_event_t {        /* Little Endian */
	unsigned char                   reserved_at_0[4];
	/*----------------------------------------------------------*/
	uint32_t                  reserved_at_30:16;
	uint32_t                  vhca_id:16;
	/*----------------------------------------------------------*/
	struct mlnx_cc_event_general_dword2_t                   ev_dword2;                      /* flow qpn, TTL/hoplimit */
	/*----------------------------------------------------------*/
	struct mlnx_cc_event_general_attr_t                     ev_attr;                        /* event general attributes */
	/*----------------------------------------------------------*/
	uint32_t                  flow_tag;                       /* unique flow id */
	/*----------------------------------------------------------*/
	uint32_t                  sn;                     /* serial number */
	/*----------------------------------------------------------*/
	uint32_t                  timestamp;                      /* event timestamp */
	/*----------------------------------------------------------*/
	union mlnx_cc_event_spec_attr_t                 ev_spec_attr;                   /* attributes which are different for different events */
/* --------------------------------------------------------- */
};

union union_mlnx_cc_event_general_dword2_t {    /* Little Endian */
	struct val_t                    val;                    /* entire value */
	/*----------------------------------------------------------*/
	struct mlnx_cc_event_general_dword2_t                   mlnx_cc_event_general_dword2;
	/*----------------------------------------------------------*/
	unsigned char                   reserved_at_0[4];
/* --------------------------------------------------------- */
};

struct mlnx_cc_attr_t { /* Little Endian */
	uint32_t                  algo_slot:4;                    /* algorithm slot defined in API.h, 15 - DCQCN */
	uint32_t                  overload:1;                     /* overload flag */
	uint32_t                  reserved_at_0:27;
/* --------------------------------------------------------- */
};

union union_mlnx_cc_ack_nack_cnp_extra_t {      /* Little Endian */
	struct val_t                    val;                    /* entire value */
	/*----------------------------------------------------------*/
	struct mlnx_cc_ack_nack_cnp_extra_t                     mlnx_cc_ack_nack_cnp_extra;                     /* attributes for ack/nack/cnp */
	/*----------------------------------------------------------*/
	unsigned char                   reserved_at_0[4];
/* --------------------------------------------------------- */
};

union union_mlnx_cc_roce_tx_cntrs_t {   /* Little Endian */
	struct val_t                    val;                    /* entire value */
	/*----------------------------------------------------------*/
	struct mlnx_cc_roce_tx_cntrs_t                  mlnx_cc_roce_tx_cntrs;                  /* tx counters */
	/*----------------------------------------------------------*/
	unsigned char                   reserved_at_0[4];
/* --------------------------------------------------------- */
};

union union_mlnx_cc_event_general_attr_t {      /* Little Endian */
	struct val_t                    val;                    /* entire value */
	/*----------------------------------------------------------*/
	struct mlnx_cc_event_general_attr_t                     mlnx_cc_event_general_attr;                     /* event general attributes */
	/*----------------------------------------------------------*/
	unsigned char                   reserved_at_0[4];
/* --------------------------------------------------------- */
};

#endif /* DOCA_PCC_DEV_DATA_STRUCTURE_LE_CX9_H_*/
