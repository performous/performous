FILE(GLOB THEME_FILES "default/*.ogg" "default/*.svg" "default/*.png" "default/*.bmp")

install( FILES ${THEME_FILES} DESTINATION ${SHARE_INSTALL}/themes/default/)

