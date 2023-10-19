set(TARGET_NAME "${PROJECT_NAME}-pricer")


add_library( "${TARGET_NAME}" STATIC
  "manager.cpp"
)
target_compile_definitions(${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${PROJECT_NAME}")
target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_20)


#add_executable( "${TARGET_NAME}-test"
#  "tests/pricer.ut.cpp"
#)
#target_compile_features("${TARGET_NAME}-test" PUBLIC cxx_std_20)

install(TARGETS ${TARGET_NAME})
