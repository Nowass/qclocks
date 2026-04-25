idf_component_register(
    SRCS "provisioning_service.cpp"
    INCLUDE_DIRS "."
    REQUIRES wifi_service settings_store common
)
