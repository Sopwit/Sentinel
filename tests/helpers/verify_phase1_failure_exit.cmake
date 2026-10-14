# A failed acceptance assertion must report failure and finish normal QML teardown.
execute_process(COMMAND "${PROBE}" --phase1-failure-smoke
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 20)
if(NOT "${result}" STREQUAL "97")
    message(FATAL_ERROR "Expected acceptance failure exit 97; got ${result}: ${error}")
endif()
if(NOT "${error}" MATCHES "phase1-failure-exit" OR
   NOT "${error}" MATCHES "qml-root-teardown-complete")
    message(FATAL_ERROR "Acceptance failure did not complete normal teardown: ${error}")
endif()
