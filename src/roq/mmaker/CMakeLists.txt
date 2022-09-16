set(TARGET_NAME "${PROJECT_NAME}-app")

add_executable(
  "${TARGET_NAME}"
  "application.cpp"
  "context.cpp"
  "strategy.cpp"
  "order_manager.cpp"
  "flags/flags.cpp"
)

target_link_libraries("${TARGET_NAME}" PRIVATE roq-client::roq-client roq-logging::roq-logging absl::flags fmt::fmt ) #umm

target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_17)

install(TARGETS ${TARGET_NAME})
