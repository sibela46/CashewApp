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
	void Render(const Camera& camera, Scene& scene);

	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }
	std::shared_ptr<Scene> GetScene() const { return m_Scene; }

	bool IsPointInside(glm::vec3 point, Scene& scene) const;

	glm::vec3& GetCameraPos() { return m_cameraPos; };

private:
	glm::vec3 Trace(Ray& ray, Scene& scene);

private:
	uint32_t* m_FinalImageData = nullptr;
	std::shared_ptr<Walnut::Image> m_FinalImage;
	std::shared_ptr<Scene> m_Scene;
	glm::vec3 m_cameraPos;
};