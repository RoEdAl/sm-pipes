#
# SMClientApp
#

file(GLOB PROJECT_PUBLIC_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.hpp")
SOURCE_GROUP("Public Header Files" FILES ${PROJECT_PUBLIC_HEADER_FILES})

SET_SOURCE_FILES_PROPERTIES(commands.json PROPERTIES HEADER_FILE_ONLY TRUE)

SOURCE_GROUP("Resource files" FILES
	client-app.rc
	commands.json
)

ADD_EXECUTABLE(SMClientApp
	client-app.cpp
	client-app.rc resource.h commands.json
	${PROJECT_PUBLIC_HEADER_FILES}
)

ADD_DEPENDENCIES(SMClientApp RapidJSON)
TARGET_LINK_LIBRARIES(SMClientApp PRIVATE RapidJSON)

SET_TARGET_PROPERTIES(SMClientApp
   PROPERTIES
   CXX_STANDARD 14
   FOLDER "Client"
)

INSTALL(TARGETS SMClientApp DESTINATION bin)