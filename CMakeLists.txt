# esp_ptp_rpc — PTP custom-RPC bridge over ESP-Hosted.
#
# Always exports the shared wire format header. The coprocessor-side
# handler (which links against esp_wifi to install Vendor IEs locally)
# is gated behind CONFIG_PTP_RPC_BUILD_COPROCESSOR_HANDLER so that
# host-side consumers can use this component for the proto without
# dragging in coprocessor deps.

set(srcs "")
if(CONFIG_PTP_RPC_BUILD_COPROCESSOR_HANDLER)
    list(APPEND srcs "src/ptp_custom_rpc.c")
endif()

# PRIV_REQUIRES must be set unconditionally — IDF resolves component
# dependencies in an early phase before CONFIG_* values are populated.
# Gating PRIV_REQUIRES on a Kconfig option yields an empty list at
# that phase and the build fails with "esp_wifi not in requirements".
# These deps only have a real effect when src/ptp_custom_rpc.c is
# compiled (i.e., when CONFIG_PTP_RPC_BUILD_COPROCESSOR_HANDLER=y).
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_wifi log
    WHOLE_ARCHIVE
)
