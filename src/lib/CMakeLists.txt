# PARTIO SOFTWARE
# Copyright 2010 Disney Enterprises, Inc. All rights reserved
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
# 
# * The names "Disney", "Walt Disney Pictures", "Walt Disney Animation
# Studios" or the names of its contributors may NOT be used to
# endorse or promote products derived from this software without
# specific prior written permission from Walt Disney Pictures.
# 
# Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
# IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

FILE(GLOB io_cpp "io/*.cpp")
FILE(GLOB core_cpp "core/*.cpp")
ADD_LIBRARY (partio SHARED ${io_cpp} ${core_cpp})
SET_TARGET_PROPERTIES(partio PROPERTIES OUTPUT_NAME partio)
IF(ZLIB_FOUND)
target_link_libraries(partio ${ZLIB_LIBRARY})
ENDIF(ZLIB_FOUND)
INSTALL (TARGETS partio DESTINATION ${CMAKE_INSTALL_LIBDIR})

IF(SEEXPR_BASE)
INCLUDE_DIRECTORIES(${SEEXPR_INCLUDE_PATH})
LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
LINK_DIRECTORIES(${SEEXPR_LINK_PATH})
if (${PARTIO_SE_ENABLED})
    ADD_LIBRARY (partioSe SHARED PartioSe.cpp)
    target_link_libraries(partioSe ${PARTIO_LIBRARIES} ${SEEXPR_LIBS})
    INSTALL (TARGETS partioSe DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()
ENDIF(SEEXPR_BASE)

FILE(GLOB public_includes "*.h")
INSTALL (FILES ${public_includes} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
