idf_component_register(
    SRCS "diagnostics_service.cpp"
    INCLUDE_DIRS "."
    REQUIRES common esp_timer nvs_flash esp_event
)
