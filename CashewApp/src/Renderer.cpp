#include "utils.h"

#include <glm/glm.hpp>

Renderer::Renderer()
{
	m_Scene = std::make_shared<Scene>();
	m_cameraPos = glm::vec3(0.f, 1.f, -3.f);
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_FinalImageData;
	m_FinalImageData = new uint32_t[width * height];
}

void Renderer::Render(const Camera& camera, Scene& scene)
{
	Ray ray = Ray();
	ray.O = camera.GetPosition();
	if (!m_FinalImageData) return;

	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			Ray ray (camera.GetPosition(), camera.GetRayDirections()[x + y * m_FinalImage->GetWidth()]);
			m_FinalImageData[x + y * m_FinalImage->GetWidth()] = ConvertToRGBA(Trace(ray, scene));
		}
	}

	m_FinalImage->SetData(m_FinalImageData);
}

glm::vec3 Renderer::Trace(Ray& ray, Scene& scene)
{
	scene.FindNearest(ray);

	if (ray.hitObjIdx == -1) return glm::vec3(0);
	return scene.GetShading(ray);
}

bool Renderer::IsPointInside(glm::vec3 point, Scene& scene) const
{
	// Shoot ray from query point in negative z-axis
	glm::vec3 direction = glm::vec3(0.f, 0.f, -1.f);
	Ray ray(point, direction);

	scene.FindNearest(ray);

	// If we don't hit anything, it lies ouside
	if (ray.hitObjIdx == -1) return false;

	// Shoot a second ray from intersection point
	ray = Ray(ray.GetIntersectionPoint(), direction);

	scene.FindNearest(ray);

	// If we don't hit anything, the query point was inside
	if (ray.hitObjIdx == -1) return true;

	return false;
}
