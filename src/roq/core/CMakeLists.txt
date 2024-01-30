set(TARGET_NAME "${PROJECT_NAME}-core")


add_library( "${TARGET_NAME}" STATIC
  "gateways.cpp"
  "markets.cpp"
  "manager.cpp"
  "portfolios.cpp"
  "price.cpp"
  "best_quotes.cpp"
  "best_quotes_cache.cpp"
  "utils/string_utils.cpp"
)

target_compile_definitions(${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${PROJECT_NAME}")

target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_20)

install(TARGETS ${TARGET_NAME})
