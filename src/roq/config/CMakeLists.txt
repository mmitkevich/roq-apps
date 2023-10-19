set(TARGET_NAME "${PROJECT_NAME}-config")


add_library( "${TARGET_NAME}" STATIC
  "toml_config.cpp"
)

target_compile_definitions(${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${PROJECT_NAME}")

target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_20)

install(TARGETS ${TARGET_NAME})
