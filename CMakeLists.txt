cmake_minimum_required (VERSION 3.16)

macro(configure_files srcDir destDir)
    message(STATUS "Configuring directory ${destDir}")
    make_directory(${destDir})

    file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/*)
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        if(NOT IS_DIRECTORY ${srcTemplatePath})
#            message(STATUS "Configuring file ${templateFile}")
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${templateFile}
                    COPYONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
    file(GLOB destFiles RELATIVE ${destDir} ${destDir}/*)
    foreach(destFile ${destFiles})
        if(NOT IS_DIRECTORY ${destDir}/${destFile})
            list(FIND templateFiles ${destFile} destIndex)
            if(destIndex EQUAL -1)
                file(REMOVE ${destDir}/${destFile})
            endif()
        endif()
    endforeach()
endmacro(configure_files)


macro(configure_files_recurse srcDir destDir)
    message(STATUS "Configuring directory ${destDir}")
    make_directory(${destDir})

    file(GLOB_RECURSE templateFiles RELATIVE ${srcDir} ${srcDir}/*)
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        if(NOT IS_DIRECTORY ${srcTemplatePath})
#            message(STATUS "Configuring file ${templateFile}")
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${templateFile}
                    COPYONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
    file(GLOB_RECURSE destFiles RELATIVE ${destDir} ${destDir}/*)
    foreach(destFile ${destFiles})
        if(NOT IS_DIRECTORY ${destDir}/${destFile})
            list(FIND templateFiles ${destFile} destIndex)
            if(destIndex EQUAL -1)
                file(REMOVE ${destDir}/${destFile})
            endif()
        endif()
    endforeach()
endmacro(configure_files_recurse)

project (CubicFX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG 6.0.1
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib rtaudio nlohmann_json)
FetchContent_GetProperties(raygui)
if(NOT raygui_POPULATED)
    FetchContent_Populate(raygui)
endif()

SET (cubicfx_sources
    src/main.cpp
    src/app/Session.cpp
    src/audio/AudioCapture.cpp
    src/beat/BeatEngine.cpp
    src/beat/BeatDetektor.cpp
    src/ui/SetupDialog.cpp
    src/ui/UiStyle.cpp
    src/viz/Visualizer.cpp
    src/viz/VizConfig.cpp
)

SET (cubicfx_headers
    src/app/Session.h
    src/audio/AudioCapture.h
    src/audio/FFT.h
    src/beat/BeatEngine.h
    src/beat/BeatDetektor.h
    src/ui/SetupDialog.h
    src/ui/UiStyle.h
    src/ui/Flex.h
    src/viz/Visualizer.h
    src/viz/VizConfig.h
)

configure_files_recurse(${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/shaders COPYONLY)
configure_files(${PROJECT_SOURCE_DIR}/images ${PROJECT_BINARY_DIR}/images COPYONLY)
configure_files(${PROJECT_SOURCE_DIR}/overlay ${PROJECT_BINARY_DIR}/overlay COPYONLY)
configure_files(${PROJECT_SOURCE_DIR}/fonts ${PROJECT_BINARY_DIR}/fonts COPYONLY)


add_executable(CubicFX ${cubicfx_sources} ${cubicfx_headers})

target_include_directories(CubicFX PRIVATE ${PROJECT_SOURCE_DIR}/src ${raygui_SOURCE_DIR}/src)

if (MSVC)
    target_compile_definitions(CubicFX PRIVATE _USE_MATH_DEFINES)
endif ()

target_link_libraries(CubicFX PRIVATE raylib rtaudio nlohmann_json::nlohmann_json)

enable_testing()
add_executable(FlexTest tests/FlexTest.cpp)
target_include_directories(FlexTest PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(FlexTest PRIVATE raylib)
add_test(NAME FlexTest COMMAND FlexTest)

add_executable(VizConfigTest tests/VizConfigTest.cpp src/viz/VizConfig.cpp)
target_include_directories(VizConfigTest PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(VizConfigTest PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME VizConfigTest COMMAND VizConfigTest)

add_executable(FeedbackTest tests/FeedbackTest.cpp src/viz/VizConfig.cpp)
target_include_directories(FeedbackTest PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(FeedbackTest PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME FeedbackTest COMMAND FeedbackTest)

add_executable(TransitionTest tests/TransitionTest.cpp)
target_include_directories(TransitionTest PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME TransitionTest COMMAND TransitionTest)

add_executable(ShaderSnap tools/shadersnap.cpp src/viz/VizConfig.cpp)
target_include_directories(ShaderSnap PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(ShaderSnap PRIVATE raylib nlohmann_json::nlohmann_json)


