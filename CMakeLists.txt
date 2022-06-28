cmake_minimum_required(VERSION 3.16)
project(xetrov CXX ASM)
include(FetchContent)

FetchContent_Declare(xe GIT_REPOSITORY https://github.com/ilikdoge/xe.git GIT_TAG master)
FetchContent_MakeAvailable(xe)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

set(XE_LINKER_FLAGS "")
include_directories(".")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines -fno-pie -no-pie")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts -stdlib=libc++")
	set(XE_LINKER_FLAGS "-fuse-ld=lld")
else()
	message(SEND_ERROR "Unsupported Compiler")
endif()

set(XE_FLTO TRUE)
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -march=native -mtune=native")
	if(XE_FLTO)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${XE_LINKER_FLAGS}")
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