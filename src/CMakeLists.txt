# wandb_client static library
add_library(wandb_client STATIC
    py_runtime.cc
    config.cc
    run.cc
)

target_include_directories(wandb_client
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(wandb_client
    PUBLIC
        pybind11::embed
        Python3::Python
)
