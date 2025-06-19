cmake_minimum_required(VERSION 3.16)
project(DeribitTradingSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific definitions
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Define standalone Asio and WebSocket++ mode
add_definitions(-DASIO_STANDALONE -DBOOST_ASIO_STANDALONE)

# Find required packages (via vcpkg or system)
find_package(Boost 1.72 REQUIRED COMPONENTS system thread random)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Project include directories
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/websocketpp
)

# Add the executable and source files
add_executable(DeribitTradingSystem
    src/main.cpp
    src/deribit_client.cpp
    src/websocket_server.cpp
    src/order_manager.cpp
    src/market_data.cpp
    src/utils.cpp
)

# Include directories
target_include_directories(DeribitTradingSystem PRIVATE
    ${PROJECT_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(DeribitTradingSystem
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Add pthread for Unix if needed
if (UNIX)
    target_link_libraries(DeribitTradingSystem pthread)
endif()
