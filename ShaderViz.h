//
//  ShaderRenderTest.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-03-11.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__ShaderRenderTest__
#define __CubicVR2__ShaderRenderTest__

#include <iostream>
#include "cubicvr2/opengl/Shader.h"
#include "cubicvr2/opengl/Material.h"
#include "cubicvr2/core/Mesh.h"
#include "cubicvr2/core/Camera.h"
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <math.h>

//#include <GLUT/glut.h>
#pragma once

namespace CubicVR {
    
    
    class ShaderViz {
    public:
        Shader testShader;
//        static Material mat;
//        static Camera testCam;
        
        shaderVariables shaderVars;
        
        shaderAttributeVec3 a_vertexPosition;
        shaderUniformVec2 u_resolution;
        shaderUniformFloat u_time;
        shaderUniformVec2 u_mouse;
        
        shaderUniformFloatVector u_sampleData;
        shaderUniformFloatVector u_vuData;
        shaderUniformVec3 u_baseColor;
        
        Mesh testMesh;
        float time_value;
        
//        static GLuint vao;
        
        void display(vector<float> &sample_data, vector<float> &vu_data)
        {
            // Clear frame buffer and depth buffer
            glClearColor(0.3f,0.3f,0.3f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, 1280, 720);
            
            time_value += 1.0f/60.0f;
//            testCam.setPosition(vec3(1.5f*sinf((float)i/100.0f),1.5f*sinf(i/100.0f),1.5f*cosf((float)i/100.0f)));
//            testCam.calcProjection();
//            mat.setMatrixModelView(testCam.getMatrixModelView());
//            mat.setMatrixProjection(testCam.getMatrixProjection());

//            mat.use(LIGHT_NONE,0,&testMesh);

            testShader.use();
            checkError(11);
            a_vertexPosition.set(testMesh.getVBO()->gl_points);
            a_vertexPosition.update();
            u_time.set(time_value);
            u_time.update();
            u_resolution.set(vec2(1280,720));
            u_resolution.update();
            u_mouse.set(vec2(sinf(time_value/10.0f),cosf(time_value/12.0f)));
            u_mouse.update();
            u_sampleData.set(&sample_data[0]);
            u_sampleData.update();
            u_vuData.set(&vu_data[0]);
            u_vuData.update();
//            u_sampleData.

            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,testMesh.getVBO()->gl_elements);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
            checkError(414);
        }
        
        
        
        
        void init() {
            cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
            cout << "Shader language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

            
//            testShader.loadTextFile("shaders/vertex_common.vs", "shaders/greyscale_cube_matrix.fs");
//            testShader.loadTextFile("shaders/vertex_common.vs", "shaders/raymarch_tunnel.fs");
            testShader.loadTextFile("shaders/vertex_common.vs", "shaders/rgb_flares_in_tunnel.fs");
//            testShader.loadTextFile("shaders/vertex_common.vs", "shaders/raymarch_clod.fs");
//            testShader.loadTextFile("shaders/vertex_common.vs", "shaders/rm_box_floor.fs");
//            testShader.loadTextFile("shaders/vertex_common.vs", "shaders/binary_tunnel.fs");
//            testShader.loadTextFile("shaders/vertex_common.vs", "shaders/rm_cube_matrix.fs");
//            testShader.loadTextFile("shaders/vertex_common.vs", "shaders/red_blue_crawl_pattern.fs");
            //       testShader.loadTextFile("CubicVR_TestShader.vert", "CubicVR_TestShader.frag");
            testShader.compile();
            if (!testShader.isCompiled()) {
                testShader.dumpShaderLog();
            } else {
                cout << "Shader compile success." << endl;
                shaderVariables vars;
                testShader.getVariables(vars);
                vars.dump();
            }
            
            
            testShader.getVariables(shaderVars);
            
            a_vertexPosition = shaderVars.getAttribute("vertexPosition");
            u_resolution = shaderVars.getUniform("resolution");
            u_time = shaderVars.getUniform("time");
            u_mouse = shaderVars.getUniform("mouse");
            
            u_sampleData = shaderVars.getUniform("sampleData[0]");
            u_vuData = shaderVars.getUniform("vuData[0]");
            u_baseColor = shaderVars.getUniform("baseColor");
            
//            testCam.setDimensions(640,480);
//            testCam.setFOV(60);
//            testCam.setTarget(vec3(0,0,0));
////            testCam.setPosition(vec3(0.75,0.75,0.75));
//            testCam.calcProjection();
            
            
            testMesh.addPoint(vec3(-1.0f, -1.0f, -1.0f));
            testMesh.addPoint(vec3(1.0f, -1.0f, -1.0f));
            testMesh.addPoint(vec3(1.0f, 1.0f, -1.0f));
            testMesh.addPoint(vec3(-1.0f, 1.0f, -1.0f));

            testMesh.newFace().setPoints(triangleFaceRef(0,1,2));
            testMesh.newFace().setPoints(triangleFaceRef(2,3,0));
            
            testMesh.prepare();
            


//            glGenVertexArrays(1, &vao);

        }
    };
}

#endif /* defined(__CubicVR2__ShaderRenderTest__) */
