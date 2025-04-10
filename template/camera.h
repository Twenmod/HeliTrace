#pragma once
#include "scene.h"

namespace Tmpl8
{
	class Camera
	{
	public:
		Camera()
		{
			// setup a basic view frustum
			camPos = float3(0, 0, -2);
			camTarget = float3(0, 0, -1);
			CalcView();
		}
		Ray GetPrimaryRay(const float x, const float y, bool defocus, uint& seed)
		{
			// calculate pixel position on virtual screen plane

			float2 offset = float2(RandomFloat(seed), RandomFloat(seed));

			offset = float2(0);

			float3 sample = topLeft + ((x+offset.x) * pixelDeltaU) + ((y+offset.y) * pixelDeltaV);

			float3 pos;
			if (defocusAngle > 0.f && defocus)
			{
				float3 defocusOffset = RandomUnitVectorCircle(seed);

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

				float x_panini = (A * tan(dirCameraSpace.x)) / (sqrt(1 + (A * A - 1.f) * ((dirCameraSpace.x) * (dirCameraSpace.x))));
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
		void SetFocusDistance(float distance)
		{
			focusDistance = distance;
			CalcView();
		}
		float GetFocusDistance() const { return focusDistance; }

		bool HandleInput(const float t)
		{
			if (!WindowHasFocus()) return false;
			float speed = 0.0155f * t;

			if (IsKeyDown(GLFW_KEY_LEFT_SHIFT)) speed *= 0.5f;

			bool changed = false;
			if (IsKeyDown(GLFW_KEY_A)) camPos -= speed * 2 * right, changed = true;
			if (IsKeyDown(GLFW_KEY_D)) camPos += speed * 2 * right, changed = true;
			if (IsKeyDown(GLFW_KEY_W)) camPos -= speed * 2 * front, changed = true;
			if (IsKeyDown(GLFW_KEY_S)) camPos += speed * 2 * front, changed = true;
			if (IsKeyDown(GLFW_KEY_R)) camPos += speed * 2 * up, changed = true;
			if (IsKeyDown(GLFW_KEY_F)) camPos -= speed * 2 * up, changed = true;
			camTarget = camPos + front;

			speed = 0.001f * t;
			if (IsKeyDown(GLFW_KEY_LEFT_SHIFT)) speed *= 0.5f;
			if (IsKeyDown(GLFW_KEY_UP)) camTarget -= speed * up, changed = true;
			if (IsKeyDown(GLFW_KEY_DOWN)) camTarget += speed * up, changed = true;
			if (IsKeyDown(GLFW_KEY_LEFT)) camTarget += speed * right, changed = true;
			if (IsKeyDown(GLFW_KEY_RIGHT)) camTarget -= speed * right, changed = true;
			if (!changed) return false;

			CalcView();

			return true;
		}

		void CalcView()
		{
			front = normalize(camTarget - camPos);
			right = normalize(cross(float3(0, 1, 0), front));
			up = normalize(cross(front, right));

			float theta = DegToRad(vFov);
			float h = std::tan(theta / 2);
			float viewportHeight = 2 * h * focusDistance;
			float viewportWidth = viewportHeight * aspect;

			viewportU = viewportWidth * right;
			viewportV = viewportHeight * -up;

			pixelDeltaU = viewportU / RENDER_WIDTH;
			pixelDeltaV = viewportV / RENDER_HEIGHT;
			topLeft = (camPos - focusDistance * front - viewportU * 0.5f - viewportV * 0.5f) + (0.5f * (pixelDeltaU + pixelDeltaV));
			defocusRadius = focusDistance * std::tan(DegToRad(defocusAngle / 2.f));
		}
		float vFov = 70;
		float aspect = (float)RENDER_WIDTH / (float)RENDER_HEIGHT;
		float3 camPos, camTarget;
		float3 front, right, up;
		float3 topLeft;
		float3 pixelDeltaU;
		float3 pixelDeltaV;
		float3 viewportU;
		float3 viewportV;
		float defocusAngle = 0.f;
		bool paniniProjected = false;
		float A = 1.1f;
	private:
		float focusDistance = 1.f;
		float defocusRadius = focusDistance * std::tan(DegToRad(defocusAngle / 2.f));

	};
}