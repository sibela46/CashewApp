#pragma once

class Parser
{
public:
	Parser();

public:
	bool ParseFile(const char* fileName);

	void CalculateVertexNormals();

	void CalculateSmallestTriangleArea(const std::vector<Triangle>& triangles, int startIdx, int endIdx, int& triIdx, float& minArea) const;
	void CalculateLargestTriangleArea(const std::vector<Triangle>& triangles, int startIdx, int endIdx, int& triIdx, float& maxArea) const;
	void CalculateAverageTriangleArea(const std::vector<Triangle>& triangles, int startIdx, int endIdx, float& areaSum) const;

	float CalculateSmallestAreaMultithreaded() const;
	float CalculateLargestAreaMultithreaded() const;
	float CalculateAverageAreaMultithreaded() const;

	void CalculateSmallestAreaCompared() const;
	void CalculateLargestAreaCompared() const;
	void CalculateAverageAreaCompared() const;

	bool IsClosedMesh() const;

	const std::vector<Triangle>& GetTriangles() const;

private:
	std::vector<Triangle> m_triangles;
	std::vector<Triangle> m_quadingles;
	std::vector<Vertex> m_vertices;
	std::map<Edge, std::vector<int>> m_edges;
};