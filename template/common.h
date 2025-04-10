// Based on Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

// common.h is to be included in host and device code and stores
// global settings and defines.

#pragma once

///---------- GLOBAL OPTIONS ----------//

/* Enable debug UI */
//#define DEBUG_UI


/* If your computer doesnt support tinybvh::bvh8_cpu use this */
//#define MAXIMUM_COMPATABILITY
/* Define this to disable features that arent reprojectable and thus will look noisy like stochastic fresnel. */
#define DISABLE_NON_REPROJECTABLE
/* Enable SIMD processing of 4 lights at once */
//#define USE_SIMD
/* Limit threads to certain cores, to make your computer not slow down as much or for profiling*/
//#define LIMIT_THREADS
constexpr uint THREAD_MASK = 0b00000000000000000000011111111111;
/* Display screen energy in the top right for debugging (only in debug mode)*/
//#define CALCULATE_ENERGY
/* Aggresively try to get the fps above 30 by unaiming or removing volumes*/
#define HARD_FPS_LIMIT


/* Displays FPS in top left corner */
#define DISPLAY_FPS

///---------- GLOBAL SETTINGS ----------//

//- ILO MODE -// Set this to one to make the game run at >30fps on the resolution
//This removes a lot of quality from volumes because they are the only reason my game runs slower
#if 0
#pragma warning (push)
#pragma warning (disable: 4005)
#define RENDER_WIDTH	256
#define RENDER_HEIGHT	212
#define DISABLE_MUZZLEFLASH
#pragma warning (pop)
#undef LIMIT_THREADS
#undef DISABLE_NON_REPROJECTABLE
#define HARD_FPS_LIMIT
constexpr float VOLUME_STEP_FULL = 2.0f;
constexpr float VOLUME_STEP_SIMPLE = 2.5f;
constexpr float VOLUME_CLIP_TRESHOLD = 0.2f;
constexpr int MAX_SIMUL_VOLUMES = 4;
#else
#define RENDER_WIDTH  400
#define RENDER_HEIGHT 300
constexpr float VOLUME_STEP_FULL = 1.5f;
constexpr float VOLUME_STEP_SIMPLE = 2.f;
constexpr float VOLUME_CLIP_TRESHOLD = 0.05f;
constexpr int MAX_SIMUL_VOLUMES = 8;

#endif

static uint WINDOW_WIDTH = 800;
static uint WINDOW_HEIGTH = 600;

constexpr uint MAX_BOUNCES = 5;

#define UI_WIDTH	(200)
#define UI_HEIGHT	(150)
///---------- GAME SETTINGS ----------//
constexpr float MOUSE_SENSITIVITY = 0.003f;
constexpr float VERTICAL_FOV = 90.f;
constexpr float AIM_FOV = 40.f;
constexpr float EXPOSURE = 1.2f;
constexpr float BASE_VOLUME = 5.f;
constexpr float SFX_VOLUME = 0.7f;
constexpr float MUSIC_VOLUME = 0.5f; // Multiplied with base


///----------=================----------//


constexpr float ASPECT_RATIO = (float)RENDER_WIDTH / (float)RENDER_HEIGHT;


// constants
#define PI		3.14159265358979323846264f
#define INVPI		0.31830988618379067153777f
#define INV2PI		0.15915494309189533576888f
#define PIDIV2		3.14159265358979323846264f
#define TWOPI		6.28318530717958647692528f
#define SQRT_PI_INV	0.56418958355f
#define LARGE_FLOAT	1e34f


//Scene data
constexpr uint MAX_MESH_VERTICES = 20000;
constexpr uint MAX_PRIMITIVES = 20;
constexpr uint MAX_BLAS = 140;
constexpr uint MAX_LIGHTS = 30;
constexpr float EPSILON = 0.0001f;


#if 0
//Easy debug timer function from Seb
thread_local static int counter = 0;
thread_local static float sum = 0;
thread_local static bool resultPrinted = false;
constexpr int SAMPLES = 500000;

struct ScopedTimer
{
	Timer t;
	const char* name;

	ScopedTimer(const char* funcName)
	{
		name = funcName;
	}

	~ScopedTimer()
	{
		if (counter++ < SAMPLES)
		{
			float seconds = t.elapsed();
			sum += seconds * 1000 * 1000;
		}
		else
		{
			if (resultPrinted) return;
			printf("%f\n", sum / (float)SAMPLES);
			resultPrinted = true;
		}
	}
};

#define PROFILE_FUNCTION() ScopedTimer timer(__FUNCTION__)
#else
#define PROFILE_FUNCTION()
#endif

