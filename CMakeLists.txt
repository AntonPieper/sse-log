cmake_minimum_required(VERSION 3.22)

project(RestAPI)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
find_package(CppRestSDK REQUIRED)

add_executable(RestAPI main.cpp)

target_link_libraries(RestAPI PRIVATE cpprestsdk::cpprest OpenSSL::SSL OpenSSL::Crypto)