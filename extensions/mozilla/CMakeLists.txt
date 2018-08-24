#
# Mozilla add-on
#

ADD_CUSTOM_TARGET(mozilla
    SOURCES
    manifest.json
    background.js
	content-script.js
)

SET_TARGET_PROPERTIES(mozilla
   PROPERTIES
   FOLDER "Extension"
)

INSTALL(
	DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
	DESTINATION extensions/mozilla
	PATTERN "CMakeLists.txt" EXCLUDE
)
