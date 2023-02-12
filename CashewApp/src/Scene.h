#pragma once

class Bvh;

class Scene
{
public:
	Scene();

	void LoadModelToScene(const std::vector<Triangle>& triangles, const std::vector<Vertex>& vertices);
	void FindNearest(Ray& ray) const;

	glm::vec3 ComputeShadingNormal(int triIdx, float u, float v) const;
	glm::vec3 GetShading(const Ray& ray) const;

	glm::vec3& GetLightPos() { return m_lightPos; };
	float& GetLightIntensity() { return m_lightIntensity; };
	bool& GetSmoothShading() { return m_smoothShading; };

private:
	Bvh* m_Bvh;
	std::vector<Triangle> m_triangles;
	std::vector<Vertex> m_vertices;
	glm::vec3 m_lightPos;
	float m_lightIntensity = 2.f;
	bool m_smoothShading = false;
};