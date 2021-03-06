#
# SMPipes
#
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
INCLUDE(ExternalProject)

PROJECT(SMPipes VERSION 0.9.1.0)

# Test 32/64 bits
IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    SET( EX_PLATFORM 64 )
    SET( EX_PLATFORM_NAME "x64" )
ELSE("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    SET( EX_PLATFORM 32 )
    SET( EX_PLATFORM_NAME "x86" )
ENDIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")

# version strings
SET(PROJECT_VERSION_STR "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} [${EX_PLATFORM_NAME}]")
SET(PROJECT_VERSION_STR_NO_PLATFORM "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
SET(PROJECT_VERSION_STR4 "${PROJECT_VERSION_MAJOR}, ${PROJECT_VERSION_MINOR}, ${PROJECT_VERSION_PATCH}, ${PROJECT_VERSION_TWEAK}")
SET(PROJECT_VERSION_STR4_DOTTED "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}")

# CPack
SET(CPACK_GENERATOR "ZIP")
SET(CPACK_PACKAGE_FILE_NAME "SMPipes-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-${EX_PLATFORM_NAME}")
SET(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_STR}")
SET(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
SET(CPACK_PACKAGE_CHECKSUM "SHA1")

INCLUDE(CPack)

# global settings
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH "..." FORCE)
ENDIF()

STRING(TIMESTAMP PROJECT_TIMESTAMP UTC)
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

OPTION(USE_LOGON_SESSION "Logon session" OFF)
OPTION(USE_STATIC_RUNTIME_LIBS "Use static runtime libraries" ON)

IF(USE_STATIC_RUNTIME_LIBS)
    FOREACH(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
       IF(${flag_var} MATCHES "/MD")
          STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
       ENDIF(${flag_var} MATCHES "/MD")
    ENDFOREACH(flag_var)
ENDIF(USE_STATIC_RUNTIME_LIBS)

# installation directory of external libraries
SET_PROPERTY(DIRECTORY PROPERTY EP_BASE extlib)

# RapidJSON
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/extlib/Install/project_RapidJSON/include")
    
ExternalProject_Add(
    project_RapidJSON
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/third-party/rapidjson"
    DOWNLOAD_COMMAND ""
    CMAKE_ARGS
       -DRAPIDJSON_BUILD_DOC=OFF
       -DRAPIDJSON_BUILD_EXAMPLES=OFF
       -DRAPIDJSON_BUILD_TESTS=OFF
       -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF
       -DRAPIDJSON_BUILD_CXX11=ON
       -DRAPIDJSON_BUILD_ASAN=OFF
       -DRAPIDJSON_BUILD_UPSAN=OFF
       -DRAPIDJSON_ENABLE_INSTRUMENTATION_OPT=OFF
       -DRAPIDJSON_HAS_STDSTRING=OFF
       -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/extlib/Install/project_RapidJSON
)

ExternalProject_Get_Property(project_RapidJSON INSTALL_DIR)

ADD_LIBRARY(RapidJSON INTERFACE IMPORTED)
SET_TARGET_PROPERTIES(RapidJSON PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${INSTALL_DIR}/include
)
ADD_DEPENDENCIES(RapidJSON project_RapidJSON)

# AutoIt
FIND_PROGRAM( CMD_AUT2EXE "Aut2Exe" PATH_SUFFIXES "AutoIt3/Aut2Exe" )

# npm, npx
FIND_PROGRAM( CMD_NPM "npm.cmd")
FIND_PROGRAM( CMD_NPX "npx.cmd")

# Configuration file`
CONFIGURE_FILE(
    "${PROJECT_SOURCE_DIR}/SMPipesConfig.h.in"
    "${PROJECT_BINARY_DIR}/include/SMPipesConfig.h"
)

# common includes
INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}/include")
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/include")

# UNICODE apps and libraries
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

# subdirectories
ADD_SUBDIRECTORY(server-echo-app)
ADD_SUBDIRECTORY(server-mbdd)
ADD_SUBDIRECTORY(client-app)
ADD_SUBDIRECTORY(native-msg-proxy)
ADD_SUBDIRECTORY(include)
ADD_SUBDIRECTORY(extensions)

