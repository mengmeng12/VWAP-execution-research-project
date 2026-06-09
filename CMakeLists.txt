cmake_minimum_required(VERSION 3.10)

project(VWAPExecutionResearch)

# Use beginner-friendly modern C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create executable
add_executable(vwap_execution
    src/main.cpp
    src/TimeUtils.cpp
    src/CsvParser.cpp
    src/MapReduceProcessor.cpp
    src/VolumeProfile.cpp
    src/VWAPExecution.cpp
    src/OutputWriter.cpp
)

# Tell compiler where to find header files
target_include_directories(vwap_execution PRIVATE include)

# Enable pthread for std::thread on Linux/macOS
find_package(Threads REQUIRED)
target_link_libraries(vwap_execution PRIVATE Threads::Threads)