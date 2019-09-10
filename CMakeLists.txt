# CMakeLists.txt for <OpusEncoderFilter> 
cmake_minimum_required(VERSION 3.11)

# hints from https://github.com/robotology/how-to-export-cpp-library/blob/master/CMakeLists.txt
project(OpusEncoderFilter
        LANGUAGES CXX C
        VERSION 0.0.0.0)

#Make sure that custom modules like FindRapidJSON are found
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

LIST(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/externals/vcpkg/packages/opus_x86-windows/share/opus)
##############################################
include(FetchContent)
FetchContent_Declare(
  Vpp
  GIT_REPOSITORY https://github.com/CSIR-RTVC/OpusCodec
)
# Declare dependencies
find_package(OpusCodec 1.0.0 REQUIRED)

FetchContent_Declare(
  DirectShowExt
  GIT_REPOSITORY https://github.com/CSIR-RTVC/DirectShowExt
)
# Declare dependencies
find_package(DirectShowExt 1.0.0 REQUIRED)

#SET (DSE_INCLUDE_DIRS 
#	${PROJECT_SOURCE_DIR}/include/
#	${PROJECT_SOURCE_DIR}/externals/DirectShowExt/include
#	${PROJECT_SOURCE_DIR}/externals/DShowBaseClasses/baseclasses
#	${PROJECT_SOURCE_DIR}/externals/Opus/include
#	${PROJECT_SOURCE_DIR}/externals/Opus/externals/CodecUtils/include
#	${PROJECT_SOURCE_DIR}/externals/Opus/externals/CodecUtils/include/CodecUtils
#)

#LIST(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/externals/DShowBaseClasses/build)
#LIST(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/externals/Opus/build/src)
#LIST(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/externals/DirectShowExt/build/src)
#LIST(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/externals/Opus/externals/vcpkg/packages/opus_x86-windows/share/opus)
#find_package(OpusCodec REQUIRED)
#find_package(BaseClasses REQUIRED)
#find_package(DirectShowExt REQUIRED)
#find_package(Opus REQUIRED)


SET(FLT_HDRS
AudioBuffer.h
FilterParameters.h
OpusEncoderFilter.h
OpusEncoderProperties.h
resource.h
stdafx.h
)

SET(FLT_SRCS 
DLLSetup.cpp
OpusEncoderFilter.cpp
OpusEncoderFilter.def
OpusEncoderFilter.rc
stdafx.cpp
)

ADD_LIBRARY(
OpusEncoderFilter SHARED ${FLT_SRCS} ${FLT_HDRS})

target_include_directories(OpusEncoderFilter
    PUBLIC 
        $<INSTALL_INTERFACE:install>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

LINK_DIRECTORIES(
    $ENV{MSSDK}/Lib
)

TARGET_LINK_LIBRARIES (
    OpusEncoderFilter
#    BaseClasses::BaseClasses
    OpusCodec::OpusCodec
    DirectShowExt::DirectShowExt
	strmiids
#	winmm
	comctl32
) 

INSTALL(
  TARGETS OpusEncoderFilter
  ARCHIVE DESTINATION bin
  LIBRARY DESTINATION bin
)

IF (REGISTER_DS_FILTERS)
ADD_CUSTOM_COMMAND(
TARGET OpusEncoderFilter 
POST_BUILD COMMAND 
regsvr32 /s \"$(TargetPath)\"
)
ENDIF(REGISTER_DS_FILTERS)
