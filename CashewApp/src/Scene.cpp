#include "utils.h"

Scene::Scene()
{
	// Initialise all objects in scene
	m_triangles.push_back(Triangle(0, float3(-0.5f, -0.5f, 0.f), float3(0.5f, -0.5f, 0.f), float3(0.5f, 0.5f, 0.f), float3(0.f, 0.f, -1.0f)));
}

void Scene::LoadModelToScene(std::vector<Triangle> triangles)
{
	for (auto& triangle : triangles)
	{
		triangle.id += m_triangles.size();
		m_triangles.push_back(triangle);
	}
}

void Scene::FindNearest(Ray& ray)
{
	for (auto& triangle : m_triangles)
	{
		triangle.Intersect(ray);
	}
}