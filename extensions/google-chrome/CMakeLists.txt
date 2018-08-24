#
# Chrome add-on
#

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

INSTALL(
	DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
	DESTINATION extensions/google-chrome
	PATTERN "CMakeLists.txt" EXCLUDE
)
