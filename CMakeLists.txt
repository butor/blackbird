cmake_minimum_required(VERSION 3.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

project(blackbird)

set(CMAKE_CXX_STANDARD 11)

include_directories(src)

set(HEADER_FILES
        src/exchange/bitfinex.h
        src/exchange/bitstamp.h
        src/exchange/btce.h
        src/exchange/gemini.h
        src/exchange/itbit.h
        src/exchange/kraken.h
        src/exchange/okcoin.h
        src/exchange/poloniex.h
        src/utils/base64.h
        src/utils/send_email.h
        src/bitcoin.h
        src/check_entry_exit.h
        src/curl_fun.h
        src/db_fun.h
        src/parameters.h
        src/result.h
        src/time_fun.h
        )

set(SOURCE_FILES
        src/exchange/bitfinex.cpp
        src/exchange/bitstamp.cpp
        src/exchange/btce.cpp
        src/exchange/gemini.cpp
        src/exchange/itbit.cpp
        src/exchange/kraken.cpp
        src/exchange/okcoin.cpp
        src/exchange/poloniex.cpp
        src/utils/base64.cpp
        src/utils/send_email.cpp
        src/bitcoin.cpp
        src/check_entry_exit.cpp
        src/curl_fun.cpp
        src/db_fun.cpp
        src/main.cpp
        src/parameters.cpp
        src/result.cpp
        src/time_fun.cpp
        )

add_executable(blackbird ${SOURCE_FILES})

find_package(Jansson REQUIRED)
if (NOT JANSSON_FOUND)
    message(FATAL_ERROR "Missing Jansson")
endif (NOT JANSSON_FOUND)
include_directories(${JANSSON_INCLUDE_DIR})
set(LIBS ${LIBS} ${JANSSON_LIBRARIES})

find_package(OpenSSL REQUIRED)
if (NOT OpenSSL_FOUND)
    message(FATAL_ERROR "Missing OpenSSL")
endif (NOT OpenSSL_FOUND)
include_directories(${OPENSSL_INCLUDE_DIR})
set(LIBS ${LIBS} ${OPENSSL_LIBRARIES})

find_package(MySQL REQUIRED)
if (NOT MYSQL_FOUND)
    message(FATAL_ERROR "Missing MySQL")
endif (NOT MYSQL_FOUND)
include_directories(${MYSQL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${MYSQL_LIBRARIES})

find_package(CURL REQUIRED)
if (NOT CURL_FOUND)
    message(FATAL_ERROR "Missing CURL")
endif (NOT CURL_FOUND)
include_directories(${CURL_INCLUDE_DIRS})
set(LIBS ${LIBS} ${CURL_LIBRARIES})

target_link_libraries(blackbird ${LIBS})