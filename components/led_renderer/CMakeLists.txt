idf_component_register(
    SRCS "led_renderer.cpp" "led_effects.cpp"
    INCLUDE_DIRS "."
    REQUIRES word_layout common esp_driver_rmt esp_driver_gpio
)
