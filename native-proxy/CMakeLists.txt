#
# SMNativeProxy
#

file(GLOB PROJECT_PUBLIC_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.hpp")
SOURCE_GROUP("Public Header Files" FILES ${PROJECT_PUBLIC_HEADER_FILES})

SET_SOURCE_FILES_PROPERTIES(native-proxy.json PROPERTIES HEADER_FILE_ONLY TRUE)

SOURCE_GROUP("Resource files" FILES
	native-proxy.rc
	native-proxy.json)

ADD_EXECUTABLE(SMNativeProxy 
	native-proxy.cpp
	registry.cpp registry.h
	native-proxy.rc resource.h
	native-proxy.json
	${PROJECT_PUBLIC_HEADER_FILES}
)

SET_PROPERTY(TARGET SMNativeProxy PROPERTY CXX_STANDARD 14)

INSTALL(TARGETS SMNativeProxy DESTINATION bin)