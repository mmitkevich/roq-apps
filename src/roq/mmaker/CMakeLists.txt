# (c) copyright 2023 Mikhail Mitkevich
set(TARGET_NAME "${PROJECT_NAME}-app")


add_executable(
  "${TARGET_NAME}"
  "application.cpp"
  "strategy.cpp"
  "flags/flags.cpp"
)

target_compile_definitions(${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${PROJECT_NAME}")

add_subdirectory(flags)

target_link_libraries("${TARGET_NAME}" PRIVATE  ${PROJECT_NAME}-flags 
  roq-client::roq-client 
  roq-client::roq-client-flags
  roq-logging::roq-logging 
  absl::flags 
  fmt::fmt 
  roq-flags::roq-flags
  roq-logging::roq-logging-flags
  roq-mmaker-core
  roq-mmaker-config  
  roq-mmaker-pricer
  roq-mmaker-pricer-ops
  roq-mmaker-oms
  )

target_link_libraries("${TARGET_NAME}" PRIVATE _lightgbm)
target_link_libraries("${TARGET_NAME}" PRIVATE umm)

target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_20)

install(TARGETS ${TARGET_NAME})
