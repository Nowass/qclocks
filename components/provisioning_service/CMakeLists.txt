idf_component_register(
    SRCS "provisioning_service.cpp"
    INCLUDE_DIRS "."
    REQUIRES common nvs_flash esp_event
)
