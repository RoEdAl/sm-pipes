#
# extensions
#

SET(EXTENSION_NAME "Send to SuperMemo" CACHE STRING "Add-on name")
SET(EXTENSION_DESC "Send to SuperMemo add-on" CACHE STRING "Add-on description")

FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/install-manifest.cmake
    "CONFIGURE_FILE(manifest.json \${BINARY_DIR}/manifest.json @ONLY)"
)

SET(MANIFEST_INSTALLATION_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/install-manifest.cmake)

# specify common files to compile here
SET(JS_FILES
    background.js
    content-script.js
)

FUNCTION(CREATE_BABEL_TARGET BROWSER)
    SET(JS_FILES)
    SET(JS_FILES_GEN)

    # manifest
    ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/manifest.json
        COMMAND ${CMAKE_COMMAND}
            -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -DEXTENSION_NAME=${EXTENSION_NAME}
            -DEXTENSION_DESC=${EXTENSION_DESC}
            -DEXTENSION_VERSION=${PROJECT_VERSION_STR_NO_PLATFORM}
            -P ${MANIFEST_INSTALLATION_SCRIPT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        MAIN_DEPENDENCY manifest.json
    )
    LIST(APPEND JS_FILES manifest.json)
    LIST(APPEND JS_FILES_GEN ${CMAKE_CURRENT_BINARY_DIR}/manifest.json)

    # JS files
    FOREACH(JS_FILE ${ARGN})
        GET_FILENAME_COMPONENT(JS_FILE_ABS ../${JS_FILE} ABSOLUTE)
        ADD_CUSTOM_COMMAND(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${JS_FILE}
            COMMAND ${CMD_NPX} babel 
                --source-type=script
                --env-name=$<CONFIG>
                --compact=$<IF:$<CONFIG:MinSizeRel>,true,false>
                --no-babelrc
                $<$<CONFIG:Release>:--no-comments>
                $<$<CONFIG:MinSizeRel>:--no-comments>
                $<$<CONFIG:RelWithDebInfo>:--no-comments>
                ${JS_FILE_ABS}
                --out-file ${CMAKE_CURRENT_BINARY_DIR}/${JS_FILE}
                --presets ${BABEL_PACKAGE}/babel-preset-$<CONFIG>.js,${BABEL_PACKAGE}/babel-preset-${BROWSER}.js
            WORKING_DIRECTORY ${BABEL_PACKAGE}
            MAIN_DEPENDENCY ${JS_FILE_ABS}
            DEPENDS babel-package
        )
        LIST(APPEND JS_FILES ${JS_FILE_ABS})
        LIST(APPEND JS_FILES_GEN ${CMAKE_CURRENT_BINARY_DIR}/${JS_FILE})
    ENDFOREACH(JS_FILE)

    ADD_CUSTOM_TARGET(
        extension-${BROWSER} ALL
        SOURCES 
            manifest.json
            ${JS_FILES}
        DEPENDS 
            babel-package
            ${JS_FILES_GEN}
    )

    SET_TARGET_PROPERTIES(
        extension-${BROWSER}
        PROPERTIES
        FOLDER "Extension"
    )

    INSTALL(
        FILES ${JS_FILES_GEN}
        DESTINATION extensions/${BROWSER}
    )
ENDFUNCTION(CREATE_BABEL_TARGET)

ADD_SUBDIRECTORY(mozilla)
ADD_SUBDIRECTORY(google-chrome)
