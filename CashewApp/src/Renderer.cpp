#include "utils.h"

#include <execution>
#include <glm/glm.hpp>

Renderer::Renderer()
{
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

	m_ImageHorizontalIter.resize(width);
	m_ImageVerticalIter.resize(height);
	for (uint32_t i = 0; i < height; i++)
		m_ImageVerticalIter[i] = i;
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizontalIter[i] = i;
}

void Renderer::Render(const Camera& camera, const Scene& scene)
{
	m_Camera = std::make_unique<Camera>(camera);
	m_Scene = std::make_unique<Scene>(scene);

	if (!m_FinalImageData) return;

#if MULTI_THREADING
	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y)
		{
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
				[this, y](uint32_t x)
				{
					m_FinalImageData[x + y * m_FinalImage->GetWidth()] = ConvertToRGBA(Trace(x, y));
				});
		});
#else
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			Ray ray (camera.GetPosition(), camera.GetRayDirections()[x + y * m_FinalImage->GetWidth()]);
			m_FinalImageData[x + y * m_FinalImage->GetWidth()] = ConvertToRGBA(Trace(ray, scene));
		}
	}
#endif
	m_FinalImage->SetData(m_FinalImageData);
}

glm::vec3 Renderer::Trace(uint32_t x, uint32_t y)
{
	Ray ray(m_Camera->GetPosition(), m_Camera->GetRayDirections()[x + y * m_FinalImage->GetWidth()]);

	m_Scene->FindNearest(ray);

	if (ray.hitObjIdx == -1) return glm::vec3(0);
	return m_Scene->GetShading(ray);
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
