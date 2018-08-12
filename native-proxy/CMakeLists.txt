#
# SMNativeProxy
#

file(GLOB PROJECT_PUBLIC_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.hpp")
SOURCE_GROUP("Public Header Files" FILES ${PROJECT_PUBLIC_HEADER_FILES})

SET_SOURCE_FILES_PROPERTIES(native-proxy-google.json native-proxy-mozilla.json PROPERTIES HEADER_FILE_ONLY TRUE)

SOURCE_GROUP("Resource files" FILES
	native-proxy.rc
	native-proxy-google.json
	native-proxy-mozilla.json
)

ADD_EXECUTABLE(SMNativeProxy 
	native-proxy.cpp
	registry.cpp registry.h
	native-proxy.rc resource.h
	native-proxy-google.json
	native-proxy-mozilla.json
	${PROJECT_PUBLIC_HEADER_FILES}
)

SET_TARGET_PROPERTIES(SMNativeProxy
   PROPERTIES
   CXX_STANDARD 14
   FOLDER "Client"
)

INSTALL(TARGETS SMNativeProxy DESTINATION bin)
