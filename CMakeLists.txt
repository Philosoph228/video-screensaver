cmake_minimum_required(VERSION 3.16)
set(APPNAME "screensaver")

project(${APPNAME})

add_executable(${APPNAME} WIN32 mfgw.c video.c playback.c scrnsave.c main.c)
target_compile_definitions(${APPNAME} PRIVATE -D_UNICODE -DUNICODE)
set_target_properties(${APPNAME} PROPERTIES SUFFIX ".scr")
target_link_libraries(${APPNAME} ole32 strmiids)
