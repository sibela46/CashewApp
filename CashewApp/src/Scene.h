#pragma once

class Scene
{
public:
	Scene();

	void FindNearest(Ray& ray);

private:
	std::vector<Triangle> m_triangles;
};