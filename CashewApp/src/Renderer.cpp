#include "utils.h"

#include <glm/glm.hpp>

Renderer::Renderer()
{
	m_Scene = std::make_shared<Scene>();
	m_cameraPos = float3(0.f, 1.f, -2.f);
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

void Renderer::Render()
{
	if (!m_FinalImageData) return;

	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			glm::vec2 coord = { (float)x / (float)m_FinalImage->GetWidth(), (float)y / (float)m_FinalImage->GetWidth() };
			coord = coord * 2.0f - 1.0f; // -1 -> 1
			m_FinalImageData[x + y * m_FinalImage->GetWidth()] = ConvertToRGBA(Trace(coord));
		}
	}

	m_FinalImage->SetData(m_FinalImageData);
}

float3 Renderer::Trace(glm::vec2 coord)
{
	float3 rayDirection = float3(coord.x, coord.y, 1.f);
	Ray ray = Ray(m_cameraPos, rayDirection);

	m_Scene.get()->FindNearest(ray);

	if (ray.hitObjIdx == -1) return float3(0);
	return m_Scene->GetShading(ray);
}

bool Renderer::IsPointInside(float3 point) const
{
	// Shoot ray from query point in positive z-axis
	float3 direction = float3(0.f, 0.f, 1.f);
	Ray ray(point, direction);

	m_Scene->FindNearest(ray);

	// If we don't hit anything, it lies ouside
	if (ray.hitObjIdx == -1) return false;

	// Shoot a second ray from intersection point
	float3 hitPoint = ray.GetIntersectionPoint();
	ray = Ray(hitPoint, direction);

	// If we don't hit anything, the query point was inside
	if (ray.hitObjIdx == -1) return true;

	return false;
}
