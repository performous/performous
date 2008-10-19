FILE(GLOB THEME_FILES "lima/*.ogg" "lima/*.svg" "lima/*.png" "lima/*.bmp")

install( FILES ${THEME_FILES} DESTINATION ${SHARE_INSTALL_PREFIX}/themes/lima/)

