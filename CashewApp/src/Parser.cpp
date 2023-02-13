#include "utils.h"
#include <execution>

std::mutex mtx;

Parser::Parser()
{

}

bool Parser::ParseFile(const char* fileName, float scale, glm::vec3 colour)
{
	m_triangles.clear();
	m_vertices.clear();
	m_edges.clear();
	m_quadingles.clear();

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

	glm::mat4 rotateX = glm::mat4(1.0f);
	rotateX = glm::scale(rotateX, glm::vec3(scale));
	rotateX = glm::rotate(rotateX, PI / 2, glm::vec3(1, 0, 0));

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
					m_vertices.push_back(Vertex(glm::vec3(x, y, z)));
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

					glm::vec3 v0 = glm::vec4(vertex0.GetPosition(), 1.f) * rotateX;
					glm::vec3 v1 = glm::vec4(vertex1.GetPosition(), 1.f) * rotateX;
					glm::vec3 v2 = glm::vec4(vertex2.GetPosition(), 1.f) * rotateX;

					// Add face index to each of each vertices' struct (for calculating smooth normals after that)
					vertex0.AddFace(triangleIdx);
					vertex1.AddFace(triangleIdx);
					vertex2.AddFace(triangleIdx);

					glm::vec3 normal = cross(normalize(v2 - v0), normalize(v1 - v0));

					Triangle newTriangle = Triangle(triangleIdx, v0, v1, v2, vertex0Idx, vertex1Idx, vertex2Idx, normal, colour);
					m_triangles.push_back(newTriangle);
					
					// Find median of one triangle edge
					glm::vec3 edgeMedian = (v1 - v0) / glm::vec3(2);

					// Split triangle into four smaller ones
					m_quadingles.push_back(Triangle(triangleIdx, v0, edgeMedian, newTriangle.centroid, normal));
					m_quadingles.push_back(Triangle(triangleIdx +1, edgeMedian, v1, newTriangle.centroid, normal));
					m_quadingles.push_back(Triangle(triangleIdx +2, v0, v2, newTriangle.centroid, normal));
					m_quadingles.push_back(Triangle(triangleIdx +3, v1, v2, newTriangle.centroid, normal));

					// Calculate edges for fast closed mesh calculation
					Edge edge = Edge(vertex0Idx, vertex1Idx);
					Edge edgeSwap = Edge(vertex1Idx, vertex0Idx);
					auto itEdge = m_edges.find(edge);
					auto itEdgeSwap = m_edges.find(edgeSwap);

					if (itEdge == m_edges.end() && itEdgeSwap == m_edges.end())
						m_edges[edge].push_back(triangleIdx);
					else if (itEdge != m_edges.end())
						itEdge->second.push_back(triangleIdx);
					else if (itEdgeSwap != m_edges.end())
						itEdgeSwap->second.push_back(triangleIdx);

					edge = Edge(vertex0Idx, vertex2Idx);
					edgeSwap = Edge(vertex2Idx, vertex0Idx);
					itEdge = m_edges.find(edge);
					itEdgeSwap = m_edges.find(edgeSwap);

					if (itEdge == m_edges.end() && itEdgeSwap == m_edges.end())
						m_edges[edge].push_back(triangleIdx);
					else if (itEdge != m_edges.end())
						itEdge->second.push_back(triangleIdx);
					else if (itEdgeSwap != m_edges.end())
						itEdgeSwap->second.push_back(triangleIdx);

					edge = Edge(vertex1Idx, vertex2Idx);
					edgeSwap = Edge(vertex2Idx, vertex1Idx);
					itEdge = m_edges.find(edge);
					itEdgeSwap = m_edges.find(edgeSwap);

					if (itEdge == m_edges.end() && itEdgeSwap == m_edges.end())
						m_edges[edge].push_back(triangleIdx);
					else if (itEdge != m_edges.end())
						itEdge->second.push_back(triangleIdx);
					else if (itEdgeSwap != m_edges.end())
						itEdgeSwap->second.push_back(triangleIdx);
						
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

const std::vector<Vertex>& Parser::GetVertices() const
{
	return m_vertices;
}

void Parser::CalculateVertexNormals()
{
	for (Vertex& vertex : m_vertices)
	{
		glm::vec3 averageNormal = glm::vec3(0);
		for (int faceIdx : vertex.faces)
		{
			averageNormal += m_triangles[faceIdx].normal;
		}
		averageNormal /= (int)vertex.faces.size();
		vertex.normal = averageNormal;
	}
}

float Parser::CalculateArea(const Triangle& triangle) const
{
	glm::vec3 v0 = triangle.verticesPos[0];
	glm::vec3 v1 = triangle.verticesPos[1];
	glm::vec3 v2 = triangle.verticesPos[2];
	float a = length(v2 - v0);
	float b = length(v1 - v0);
	float c = length(v2 - v1);
	return Area(a, b, c);
}

float Parser::CalculateSmallestTriangleArea() const
{
	float minArea = 1e34f;
	int triIdx = -1;
	for (const auto& triangle : m_triangles)
	{
		float area = CalculateArea(triangle);
		if (area > 0 && area < minArea)
		{
			minArea = area;
			triIdx = triangle.id;
		}
	}

	std::cout << "Single thread: Calculated smallest triangle index is " << triIdx << " and its area is " << minArea << std::endl;

	return minArea;
}

float Parser::CalculateLargestTriangleArea() const
{
	float maxArea = -1e34f;
	int triIdx = -1;
	for (const auto& triangle : m_triangles)
	{
		float area = CalculateArea(triangle);
		if (area > 0 && area > maxArea)
		{
			maxArea = area;
			triIdx = triangle.id;
		}
	}

	std::cout << "Single thread: Calculated largest triangle index is " << triIdx << " and its area is " << maxArea << std::endl;

	return maxArea;
}

float Parser::CalculateAverageTriangleArea() const
{
	float areaSum = 0.f;
	for (const auto& triangle : m_triangles)
	{
		areaSum += CalculateArea(triangle);
	}

	areaSum /= static_cast<int>(m_triangles.size());

	std::cout << "Single thread: Average triangle area is " << areaSum << std::endl;

	return areaSum;
}

float Parser::CalculateSmallestAreaMultithreaded() const
{
	int trianglesCount = static_cast<int>(m_triangles.size());

	int triIdx = 0;
	float minArea = 1e34f;
	std::for_each(std::execution::par, m_triangles.begin(), m_triangles.end(),
		[this, &triIdx, &minArea](const Triangle& triangle)
		{
			float area = CalculateArea(triangle);
			if (area > 0 && area < minArea)
			{
				minArea = area;
				triIdx = triangle.id;
			}
		});

	std::cout << "Multithread: Calculated smallest triangle index is " << triIdx << " and its area is " << minArea << std::endl;
	
	return minArea;
}

float Parser::CalculateLargestAreaMultithreaded() const
{
	int trianglesCount = static_cast<int>(m_triangles.size());

	int triIdx = 0;
	float maxArea = -1e34f;
	std::for_each(std::execution::par, m_triangles.begin(), m_triangles.end(),
		[this, &triIdx, &maxArea](const Triangle& triangle)
		{
			float area = CalculateArea(triangle);
			if (area > 0 && area > maxArea)
			{
				maxArea = area;
				triIdx = triangle.id;
			}
		});

	std::cout << "Multithread: Calculated largest triangle index is " << triIdx << " and its area is " << maxArea << std::endl;
	
	return maxArea;
}

float Parser::CalculateAverageAreaMultithreaded() const
{
	int trianglesCount = static_cast<int>(m_triangles.size());

#ifdef OWN_MULTI_THREADING
	const int num_threads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	threads.resize(num_threads);

	float areaSum = 0.f;
	int splitStep = trianglesCount / num_threads;
	for (int i = 0; i < num_threads; i++)
	{
		threads.emplace_back(std::thread([this, i, splitStep, &areaSum]()
			{
				int startIdx = i * splitStep;
				int endIdx = startIdx + splitStep;
				for (int i = startIdx; i < endIdx; i++)
				{
					std::lock_guard<std::mutex> lock(mtx);
					const Triangle& triangle = m_triangles[i];
					areaSum += CalculateArea(triangle);
				}
			}));
	}

	for (auto& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	float averageArea = areaSum / trianglesCount;
	std::cout << "Multithread: Average triangle area is " << averageArea << std::endl;

#else
	std::atomic<float> areaSum{ 0.f };

	std::for_each(std::execution::par, m_triangles.begin(), m_triangles.end(),
		[this, &areaSum](const Triangle& triangle)
		{
			auto current = areaSum.load();
			while (!areaSum.compare_exchange_weak(current, current + CalculateArea(triangle)));
		});

	float averageArea = areaSum / trianglesCount;

	std::cout << "Multithread: Average triangle area is " << averageArea << std::endl;

#endif // OWN_MULTI_THREADING

	return averageArea;
}

float Parser::CalculateSmallestAreaCompared() const
{
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	CalculateSmallestTriangleArea();

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on one thread = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
	
	begin = std::chrono::steady_clock::now();

	float area = CalculateSmallestAreaMultithreaded();

	end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on all available threads = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
	
	return area;
}

float Parser::CalculateLargestAreaCompared() const
{
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	CalculateLargestTriangleArea();

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on one thread = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
	begin = std::chrono::steady_clock::now();

	float area = CalculateLargestAreaMultithreaded();

	end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on all available threads = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
	
	return area;
}

float Parser::CalculateAverageAreaCompared() const
{
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	CalculateAverageTriangleArea();

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on one thread = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
	begin = std::chrono::steady_clock::now();

	float area = CalculateAverageAreaMultithreaded();

	end = std::chrono::steady_clock::now();

	std::cout << "Time for execution on all available threads = " << (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) / 1000000.0 << "s" << std::endl;
	
	return area;
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

