/*
 * Copyright 2026 Scramble Tools
 * License: MIT
 *
 * Coprocessor-side handler init for the PTP custom-RPC channel.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ptp_custom_rpc_init(void);

#ifdef __cplusplus
}
#endif
