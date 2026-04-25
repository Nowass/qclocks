idf_component_register(
    SRCS "app_controller.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        common
        clock_engine
        led_renderer
        time_service
        wifi_service
        provisioning_service
        ota_service
        settings_store
        diagnostics_service
        esp_event
)
