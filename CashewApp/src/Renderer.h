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

private:
	glm::vec3 Trace(uint32_t x, uint32_t y);

private:
	uint32_t* m_FinalImageData = nullptr;
	std::shared_ptr<Walnut::Image> m_FinalImage;
	std::unique_ptr<Scene> m_Scene;
	std::unique_ptr<Camera> m_Camera;
	glm::vec3 m_cameraPos;

	std::vector<uint32_t> m_ImageHorizontalIter, m_ImageVerticalIter;
};