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

/* Scramble Tools Vendor IE constants shared between host marshaller,
 * coprocessor RPC handler, and STA parser. OUI is the Scramble Tools
 * IEEE-registered MA-L prefix 8C:1F:64 (24-bit OUI of the MA-S OUI
 * 0x8C1F6436C; see Development/profiles/avb_lite.md). Two sub-types
 * are defined:
 *
 *   FOLLOWUP (0x00): IEEE 802.1AS-2020 §12.7 FollowUpInformation
 *     payload, byte-format compliant; carries the gPTP marker
 *     (preciseOriginTimestamp).
 *
 *   TSF_MAPPING (0x01): Scramble Tools-private mapping IE; carries
 *     8 bytes of bridge AP-side TSF (uint64_t little-endian µs)
 *     captured at coprocessor publish time. Paired with the FOLLOWUP
 *     IE's preciseOriginTimestamp, this gives the STA a (gPTP,TSF)
 *     anchor that converts FTM-measurement t1 (in TSF µs) to GM time
 *     for sub-ms phase precision over the beacon-IE carrier.
 *
 * The TSF_MAPPING IE is filled in by the coprocessor RPC handler at
 * publish time — the host ships zeros and the handler patches in
 * esp_wifi_get_tsf_time(WIFI_IF_AP) before calling
 * esp_wifi_set_vendor_ie(). Selection is by vendor_oui_type field. */
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
