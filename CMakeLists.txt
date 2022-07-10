cmake_minimum_required(VERSION 3.16)
project(xetrov CXX ASM)
cmake_policy(SET CMP0069 NEW)
include(CheckIPOSupported)
check_ipo_supported()
include(FetchContent)

FetchContent_Declare(xe GIT_REPOSITORY https://github.com/ilikdoge/xe.git GIT_TAG master)
FetchContent_MakeAvailable(xe)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

include_directories(".")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines -fno-pie -no-pie")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts -stdlib=libc++")
else()
	message(SEND_ERROR "Unsupported Compiler")
endif()

set(XE_FLTO TRUE)
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")

elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -march=native -mtune=native")
	if(XE_FLTO)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
	endif()
endif()

set(SOURCES "")
file(GLOB SOURCES
	"xetrov/demuxers/*.cc"
	"xetrov/codecs/*.cc"
	"xetrov/resource/*.cc"
	"xetrov/reader/*.cc"
	"xetrov/fiber/*.cc"
	"xetrov/fiber/*.s"
	"xetrov/*.cc"
)

add_library(xetrov ${SOURCES})
target_link_libraries(xetrov xe opus avcodec avutil)