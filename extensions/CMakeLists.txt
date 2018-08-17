#
# extensions
#

INSTALL(DIRECTORY google-chrome DESTINATION extensions PATTERN "LICENSE.txt" EXCLUDE)
INSTALL(DIRECTORY mozilla DESTINATION extensions PATTERN "LICENSE.txt" EXCLUDE)
INSTALL(DIRECTORY icons DESTINATION extensions/mozilla)
INSTALL(DIRECTORY icons DESTINATION extensions/google-chrome)
