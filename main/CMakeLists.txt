idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES app_controller nvs_flash esp_netif esp_event
)
