#pragma once

#include "Walnut/Image.h"
#include <memory>
#include <glm/fwd.hpp>

class Scene;
class Camera;

class Renderer
{
public:
	Renderer();

	void OnResize(uint32_t width, uint32_t height);
	void Render(const Camera& camera, const Scene& scene);

	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

	bool IsPointInside(glm::vec3 point, Scene& scene) const;

	glm::vec3& GetCameraPos() { return m_cameraPos; };
	int& GetBounces() { return m_bounces; };
	void ResetAccumulatedBuffer() { m_frameIndex = 1; };

private:
	glm::vec3 Trace(uint32_t x, uint32_t y);
	glm::vec3 Renderer::SampleHemisphere(const glm::vec3& N);


private:
	uint32_t* m_FinalImageData = nullptr;
	glm::vec4* m_AccumulationData = nullptr;
	std::shared_ptr<Walnut::Image> m_FinalImage;
	const Scene* m_Scene;
	const Camera* m_Camera;
	glm::vec3 m_cameraPos;
	std::vector<uint32_t> m_ImageHorizontalIter, m_ImageVerticalIter;

	int m_frameIndex = 1, m_bounces = 5;
};