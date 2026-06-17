if (USE_FURIOSA)
    file(GLOB TENT_PLATFORM_FURIOSA_SOURCES "*.cpp")
    add_library(platform_furiosa STATIC
        ${TENT_PLATFORM_FURIOSA_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/transport/rdma_transport/furiosa_npu.cpp)
    target_include_directories(platform_furiosa PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include)
    target_link_libraries(platform_furiosa PUBLIC tent_common tent_interface)
endif()
