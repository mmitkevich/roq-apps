# (c) copyright 2023 Mikhail Mitkevich
set(TARGET_NAME "${PROJECT_NAME}-dag")


add_library( "${TARGET_NAME}" STATIC
  "node.cpp"
  "compute.cpp"
  "pricer.cpp"
  "factory.cpp"

)
target_compile_definitions(${TARGET_NAME} PRIVATE ROQ_PACKAGE_NAME="${PROJECT_NAME}")
target_compile_features("${TARGET_NAME}" PUBLIC cxx_std_20)


add_executable( "graph-test"
  "graph.ut.cpp"
)
target_compile_features("graph-test" PUBLIC cxx_std_20)

install(TARGETS ${TARGET_NAME})
