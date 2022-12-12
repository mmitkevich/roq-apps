set(TARGET_NAME "${PROJECT_NAME}-app")

add_executable(
  "${TARGET_NAME}"
  "application.cpp"
  "context.cpp"
  "strategy.cpp"
  "order_manager.cpp"
  "publisher.cpp"
  "mbp_depth_array.cpp"
  "flags/flags.cpp"
)

add_subdirectory(flags)

target_link_libraries("${TARGET_NAME}" PRIVATE  ${PROJECT_NAME}-flags roq-client::roq-client roq-logging::roq-logging absl::flags fmt::fmt ) #umm
target_link_libraries("${TARGET_NAME}" PRIVATE _lightgbm)
target_link_libraries("${TARGET_NAME}" PRIVATE umm)

target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_20)

install(TARGETS ${TARGET_NAME})
