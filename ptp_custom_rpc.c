/*
 * Copyright 2026 Scramble Tools
 * License: MIT
 *
 * Coprocessor-side handler for the PTP custom-RPC channel.
 * See README.md for protocol and rationale.
 */

#include "ptp_custom_rpc.h"
#include "ptp_rpc_proto.h"

#include <stddef.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"

/* esp_hosted_peer_data.h lives inside upstream
 * esp-hosted-mcu/slave/main and is not exported as a component
 * header — forward-declare locally to stay decoupled. */
extern esp_err_t esp_hosted_send_custom_data(uint32_t msg_id,
                                             const uint8_t *data,
                                             size_t data_len);
extern esp_err_t esp_hosted_register_custom_callback(
    uint32_t msg_id,
    void (*cb)(uint32_t, const uint8_t *, size_t, void *),
    void *user);

static const char *TAG = "ptp";

static void set_vendor_ie_callback(uint32_t msg_id, const uint8_t *data,
                                   size_t data_len, void *user) {
  (void)msg_id;
  (void)user;

  ptp_rpc_set_vendor_ie_ack_t ack = {.esp_err = ESP_OK};

  if (data == NULL || data_len < sizeof(ptp_rpc_set_vendor_ie_t)) {
    ESP_LOGW(TAG, "SET_VENDOR_IE_REQ too short: %zu", data_len);
    ack.esp_err = ESP_ERR_INVALID_SIZE;
    goto send_ack;
  }

  const ptp_rpc_set_vendor_ie_t *hdr = (const ptp_rpc_set_vendor_ie_t *)data;
  const void *vnd_ie = data + sizeof(ptp_rpc_set_vendor_ie_t);
  const size_t vnd_ie_len = data_len - sizeof(ptp_rpc_set_vendor_ie_t);

  /* vendor_ie_data_t needs at least its 6-byte fixed header. */
  if (hdr->enable && vnd_ie_len < 6) {
    ESP_LOGW(TAG, "SET_VENDOR_IE_REQ vnd_ie too short: %zu", vnd_ie_len);
    ack.esp_err = ESP_ERR_INVALID_SIZE;
    goto send_ack;
  }

  /* esp_wifi_set_vendor_ie returns ESP_ERR_INVALID_ARG (wifi:"the
   * vendor ie has been setted, clear it before setting again") when a
   * Vendor IE is already installed at the same (type, idx) slot. For
   * the gPTP FollowUpInformation publish use case the IE payload
   * changes every Sync interval, so we clear-then-set on every
   * install. The clear is a no-op the first time and cheap thereafter. */
  if (hdr->enable) {
    (void)esp_wifi_set_vendor_ie(false, (wifi_vendor_ie_type_t)hdr->type,
                                 (wifi_vendor_ie_id_t)hdr->idx, NULL);
  }
  ack.esp_err = esp_wifi_set_vendor_ie((bool)hdr->enable,
                                       (wifi_vendor_ie_type_t)hdr->type,
                                       (wifi_vendor_ie_id_t)hdr->idx, vnd_ie);
  if (ack.esp_err != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_set_vendor_ie: %s", esp_err_to_name(ack.esp_err));
  }
  /* Periodic-publish path: no INFO log on success — would spam at the
   * Sync interval (8 Hz for gPTP default). Errors still log. */

send_ack: {
  esp_err_t s = esp_hosted_send_custom_data(PTP_RPC_MSG_SET_VENDOR_IE_ACK,
                                            (const uint8_t *)&ack, sizeof(ack));
  if (s != ESP_OK) {
    ESP_LOGE(TAG, "SET_VENDOR_IE_ACK send failed: %s", esp_err_to_name(s));
  }
}
}

esp_err_t ptp_custom_rpc_init(void) {
  esp_err_t r = esp_hosted_register_custom_callback(
      PTP_RPC_MSG_SET_VENDOR_IE_REQ, set_vendor_ie_callback, NULL);
  if (r != ESP_OK) {
    ESP_LOGE(TAG, "register SET_VENDOR_IE_REQ failed: %s", esp_err_to_name(r));
    return r;
  }
  ESP_LOGI(TAG, "PTP custom RPC handler registered (msg_id=0x%08x)",
           (unsigned)PTP_RPC_MSG_SET_VENDOR_IE_REQ);
  return ESP_OK;
}

/* Auto-register so the coprocessor app doesn't need to call us from
 * app_main. Safe at constructor time: upstream's register API lazy-
 * initializes its mutex and the dispatch table is BSS-zero. Re-calling
 * ptp_custom_rpc_init() from user code is harmless. */
__attribute__((constructor)) static void ptp_custom_rpc_autoreg(void) {
  (void)ptp_custom_rpc_init();
}
