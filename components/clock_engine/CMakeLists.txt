idf_component_register(
    SRCS "clock_engine.cpp" "clock_rules.cpp"
    INCLUDE_DIRS "."
    REQUIRES word_layout common
)
