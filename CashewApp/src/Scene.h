#pragma once

class Scene
{
public:
	Scene();

	void LoadModelToScene(std::vector<Triangle> triangles);
	void FindNearest(Ray& ray);

private:
	std::vector<Triangle> m_triangles;
};