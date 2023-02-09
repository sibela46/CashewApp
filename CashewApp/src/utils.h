#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <map>

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

struct float3
{
	float3() = default;
	float3(const float a, const float b, const float c) : x(a), y(b), z(c) {}
	float3(const float a) : x(a), y(a), z(a) {}
	union { struct { float x, y, z; }; float cell[3]; };
	float& operator [] (const int n) { return cell[n]; }
};

inline float3 operator*(const float3& a, float b) { return float3(a.x * b, a.y * b, a.z * b); }
inline float3 operator*(float b, const float3& a) { return float3(b * a.x, b * a.y, b * a.z); }
inline float3 operator+(const float3& a, const float3& b) { return float3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline float3 operator-(const float3& a, const float3& b) { return float3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline float3 operator/(float3& a, int b) { return float3(a.x / b, a.y / b, a.z / b); }
inline bool	  operator==(const float3& a, const float3& b) { return std::tie(a.x, a.y, a.z) == std::tie(b.x, b.y, b.z); }
inline bool	  operator<(const float3 & a, const float3 & b) { return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z); }
inline bool	  operator!=(const float3& a, const float3& b) { return a.x != b.x && a.y != b.y && a.z != b.z; }
inline void	  operator+=(float3& a, float3& b) { a.x += b.x; a.y += b.y; a.z += b.z; }
inline void	  operator/=(float3& a, int b) { a.x /= b; a.y /= b; a.z /= b; }

struct Vertex
{
	float v0, v1, v2;
	float3 vn;
	std::vector<int> faces;

	Vertex() = default;
	Vertex(float v0, float v1, float v2) : v0(v0), v1(v1), v2(v2) {};

	float3	GetPosition() { return float3(v0, v1, v2); }
	void	AddFace(int faceIdx) { faces.push_back(faceIdx); }
};

struct Triangle
{
	Vertex vertex0, vertex1, vertex2;
	float3 normal;

	Triangle() = default;
	Triangle(Vertex v0, Vertex v1, Vertex v2, float3 n) : vertex0(v0), vertex1(v1), vertex2(v2), normal(n) {};
};

struct Edge
{
	int vertexIdxA, vertexIdxB;

	Edge() = default;
	Edge(int vIdxA, int vIdxB) : vertexIdxA(vIdxA), vertexIdxB(vIdxB) {};

	bool const operator==(const Edge& o) const { return std::tie(vertexIdxA, vertexIdxB) == std::tie(o.vertexIdxA, o.vertexIdxB); }
	bool const operator<(const Edge& o) const { return std::tie(vertexIdxA, vertexIdxB) < std::tie(o.vertexIdxA, o.vertexIdxB); }
};

// Algebra
inline float rsqrtf(float x) { return 1.0f / sqrtf(x); }
inline float Area(float a, float b, float c)
{
	float inv2 = (1.f / 2.f);
	float s = (a + b + c) * inv2;
	return sqrtf(s * (s - a) * (s - b) * (s - c));
}
// Linear Algebra
inline float dot(const float3& a, const float3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float length(const float3& v) { return sqrtf(dot(v, v)); }
inline float3 cross(const float3& a, const float3& b) { return float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
inline float3 normalize(const float3& v) { float invLen = rsqrtf(dot(v, v)); return v * invLen; }
