enable_testing()

# Helper: add a test executable linked against wandb_client and GTest
function(wandb_add_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            wandb_client
            GTest::gtest_main
    )
    # Set WANDB_MODE=offline so tests never hit the network
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
        ENVIRONMENT "WANDB_MODE=offline"
        LABELS "unit"
    )
endfunction()

# -- Unit test targets --
wandb_add_test(test_py_runtime test_py_runtime.cc)
wandb_add_test(test_config test_config.cc)
wandb_add_test(test_run test_run.cc)

# -- Integration tests (separate subdirectory) --
add_subdirectory(integration)
