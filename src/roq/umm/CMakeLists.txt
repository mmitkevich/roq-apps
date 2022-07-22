set(TARGET_NAME "${PROJECT_NAME}-app")

add_library(
  "${TARGET_NAME}"
  "application.cpp"
  "main.cpp"
)

target_link_libraries("${TARGET_NAME}" PRIVATE roq-client::roq-client roq-logging::roq-logging absl::flags fmt::fmt)

target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_17)

install(TARGETS ${TARGET_NAME})
