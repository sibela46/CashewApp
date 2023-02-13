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

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

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

	if (m_frameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

	std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(),
		[this](uint32_t y)
		{
			std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(),
			[this, y](uint32_t x)
				{
					if (m_bounces == 1)
					{
						m_FinalImageData[x + y * m_FinalImage->GetWidth()] = ConvertToRGBA(Trace(x, y));
						return;
					}

					glm::vec4 colour = glm::vec4(Trace(x, y), 1.f);

					m_AccumulationData[x + y * m_FinalImage->GetWidth()] += colour;

					glm::vec4 accumulatedColour = m_AccumulationData[x + y * m_FinalImage->GetWidth()];
					accumulatedColour /= (float)m_frameIndex;

					m_FinalImageData[x + y * m_FinalImage->GetWidth()] = ConvertToRGBA(accumulatedColour);
				});
		});

	m_FinalImage->SetData(m_FinalImageData);
	m_frameIndex++;
}

glm::vec3 Renderer::SampleHemisphere(const glm::vec3& N)
{
	float r1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);;
	float r2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);;
	float sinTheta = sqrtf(1 - r1 * r1);
	float phi = 2 * PI * r2;
	float x = sinTheta * cosf(phi);
	float y = sinTheta * sinf(phi);
	glm::vec3 randomVec = glm::vec3(x, y, r1);
	if (dot(randomVec, N) < 0) randomVec = -randomVec;
	return randomVec;
}


glm::vec3 Renderer::Trace(uint32_t x, uint32_t y)
{
	Ray ray(m_Camera->GetPosition(), m_Camera->GetRayDirections()[x + y * m_FinalImage->GetWidth()]);

	glm::vec3 colour(0.f);
	float multiplier = 1.0f;

	m_bounces = std::clamp(m_bounces, 0, 10);

	for (int i = 0; i < m_bounces; i++)
	{
		m_Scene->FindNearest(ray);

		if (ray.hitObjIdx == -1)
		{
			glm::vec3 skyColour = glm::vec3(0.41176f, 0.41176f, 0.41176f);
			colour += multiplier * skyColour;
			break;
		}

		colour += m_Scene->GetShading(ray) * multiplier;

		multiplier *= 0.5f;

		ray.O = ray.GetIntersectionPoint() + ray.faceNormal * 0.001f;
		ray.D = glm::reflect(ray.D, ray.faceNormal + Walnut::Random::Vec3(-0.5f, 0.5f));
	}

	return colour;
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
