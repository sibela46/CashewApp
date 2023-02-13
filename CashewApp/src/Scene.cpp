#include "utils.h"

Scene::Scene()
{
	// Initialise all objects in scene
	m_Bvh = new Bvh();
	m_lightPos = glm::vec3(4.f, 2.f, 10.f);
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

void Scene::FindNearest(Ray& ray) const
{
	if (m_triangles.empty()) return;

	m_Bvh->IntersectBVH(ray, 0);
}

glm::vec3 Scene::ComputeShadingNormal(int triIdx, float u, float v) const
{
	const Triangle& triangle = m_triangles[triIdx];

	return glm::vec3((1 - u - v) * m_vertices[triangle.verIndices[0]].normal + u * m_vertices[triangle.verIndices[1]].normal + v * m_vertices[triangle.verIndices[2]].normal);
}

glm::vec3 Scene::GetShading(const Ray& ray) const
{
	glm::vec3 albedo = m_triangles[ray.hitObjIdx].colour;
	glm::vec3 I = ray.O + ray.t * ray.D;
	glm::vec3 dirToLight = (m_lightPos - I);
	glm::vec3 N = m_smoothShading ? ComputeShadingNormal(ray.hitObjIdx, ray.u, ray.v) : ray.faceNormal;
	float dotProduct = std::max(0.f, glm::dot(glm::normalize(dirToLight), N));
	return albedo * dotProduct * (1/PI) * m_lightIntensity;
}