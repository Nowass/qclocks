idf_component_register(
    SRCS "wifi_service.cpp"
    INCLUDE_DIRS "."
    REQUIRES common esp_wifi esp_netif nvs_flash esp_event
)
