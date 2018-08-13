#
# SMNativeMsgProxy
#

file(GLOB PROJECT_PUBLIC_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.hpp")
SOURCE_GROUP("Public Header Files" FILES ${PROJECT_PUBLIC_HEADER_FILES})

SET_SOURCE_FILES_PROPERTIES(
    native-msg-proxy-google.json native-msg-proxy-mozilla.json
    PROPERTIES HEADER_FILE_ONLY ON
)

SOURCE_GROUP("Resource files" FILES
	native-msg-proxy.rc
	native-msg-proxy-google.json
	native-msg-proxy-mozilla.json
)

ADD_EXECUTABLE(SMNativeMsgProxy 
	native-msg-proxy.cpp
	printf.hpp
	registry.cpp registry.h
	native-msg-proxy.rc resource.h
	native-msg-proxy-google.json
	native-msg-proxy-mozilla.json
	${PROJECT_PUBLIC_HEADER_FILES}
)

SET_TARGET_PROPERTIES(SMNativeMsgProxy
   PROPERTIES
   CXX_STANDARD 14
   FOLDER "Client"
)

INSTALL(TARGETS SMNativeMsgProxy DESTINATION bin)
