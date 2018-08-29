#
# Chrome add-on
#

# Templated manifest
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.json"
    "${CMAKE_CURRENT_BINARY_DIR}/manifest.json"
    @ONLY
)

ADD_CUSTOM_TARGET(google-chrome
    SOURCES
    manifest.json
    background.js
    content-script.js
)

SET_TARGET_PROPERTIES(google-chrome
   PROPERTIES
   FOLDER "Extension"
)

# installation

INSTALL(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION extensions/google-chrome
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN "manifest.json" EXCLUDE
)

INSTALL(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/manifest.json"
    DESTINATION extensions/google-chrome
)
