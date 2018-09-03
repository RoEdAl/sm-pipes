#
# extensions
#

ADD_SUBDIRECTORY(babel-package)
ADD_SUBDIRECTORY(extension)

INSTALL(DIRECTORY icons DESTINATION extensions/mozilla)
INSTALL(DIRECTORY icons DESTINATION extensions/google-chrome)
