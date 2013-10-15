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
include_directories ( "${PROJECT_SOURCE_DIR}/../CubicVR-2/build/include" )
link_directories ( "${PROJECT_SOURCE_DIR}/../CubicVR-2/build/lib" )

SET (test_sources
    main.cpp
    ShaderRenderTest.cpp
    ShaderTest.cpp
)

SET (test_headers
    ShaderRenderTest.h
    ShaderTest.h
)



configure_files(${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/shaders COPYONLY)
configure_files(${PROJECT_SOURCE_DIR}/shaders ${PROJECT_BINARY_DIR}/shaders COPYONLY)


add_executable(test ${test_sources} ${test_headers})

target_link_libraries(test cubicvr2 glfw ${GLFW_LIBRARIES})



