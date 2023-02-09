#include "Parser.h"

std::mutex mtx;
Parser::Parser()
{

}

bool Parser::ParseFile(const char* fileName)
{
	m_triangles.clear();
	m_vertices.clear();
	m_edges.clear();

	// Open the file
	FILE* fp = fopen(fileName, "rb");

	if (!fp)
	{
		std::cerr << "Error: cannot open JSON file." << std::endl;
		return false;
	}

	// Read the file into a buffer
	char readBuffer[65536];
	rapidjson::FileReadStream is(fp, readBuffer,
		sizeof(readBuffer));

	// Parse the JSON document
	rapidjson::Document doc;
	doc.ParseStream(is);

	// Check if the document is valid
	if (doc.HasParseError())
	{
		std::cerr << "Error: failed to parse JSON document." << std::endl;
		fclose(fp);
		return false;
	}

	// Close the file
	fclose(fp);

	if (doc.HasMember("geometry_object") && doc["geometry_object"].IsObject())
	{
		// Fill vertices
		const rapidjson::Value& geometryObject = doc["geometry_object"];
		if (geometryObject.HasMember("vertices") && geometryObject["vertices"].IsArray())
		{
			const rapidjson::Value& verticesData = geometryObject["vertices"];
			for (rapidjson::SizeType i = 0; i < (verticesData.Size() - 2); i += 3)
			{
				if (verticesData[i].IsFloat() && verticesData[i + 1].IsFloat() && verticesData[i + 2].IsFloat())
				{
					float x = verticesData[i].GetFloat();
					float y = verticesData[i + 1].GetFloat();
					float z = verticesData[i + 2].GetFloat();
					m_vertices.push_back(Vertex(x, y, z));
				}
			}
		}


		// Fill triangles
		if (geometryObject.HasMember("triangles") && geometryObject["triangles"].IsArray())
		{
			const rapidjson::Value& trianglesData = geometryObject["triangles"];
			m_triangles.reserve(trianglesData.Size()/3);
			int triangleIdx = 0;
			for (rapidjson::SizeType i = 0; i < (trianglesData.Size() - 2); i += 3)
			{
				if (trianglesData[i].IsInt() && trianglesData[i + 1].IsInt() && trianglesData[i + 2].IsInt())
				{
					int vertex0Idx = trianglesData[i].GetInt();
					int vertex1Idx = trianglesData[i+1].GetInt();
					int vertex2Idx = trianglesData[i+2].GetInt();

					Vertex& vertex0 = m_vertices[vertex0Idx];
					Vertex& vertex1 = m_vertices[vertex1Idx];
					Vertex& vertex2 = m_vertices[vertex2Idx];

					float3 v0 = vertex0.GetPosition();
					float3 v1 = vertex1.GetPosition();
					float3 v2 = vertex2.GetPosition();

					float3 normal = cross(normalize(v2 - v0), normalize(v1 - v0));

					m_triangles.push_back(Triangle(vertex0, vertex1, vertex2, normal));
					
					// Find centre of current triangle
					float3 centroidPos = (v0 + v1 + v2) * 0.333f;
					Vertex centroid = Vertex(centroidPos.x, centroidPos.y, centroidPos.z);

					// Find median of one triangle edge
					float3 edgeMedianPos = (v1 - v0) / 2;
					Vertex edgeMedianVertex = Vertex(edgeMedianPos.x, edgeMedianPos.y, edgeMedianPos.z);

					// Split triangle into four smaller ones
					m_quadingles.push_back(Triangle(vertex0, edgeMedianVertex, centroid, normal));
					m_quadingles.push_back(Triangle(edgeMedianVertex, vertex1, centroid, normal));
					m_quadingles.push_back(Triangle(vertex0, vertex2, centroid, normal));
					m_quadingles.push_back(Triangle(vertex1, vertex2, centroid, normal));

					// Add face index to each of each vertices' struct (for calculating smooth normals after that)
					vertex0.AddFace(triangleIdx);
					vertex1.AddFace(triangleIdx);
					vertex2.AddFace(triangleIdx);

					// Calculate edges for fast closed mesh calculation
					Edge edge1 = Edge(vertex0Idx, vertex1Idx);
					Edge edge2 = Edge(vertex0Idx, vertex2Idx);
					Edge edge3 = Edge(vertex1Idx, vertex2Idx);

					m_edges[edge1].push_back(triangleIdx);
					m_edges[edge2].push_back(triangleIdx);
					m_edges[edge3].push_back(triangleIdx);

					// Keep track of current triangle idx
					triangleIdx++;
				}
			}
		}
	}

	return true;
}

const std::vector<Triangle>& Parser::GetTriangles() const
{
	return m_triangles;
}

void Parser::CalculateVertexNormals()
{
	for (Vertex& vertex : m_vertices)
	{
		float3 averageNormal = float3(0);
		for (int faceIdx : vertex.faces)
		{
			averageNormal += m_triangles[faceIdx].normal;
		}
		averageNormal /= (int)vertex.faces.size();
		vertex.vn = averageNormal;
	}
}

void Parser::CalculateSmallestTriangleArea(const std::vector<Triangle>& triangles, int startIdx, int endIdx, int& triIdx, float& minArea) const
{
	for (int i = startIdx; i < endIdx; i++)
	{
		const Triangle& triangle = triangles[i];
		Vertex vertex0 = triangle.vertex0;
		Vertex vertex1 = triangle.vertex1;
		Vertex vertex2 = triangle.vertex2;
		float a = length(vertex2.GetPosition() - vertex0.GetPosition());
		float b = length(vertex1.GetPosition() - vertex0.GetPosition());
		float c = length(vertex2.GetPosition() - vertex1.GetPosition());
		float area = Area(a, b, c);
		if (area > 0 && area < minArea)
		{
			std::lock_guard<std::mutex> lock(mtx);
			minArea = area;
			triIdx = i;
		}
	}
}

void Parser::CalculateLargestTriangleArea(const std::vector<Triangle>& triangles, int startIdx, int endIdx, int& triIdx, float& maxArea) const
{
	for (int i = startIdx; i < endIdx; i++)
	{
		const Triangle& triangle = triangles[i];
		Vertex vertex0 = triangle.vertex0;
		Vertex vertex1 = triangle.vertex1;
		Vertex vertex2 = triangle.vertex2;
		float a = length(vertex2.GetPosition() - vertex0.GetPosition());
		float b = length(vertex1.GetPosition() - vertex0.GetPosition());
		float c = length(vertex2.GetPosition() - vertex1.GetPosition());
		float area = Area(a, b, c);
		if (area > 0 && area > maxArea)
		{
			std::lock_guard<std::mutex> lock(mtx);
			maxArea = area;
			triIdx = i;
		}
	}
}

void Parser::CalculateAverageTriangleArea(const std::vector<Triangle>& triangles, int startIdx, int endIdx, float& areaSum) const
{
	for (int i = startIdx; i < endIdx; i++)
	{
		const Triangle& triangle = triangles[i];
		Vertex vertex0 = triangle.vertex0;
		Vertex vertex1 = triangle.vertex1;
		Vertex vertex2 = triangle.vertex2;
		float a = length(vertex2.GetPosition() - vertex0.GetPosition());
		float b = length(vertex1.GetPosition() - vertex0.GetPosition());
		float c = length(vertex2.GetPosition() - vertex1.GetPosition());

		std::lock_guard<std::mutex> lock(mtx);
		areaSum += Area(a, b, c);
	}
}

float Parser::CalculateSmallestAreaMultithreaded() const
{
	const int num_threads = 2;
	std::vector<std::thread> threads;
	
	int trianglesCount = static_cast<int>(m_triangles.size());
	int splitStep = trianglesCount / num_threads;

	int triIdx = 0;
	float minArea = 1e34f;
	for (int i = 0; i < num_threads; i++)
	{
		int startIdx = i * splitStep;
		int endIdx = std::min(startIdx + splitStep, trianglesCount);
		threads.emplace_back(std::thread(&Parser::CalculateSmallestTriangleArea, this, std::ref(m_triangles), startIdx, endIdx, std::ref(triIdx), std::ref(minArea)));
	}
	
	for (std::thread& thread : threads)
	{
		if (thread.joinable())
		{ 
			thread.join();
		}
	}

	std::cout << "Multithread: Calculated smallest triangle index is " << triIdx << " and its area is " << minArea << std::endl;
	
	return minArea;
}

float Parser::CalculateLargestAreaMultithreaded() const
{
	const int num_threads = 2;
	std::vector<std::thread> threads;

	int trianglesCount = static_cast<int>(m_triangles.size());
	int splitStep = trianglesCount / num_threads;

	int triIdx = 0;
	float maxArea = -1e34f;
	for (int i = 0; i < num_threads; i++)
	{
		int startIdx = i * splitStep;
		int endIdx = std::min(startIdx + splitStep, trianglesCount);
		threads.emplace_back(std::thread(&Parser::CalculateLargestTriangleArea, this, std::ref(m_triangles), startIdx, endIdx, std::ref(triIdx), std::ref(maxArea)));
	}

	for (std::thread& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	std::cout << "Multithread: Calculated largest triangle index is " << triIdx << " and its area is " << maxArea << std::endl;
	
	return maxArea;
}

float Parser::CalculateAverageAreaMultithreaded() const
{
	const int num_threads = 2;
	std::vector<std::thread> threads;

	int trianglesCount = static_cast<int>(m_triangles.size());
	int splitStep = trianglesCount / num_threads;

	float areaSum = 0.f;
	for (int i = 0; i < num_threads; i++)
	{
		int startIdx = i * splitStep;
		int endIdx = std::min(startIdx + splitStep, trianglesCount);
		threads.emplace_back(std::thread(&Parser::CalculateAverageTriangleArea, this, std::ref(m_triangles), startIdx, endIdx, std::ref(areaSum)));
	}

	for (std::thread& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	std::cout << "Multithread: Average triangle area is " << areaSum / trianglesCount << std::endl;

	return areaSum / trianglesCount;
}

void Parser::CalculateSmallestAreaCompared() const
{
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	float minArea = 1e34f;
	int triIdx = -1;
	int startIdx = 0;
	int endIdx = static_cast<int>(m_triangles.size());
	CalculateSmallestTriangleArea(m_triangles, startIdx, endIdx, triIdx, minArea);

	std::cout << "Single thread: Calculated smallest triangle index is " << triIdx << " and its area is " << minArea << std::endl;

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on one thread = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
	begin = std::chrono::steady_clock::now();

	CalculateSmallestAreaMultithreaded();

	end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on two threads = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
}

void Parser::CalculateLargestAreaCompared() const
{
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	float maxArea = -1e34f;
	int triIdx = -1;
	int startIdx = 0;
	int endIdx = static_cast<int>(m_triangles.size());
	CalculateLargestTriangleArea(m_triangles, startIdx, endIdx, triIdx, maxArea);

	std::cout << "Single thread: Calculated largest triangle index is " << triIdx << " and its area is " << maxArea << std::endl;

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on one thread = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
	begin = std::chrono::steady_clock::now();

	CalculateLargestAreaMultithreaded();

	end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on two threads = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
}

void Parser::CalculateAverageAreaCompared() const
{
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	float areaSum = 0.f;
	int startIdx = 0;
	int trianglesCount = static_cast<int>(m_triangles.size());
	CalculateAverageTriangleArea(m_triangles, startIdx, trianglesCount, areaSum);

	std::cout << "Single thread: Average triangle area is " << areaSum / trianglesCount << std::endl;

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on one thread = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
	begin = std::chrono::steady_clock::now();

	CalculateAverageAreaMultithreaded();

	end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on two threads = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
}

bool Parser::IsClosedMesh() const
{
	for (auto& edge : m_edges)
	{
		if (edge.second.size() < 2)
			return false;
	}
	return true;
}

