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
