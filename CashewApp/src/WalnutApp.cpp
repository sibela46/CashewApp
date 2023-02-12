#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "utils.h"

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
public:
	virtual void OnUpdate(float ts) override
	{
		/*if (GetKeyState('S') & 0x8000)
		{
			m_Renderer.TranslateCamera(float3(0.f, 0.f, -0.5f));
		}*/
	}

	virtual void OnUIRender() override
	{
		auto scene = m_Renderer.GetScene();

		ImGui::Begin("Settings");

		ImGui::Text("Render Settings");

		ImGui::Checkbox("Smooth Shading", &scene->GetSmoothShading());
		ImGui::Text("Last render: %.3fms", m_LastRenderTime);
		if (ImGui::Button("Render"))
		{
			Render();
		}
		
		ImGui::Separator();
		ImGui::Spacing();

		RenderJSONStatsFields();

		ImGui::Separator();

		ImGui::Text("Camera Settings");

		ImGui::DragFloat("Camera X", &m_Renderer.GetCameraPos().x, 0.1f);
		ImGui::DragFloat("Camera Y", &m_Renderer.GetCameraPos().y, 0.1f);
		ImGui::DragFloat("Camera Z", &m_Renderer.GetCameraPos().z, 0.1f);

		ImGui::Separator();

		ImGui::Text("Light Settings");

		ImGui::DragFloat("Light X", &m_Renderer.GetScene()->GetLightPos().x, 0.1f);
		ImGui::DragFloat("Light Y", &m_Renderer.GetScene()->GetLightPos().y, 0.1f);
		ImGui::DragFloat("Light Z", &m_Renderer.GetScene()->GetLightPos().z, 0.1f);

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
	}
	void RenderJSONStatsFields()
	{
		ImGui::Text("Mesh Statistics");

		static char jsonFileBuffer[128];
		ImGui::InputText("JSON File Name", jsonFileBuffer, IM_ARRAYSIZE(jsonFileBuffer));
		if (ImGui::Button("Load"))
		{
			std::string file(jsonFileBuffer);
			std::string path = "./data/";
			std::string jsonExt = ".json";
			if (m_Parser.ParseFile(path.append(file).append(jsonExt).c_str()))
			{
				m_Parser.CalculateVertexNormals();
				m_Renderer.GetScene()->LoadModelToScene(m_Parser.GetTriangles(), m_Parser.GetVertices());
				m_outputText = "File " + file + ".json loaded.";
				m_error = false;
			}
			else
			{
				m_error = true;
				m_outputText = "File " + file + ".json failed to load.";
			}
		}

		if (ImGui::Button("Average Area"))
		{
			float area = m_Parser.CalculateAverageAreaMultithreaded();
			m_outputText = "Average triangle area for loaded file is " + std::to_string(area);
		}
		ImGui::SameLine();
		if (ImGui::Button("Smallest Area"))
		{
			float area = m_Parser.CalculateSmallestAreaMultithreaded();
			m_outputText = "Smallest triangle area for loaded file is " + std::to_string(area);
		}

		if (ImGui::Button("Largest Area"))
		{
			float area = m_Parser.CalculateLargestAreaMultithreaded();
			m_outputText = "Largesst triangle area for loaded file is " + std::to_string(area);
		}
		ImGui::SameLine();
		if (ImGui::Button("Closed Mesh"))
		{
			bool closed = m_Parser.IsClosedMesh();
			m_outputText = closed ? "True" : "False";
		}

		ImGui::InputFloat("Query point X", &m_queryPoint.x);
		ImGui::InputFloat("Query point Y", &m_queryPoint.y);
		ImGui::InputFloat("Query point Z", &m_queryPoint.z);

		if (ImGui::Button("Check inside"))
		{
			bool inside = m_Renderer.IsPointInside(m_queryPoint);
			m_outputText = inside ? "Point is inside loaded mesh." : "Point is outside loaded mesh.";
		}

		ImGui::TextColored(m_error ? ImVec4(255, 0, 0, 255) : ImVec4(0, 255, 0, 255), m_outputText.c_str());
	}
	void Render()
	{
		Timer timer;

		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render();

		m_LastRenderTime = timer.ElapsedMillis();
	}
private:
	Renderer m_Renderer;
	Parser m_Parser;
	std::string m_outputText = "";
	bool m_error = false;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	float m_LastRenderTime = 0;
	bool m_smoothShading;
	float3 m_queryPoint = float3(0);
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