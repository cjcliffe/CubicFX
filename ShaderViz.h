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
#include "BeatDetektor.h"

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
        
        shaderUniformFloat u_vuLow;
        shaderUniformFloat u_vuMid;
        shaderUniformFloat u_vuHigh;
        
        shaderUniformVec3 u_randColor;
        
        vec3 randColor;
        int lastBeat;
        
        Mesh testMesh;
        
//        static GLuint vao;
        
        float floatArrayAverage(float *data, int start, int end) {
            float accum = 0;
            for (int i = start; i < end; i++) {
                accum += data[i];
            }
            accum /= (float)end-start;
            
            return accum;
        }
        
        
        void display(float time_value, vector<float> &sample_data, vector<float> &vu_data, BeatDetektorContest *contest)
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
            
            if (u_vuLow.size) {
                u_vuLow.set(floatArrayAverage(&vu_data[0], 0, 5));
                u_vuLow.update();
            }
            
            if (u_vuMid.size) {
                u_vuMid.set(floatArrayAverage(&vu_data[0], 50, 60));
                u_vuMid.update();
            }

            if (u_vuHigh.size) {
                u_vuHigh.set(floatArrayAverage(&vu_data[0], 100, 128));
                u_vuHigh.update();
            }

            if (lastBeat != contest->beat_counter) {
                randColor.x = 0.5 + ((float)rand()/(float)RAND_MAX) / 2.0;
                randColor.y = 0.5 + ((float)rand()/(float)RAND_MAX) / 2.0;
                randColor.z = 0.5 + ((float)rand()/(float)RAND_MAX) / 2.0;
                lastBeat = contest->beat_counter;
                cout << randColor << endl;
            }
            
            u_randColor.set(randColor);
            u_randColor.update();

            
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
            
            lastBeat = 0;
            
            
            testShader.getVariables(shaderVars);
            
            a_vertexPosition = shaderVars.getAttribute("vertexPosition");
            u_resolution = shaderVars.getUniform("resolution");
            u_time = shaderVars.getUniform("time");
            u_mouse = shaderVars.getUniform("mouse");
            
            u_sampleData = shaderVars.getUniform("sampleData[0]");
            u_vuData = shaderVars.getUniform("vuData[0]");
            u_baseColor = shaderVars.getUniform("baseColor");
            
            u_vuLow = shaderVars.getUniform("vuLow");
            u_vuMid = shaderVars.getUniform("vuMid");
            u_vuHigh = shaderVars.getUniform("vuHigh");
            
            u_randColor = shaderVars.getUniform("randColor");
            
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
