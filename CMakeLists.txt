# CMakeLists.txt for <OpusEncoderFilter> 

INCLUDE_DIRECTORIES(
${artistIncludes}
${artistDirectShowIncludes}
${artistCodecIncludes}
${artistUtilIncludes}
)
 
# Lib directories
LINK_DIRECTORIES(
${vppLink}
)

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

TARGET_LINK_LIBRARIES (
OpusEncoderFilter
OpusCodec
${DsFilterLibs}
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
