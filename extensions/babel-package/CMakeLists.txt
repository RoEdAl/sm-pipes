#
# babel-package
#

# create package directory

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/package)
SET(BABEL_PACKAGE ${CMAKE_CURRENT_BINARY_DIR}/package PARENT_SCOPE)

# presets list

SET(BABEL_PRESETS
    babel-preset-google-chrome.js
    babel-preset-mozilla.js
    babel-preset-Debug.js
    babel-preset-Release.js
    babel-preset-MinSizeRel.js
    babel-preset-RelWithDebInfo.js
)

SET(BABEL_PACKAGE_DEPENDENCIES)

# package.json
ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/package/package.json
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/package.json ${CMAKE_CURRENT_BINARY_DIR}/package/package.json
    COMMAND ${CMD_NPM} install --loglevel error --no-optional --only-dev
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/package
    MAIN_DEPENDENCY package.json
)
LIST(APPEND BABEL_PACKAGE_DEPENDENCIES ${CMAKE_CURRENT_BINARY_DIR}/package/package.json)

# presets
FOREACH(PRESET ${BABEL_PRESETS})
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/package/${PRESET}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PRESET} ${CMAKE_CURRENT_BINARY_DIR}/package/${PRESET}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        MAIN_DEPENDENCY ${PRESET}
    )
    LIST(APPEND BABEL_PACKAGE_DEPENDENCIES ${CMAKE_CURRENT_BINARY_DIR}/package/${PRESET})
ENDFOREACH(PRESET)

ADD_CUSTOM_TARGET(
    babel-package
    SOURCES package.json ${BABEL_PRESETS}
    DEPENDS ${BABEL_PACKAGE_DEPENDENCIES}
)

SET_TARGET_PROPERTIES(
    babel-package
    PROPERTIES
    FOLDER "Extension"
)
