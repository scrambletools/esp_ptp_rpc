/*
 * Copyright 2026 Scramble Tools
 * License: MIT
 *
 * PTP <-> ESP-Hosted coprocessor custom-RPC wire format.
 *
 * Background. The host (e.g. ESP32-P4) runs gPTP; Wi-Fi (incl.
 * SoftAP, beacon Vendor IEs, FTM responder) runs on the onboard
 * coprocessor. ESP-Hosted's upstream RPC catalogue does not expose
 * esp_wifi_set_vendor_ie — the stub at host/api/src/esp_hosted_api.c
 * is commented out and the local libnet80211 the host links against
 * has no radio behind it, so calling esp_wifi_set_vendor_ie() from
 * the host is a silent no-op. tshark over-air on 2026-05-08 confirmed:
 * 293 beacons in 30 s, zero Vendor IE bytes from our OUI.
 *
 * Rather than fork ESP-Hosted ("no fork, additive extensions only"),
 * we use the official additive escape hatch already in upstream:
 * peer_data_transfer / Custom RPC. Host sends a packed buffer via
 * esp_hosted_send_custom_data(msg_id, ...); the coprocessor's
 * registered callback unpacks and dispatches to the matching
 * esp_wifi_* call locally. Zero changes to managed_components.
 *
 * This header is the single source of truth for msg IDs + payload
 * layout, shipped by the esp_ptp_rpc component and consumed by both
 * the host (via esp_ptp/ptp_beacon_ie.c) and the coprocessor (via
 * this component's own src/ptp_custom_rpc.c) through that component's
 * public include dir.
 */

#pragma once

#include <stdint.h>

/* Msg-ID namespace. The peer_data_transfer framework uses arbitrary
 * uint32_t IDs picked by the user; 0xFFFFFFFF is reserved by the
 * framework as "invalid". We pick a magic prefix ("PTP\1" =
 * 0x50545001) that is hard to collide with the upstream
 * peer_data_transfer example (which uses 1..6 and 99). The low byte
 * is a per-message opcode. */
#define PTP_RPC_MSG_BASE              0x50545000U /* 'P''T''P' << 8 */
#define PTP_RPC_MSG_SET_VENDOR_IE_REQ (PTP_RPC_MSG_BASE | 0x01U) /* 0x50545001 */
#define PTP_RPC_MSG_SET_VENDOR_IE_ACK (PTP_RPC_MSG_BASE | 0x02U) /* 0x50545002 */

/* Wire layout for PTP_RPC_MSG_SET_VENDOR_IE_REQ. The first 4 bytes
 * are a fixed header; the rest is the full vendor_ie_data_t buffer
 * (element_id + length + OUI + oui_type + payload) byte-identical to
 * what would be passed to esp_wifi_set_vendor_ie() locally on the
 * coprocessor.
 *
 * Total wire size = sizeof(ptp_rpc_set_vendor_ie_t) + vnd_ie_len.
 * For Path C FollowUpInformation today: 4 + 6 + 44 = 54 bytes. The
 * peer_data_transfer framework caps payloads at 8166 bytes, so we
 * have headroom for any 802.11 IE (max ~257 bytes anyway). */
typedef struct __attribute__((packed)) {
  uint8_t enable;   /* 0=remove the IE, 1=install/update */
  uint8_t type;     /* wifi_vendor_ie_type_t (0=BEACON .. 4=ASSOC_RESP) */
  uint8_t idx;      /* wifi_vendor_ie_id_t  (0=ID_0, 1=ID_1) */
  uint8_t reserved; /* zero, kept so the trailing IE buffer is 4-byte
                     * aligned for cleaner cast on receive */
  /* uint8_t vnd_ie[]; — appended after this header on the wire */
} ptp_rpc_set_vendor_ie_t;

/* Wire layout for PTP_RPC_MSG_SET_VENDOR_IE_ACK. The coprocessor
 * sends back a single int32 esp_err_t so the host can log failures
 * (the peer_data_transfer framework itself only confirms transport-
 * level delivery, not call-level success). */
typedef struct __attribute__((packed)) {
  int32_t esp_err; /* esp_err_t from esp_wifi_set_vendor_ie() */
} ptp_rpc_set_vendor_ie_ack_t;
