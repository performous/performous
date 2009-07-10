INCLUDE_DIRECTORIES(../src)
LINK_DIRECTORIES(../src)

LINK_LIBRARIES(silkystrings-convenience)

ADD_EXECUTABLE(wm-openwindow-test wm-openwindow-test.cpp)
ADD_EXECUTABLE(wm-busyloop-update-test wm-busyloop-update-test.cpp)
ADD_EXECUTABLE(wm_input-keyevent-propagation-test wm_input-keyevent-propagation-test.cpp)
ADD_EXECUTABLE(wm-get_clock-sleep-test wm-get_clock-sleep-test.cpp)
ADD_EXECUTABLE(wm-gl-extensions-test wm-gl-extensions-test.cpp)

ADD_EXECUTABLE(input-keyevent-handling-test input-keyevent-handling-test.cpp)

ADD_EXECUTABLE(vertexformat-test vertexformat-test.cpp)

ADD_EXECUTABLE(vertex_data_buffer-test vertex_data_buffer-test.cpp)

ADD_EXECUTABLE(mesh-cube-test mesh-cube-test.cpp)

ADD_EXECUTABLE(mesh-factory-test mesh-factory-test.cpp)

ADD_EXECUTABLE(textrenderer-load-font-test textrenderer-load-font-test.cpp)

ADD_EXECUTABLE(keyevent_client-test keyevent_client-test.cpp)

ADD_EXECUTABLE(gameview-test gameview-test.cpp)

ADD_EXECUTABLE(reaction-test reaction-test.cpp)

