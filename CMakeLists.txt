#
# SMPipes
#
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

# specify project name and its version
PROJECT(SMPipes VERSION 0.9.0.0)

OPTION(USE_LOGON_SESSION "Logon session" OFF) 

# generate configure file
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
ADD_SUBDIRECTORY(server-app)
ADD_SUBDIRECTORY(client-app)
