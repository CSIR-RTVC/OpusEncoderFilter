# CMakeLists.txt for <OpusEncoderFilter> 
cmake_minimum_required(VERSION 3.0)

# hints from https://github.com/robotology/how-to-export-cpp-library/blob/master/CMakeLists.txt
project(OpusEncoderFilter
        LANGUAGES CXX C
        VERSION 0.0.0.0)

SET (DSE_INCLUDE_DIRS 
	${PROJECT_SOURCE_DIR}/include/
	${PROJECT_SOURCE_DIR}/externals/DirectShowExt/include
	${PROJECT_SOURCE_DIR}/externals/DShowBaseClasses/baseclasses
	${PROJECT_SOURCE_DIR}/externals/Opus/include
	${PROJECT_SOURCE_DIR}/externals/Opus/externals/CodecUtils/include
	${PROJECT_SOURCE_DIR}/externals/Opus/externals/CodecUtils/include/CodecUtils
)

LIST(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/externals/DShowBaseClasses/build)
LIST(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/externals/Opus/build/src)
LIST(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/externals/DirectShowExt/build/src)
LIST(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/externals/Opus/externals/vcpkg/packages/opus_x86-windows/share/opus)
find_package(OpusCodec REQUIRED)
find_package(BaseClasses REQUIRED)
find_package(DirectShowExt REQUIRED)
find_package(Opus REQUIRED)


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
        $<INSTALL_INTERFACE:>    
        $<BUILD_INTERFACE:>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
		${DSE_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES (
OpusEncoderFilter
DirectShowExt::DirectShowExt
OpusCodec::OpusCodec
BaseClasses::BaseClasses
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
