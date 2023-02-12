#pragma once

class Bvh;

class Scene
{
public:
	Scene();

	void LoadModelToScene(const std::vector<Triangle>& triangles, const std::vector<Vertex>& vertices);
	void FindNearest(Ray& ray);

	float3 ComputeShadingNormal(int triIdx, float u, float v);
	float3 GetShading(const Ray& ray);

	float3& GetLightPos() { return m_lightPos; };
	bool& GetSmoothShading() { return m_smoothShading; };

private:
	bool IsOccluded(const Ray& ray);

private:
	Bvh* m_Bvh;
	std::vector<Triangle> m_triangles;
	std::vector<Vertex> m_vertices;
	float3 m_lightPos;
	bool m_smoothShading = false;
};