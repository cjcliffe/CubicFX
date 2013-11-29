//
//  main.cpp
//  CubicVR2Test
//
//  Created by Charles J. Cliffe on 2013-02-23.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#include <iostream>
#include <time.h>

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
#include <cubicvr2/opengl/Texture.h>

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

BeatDetektor *det_low;
BeatDetektor *det_high;
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

    det_low = new BeatDetektor(90,160);
	det_high = new BeatDetektor(140, 260);

	det = det_low;
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
    
    det_low->process(timer_seconds,fft_collapse);
	det_high->process(timer_seconds, fft_collapse);
	contest->process(timer_seconds, det);
    
	//det->process(timer_seconds,fft_collapse);

	if (contest->win_bpm_int)
        vu->process(det,visTimer.lastUpdateSeconds(),((float)contest->win_bpm_int/10.0));


	contest->run();
    
}


Timer fpsTimer;
ShaderViz *currentViz=NULL;
vector<ShaderViz *> visualizers;
vector<Texture *> overlays;


static void loadOverlays() {
	Texture *tex;

	tex = new Texture();
	tex->loadPNG("png/OFFSET.png");
	overlays.push_back(tex);

	tex = new Texture();
	tex->loadPNG("png/JACK.png");
	overlays.push_back(tex);

	tex = new Texture();
	tex->loadPNG("png/synthamesk.png");
	overlays.push_back(tex);

	tex = new Texture();
	tex->loadPNG("png/C64.png");
	overlays.push_back(tex);

	tex = new Texture();
	tex->loadPNG("png/LAFO.png");
	overlays.push_back(tex);

	tex = new Texture();
	tex->loadPNG("png/LAFOLOGO.png");
	overlays.push_back(tex);

}

ShaderViz *overlayImage;
ShaderViz *targetViz = NULL;

float transition_timer = 0.0;
int transition = 0;
bool in_transition = false;

enum transitionType {
	TRANS_CROSSFADE,
	TRANS_WIPERIGHT,
	TRANS_WIPELEFT,
	TRANS_WIPEDOWN,
	TRANS_WIPEUP,
	TRANS_ZOOMIN,
	TRANS_MAX
};
transitionType trans_type;


int lastImage = -1;
int thisImage = 0;
float overlayAlpha = 0.0f;
float overlayDelta = 0.0f;
bool overlayEnabled = false;
float autoTimer = 0;
float autoTimerLimit = 30;
bool manualOverride = false;





void vizRandom() {
	int nextViz = (int)floor(((float)rand() / (float)RAND_MAX)*visualizers.size());
	if (nextViz > visualizers.size() - 1) nextViz = visualizers.size() - 1;
	targetViz = visualizers[nextViz];
	if (currentViz == NULL) {
		currentViz = targetViz;
	}
}

void transRandom() {
	trans_type = (transitionType)(int)floor(((float)rand() / (float)RAND_MAX)*TRANS_MAX);
}




static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }
    
    
    
    if (action == GLFW_PRESS) {
		bool doChange = false;
		if (mods&GLFW_MOD_SHIFT) {
			thisImage = -1;
			switch (key) {
			case GLFW_KEY_1:
				overlayImage->setOverlayTexture(overlays[0]);
				thisImage = 0;
				doChange = true;
				break;
			case GLFW_KEY_2:
				overlayImage->setOverlayTexture(overlays[1]);
				thisImage = 1;
				doChange = true;
				break;
			case GLFW_KEY_3:
				overlayImage->setOverlayTexture(overlays[2]);
				thisImage = 2;
				doChange = true;
				break;
			case GLFW_KEY_4:
				overlayImage->setOverlayTexture(overlays[3]);
				thisImage = 3;
				doChange = true;
				break;
			case GLFW_KEY_5:
				overlayImage->setOverlayTexture(overlays[4]);
				thisImage = 4;
				doChange = true;
				break;
			case GLFW_KEY_6:
				overlayImage->setOverlayTexture(overlays[5]);
				thisImage = 5;
				doChange = true;
				break;
			case GLFW_KEY_0:
				overlayEnabled = !overlayEnabled;
				overlayDelta = 0;
				break;
			}

			if (doChange && thisImage == lastImage) {
				overlayDelta = overlayDelta ? -overlayDelta : -1.0;
			}

			if (doChange && !overlayEnabled) {
				overlayEnabled = true;
				overlayDelta = -1.0;
			}

			if (thisImage != -1) lastImage = thisImage;

		} else {
			ShaderViz *tempViz = targetViz;
			switch (key) {
			case GLFW_KEY_1:
				targetViz = visualizers[0];
				break;
			case GLFW_KEY_2:
				targetViz = visualizers[1];
				break;
			case GLFW_KEY_3:
				targetViz = visualizers[2];
				break;
			case GLFW_KEY_4:
				targetViz = visualizers[3];
				break;
			case GLFW_KEY_5:
				targetViz = visualizers[4];
				break;
			case GLFW_KEY_6:
				targetViz = visualizers[5];
				break;
			case GLFW_KEY_7:
				targetViz = visualizers[6];
				break;
			case GLFW_KEY_8:
				targetViz = visualizers[7];
				break;
			case GLFW_KEY_9:
				targetViz = visualizers[8];
				break;
			case GLFW_KEY_0:
				targetViz = visualizers[9];
				break;

			case GLFW_KEY_Q:
				targetViz = visualizers[10];
				break;
			case GLFW_KEY_W:
				targetViz = visualizers[11];
				break;
			case GLFW_KEY_E:
				targetViz = visualizers[12];
				break;
			case GLFW_KEY_R:
				targetViz = visualizers[13];
				break;
			case GLFW_KEY_T:
				targetViz = visualizers[14];
				break;
			case GLFW_KEY_Y:
				targetViz = visualizers[15];
				break;
			case GLFW_KEY_U:
				targetViz = visualizers[16];
				break;
			case GLFW_KEY_I:
				targetViz = visualizers[17];
				break;
			case GLFW_KEY_O:
				targetViz = visualizers[18];
				break;
			case GLFW_KEY_P:
				targetViz = visualizers[19];
				break;

			case GLFW_KEY_A:
				targetViz = visualizers[20];
				break;
			case GLFW_KEY_S:
				targetViz = visualizers[21];
				break;
			case GLFW_KEY_D:
				targetViz = visualizers[22];
				break;
			case GLFW_KEY_F:
				targetViz = visualizers[23];
				break;
			case GLFW_KEY_G:
				targetViz = visualizers[24];
				break;
			case GLFW_KEY_H:
				targetViz = visualizers[25];
				break;
			case GLFW_KEY_J:
				targetViz = visualizers[26];
				break;
			case GLFW_KEY_K:
				targetViz = visualizers[27];
				break;
			case GLFW_KEY_L:
				targetViz = visualizers[28];
				break;
			case GLFW_KEY_Z:
				targetViz = visualizers[29];
				break;
			case GLFW_KEY_X:
				targetViz = visualizers[30];
				break;
			case GLFW_KEY_C:
				targetViz = visualizers[31];
				break;
			case GLFW_KEY_V:
				targetViz = visualizers[32];
				break;
			case GLFW_KEY_B:
				targetViz = visualizers[33];
				break;
			case GLFW_KEY_N:
				targetViz = visualizers[34];
				break;
			case GLFW_KEY_M:
				targetViz = visualizers[35];
				break;
			case GLFW_KEY_SPACE:
				vizRandom();
				transRandom();
				manualOverride = false;
				autoTimer = 0;
				break;
			case GLFW_KEY_MINUS:
				cout << "Low mode" << endl;
				det = det_low;
				break;
			case GLFW_KEY_EQUAL:
				cout << "High mode" << endl;
				det = det_high;
				break;

			}

			if (key != GLFW_KEY_SPACE && targetViz != tempViz) {
				manualOverride = true;
				transRandom();
			}
		}

		switch (key) {
			// Alpha setting
			case GLFW_KEY_PAGE_DOWN:
			case GLFW_KEY_COMMA:
				alphaVec = (mods&GLFW_MOD_SHIFT)?4.0 : 1.0;
				break;
			case GLFW_KEY_PAGE_UP:
			case GLFW_KEY_PERIOD:
				alphaVec = (mods&GLFW_MOD_SHIFT) ? -4.0 : -1.0;
				break;
		}
    }

	if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_PAGE_DOWN:
			case GLFW_KEY_COMMA:
				if (alphaVec > 0.0) {
					alphaVec = 0.0;
				}
				break;
			case GLFW_KEY_PAGE_UP:
			case GLFW_KEY_PERIOD:
				if (alphaVec < 0.0) {
					alphaVec = 0.0;
				}
				break;
		}
	}
}



int main(int argc, char * argv[])
{

	int doFullscreen;
	int ovrWidth = 0;
	int ovrHeight = 0;
	int monitorNum = 0;


	if (argc > 1) {
		if (argc >= 2) {
			sscanf(argv[1], "%d", &monitorNum);
		}
		if (argc >= 4) {
			sscanf(argv[2], "%d", &ovrWidth);
			sscanf(argv[3], "%d", &ovrHeight);
		}
		if (argc >= 5) {
			sscanf(argv[4], "%f", &autoTimerLimit);
		}
	}

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
	GLFWwindow *window;

	if (ovrWidth && ovrHeight) {
		VIZ_WIDTH = ovrWidth;
		VIZ_HEIGHT = ovrHeight;
	}

	if (monitorNum > 0 && monitorNum <= numMonitors) {
		window = glfwCreateWindow(VIZ_WIDTH, VIZ_HEIGHT, "BeatDetektor ShaderFX", monitors[monitorNum-1], NULL);
	}
	else {
		window = glfwCreateWindow(VIZ_WIDTH, VIZ_HEIGHT, "BeatDetektor ShaderFX", NULL, NULL);
	}
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
//    ShaderViz colorSpiral("shaders/vertex_common.vs","shaders/color_spiral_complex.fs");
    ShaderViz mandelBlob("shaders/vertex_common.vs","shaders/raymarch_fractal3d.fs");
    ShaderViz lineZoom("shaders/vertex_common.vs","shaders/raymarch_lines_zoom.fs");
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
	ShaderViz musicBall("shaders/vertex_common.vs", "shaders/music_ball.fs");
	ShaderViz ledSpectrum("shaders/vertex_common.vs", "shaders/led_spectrum.fs");
	ShaderViz kifs2D("shaders/vertex_common.vs", "shaders/2dkifs.fs");
	ShaderViz planeDTunEffect("shaders/vertex_common.vs", "shaders/planed_tunnel_effect.fs");
	ShaderViz noxiousBox("shaders/vertex_common.vs", "shaders/noxious_box.fs");
	ShaderViz waveFloor("shaders/vertex_common.vs", "shaders/wave_floor.fs");
	ShaderViz hexField2("shaders/vertex_common.vs", "shaders/hex_field.fs");
	ShaderViz spaceRace("shaders/vertex_common.vs", "shaders/space_race.fs");

	overlayImage = new ShaderViz("shaders/vertex_common.vs", "shaders/overlay_image.fs");

	loadOverlays();

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
//    visualizers.push_back(&colorSpiral);
    visualizers.push_back(&mandelBlob);
    visualizers.push_back(&lineZoom);
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
	visualizers.push_back(&musicBall);
	visualizers.push_back(&ledSpectrum);
	visualizers.push_back(&kifs2D);
	visualizers.push_back(&planeDTunEffect);
	visualizers.push_back(&noxiousBox);
	visualizers.push_back(&waveFloor);
	visualizers.push_back(&hexField2);
	visualizers.push_back(&spaceRace);

	
//    visualizers.push_back(&torusSwirl);
//    visualizers.push_back(&rmCorridorBalls);
//    visualizers.push_back(&cubeMatrix);
//    visualizers.push_back(&rmBoxFloor);
    
	//currentViz = &spaceRace;
	srand(time(NULL));
	vizRandom();
	transRandom();
    
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
				if (alphaVal > 1.0) {
					alphaVal = 1.0;
				}
				if (alphaVal < 0.05) {
					alphaVal = 0.05;
				}
				cout << alphaVal << endl;
			}


//			alphaVal = 1.0 - vu->vu_levels[0];


			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


			if (currentViz != targetViz) {
				in_transition = true;
				transition_timer += visTimer.lastUpdateSeconds();
				if (transition_timer > 1.0) {
					in_transition = false;
					transition_timer = 0;
					currentViz = targetViz;
				}
			}


			glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
			glClear(GL_DEPTH_BUFFER_BIT);

			switch (trans_type) {
			case TRANS_WIPERIGHT:
				glViewport((transition_timer*VIZ_WIDTH), 0, VIZ_WIDTH, VIZ_HEIGHT);
				break;
			case TRANS_WIPELEFT:
				glViewport(-(transition_timer*VIZ_WIDTH), 0, VIZ_WIDTH, VIZ_HEIGHT);
				break;
			case TRANS_WIPEUP:
				glViewport(0, -(transition_timer*VIZ_HEIGHT), VIZ_WIDTH, VIZ_HEIGHT);
				break;
			case TRANS_WIPEDOWN:
				glViewport(0, (transition_timer*VIZ_HEIGHT), VIZ_WIDTH, VIZ_HEIGHT);
				break;
			default:
				glViewport(0, 0, VIZ_WIDTH, VIZ_HEIGHT);
			}
			

			if (in_transition) {
				currentViz->updateVariables(visTimer.getSeconds(), sample_data, vu->vu_levels, contest);

//				if (trans_type == TRANS_CROSSFADE) {
				currentViz->setBlendAlpha((1.0 - transition_timer)*alphaVal);
//				}
//				else {
//					currentViz->setBlendAlpha(alphaVal);
//				}

				currentViz->display();

				glClear(GL_DEPTH_BUFFER_BIT);

				switch (trans_type) {
				case TRANS_WIPERIGHT:
					glViewport(-VIZ_WIDTH + (transition_timer*VIZ_WIDTH), 0, VIZ_WIDTH, VIZ_HEIGHT);
					break;
				case TRANS_WIPELEFT:
					glViewport(VIZ_WIDTH - (transition_timer*VIZ_WIDTH), 0, VIZ_WIDTH, VIZ_HEIGHT);
					break;
				case TRANS_WIPEUP:
					glViewport(0, VIZ_HEIGHT - (transition_timer*VIZ_HEIGHT), VIZ_WIDTH, VIZ_HEIGHT);
					break;
				case TRANS_WIPEDOWN:
					glViewport(0, -VIZ_HEIGHT + (transition_timer*VIZ_HEIGHT), VIZ_WIDTH, VIZ_HEIGHT);
					break;
				case TRANS_ZOOMIN:
					glViewport(VIZ_WIDTH / 2 - (transition_timer*VIZ_WIDTH) / 2, VIZ_HEIGHT / 2 - (transition_timer*VIZ_HEIGHT) / 2, transition_timer*VIZ_WIDTH, transition_timer*VIZ_HEIGHT);
					break;
				default:
					glViewport(0, 0, VIZ_WIDTH, VIZ_HEIGHT);
				}

				targetViz->updateVariables(visTimer.getSeconds(), sample_data, vu->vu_levels, contest);

//				if (trans_type == TRANS_CROSSFADE) {
				targetViz->setBlendAlpha(alphaVal*transition_timer);
//				}
//				else {
//					targetViz->setBlendAlpha(alphaVal);
//				}

				targetViz->display();
			}
			else {
				currentViz->updateVariables(visTimer.getSeconds(), sample_data, vu->vu_levels, contest);
				currentViz->setBlendAlpha(alphaVal);
				currentViz->display();
			}
            




			if (overlayDelta) {
				if (overlayEnabled) {
					overlayAlpha += overlayDelta * visTimer.lastUpdateSeconds();
				}
				
				if (overlayAlpha < -1.0) {
					overlayAlpha = -1.0;
				}
				else if (overlayAlpha > 1.0) {
					overlayAlpha = 1.0;
				}
				
			}
			else if (overlayAlpha) {
				overlayAlpha -= overlayAlpha * visTimer.lastUpdateSeconds();
				if (fabs(overlayAlpha) < 0.01) {
					overlayAlpha = 0.0;
				}
			}

			if (overlayEnabled || overlayAlpha != 0.0) {
				overlayImage->setBlendAlpha(overlayAlpha);
				overlayImage->updateVariables(visTimer.getSeconds(), sample_data, vu->vu_levels, contest);

				glViewport(0, 0, VIZ_WIDTH, VIZ_HEIGHT);
				glClear(GL_DEPTH_BUFFER_BIT);

				overlayImage->display();
			}

            glfwSwapBuffers(window);
            frameSlice = 0.0f;

			if (!in_transition) autoTimer += visTimer.lastUpdateSeconds();
			if (autoTimer > autoTimerLimit && !manualOverride) {
				autoTimer = 0;
				vizRandom();
			}

			if (!manualOverride) {
				alphaVal = 1.0+ (sin(visTimer.getSeconds()*0.115+50.0) + cos(visTimer.getSeconds()*0.120+200.0) - sin(100.0+visTimer.getSeconds()*0.132))/2.0;
				if (alphaVal < 0.20) {
					alphaVal = 0.20;
				}
				//cout << alphaVal << endl;
			}

        }
        
        glfwPollEvents();
    }
    
    std::cout << endl << "-----" << endl << "Done." << endl;
    
    glfwTerminate();

    return 0;
}

