#pragma once

#include "Walnut/Image.h"
#include <memory>
#include <glm/fwd.hpp>

class Scene;

class Renderer
{
public:
	Renderer();

	void OnResize(uint32_t width, uint32_t height);
	void Render();

	uint32_t Trace(glm::vec2 coord);

	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

private:
	uint32_t PerPixel(glm::vec2 coord);

private:
	uint32_t* m_FinalImageData;
	std::shared_ptr<Walnut::Image> m_FinalImage;
	std::unique_ptr<Scene> m_Scene;
};