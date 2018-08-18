#
# extensions
#

ADD_SUBDIRECTORY(google-chrome)
ADD_SUBDIRECTORY(mozilla)

#INSTALL(DIRECTORY google-chrome DESTINATION extensions)
#INSTALL(DIRECTORY mozilla DESTINATION extensions)
INSTALL(DIRECTORY icons DESTINATION extensions/mozilla)
INSTALL(DIRECTORY icons DESTINATION extensions/google-chrome)
