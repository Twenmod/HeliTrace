#include "precomp.h"
#include "camera.h"

Ray Tmpl8::Camera::GetPrimaryRay(const float x, const float y, bool defocus, RngSeed& seed)
{
	// calculate pixel position on virtual screen plane

	float2 offset = float2(RandomFloat(seed.s), RandomFloat(seed.s));

	offset = float2(0);

	float3 sample = topLeft + ((x + offset.x) * pixelDeltaU) + ((y + offset.y) * pixelDeltaV);

	float3 pos;
	if (defocusAngle > 0.f && defocus)
	{
		float3 defocusOffset = RandomUnitVectorCircle(seed.s);

		pos = camPos + (defocusOffset.x * (up * defocusRadius) + defocusOffset.y * (right * defocusRadius));
	}
	else
	{
		pos = camPos;
	}

	float3 direction = sample - pos;


	if (paniniProjected) // Panini projection
	{
		float3 dirCameraSpace = float3(dot(direction, right), dot(direction, up), dot(direction, front));

		// Apply Panini projection formula in camera space

		float x_panini = (A * tan(dirCameraSpace.x)) / (sqrt(
			1 + (A * A - 1.f) * ((dirCameraSpace.x) * (dirCameraSpace.x))));
		float y_panini = -dirCameraSpace.y / dirCameraSpace.z;

		//bool fisheye = 0;

		//if (fisheye)
		//{
		//	y_panini = (A * tan(dirCameraSpace.y)) / (sqrt(1 + (A * A - 1.f) * ((dirCameraSpace.y) * (dirCameraSpace.y))));
		//}

		//if (isnan(x_panini) || isnan(y_panini))
		//{
		//	DebugBreak();
		//}

		dirCameraSpace = float3(x_panini, y_panini, dirCameraSpace.z);
		direction = normalize(dirCameraSpace.x * right + dirCameraSpace.y * up + dirCameraSpace.z * front);
	}


	return Ray(pos, normalize(direction));
}
