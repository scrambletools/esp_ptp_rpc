/*
 * Copyright 2026 Scramble Tools
 * License: MIT
 *
 * Coprocessor-side handler for the PTP custom-RPC channel. The host
 * sends PTP_RPC_MSG_SET_VENDOR_IE_REQ via esp_hosted_send_custom_data();
 * we unpack it, call esp_wifi_set_vendor_ie() locally on the
 * coprocessor (where the radio lives), and reply with
 * PTP_RPC_MSG_SET_VENDOR_IE_ACK carrying the esp_err_t so the host
 * can log call-level success.
 *
 * Auto-registration. A __attribute__((constructor)) registers the
 * handler before app_main runs, so consumers do not have to touch
 * upstream esp-hosted-mcu's main/esp_hosted_coprocessor.c. This is
 * safe because esp_hosted_register_custom_callback (see upstream
 * slave_control.c) lazy-initializes its mutex and the dispatch table
 * is BSS-allocated (zero'd at boot). Constructors run after IDF's
 * heap + log are up but before the FreeRTOS scheduler starts, which
 * is well-defined for these operations.
 */

#include "ptp_custom_rpc.h"
#include "ptp_rpc_proto.h"

#include <stddef.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"

/* Forward declarations of the coprocessor-side ESP-Hosted custom-RPC
 * API. Declared in esp_hosted_peer_data.h, which lives inside the
 * upstream esp-hosted-mcu/slave/main tree and is not exported as a
 * component header — declaring locally keeps this component
 * decoupled from the upstream coprocessor-example layout. */
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

  /* When enabling, we need at least the 6-byte header
   * (element_id + length + OUI + oui_type) of vendor_ie_data_t. */
  if (hdr->enable && vnd_ie_len < 6) {
    ESP_LOGW(TAG, "SET_VENDOR_IE_REQ vnd_ie too short: %zu", vnd_ie_len);
    ack.esp_err = ESP_ERR_INVALID_SIZE;
    goto send_ack;
  }

  ack.esp_err = esp_wifi_set_vendor_ie((bool)hdr->enable,
                                       (wifi_vendor_ie_type_t)hdr->type,
                                       (wifi_vendor_ie_id_t)hdr->idx, vnd_ie);
  if (ack.esp_err == ESP_OK) {
    const uint8_t *p = (const uint8_t *)vnd_ie;
    ESP_LOGI(TAG,
             "Vendor IE %s: wifi_type=%u idx=%u OUI=%02x:%02x:%02x oui_type=%u "
             "vnd_ie_len=%zu",
             hdr->enable ? "installed" : "removed", hdr->type, hdr->idx,
             p[2], p[3], p[4], p[5], vnd_ie_len);
  } else {
    ESP_LOGE(TAG, "esp_wifi_set_vendor_ie: %s", esp_err_to_name(ack.esp_err));
  }

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

/* Auto-register without requiring a call from app_main. Re-calling
 * ptp_custom_rpc_init() from user code is harmless (upstream's
 * register API updates the existing entry rather than duplicating). */
__attribute__((constructor)) static void ptp_custom_rpc_autoreg(void) {
  (void)ptp_custom_rpc_init();
}
