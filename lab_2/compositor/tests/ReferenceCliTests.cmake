file(MAKE_DIRECTORY "${TEST_DIR}")
foreach(mode IN ITEMS sequential parallel-safe)
    configure_file("${REFERENCE_DIR}/${mode}.json" "${TEST_DIR}/${mode}.json" COPYONLY)
    execute_process(COMMAND "${PROGRAM}" "${TEST_DIR}/${mode}.json"
                    RESULT_VARIABLE status ERROR_VARIABLE error)
    if(NOT status EQUAL 0)
        message(FATAL_ERROR "Reference ${mode} execution failed: ${error}")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files
                    "${REFERENCE_DIR}/expected.ppm" "${TEST_DIR}/actual-${mode}.ppm"
                    RESULT_VARIABLE comparison)
    if(NOT comparison EQUAL 0)
        message(FATAL_ERROR "Reference ${mode} PPM differs from the hand-calculated result")
    endif()
endforeach()
