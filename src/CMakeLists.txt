set(APP_FILES app.cpp menu.cpp shutdown.cpp util.cpp log.cpp)

add_executable(RobotRL ${APP_FILES})

target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} ${SFML_LIBRARIES})
