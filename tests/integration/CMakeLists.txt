# Integration tests — run against the live wandb service.
# These tests are labelled "integration" and excluded from default builds.
# Run with: ctest -L integration --output-on-failure

# Helper: add an integration test executable (no WANDB_MODE=offline).
function(wandb_add_integration_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            wandb_client
            GTest::gtest_main
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "integration"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endfunction()

# -- Integration test targets --
wandb_add_integration_test(test_run_online test_run_online.cc)
wandb_add_integration_test(test_metrics_online test_metrics_online.cc)
