/*
 * Copyright 2026 Scramble Tools
 * License: MIT
 *
 * PTP custom-RPC wire format. See README.md for rationale and the
 * end-to-end host ↔ coprocessor flow.
 */

#pragma once

#include <stdint.h>

/* Msg-ID base "PTP\0" (0x50545000); low byte is the opcode.
 * 0xFFFFFFFF is reserved by peer_data_transfer as "invalid". */
#define PTP_RPC_MSG_BASE              0x50545000U
#define PTP_RPC_MSG_SET_VENDOR_IE_REQ (PTP_RPC_MSG_BASE | 0x01U)
#define PTP_RPC_MSG_SET_VENDOR_IE_ACK (PTP_RPC_MSG_BASE | 0x02U)

/* Vendor IE constants shared between host marshaller, coprocessor RPC
 * handler, and STA parser. OUI is the Scramble Tools MA-L prefix
 * 8C:1F:64. Two sub-types, selected by vendor_oui_type:
 *
 *   FOLLOWUP (0x00) — IEEE 802.1AS-2020 §12.7 FollowUpInformation
 *     payload; carries the BTC marker (preciseOriginTimestamp).
 *
 *   TSF_MAPPING (0x01) — Scramble Tools-private mapping IE; 8 bytes
 *     of bridge AP-side TSF (uint64_t LE µs). Paired with the
 *     FOLLOWUP IE's preciseOriginTimestamp this gives the STA a
 *     (BTC, TSF) anchor that converts FTM t1 (TSF µs) to BTC time. */
#define PTP_VND_IE_OUI0          0x8C
#define PTP_VND_IE_OUI1          0x1F
#define PTP_VND_IE_OUI2          0x64
#define PTP_VND_IE_OUI_TYPE_FOLLOWUP     0x00
#define PTP_VND_IE_OUI_TYPE_TSF_MAPPING  0x01
#define PTP_VND_IE_TSF_MAPPING_PAYLOAD_LEN  8   /* uint64_t LE µs */

/* SET_VENDOR_IE_REQ wire layout: this 4-byte header followed by a
 * full vendor_ie_data_t buffer (element_id + length + OUI + oui_type
 * + payload), byte-identical to esp_wifi_set_vendor_ie()'s arg.
 * Reserved byte keeps the trailing IE buffer 4-byte aligned. */
typedef struct __attribute__((packed)) {
  uint8_t enable;   /* 0=remove, 1=install/update */
  uint8_t type;     /* wifi_vendor_ie_type_t */
  uint8_t idx;      /* wifi_vendor_ie_id_t */
  uint8_t reserved;
} ptp_rpc_set_vendor_ie_t;

/* SET_VENDOR_IE_ACK wire layout. peer_data_transfer confirms only
 * transport delivery, so the call-level esp_err_t comes back here. */
typedef struct __attribute__((packed)) {
  int32_t esp_err;
} ptp_rpc_set_vendor_ie_ack_t;
