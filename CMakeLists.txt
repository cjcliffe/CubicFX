cmake_minimum_required (VERSION 2.6)

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
endmacro(configure_files_recurse)

project (ShaderFX)


SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR})
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})

add_subdirectory(${PROJECT_SOURCE_DIR}/glfw-3.0.3 ${PROJECT_BINARY_DIR}/glfw/)
include_directories(${PROJECT_SOURCE_DIR}/glfw-3.0.3/include)
include_directories ( ${PROJECT_SOURCE_DIR}/../CubicVR-2/build/include )
link_directories ( ${PROJECT_SOURCE_DIR}/../CubicVR-2/build/lib )


IF (DEFINED WIN32)
	include_directories ( ${PROJECT_SOURCE_DIR}/openal-soft-1.15.1/include )
	link_directories ( ${PROJECT_SOURCE_DIR}/openal-soft-1.15.1/lib/Win64 )
	SET (OPENAL_LIBRARY libOpenAL32.dll winmm)
ELSE (DEFINED WIN32)
	find_package (OpenAL)
ENDIF (DEFINED WIN32)

SET (shaderfx_sources
    main.cpp
    ShaderViz.cpp
	BeatDetektor.cpp
)

SET (shaderfx_headers
    ShaderViz.h
	FFT.h
	BeatDetektor.h
)

configure_files(${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/shaders COPYONLY)
configure_files(${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/shaders COPYONLY)


add_executable(ShaderFX ${shaderfx_sources} ${shaderfx_headers})

target_link_libraries(ShaderFX cubicvr2 glfw ${GLFW_LIBRARIES} ${OPENAL_LIBRARY})



