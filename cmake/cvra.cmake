include(AddCXXCompilerFlag)
include(AddCCompilerFlag)

# Generate a test exec given test sources and their dependencies
# Called like cvra_add_test(TARGET foo_test SOURCES foo_test.cpp DEPENDENCIES foo_lib)
function(cvra_add_test)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES DEPENDENCIES)
    cmake_parse_arguments(TEST_EXECUTABLE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ${CMAKE_CROSSCOMPILING})
        add_executable(${TEST_EXECUTABLE_TARGET} ${TEST_EXECUTABLE_SOURCES})

        target_link_libraries(${TEST_EXECUTABLE_TARGET} ${TEST_EXECUTABLE_DEPENDENCIES} test-runner)
        add_test(${TEST_EXECUTABLE_TARGET} ${TEST_EXECUTABLE_TARGET})
    endif()
endfunction()

macro(add_c_cxx_compiler_flag TARGET)
    add_c_compiler_flag(${TARGET})
    add_cxx_compiler_flag(${TARGET})
endmacro()
