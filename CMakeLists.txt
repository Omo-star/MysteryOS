cmake_minimum_required(VERSION 3.16)
project(MysteryOS CXX)
set(CMAKE_CXX_STANDARD 17)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(GAME_SOURCES
    src/apps/image_viewer.cpp
    src/apps/anomaly_message.cpp
    src/apps/session_monitor.cpp
    src/main.cpp
    src/kernel/kernel.cpp
    src/kernel/vfs.cpp
    src/kernel/puzzle.cpp
    src/apps/file_explorer.cpp
    src/apps/text_editor.cpp
    src/apps/terminal.cpp
    src/apps/terminal_tools.cpp
    src/apps/password_dialog.cpp
    src/fx/glitch.cpp
    src/fx/scare_director.cpp
)

if(EMSCRIPTEN)
    add_executable(mysteryos ${GAME_SOURCES} ${IMGUI_SOURCES})
    target_include_directories(mysteryos PRIVATE
        src ${IMGUI_DIR} ${IMGUI_DIR}/backends third_party)
    set_target_properties(mysteryos PROPERTIES SUFFIX ".html")
    target_compile_options(mysteryos PRIVATE -sUSE_SDL=2)
    target_link_options(mysteryos PRIVATE
        -sUSE_SDL=2
        -sUSE_WEBGL2=1
        -sFULL_ES3=1
        -sALLOW_MEMORY_GROWTH=1
        "-sEXPORTED_FUNCTIONS=['_main','_mysteryos_anomaly_response','_mysteryos_anomaly_response_to','_mysteryos_anomaly_artifact','_mysteryos_anomaly_artifact_to']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall']"
        "SHELL:--embed-file ${CMAKE_SOURCE_DIR}/data@/data"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/docs/shell.html"
    )
else()
    add_executable(test_logic
        tests/test_main.cpp
        src/apps/terminal_tools.cpp
        src/kernel/vfs.cpp
        src/kernel/puzzle.cpp
        src/fx/glitch.cpp
        src/fx/scare_director.cpp
    )
    target_include_directories(test_logic PRIVATE src third_party)
    target_compile_definitions(test_logic PRIVATE MYSTERYOS_TEST)
endif()
