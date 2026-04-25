idf_component_register(
    SRCS "time_service.cpp"
    INCLUDE_DIRS "."
    REQUIRES common settings_store esp_netif lwip esp_event
)
