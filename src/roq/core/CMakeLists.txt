set(TARGET_NAME "${PROJECT_NAME}-core")


add_library( "${TARGET_NAME}" STATIC
  "market/manager.cpp"
  "portfolio/manager.cpp"
  "gateway/manager.cpp"
  
  "best_quotes.cpp"
  "best_quotes/manager.cpp"
  
  "manager.cpp"
  "portfolio.cpp"
  "price.cpp"
  "string_utils.cpp"
)

target_compile_definitions(${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${PROJECT_NAME}")


target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_20)


install(TARGETS ${TARGET_NAME})


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/config")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/oms")
add_subdirectory(flags)


add_executable(
  "${PROJECT_NAME}"
  "main.cpp"
  "factory.cpp"
  "strategy.cpp"
  "application.cpp"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ROQ_PACKAGE_NAME="${PROJECT_NAME}")

target_link_libraries("${PROJECT_NAME}" PRIVATE  
  roqa-spreader
  
  roqa-lqs
  
  roqa-dag
  roqa-dag-ops


  roqa-oms
  roqa-core  
  roqa-config    

  roq-logging::roq-logging 
  ${PROJECT_NAME}-flags 
  roq-client::roq-client 
  roq-client::roq-client-flags
  roq-cache::roq-cache
  absl::flags 
  fmt::fmt 
  roq-flags::roq-flags
  roq-logging::roq-logging-flags
  roq-io::roq-io
  roq-web::roq-web
  )

target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_20)

install(TARGETS ${TARGET_NAME})
install(TARGETS ${PROJECT_NAME})
