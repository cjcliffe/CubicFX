//
//  main.cpp
//  CubicVR2Test
//
//  Created by Charles J. Cliffe on 2013-02-23.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#include <iostream>

#include "ShaderViz.h"
#ifdef _WIN32
	#define GLFW_INCLUDE_NONE
#else
	#define GLFW_INCLUDE_GLCOREARB
#endif
#include <GLFW/glfw3.h>

#ifdef WIN32
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include <cubicvr2/core/Timer.h>

#include "FFT.h"
#include "BeatDetektor.h"

#define SRATE 44100
#define BUF 2048

using namespace std;

ALCdevice *audio_device;
ALbyte audio_buffer[SRATE*2];
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

float alphaVec;
float alphaVal = 1.0;

int initAudio() {
	ALenum err = alGetError();
	const ALchar *pDeviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	if (pDeviceList) {
		printf("Available Capture Devices are:\n");
		while (*pDeviceList) {
			printf("**%s**\n", pDeviceList);
			pDeviceList += strlen(pDeviceList) + 1;
		}
	}

	audio_device = alcCaptureOpenDevice(NULL, SRATE, AL_FORMAT_STEREO16, BUF);
	err = alGetError();
	if (err != AL_NO_ERROR) {
		switch (err) {
		case AL_INVALID_OPERATION:
			printf("alcCaptureOpenDevice reports Invalid Operation?\n");
			break;
		case AL_INVALID_VALUE:
			printf("alcCaptureOpenDevice reports Invalid Value\n");
			return 0;
		case ALC_OUT_OF_MEMORY:
			printf("alcCaptureOpenDevice reports Out Of Memory\n");
			return 0;
		}
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
ShaderViz *currentViz;
vector<ShaderViz *> visualizers;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }
    
    
    
    if (action == GLFW_PRESS) {
        
        switch (key) {
            case GLFW_KEY_1:
                currentViz = visualizers[0];
				break;
            case GLFW_KEY_2:
                currentViz = visualizers[1];
                break;
            case GLFW_KEY_3:
                currentViz = visualizers[2];
                break;
            case GLFW_KEY_4:
                currentViz = visualizers[3];
                break;
            case GLFW_KEY_5:
                currentViz = visualizers[4];
				break;
            case GLFW_KEY_6:
                currentViz = visualizers[5];
				break;
            case GLFW_KEY_7:
                currentViz = visualizers[6];
                break;
            case GLFW_KEY_8:
                currentViz = visualizers[7];
				break;
            case GLFW_KEY_9:
                currentViz = visualizers[8];
				break;
            case GLFW_KEY_0:
                currentViz = visualizers[9];
				break;

			case GLFW_KEY_Q:
				currentViz = visualizers[10];
				break;
			case GLFW_KEY_W:
				currentViz = visualizers[11];
				break;
			case GLFW_KEY_E:
				currentViz = visualizers[12];
				break;
			case GLFW_KEY_R:
				currentViz = visualizers[13];
				break;
			case GLFW_KEY_T:
				currentViz = visualizers[14];
				break;
			case GLFW_KEY_Y:
				currentViz = visualizers[15];
				break;
			case GLFW_KEY_U:
				currentViz = visualizers[16];
				break;
			case GLFW_KEY_I:
				currentViz = visualizers[17];
				break;
			case GLFW_KEY_O:
				currentViz = visualizers[18];
				break;
			case GLFW_KEY_P:
				currentViz = visualizers[19];
				break;

			case GLFW_KEY_A:
				currentViz = visualizers[20];
				break;
			case GLFW_KEY_S:
				currentViz = visualizers[21];
				break;
			case GLFW_KEY_D:
				currentViz = visualizers[22];
				break;
			case GLFW_KEY_F:
				currentViz = visualizers[23];
				break;
			case GLFW_KEY_G:
				currentViz = visualizers[24];
				break;
			case GLFW_KEY_H:
				currentViz = visualizers[25];
				break;
			case GLFW_KEY_J:
				currentViz = visualizers[26];
				break;
			case GLFW_KEY_K:
				currentViz = visualizers[27];
				break;
			case GLFW_KEY_L:
				currentViz = visualizers[28];
				break;
			case GLFW_KEY_Z:
				currentViz = visualizers[29];
				break;


			// Alpha setting
			case GLFW_KEY_PAGE_DOWN:
				alphaVec = (mods&GLFW_MOD_SHIFT)?4.0 : 1.0;
				break;
			case GLFW_KEY_PAGE_UP:
				alphaVec = (mods&GLFW_MOD_SHIFT) ? -4.0 : -1.0;
				break;
		}
    }

	if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_PAGE_DOWN:
				if (alphaVec > 0.0) {
					alphaVec = 0.0;
				}
				break;
			case GLFW_KEY_PAGE_UP:
				if (alphaVec < 0.0) {
					alphaVec = 0.0;
				}
				break;
		}
	}
}


int main(int argc, char * argv[])
{

    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
	GLFWmonitor **monitors;
	int numMonitors;
	monitors = glfwGetMonitors(&numMonitors);

    GLFWwindow *window = glfwCreateWindow(VIZ_WIDTH, VIZ_HEIGHT, "BeatDetektor ShaderFX", NULL, NULL);
    //GLFWwindow *window = glfwCreateWindow(VIZ_WIDTH, VIZ_HEIGHT, "BeatDetektor ShaderFX", glfwGetPrimaryMonitor(), NULL);
//	GLFWwindow *window = glfwCreateWindow(VIZ_WIDTH, VIZ_HEIGHT, "BeatDetektor ShaderFX", monitors[1], NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }


    glfwMakeContextCurrent(window);
    
#ifdef _WIN32
	glewExperimental = true;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return -1;
	}
#endif


    glfwSetKeyCallback(window, key_callback);
    
    initBD();
    
    visTimer.start();
    fpsTimer.start();
    

    
    ShaderViz vizRgbFlares("shaders/vertex_common.vs", "shaders/rgb_flares_in_tunnel.fs");
	ShaderViz sparklingSine("shaders/vertex_common.vs", "shaders/sparkling_sine_wave.fs");
	ShaderViz sparklingBlocks("shaders/vertex_common.vs", "shaders/sparkling_blocks.fs");
	ShaderViz flareWave("shaders/vertex_common.vs", "shaders/flare_wave.fs");
	ShaderViz rbCrawl("shaders/vertex_common.vs", "shaders/red_blue_crawl_pattern.fs");
    ShaderViz rmTunnel("shaders/vertex_common.vs", "shaders/raymarch_tunnel.fs");
    ShaderViz discoFloor("shaders/vertex_common.vs", "shaders/discofloor_ceiling.fs");
    ShaderViz flareTunnel("shaders/vertex_common.vs", "shaders/flare_tunnel.fs");
    ShaderViz colorSpiral("shaders/vertex_common.vs","shaders/color_spiral_complex.fs");
    ShaderViz mandelBlob("shaders/vertex_common.vs","shaders/raymarch_fractal3d.fs");
    ShaderViz lineZoom("shaders/vertex_common.vs","shaders/raymarch_lines_zoom.fs");
	ShaderViz renderObjs("shaders/vertex_common.vs", "shaders/render_objects.fs");
	ShaderViz hexField("shaders/vertex_common.vs", "shaders/hex_twist_field.fs");
	ShaderViz discoBall("shaders/vertex_common.vs", "shaders/discoball.fs");
	ShaderViz cubeArray("shaders/vertex_common.vs", "shaders/cube_array.fs");
	ShaderViz planeDeformFly("shaders/vertex_common.vs", "shaders/planed_fly.fs");
	ShaderViz planeDeformReliefTunnel("shaders/vertex_common.vs", "shaders/planed_relief_tun.fs");
	ShaderViz planeDeformSquareTunnel("shaders/vertex_common.vs", "shaders/planed_square_tun.fs");
	ShaderViz hexTunnel("shaders/vertex_common.vs", "shaders/hex_tunnel.fs");
	ShaderViz cubeScape("shaders/vertex_common.vs", "shaders/rm_cubescape.fs");
	ShaderViz inversionMachine("shaders/vertex_common.vs", "shaders/inversion_machine.fs");
	ShaderViz mengerJourney("shaders/vertex_common.vs", "shaders/menger_journey.fs");
	ShaderViz aLotOfSpheres("shaders/vertex_common.vs", "shaders/a_lot_of_spheres.fs");
	ShaderViz fractalGears("shaders/vertex_common.vs", "shaders/fractal_gears.fs");
	ShaderViz metaBalls("shaders/vertex_common.vs", "shaders/metaballs.fs");
	ShaderViz particleTracing("shaders/vertex_common.vs", "shaders/particle_tracing.fs");
	ShaderViz flyingCubes("shaders/vertex_common.vs", "shaders/flying_cubes.fs");
	ShaderViz spaceShip("shaders/vertex_common.vs", "shaders/spaceship.fs");
	ShaderViz spaceRings("shaders/vertex_common.vs", "shaders/space_rings.fs");
	ShaderViz hologram("shaders/vertex_common.vs", "shaders/hologram.fs");

	if (!initAudio()) {
		return -1;
	}

//	ShaderViz roadToHell("shaders/vertex_common.vs", "shaders/road_to_hell.fs");

//    ShaderViz torusSwirl("shaders/vertex_common.vs", "shaders/torus_tunnel_swirl.fs");
//    ShaderViz rmBoxFloor("shaders/vertex_common.vs", "shaders/rm_box_floor.fs");
//    ShaderViz cubeMatrix("shaders/vertex_common.vs", "shaders/rm_cube_matrix.fs");
//    ShaderViz rmCorridorBalls("shaders/vertex_common.vs", "shaders/rt_balls_corridor.fs");

    
    //            vizShader.loadTextFile("shaders/vertex_common.vs", "shaders/greyscale_cube_matrix.fs");
    //            vizShader.loadTextFile("shaders/vertex_common.vs", "shaders/raymarch_clod.fs");
    //            vizShader.loadTextFile("shaders/vertex_common.vs", "shaders/binary_tunnel.fs");
    
    visualizers.push_back(&vizRgbFlares);
	visualizers.push_back(&sparklingSine);
	visualizers.push_back(&sparklingBlocks);
	visualizers.push_back(&flareWave);
	visualizers.push_back(&rbCrawl);
    visualizers.push_back(&rmTunnel);
    visualizers.push_back(&discoFloor);
    visualizers.push_back(&flareTunnel);
    visualizers.push_back(&colorSpiral);
    visualizers.push_back(&mandelBlob);
    visualizers.push_back(&lineZoom);
	visualizers.push_back(&renderObjs);
	visualizers.push_back(&hexField);
	visualizers.push_back(&discoBall);
	visualizers.push_back(&cubeArray);
	visualizers.push_back(&planeDeformFly);
	visualizers.push_back(&planeDeformReliefTunnel);
	visualizers.push_back(&planeDeformSquareTunnel);
	visualizers.push_back(&hexTunnel);
	visualizers.push_back(&cubeScape);
	visualizers.push_back(&aLotOfSpheres);
	visualizers.push_back(&fractalGears);
	visualizers.push_back(&inversionMachine);
	visualizers.push_back(&mengerJourney);
	visualizers.push_back(&metaBalls);
	visualizers.push_back(&particleTracing);
	visualizers.push_back(&flyingCubes);
	visualizers.push_back(&spaceShip);
	visualizers.push_back(&spaceRings);
	visualizers.push_back(&hologram);

	
//    visualizers.push_back(&torusSwirl);
//    visualizers.push_back(&rmCorridorBalls);
//    visualizers.push_back(&cubeMatrix);
//    visualizers.push_back(&rmBoxFloor);
    
	currentViz = &hologram;
    
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

			if (alphaVec) {
				alphaVal += alphaVec*visTimer.lastUpdateSeconds()*0.1;
			}
            
			currentViz->setBlendAlpha(alphaVal);
            currentViz->updateVariables(visTimer.getSeconds(),sample_data,vu->vu_levels,contest);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

