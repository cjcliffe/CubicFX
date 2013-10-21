//
//  main.cpp
//  CubicVR2Test
//
//  Created by Charles J. Cliffe on 2013-02-23.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#include <iostream>

#include "ShaderViz.h"
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <cubicvr2/core/Timer.h>

#include "FFT.h"
#include "BeatDetektor.h"

#define SRATE 44100
#define BUF 1024

using namespace std;

ALCdevice *audio_device;
ALbyte audio_buffer[SRATE];
ALint samples;

vector<float> sample_data;
vector<float> fft_data;
vector<float> fft_collapse;

using namespace CubicVR;

Timer visTimer;
bool bpm_latch = false;

BeatDetektor *det;
BeatDetektorVU *vu;
BeatDetektorContest *contest;

int initAudio() {
 	alGetError();
	const ALchar *pDeviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	if (pDeviceList) {
		printf("Available Capture Devices are:\n");
		while (*pDeviceList) {
			printf("**%s**\n", pDeviceList);
			pDeviceList += strlen(pDeviceList) + 1;
		}
	}
	
	audio_device = alcCaptureOpenDevice(NULL, SRATE*2, AL_FORMAT_STEREO16, BUF);
	if (alGetError() != AL_NO_ERROR) {
		return 0;
	}
	alcCaptureStart(audio_device);

	sample_data.resize(BUF);
	fft_data.resize(BUF);
    
    return 1;
}

static void captureAudio(void) {
	int x;
	
	int lval=32768;
	float bval = 0.0;
    
	alcGetIntegerv(audio_device, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &samples);
	alcCaptureSamples(audio_device, (ALCvoid *)audio_buffer, samples);
	
	for (x=0; x<BUF; x++)
	{
		bval = (((float)((ALint *)audio_buffer)[x])/32767.0) / (float)lval;
		sample_data[x]=bval;
	}
	
	fft_data = sample_data;
	
	DanielsonLanczos <(BUF/2), float> dlfft;
	
	unsigned long n, m, j, i;
	
	// reverse-binary reindexing
    n = (BUF/2)<<1;
    j=1;
    for (i=1; i<n; i+=2) {
        if (j>i) {
            swap(fft_data[j-1], fft_data[i-1]);
            swap(fft_data[j], fft_data[i]);
        }
        m = (BUF/2);
        while (m>=2 && j>m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    };
    
	
	dlfft.apply(&fft_data[0]);
    
    //	fvals = dft(cvals);
	
}

void initBD () {

    det = new BeatDetektor(85,150);
//		det->detection_rate = 20.0;
//		det->quality_reward = 20.0;
	
	vu = new BeatDetektorVU();
	
	contest = new BeatDetektorContest();
	contest->no_contest_decay = false;
	contest->finish_line = 1000;

}

void processBD() {
    float timer_seconds = visTimer.getSeconds();
    int x;
    
    fft_collapse.clear();
	for (x = 0; x < 256; x++)
	{
		fft_collapse.push_back(fft_data[x]);
	}
	for (x = BUF-256; x < BUF; x++)
	{
		fft_collapse.push_back(fft_data[x]);
	}
    
    det->process(timer_seconds,fft_collapse);
    contest->process(timer_seconds,det);
    
	det->process(timer_seconds,fft_collapse);

	if (contest->win_bpm_int)
        vu->process(det,visTimer.lastUpdateSeconds(),((float)contest->win_bpm_int/10.0));


	contest->run();
    
}

Timer fpsTimer;

int main(int argc, char * argv[])
{

    if (!initAudio()) {
        return -1;
    }
    
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    GLFWwindow *window = glfwCreateWindow(1280, 720, "CubicVR-2 Test", NULL, NULL);
    
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    initBD();
    
    visTimer.start();
    fpsTimer.start();
    
    vector<ShaderViz *> visualizers;
    
    ShaderViz vizRgbFlares("shaders/vertex_common.vs", "shaders/rgb_flares_in_tunnel.fs");
    ShaderViz rbCrawl("shaders/vertex_common.vs", "shaders/red_blue_crawl_pattern.fs");
    ShaderViz rmTunnel("shaders/vertex_common.vs", "shaders/raymarch_tunnel.fs");
    ShaderViz discoFloor("shaders/vertex_common.vs", "shaders/discofloor_ceiling.fs");
    ShaderViz sparklingSine("shaders/vertex_common.vs", "shaders/sparkling_sine_wave.fs");
    ShaderViz flareTunnel("shaders/vertex_common.vs", "shaders/flare_tunnel.fs");
    
//    ShaderViz torusSwirl("shaders/vertex_common.vs", "shaders/torus_tunnel_swirl.fs");
//    ShaderViz rmBoxFloor("shaders/vertex_common.vs", "shaders/rm_box_floor.fs");
//    ShaderViz cubeMatrix("shaders/vertex_common.vs", "shaders/rm_cube_matrix.fs");
//    ShaderViz rmCorridorBalls("shaders/vertex_common.vs", "shaders/rt_balls_corridor.fs");

    
    //            vizShader.loadTextFile("shaders/vertex_common.vs", "shaders/greyscale_cube_matrix.fs");
    //            vizShader.loadTextFile("shaders/vertex_common.vs", "shaders/raymarch_clod.fs");
    //            vizShader.loadTextFile("shaders/vertex_common.vs", "shaders/binary_tunnel.fs");
    
    visualizers.push_back(&vizRgbFlares);
    visualizers.push_back(&rbCrawl);
    visualizers.push_back(&rmTunnel);
    visualizers.push_back(&discoFloor);
    visualizers.push_back(&sparklingSine);
    visualizers.push_back(&flareTunnel);

//    visualizers.push_back(&torusSwirl);
//    visualizers.push_back(&rmCorridorBalls);
//    visualizers.push_back(&cubeMatrix);
//    visualizers.push_back(&rmBoxFloor);
    
    ShaderViz *currentViz = &sparklingSine;
    
    float frameSlice = 0.0f;
    
    while (!glfwWindowShouldClose(window))
    {
        fpsTimer.update();
        
        frameSlice += fpsTimer.lastUpdateSeconds();
        
        if (frameSlice > 0.5f) {
            frameSlice = 0.0;
        }
        
        if (frameSlice >= 1.0f/60.0f) {
            visTimer.update();
            captureAudio();
            processBD();
            
            currentViz->updateVariables(visTimer.getSeconds(),sample_data,vu->vu_levels,contest);
            currentViz->display();
            
            glfwSwapBuffers(window);
            frameSlice = 0.0f;
        }
        
        glfwPollEvents();
    }
    
    std::cout << endl << "-----" << endl << "Done." << endl;
    
    glfwTerminate();

    return 0;
}

