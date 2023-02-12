#include "utils.h"

Scene::Scene()
{
	// Initialise all objects in scene
	m_Bvh = new Bvh();
	m_lightPos = float3(0.f, 1.f, 0.f);
}

void Scene::LoadModelToScene(const std::vector<Triangle>& triangles, const std::vector<Vertex>& vertices)
{
	m_triangles.clear();
	m_vertices.clear();

	for (auto& triangle : triangles)
	{
		m_triangles.push_back(triangle);
	}

	for (auto& vertex : vertices)
	{
		m_vertices.push_back(vertex);
	}

	m_Bvh->BuildBVH(m_triangles);
}

void Scene::FindNearest(Ray& ray)
{
	if (m_triangles.empty()) return;

	m_Bvh->IntersectBVH(ray, 0);
}

bool Scene::IsOccluded(const Ray& ray)
{
	return false;
}

float3 Scene::ComputeShadingNormal(int triIdx, float u, float v)
{
	Triangle& triangle = m_triangles[triIdx];

	return float3((1 - u - v) * m_vertices[triangle.verIndices[0]].normal + u * m_vertices[triangle.verIndices[1]].normal + v * m_vertices[triangle.verIndices[2]].normal);
}

float3 Scene::GetShading(const Ray& ray)
{
	ShadingInfo shadingInfo = m_triangles[ray.hitObjIdx].shadingInfo;
	if (IsOccluded(ray)) return 0;
	float3 I = ray.O + ray.t * ray.D;
	float3 dirToLight = (m_lightPos - I);
	float3 N = m_smoothShading ? ComputeShadingNormal(ray.hitObjIdx, ray.u, ray.v) : ray.faceNormal;
	float dotProduct = std::max(0.f, dot(normalize(dirToLight), N));
	float attenuation = 1 / length(dirToLight);
	return shadingInfo.albedo * dotProduct * attenuation;
}