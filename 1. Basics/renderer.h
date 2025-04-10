#pragma once
#include "MaterialManager.h"
class RenderScene;
class GameObject;
class Ray;
class FloatTexture;
class SkyDome;

namespace Tmpl8
{
	struct DebugSettings
	{
		int2 offset = int2(300, 150);
		float2 scale = int2(40);
		bool debugDrawerEnabled = false;
		bool breakNext = false;
		float normalScale = 20;
		int primaryRay = 0;
		int yLevel = 0;
		uint seed = 1u;
	};

	struct TraceOut
	{
		float3 albedo = float3(0.f);
		float3 lighting = float3(0.f);
		float length = 0.f;
	};

	enum class Difficulty
	{
		GameReviewer,
		Normal,
		Hard,
		EldenRing
	};

	class Renderer : public TheApp
	{
	public:
		// game flow methods
		void Init() override;
		void CalculateContributionThroughVolumes(float4* _outPos, float4* _outNormal, TraceOut& _out,
		                                         const tinybvh::Ray& _tinyRay, int _bounces);
		void CalculateRefractive(uint _bounces, float4* _outNormal, float4* _outPos, FloatTexture* _debugScreen,
		                         bool _primaryDebugRay, TraceOut& _out, tinybvh::Ray _tinyRay, float3& _albedo,
		                         float3 _i,
		                         bool _frontFace, float3 _normal, const Material& _material, float _transmission,
		                         float _roughness);
		void CalculateMettalic(const Tmpl8::Ray& _ray, uint _bounces, float4* _outNormal, float4* _outPos,
		                       FloatTexture* _debugScreen, bool _primaryDebugRay, TraceOut& _out, tinybvh::Ray _tinyRay,
		                       float3& _albedo, float3 _i, float3 _normal, float _roughness, float _reflectivity);
		TraceOut Trace(const Ray& _ray, uint _bounces, float4* _outNormal, float4* _outPos,
		               FloatTexture* _debugScreen = nullptr, bool _primaryDebugRay = false);
		void Tick(float _deltaTime) override;
		void GameTick(float _deltaTime);
		void UI() override;
		void Shutdown() override;
		// input handling
		void MouseUp(int /*button*/) override;

		void MouseDown(int /*button*/) override;

		void MouseMove(const int _x, const int _y) override
		{
			m_mouseDelta = m_mousePos - int2(_x, _y);
			m_mousePos.x = _x;
			m_mousePos.y = _y;

			float actualAspect = WINDOW_WIDTH / static_cast<float>(WINDOW_HEIGTH);

			int viewportX = 0, viewportY = 0;
			int viewportWidth = WINDOW_WIDTH;
			int viewportHeight = WINDOW_HEIGTH;

			if (actualAspect < ASPECT_RATIO)
			{
				viewportHeight = static_cast<int>(WINDOW_WIDTH / ASPECT_RATIO);
				viewportY = (WINDOW_HEIGTH - viewportHeight) / 2;
			}
			else if (actualAspect > ASPECT_RATIO)
			{
				viewportWidth = static_cast<int>(WINDOW_HEIGTH * ASPECT_RATIO);
				viewportX = (WINDOW_WIDTH - viewportWidth) / 2;
			}
			float normX = (_x - viewportX) / static_cast<float>(viewportWidth);
			float normY = (_y - viewportY) / static_cast<float>(viewportHeight);

			normY = 1.0f - normY;

			m_uiMousePos.x = static_cast<int>(normX * UI_WIDTH);
			m_uiMousePos.y = static_cast<int>(normY * UI_HEIGHT);
		}

		void MouseWheel(float /*y*/) override
		{
			/* implement if you want to handle the mouse wheel */
		}

		void KeyUp(int /*key*/) override;

		void KeyDown(int _key) override;

		void DisplayImage(std::string _type, uint _texture, bool _flip = true, bool _main = false) override;


		void PlayerShoot();

		static void PlayerTakeDamage(float _damage);

		// data members
		std::vector<FloatTexture*> m_noiseTextures;

		float2 m_cameraRotationOffset;
		int2 m_mousePos{int2(0)};
		int2 m_uiMousePos{int2(0)};
		int2 m_lastMousePos{int2(0)};
		int2 m_mouseDelta{0};
		RenderScene* m_renderScene{nullptr};
		SkyDome* m_skyDome{nullptr};
		float4* m_accumulatorAlbedo{nullptr};
		float4* m_accumulatorLight{nullptr};
		uint m_accumulatorFrameCount{0u};
		float m_accumulatorReciprical{0.f};
		Scene m_scene;
		Camera m_camera;
		float m_animTime = 0;
		bool m_mipMappingEnabled = true;
		float m_lodBias = -3.0f;
		bool m_sampleBlueNoise = true;
		bool m_autoDOF = true;
		bool m_debugCamera = false;
		bool m_triggerShoot = false;
		bool m_startedMusic = false;
		//Debug
		DebugSettings m_debugSettings;
		bool m_renderingPaused = false;

		float m_rawFrameTime{0.f};
#ifdef CALCULATE_ENERGY
		float m_currentEnergy{0.f};
		float m_oldEnergy{0.f};
#endif

	private:
		float3 CalculateLighting(const float3& _I, const float3* _N, float3* _lightDirOut,
		                         FloatTexture* _debugScreen = nullptr);
		float3 CalculatePointLight(const float3& _I, const float3* _N, float3* _lightDirOut,
		                           FloatTexture* _debugScreen = nullptr);
		float3 CalculatePointLight4(const float3& _I, const float3* _N, float3* _lightDirOut,
		                            FloatTexture* _debugScreen = nullptr);
		float3 CalculateSkyLight(const float3& _I, const float3* _N, float3* _lightDirOut,
		                         FloatTexture* _debugScreen = nullptr);
		float3 CalculateDirectionalLight(const float3& _I, const float3* _N, float3* _lightDirOut,
		                                 FloatTexture* _debugScreen = nullptr);
		float3 CalculateSpotLight(const float3& _I, const float3* _N, float3* _lightDirOut,
		                          FloatTexture* _debugScreen = nullptr);
		float3 CalculateVolumeLight(const float3& _I, const float3* _N, float3* _lightDirOut,
		                            FloatTexture* _debugScreen = nullptr);
		float SampleBlueNoise();
		float3 BlueCosineRandomUnitVectorHemisphere(float3 _normal);
		float3 BlueRandomUnitVector();
		void SetDifficulty(Difficulty _diff);

	private: // Game stuff
		GameObject* m_cameraObject{nullptr};
		GameObject* m_gunObject{nullptr};
		bool m_inCamera{false};
		bool m_aiming{false};
		int m_heliStartupSound = 0;
		int m_heliSound = 0;
		float m_gunOffset = 0;
		static float m_heliHealth;
		float m_heliMaxHealth;
		static bool m_playerDied;
		bool m_inMainMenu = true;
		bool m_gameFinished = false;
		int m_playedDamageAlert = 0;
		Difficulty m_difficulty = Difficulty::Normal;
		bool m_spawnedEndsplosion = false;
#ifdef HARD_FPS_LIMIT
		bool m_fpsLimitExceeded = false;
#endif
	};
} // namespace Tmpl8
