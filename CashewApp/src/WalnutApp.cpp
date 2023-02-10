#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "utils.h"

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");

		ImGui::Text("Last render: %.3fms", m_LastRenderTime);
		if (ImGui::Button("Render"))
		{
			Render();
		}

		RenderJSONStatsFields();

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
		static char jsonFileBuffer[128];
		ImGui::InputText("JSON File Name", jsonFileBuffer, IM_ARRAYSIZE(jsonFileBuffer));
		if (ImGui::Button("Load"))
		{
			std::string file(jsonFileBuffer);
			std::string path = "./data/";
			std::string jsonExt = ".json";
			if (m_Parser.ParseFile(path.append(file).append(jsonExt).c_str()))
			{
				m_Renderer.GetScene()->LoadModelToScene(m_Parser.GetTriangles());
				outputText = "File " + file + ".json loaded.";
			}
			else
				outputText = "File " + file + ".json failed to load.";
		}

		if (ImGui::Button("Average Area"))
		{
			float area = m_Parser.CalculateAverageAreaMultithreaded();
			outputText = "Average triangle area for loaded file is " + std::to_string(area);
		}
		if (ImGui::Button("Smallest Area"))
		{
			float area = m_Parser.CalculateSmallestAreaMultithreaded();
			outputText = "Smallest triangle area for loaded file is " + std::to_string(area);
		}
		if (ImGui::Button("Largest Area"))
		{
			float area = m_Parser.CalculateLargestAreaMultithreaded();
			outputText = "Largesst triangle area for loaded file is " + std::to_string(area);
		}
		if (ImGui::Button("Closed Mesh"))
		{
			bool closed = m_Parser.IsClosedMesh();
			outputText = closed ? "True" : "False";
		}

		ImGui::InputFloat("Query point X:", &queryPoint.x);
		ImGui::InputFloat("Query point Y:", &queryPoint.y);
		ImGui::InputFloat("Query point Z:", &queryPoint.z);

		if (ImGui::Button("Check inside"))
		{
			bool inside = m_Renderer.IsPointInside(queryPoint);
			outputText = inside ? "Point is inside loaded mesh." : "Point is outside loaded mesh.";
		}

		ImGui::Text(outputText.c_str());
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
	std::string outputText = "";
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	float m_LastRenderTime = 0;
	float3 queryPoint = float3(0);
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