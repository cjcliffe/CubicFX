//
//  ShaderFX.h
//
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__ShaderRenderTest__
#define __CubicVR2__ShaderRenderTest__

#include <iostream>
#include "cubicvr2/opengl/Shader.h"
#include "cubicvr2/opengl/Material.h"
#include "cubicvr2/core/Mesh.h"
#include "cubicvr2/core/Camera.h"
#include "cubicvr2/opengl/Texture.h";

#include <math.h>
#include "BeatDetektor.h"

#pragma once

extern int VIZ_WIDTH;
extern int VIZ_HEIGHT;

namespace CubicVR {
    
    
    class ShaderViz {
    public:
        Shader vizShader;
        
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
        shaderUniformVec3 u_randColor2;
        shaderUniformInt u_beatCounter;
        shaderUniformInt u_beatCounterHalf;
        shaderUniformInt u_beatCounterQuarter;
        
        shaderUniformFloat u_timerKick;
        shaderUniformFloat u_timerMid;
        shaderUniformFloat u_timerHigh;
        shaderUniformFloat u_sampleRange;
		shaderUniformFloat u_blendAlpha;

		shaderUniformSampler2D u_samplerTex;
		shaderUniformSampler2D u_overlayImage;

		shaderUniformSampler2D u_tex0, u_tex1, u_tex2;
        
		GLuint samplerTex;
		GLuint samplerTexSampler;

		Texture *overlayTexture;
		Texture *tex0Texture;
		Texture *tex1Texture;
		Texture *tex2Texture;

        vec3 randColor, randColor2;
        int lastBeat;
        float lastTime;
		        
        float timerKick, timerMid, timerHigh;
        
        Mesh fsQuadMesh;
        
        ShaderViz(string vsFn, string fsFn);
        
		void setOverlayTexture(Texture *oTex);
		void setTex0Texture(Texture *tex);
		void setTex1Texture(Texture *tex);
		void setTex2Texture(Texture *tex);

        float floatArrayAverage(float *data, int start, int end);
        float floatArrayAbsAverage(float *data, int start, int end);
		float floatArrayMax(float *data, int start, int end);
		
        
        void updateVariables(float time_value, vector<float> &sample_data, vector<float> &vu_data, BeatDetektorContest *contest);
		void setBlendAlpha(float val);
        
        void display();

        
        void init(string vsFn, string fsFn);
    };
}

#endif /* defined(__CubicVR2__ShaderRenderTest__) */
