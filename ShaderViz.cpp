//
//  ShaderViz.cpp
//
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#include "ShaderViz.h"

using namespace CubicVR;

int VIZ_WIDTH = 1280;
int VIZ_HEIGHT = 768;


ShaderViz::ShaderViz(string vsFn, string fsFn) {
    init(vsFn,fsFn);
    timerKick = 0.0f;
	timerMid = 0.0f;
	timerHigh = 0.0f;
    lastTime = 0.0f;
	overlayTexture = NULL;
}

void ShaderViz::setOverlayTexture(Texture *oTex) {
	overlayTexture = oTex;
}

float ShaderViz::floatArrayAverage(float *data, int start, int end) {
    float accum = 0;
    for (int i = start; i < end; i++) {
        accum += data[i];
    }
    accum /= (float)end-start;
    
    return accum;
}

float ShaderViz::floatArrayAbsAverage(float *data, int start, int end) {
    float accum = 0;
    for (int i = start; i < end; i++) {
        accum += fabs(data[i]);
    }
    accum /= (float)end-start;
    
    return accum;
}


float ShaderViz::floatArrayMax(float *data, int start, int end) {
	float maxv = 0;
	for (int i = start; i < end; i++) {
		if (data[i] > maxv) maxv = data[i];
	}
	
	return maxv;
}

void ShaderViz::setBlendAlpha(float val) {
	u_blendAlpha.set(val);
}


void ShaderViz::updateVariables(float time_value, vector<float> &sample_data, vector<float> &vu_data, BeatDetektorContest *contest) {
	
	vizShader.use();

    a_vertexPosition.set(fsQuadMesh.getVBO()->gl_points);
    a_vertexPosition.update();
    
    if (lastTime == 0) {
        lastTime = time_value;
        
    }

    float last_update = time_value-lastTime;
    lastTime = time_value;
    
    if (last_update == 0.0f) {
        last_update = 1.0f/60.0f;
    }
    
    if (u_time.size) {
        u_time.set(time_value);
        u_time.update();
    }
    
    if (u_resolution.size) {
        u_resolution.set(vec2(VIZ_WIDTH,VIZ_HEIGHT));
        u_resolution.update();
    }
    
    if (u_mouse.size) {
        u_mouse.set(vec2(sinf(time_value/10.0f),cosf(time_value/12.0f)));
        u_mouse.update();
    }
    
    if (u_sampleData.size) {
        u_sampleData.set(&sample_data[0]);
        u_sampleData.update();
    }
    
    if (u_sampleRange.size) {
        u_sampleRange.set(floatArrayAbsAverage(&sample_data[0], 0, sample_data.size()));
        u_sampleRange.update();
    }
    
    if (u_vuData.size) {
        u_vuData.set(&vu_data[0]);
        u_vuData.update();
    }
    
    if (u_vuLow.size) {
		u_vuLow.set(floatArrayAverage(&vu_data[0], 0, 6));
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
        
        randColor2.x = 0.5 + ((float)rand()/(float)RAND_MAX) / 2.0;
        randColor2.y = 0.5 + ((float)rand()/(float)RAND_MAX) / 2.0;
        randColor2.z = 0.5 + ((float)rand()/(float)RAND_MAX) / 2.0;
        
        lastBeat = contest->beat_counter;
        //            cout << randColor << endl;
    }
    
    if (u_randColor.size) {
        u_randColor.set(randColor);
        u_randColor.update();
    }
    
    if (u_randColor2.size) {
        u_randColor2.set(randColor2);
        u_randColor2.update();
    }
    
    if (u_beatCounter.size) {
        u_beatCounter.set(contest->beat_counter);
        u_beatCounter.update();
    }

    if (u_beatCounterHalf.size) {
        u_beatCounterHalf.set(contest->half_counter);
        u_beatCounterHalf.update();
    }

    if (u_beatCounterQuarter.size) {
        u_beatCounterQuarter.set(contest->quarter_counter);
        u_beatCounterQuarter.update();
    }

    if (u_timerKick.size) {
		timerKick += floatArrayAverage(&vu_data[0], 0, 6)*last_update*5.0;
        u_timerKick.set(timerKick);
        u_timerKick.update();
    }
    
    if (u_timerMid.size) {
        timerMid += floatArrayAverage(&vu_data[0], 50, 60)*last_update*5.0;
        u_timerMid.set(timerMid);
        u_timerMid.update();
    }

    if (u_timerHigh.size) {
        timerHigh += floatArrayAverage(&vu_data[0], 100, 128)*last_update*5.0;
        u_timerHigh.set(timerHigh);
        u_timerHigh.update();
    }

	if (u_samplerTex.size) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_1D, samplerTex);
		glBindSampler(0, samplerTexSampler);
		glPixelStorei(GL_UNPACK_ALIGNMENT,2);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, 1024, 0, GL_RED, GL_FLOAT, &sample_data[0]);
		u_samplerTex.set(0);
		u_samplerTex.update();

//		cout << samplerTexVal[1023] << endl;
	}

	if (u_blendAlpha.size) {
		u_blendAlpha.update();
	}

	if (u_overlayImage.size && overlayTexture != NULL) {
		u_overlayImage.set(0);
		u_overlayImage.update();
		overlayTexture->bind(0);
	}
}


void ShaderViz::display()
{
    vizShader.use();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,fsQuadMesh.getVBO()->gl_elements);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}


void ShaderViz::init(string vsFn, string fsFn) {
    cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << "Shader language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    
    vizShader.loadTextFile(vsFn, fsFn);
    vizShader.compile();

    if (!vizShader.isCompiled()) {
        vizShader.dumpShaderLog();
    } else {
        cout << "Shader compile success." << endl;
        shaderVariables vars;
        vizShader.getVariables(vars);
        vars.dump();
    }
    
    lastBeat = 0;
    
    vizShader.getVariables(shaderVars);
    
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
    u_randColor2 = shaderVars.getUniform("randColor2");
    
    u_beatCounter = shaderVars.getUniform("beatCounter");
    u_beatCounterHalf = shaderVars.getUniform("beatCounterHalf");
    u_beatCounterQuarter = shaderVars.getUniform("beatCounterQuarter");
    
    u_timerKick = shaderVars.getUniform("timerKick");
    u_timerMid = shaderVars.getUniform("timerMid");
    u_timerHigh = shaderVars.getUniform("timerHigh");
    u_sampleRange = shaderVars.getUniform("sampleRange");
    
	u_blendAlpha = shaderVars.getUniform("blendAlpha");

	u_overlayImage = shaderVars.getUniform("overlayImage");

	glGenTextures(1, &samplerTex);
	glBindTexture(GL_TEXTURE_1D, samplerTex);

	glGenSamplers(1, &samplerTexSampler);
	glSamplerParameteri(samplerTexSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(samplerTexSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	u_samplerTex = shaderVars.getUniform("samplerTex");



    fsQuadMesh.addPoint(vec3(-1.0f, -1.0f, -1.0f));
    fsQuadMesh.addPoint(vec3(1.0f, -1.0f, -1.0f));
    fsQuadMesh.addPoint(vec3(1.0f, 1.0f, -1.0f));
    fsQuadMesh.addPoint(vec3(-1.0f, 1.0f, -1.0f));
    
    fsQuadMesh.newFace().setPoints(triangleFaceRef(0,1,2));
    fsQuadMesh.newFace().setPoints(triangleFaceRef(2,3,0));
    
    fsQuadMesh.prepare();
}
