cmake_minimum_required(VERSION 3.16)
project(SystemResourceMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DNOMINMAX)

add_executable(SystemResourceMonitor
    src/main.cpp
    src/SystemMonitor.cpp
    src/AlertHandler.cpp
    src/OutputHandler.cpp
    src/CpuMonitor.cpp
    src/MemoryMonitor.cpp
    src/Logger.cpp
    src/AnomalyDetector.cpp
    src/ConfigManager.cpp
    src/ProcessMonitor.cpp
)

target_include_directories(SystemResourceMonitor
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external/json
)

target_link_libraries(SystemResourceMonitor
    PRIVATE
        winhttp
)

option(BUILD_TESTING "Build unit tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    include(FetchContent)

    # Catch2 v2 exposes a single header include, which keeps the test setup
    # minimal and avoids changing the main target's build.
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.13.10
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(SystemResourceMonitorTests
        tests/test_main.cpp
        tests/test_anomaly.cpp
        src/AnomalyDetector.cpp
    )

    target_include_directories(SystemResourceMonitorTests
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/external/json
            ${catch2_SOURCE_DIR}/single_include
    )

    add_test(
        NAME SystemResourceMonitorTests
        COMMAND SystemResourceMonitorTests
    )
endif()
