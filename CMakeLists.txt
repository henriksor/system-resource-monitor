cmake_minimum_required(VERSION 3.16)
project(SystemResourceMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SystemResourceMonitor
    src/main.cpp
    src/SystemMonitor.cpp
    src/CpuMonitor.cpp
    src/MemoryMonitor.cpp
    src/Logger.cpp
    src/AnomalyDetector.cpp
    src/ConfigManager.cpp
)

target_include_directories(SystemResourceMonitor
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(SystemResourceMonitor
    PRIVATE
        include
        external/json
)