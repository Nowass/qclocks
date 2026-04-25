idf_component_register(
    SRCS "ota_service.cpp"
    INCLUDE_DIRS "."
    REQUIRES common settings_store esp_https_ota mbedtls esp_event
)
