#include "utils.h"

Scene::Scene()
{
	// Initialise all objects in scene
	m_triangles.push_back(Triangle(0, float3(-0.5f, -0.5f, 0.f), float3(0.5f, -0.5f, 0.f), float3(0.5f, 0.5f, 0.f), float3(0.f, 0.f, -1.0f)));
}

void Scene::FindNearest(Ray& ray)
{
	for (auto& triangle : m_triangles)
	{
		triangle.Intersect(ray);
	}
}