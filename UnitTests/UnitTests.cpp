#include "pch.h"
#include "CppUnitTest.h"
#include "precomp.h"
#include "SphereBVH.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TemplateMath
{
	TEST_CLASS(Float)
	{
	public:
		TEST_METHOD(Lerp)
		{
			// Basic cases
			Assert::AreEqual(lerp(0, 10, 0.5f), 5.f);
			Assert::AreEqual(lerp(10, 0, 0.5f), 5.f);
			Assert::AreEqual(lerp(5, 0, 0.f), 5.f);
			Assert::AreEqual(lerp(5, 0, 1.f), 0.f);

			// Edge cases
			Assert::AreEqual(lerp(0, 100, 0.0f), 0.f); // t=0
			Assert::AreEqual(lerp(0, 100, 1.0f), 100.f); // t=1
			Assert::AreEqual(lerp(0, 100, -0.5f), -50.f); // t < 0
			Assert::AreEqual(lerp(0, 100, 1.5f), 150.f); // t > 1
			Assert::AreEqual(lerp(100, 100, 0.5f), 100.f); // identical values
		}
	};

	TEST_CLASS(Float2)
	{
	public:
		TEST_METHOD(Lerp)
		{
			// Basic cases
			float2 result = lerp(float2(0.f), float2(10.f), 0.5f);
			Assert::AreEqual(result.x, 5.f);
			Assert::AreEqual(result.y, 5.f);

			// Edge cases
			result = lerp(float2(0.f, 0.f), float2(10.f, 10.f), 0.0f); // t=0
			Assert::AreEqual(result.x, 0.f);
			Assert::AreEqual(result.y, 0.f);

			result = lerp(float2(0.f, 0.f), float2(10.f, 10.f), 1.0f); // t=1
			Assert::AreEqual(result.x, 10.f);
			Assert::AreEqual(result.y, 10.f);

			result = lerp(float2(-10.f, 10.f), float2(10.f, -10.f), 0.5f); // mixed signs
			Assert::AreEqual(result.x, 0.f);
			Assert::AreEqual(result.y, 0.f);
		}

		TEST_METHOD(Length)
		{
			// Basic case
			Assert::AreEqual(length(float2(10.f, 0.f)), 10.f);

			// Edge cases
			Assert::AreEqual(length(float2(0.f, 0.f)), 0.f); // zero length
			Assert::AreEqual(length(float2(-10.f, 0.f)), 10.f); // negative x component
			Assert::AreEqual(length(float2(3.f, 4.f)), 5.f); // Pythagorean triple (3,4,5)
		}

		TEST_METHOD(Normalize)
		{
			// Basic case
			float2 result = normalize(float2(10.f, 0.f));
			Assert::AreEqual(length(result), 1.f);

			// Edge cases
			result = normalize(float2(0.f)); // zero vector
			Assert::IsTrue(isnan(result.x));
			Assert::IsTrue(isnan(result.y));

			result = normalize(float2(0.f, 1.f)); // already normalized
			Assert::AreEqual(result.x, 0.f);
			Assert::AreEqual(result.y, 1.f);
		}
	};

	TEST_CLASS(Float3)
	{
	public:
		TEST_METHOD(Lerp)
		{
			// Basic cases
			float3 result = lerp(float3(0.f), float3(10.f), 0.5f);
			Assert::AreEqual(result.x, 5.f);
			Assert::AreEqual(result.y, 5.f);
			Assert::AreEqual(result.z, 5.f);

			// Edge cases
			result = lerp(float3(0.f, 0.f, 0.f), float3(10.f, 10.f, 10.f), 0.0f); // t=0
			Assert::AreEqual(result.x, 0.f);
			Assert::AreEqual(result.y, 0.f);
			Assert::AreEqual(result.z, 0.f);

			result = lerp(float3(0.f, 0.f, 0.f), float3(10.f, 10.f, 10.f), 1.0f); // t=1
			Assert::AreEqual(result.x, 10.f);
			Assert::AreEqual(result.y, 10.f);
			Assert::AreEqual(result.z, 10.f);

			result = lerp(float3(-10.f, 10.f, -10.f), float3(10.f, -10.f, 10.f), 0.5f); // mixed signs
			Assert::AreEqual(result.x, 0.f);
			Assert::AreEqual(result.y, 0.f);
			Assert::AreEqual(result.z, 0.f);
		}

		TEST_METHOD(Length)
		{
			// Basic case
			Assert::AreEqual(length(float3(10.f, 0.f, 0.f)), 10.f);

			// Edge cases
			Assert::AreEqual(length(float3(0.f, 0.f, 0.f)), 0.f); // zero length
			Assert::AreEqual(length(float3(-10.f, 0.f, 0.f)), 10.f); // negative x component
			Assert::AreEqual(length(float3(1.f, 2.f, 2.f)), 3.f); // Pythagorean triple (1,2,3)
		}

		TEST_METHOD(Normalize)
		{
			// Basic case
			float3 result = normalize(float3(10.f, 0.f, 0.f));
			Assert::AreEqual(length(result), 1.f, EPSILON);

			// Edge cases
			result = normalize(float3(0.f)); // zero vector
			Assert::IsTrue(isnan(result.x));
			Assert::IsTrue(isnan(result.y));
			Assert::IsTrue(isnan(result.z));

			result = normalize(float3(0.f, 1.f, 0.f)); // already normalized
			Assert::AreEqual(result.x, 0.f);
			Assert::AreEqual(result.y, 1.f);
			Assert::AreEqual(result.z, 0.f);
		}

		TEST_METHOD(Reflect)
		{
			float3 incident = float3(1.f, 0.f, 0.f);
			float3 normal = float3(0.f, 1.f, 0.f);
			float3 result = reflect(incident, normal);
			Assert::AreEqual(result.x, 1.f);
			Assert::AreEqual(result.y, 0.f);
			Assert::AreEqual(result.z, 0.f);

			incident = float3(0.f, -1.f, 0.f);
			normal = float3(0.f, 1.f, 0.f);
			result = reflect(incident, normal);
			Assert::AreEqual(result.x, 0.f);
			Assert::AreEqual(result.y, 1.f);
			Assert::AreEqual(result.z, 0.f);

			incident = float3(1.f, 1.f, 0.f);
			normal = float3(0.f, 1.f, 0.f);
			result = reflect(incident, normal);
			Assert::AreEqual(result.x, 1.f);
			Assert::AreEqual(result.y, -1.f);
			Assert::AreEqual(result.z, 0.f);

			incident = float3(0.f, 0.f, -1.f);
			normal = float3(0.f, 0.f, 1.f);
			result = reflect(incident, normal);
			Assert::AreEqual(result.x, 0.f);
			Assert::AreEqual(result.y, 0.f);
			Assert::AreEqual(result.z, 1.f);
		}
	};
}


namespace Intersections
{
	TEST_CLASS(Sphere)
	{
	public:
		TEST_METHOD(FindNearest)
		{
			SphereBVH testObject(float3(0.f), 1.f, 0, -1);

			//Basic Hit
			tinybvh::Ray ray(float3(-10, 0, 0), float3(1, 0, 0));
			testObject.m_bvh.Intersect(ray);
			Assert::IsTrue(ray.hit.t < 1.0e20f);

			//Sphere behind ray
			ray = tinybvh::Ray(float3(10, 0, 0), float3(1, 0, 0));
			testObject.m_bvh.Intersect(ray);
			Assert::IsTrue(ray.hit.t > 1.0e20f);

			//Rotated unnormalized ray
			ray = tinybvh::Ray(float3(10, 10, 0), float3(-10, -10, 0));
			testObject.m_bvh.Intersect(ray);
			Assert::IsTrue(ray.hit.t < 1.0e20f);

			//Start inside sphere
			ray = tinybvh::Ray(float3(0, 0, 0), float3(1, 0, 0));
			testObject.m_bvh.Intersect(ray);
			Assert::IsTrue(ray.hit.t < 1.0e20f);

			SphereBVH testObject2 = SphereBVH(float3(0.f, 0.f, 10.f), 1.f, 0, -1);
			//Translated sphere
			ray = tinybvh::Ray(float3(-10, 0, 10), float3(1, 0, 0));
			testObject2.m_bvh.Intersect(ray);
			Assert::IsTrue(ray.hit.t < 1.0e20f);


			SphereBVH testObject3 = SphereBVH(float3(0.f, 0.f, 0.f), 0.1f, 0, -1);
			//Scaled sphere
			ray = tinybvh::Ray(float3(10, 0, 0), float3(-1, 0, 0));
			testObject3.m_bvh.Intersect(ray);
			Assert::IsTrue(ray.hit.t < 1.0e20f);
		}

		TEST_METHOD(IsOccluded)
		{
			SphereBVH testObject(float3(0.f), 1.f, 0, -1);

			//Basic Hit
			tinybvh::Ray ray(float3(-10, 0, 0), float3(1, 0, 0));
			Assert::IsTrue(testObject.m_bvh.IsOccluded(ray));

			//Sphere behind ray
			ray = tinybvh::Ray(float3(10, 0, 0), float3(1, 0, 0));
			Assert::IsFalse(testObject.m_bvh.IsOccluded(ray));

			//Rotated unnormalized ray
			ray = tinybvh::Ray(float3(10, 10, 0), float3(-10, -10, 0));
			Assert::IsTrue(testObject.m_bvh.IsOccluded(ray));

			//Start inside sphere
			ray = tinybvh::Ray(float3(0, 0, 0), float3(1, 0, 0));
			Assert::IsTrue(testObject.m_bvh.IsOccluded(ray));

			SphereBVH testObject2 = SphereBVH(float3(0.f, 0.f, 10.f), 1.f, 0, -1);
			//Translated sphere
			ray = tinybvh::Ray(float3(-10, 0, 10), float3(1, 0, 0));
			Assert::IsTrue(testObject2.m_bvh.IsOccluded(ray));
		}
	};
}
