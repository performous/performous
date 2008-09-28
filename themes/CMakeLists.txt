FILE(GLOB THEME_FILES "lima/*.svg" "lima/*.png" "lima/*.ogg")

install( FILES ${THEME_FILES} DESTINATION ${SHARE_INSTALL_PREFIX}/ultrastar-ng/themes/lima/)
