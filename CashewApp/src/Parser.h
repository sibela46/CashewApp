#pragma once


class Parser
{
public:
	Parser();

public:
	bool ParseFile(const char* fileName, float scale, glm::vec3 colour);

	void CalculateVertexNormals();

	float CalculateArea(const Triangle& triangle) const;

	float CalculateSmallestTriangleArea() const;
	float CalculateLargestTriangleArea() const;
	float CalculateAverageTriangleArea() const;

	float CalculateSmallestAreaMultithreaded() const;
	float CalculateLargestAreaMultithreaded() const;
	float CalculateAverageAreaMultithreaded() const;

	float CalculateSmallestAreaCompared() const;
	float CalculateLargestAreaCompared() const;
	float CalculateAverageAreaCompared() const;

	bool IsClosedMesh() const;

	const std::vector<Triangle>& GetTriangles() const;
	const std::vector<Vertex>& GetVertices() const;

private:
	std::vector<Triangle> m_triangles;
	std::vector<Triangle> m_quadingles;
	std::vector<Vertex> m_vertices;
	std::map<Edge, std::vector<int>> m_edges;
};