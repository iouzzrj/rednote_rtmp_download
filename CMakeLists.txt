cmake_minimum_required(VERSION 3.20)
project(rednote_rtmp_download LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(rednote_rtmp_download src/main.cpp)

target_link_libraries(rednote_rtmp_download PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

if (MSVC)
  target_compile_options(rednote_rtmp_download PRIVATE /W4 /permissive-)
else()
  target_compile_options(rednote_rtmp_download PRIVATE -Wall -Wextra -Wpedantic)
endif()
