#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "utils.h"

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer() : m_Camera(45.0f, 0.1f, 100.f) {}
	virtual void OnUpdate(float ts) override
	{
		m_Camera.OnUpdate(ts);
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");

		ImGui::Text("Render Settings");

		ImGui::Checkbox("Smooth Shading", &m_Scene.GetSmoothShading());
		ImGui::Text("Last render: %.3fms", m_LastRenderTime);
		ImGui::Checkbox("Interactive", &m_interactive);
		if (ImGui::Button("Render"))
		{
			Render();
		}
		
		ImGui::Separator();
		ImGui::Spacing();

		RenderJSONStatsFields();

		ImGui::Separator();

		ImGui::Text("Light Settings");

		ImGui::DragFloat("Light X", &m_Scene.GetLightPos().x, 0.1f);
		ImGui::DragFloat("Light Y", &m_Scene.GetLightPos().y, 0.1f);
		ImGui::DragFloat("Light Z", &m_Scene.GetLightPos().z, 0.1f);
		ImGui::DragFloat("Intensity", &m_Scene.GetLightIntensity(), 0.1f);

		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("USER MANUAL");
		ImGui::Text("To move around with the camera,\nhold down right mouse button and use W-A-S-D-Q-E \n (make sure the Interactive is on).");

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("Viewport");

		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = ImGui::GetContentRegionAvail().y;

		auto image = m_Renderer.GetFinalImage();
		if (image)
		{
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() }, ImVec2(0, 1), ImVec2(1, 0));
		}

		ImGui::End();
		ImGui::PopStyleVar();

		if (m_interactive)
		{
			Render();
		}
	}
	void RenderJSONStatsFields()
	{
		ImGui::Text("Load model");

		static char jsonFileBuffer[128];
		ImGui::InputText("JSON File Name", jsonFileBuffer, IM_ARRAYSIZE(jsonFileBuffer));
		ImGui::InputFloat("Scale", &m_scale);
		ImGui::InputFloat3("Colour", colour);
		if (ImGui::Button("Load"))
		{
			std::string file(jsonFileBuffer);
			std::string path = "./data/";
			std::string jsonExt = ".json";
			glm::vec3 vecColour = glm::vec3(colour[0], colour[1], colour[2]);
			if (m_Parser.ParseFile(path.append(file).append(jsonExt).c_str(), m_scale, vecColour/255.f))
			{
				m_Parser.CalculateVertexNormals();
				m_Scene.LoadModelToScene(m_Parser.GetTriangles(), m_Parser.GetVertices());
				m_loadOutputText = "File " + file + ".json loaded.";
				m_error = false;
			}
			else
			{
				m_error = true;
				m_loadOutputText = "File " + file + ".json failed to load.";
			}
		}

		ImGui::TextColored(m_error ? ImVec4(255, 0, 0, 255) : ImVec4(0, 255, 0, 255), m_loadOutputText.c_str());

		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Text("Mesh Statistics");

		if (ImGui::Button("Average Area"))
		{
			float area = m_Parser.CalculateAverageAreaCompared();
			m_statsOutputText = "Average triangle area for loaded file is " + std::to_string(area);
		}
		ImGui::SameLine();
		if (ImGui::Button("Smallest Area"))
		{
			float area = m_Parser.CalculateSmallestAreaCompared();
			m_statsOutputText = "Smallest triangle area for loaded file is " + std::to_string(area);
		}

		if (ImGui::Button("Largest Area"))
		{
			float area = m_Parser.CalculateLargestAreaCompared();
			m_statsOutputText = "Largest triangle area for loaded file is " + std::to_string(area);
		}
		ImGui::SameLine();
		if (ImGui::Button("Closed Mesh"))
		{
			bool closed = m_Parser.IsClosedMesh();
			m_statsOutputText = closed ? "True" : "False";
		}

		ImGui::InputFloat("Query point X", &m_queryPoint.x);
		ImGui::InputFloat("Query point Y", &m_queryPoint.y);
		ImGui::InputFloat("Query point Z", &m_queryPoint.z);

		if (ImGui::Button("Check inside"))
		{
			bool inside = m_Renderer.IsPointInside(m_queryPoint, m_Scene);
			m_statsOutputText = inside ? "Point is inside loaded mesh." : "Point is outside loaded mesh.";
		}

		ImGui::TextColored(m_error ? ImVec4(255, 0, 0, 255) : ImVec4(0, 255, 0, 255), m_statsOutputText.c_str());
	}
	void Render()
	{
		Timer timer;

		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_Camera, m_Scene);

		m_LastRenderTime = timer.ElapsedMillis();
	}
private:
	Renderer m_Renderer;
	Camera m_Camera;
	Scene m_Scene;
	Parser m_Parser;
	std::string m_statsOutputText = "", m_loadOutputText = "";
	bool m_error = false, m_interactive = false, m_smoothShading = false;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	float m_LastRenderTime = 0, m_scale = 1.f;
	glm::vec3 m_queryPoint = glm::vec3(0);
	float colour[3] = { 255.f, 0.f, 255.f };
	std::string fileName = "Type in the JSON file you want to load.";
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Cashew App";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}