#include "utils.h"

#include <execution>
#include <glm/glm.hpp>

#include "Walnut/Random.h"

Renderer::Renderer()
{
	m_cameraPos = glm::vec3(0.f, 0.f, -3.f);
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
	m_Camera = &camera;
	m_Scene = &scene;

	if (!m_FinalImageData) return;

	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y)
		{
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
			[this, y](uint32_t x)
				{
					m_FinalImageData[x + y * m_FinalImage->GetWidth()] = ConvertToRGBA(Trace(x, y));
				});
		});

	m_FinalImage->SetData(m_FinalImageData);
}

glm::vec3 Renderer::Trace(uint32_t x, uint32_t y)
{
	Ray ray(m_Camera->GetPosition(), m_Camera->GetRayDirections()[x + y * m_FinalImage->GetWidth()]);

	m_Scene->FindNearest(ray);

	if (ray.hitObjIdx == -1)
	{
		glm::vec3 skyColour = glm::vec3(0.41176f, 0.41176f, 0.41176f);
		return skyColour;
	}

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
