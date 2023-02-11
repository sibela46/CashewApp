#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <map>

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

// constants
#define EPSILON			0.0000001
#define PI				3.14159265358979323846264f

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
inline float3 fminf(const float3& a, const float3& b) { return float3(fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z)); }
inline float3 fmaxf(const float3& a, const float3& b) { return float3(fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z)); }

// Colours
inline uint32_t ConvertToRGBA(const float3& colour)
{
	uint8_t r = colour.x * 255.0f;
	uint8_t g = colour.y * 255.0f;
	uint8_t b = colour.z * 255.0f;
	uint8_t a = 255.0f;
	uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
	return result;
}

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

// Classes and structs
class mat4
{
public:
	mat4() = default;
	float cell[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	float& operator [] (const int idx) { return cell[idx]; }
	float operator()(const int i, const int j) const { return cell[i * 4 + j]; }
	float& operator()(const int i, const int j) { return cell[i * 4 + j]; }
	mat4& operator += (const mat4& a)
	{
		for (int i = 0; i < 16; i++) cell[i] += a.cell[i];
		return *this;
	}
	bool operator==(const mat4& m)
	{
		for (int i = 0; i < 16; i++) if (m.cell[i] != cell[i]) return false; return true;
	}
	float3 GetTranslation() const { return float3(cell[3], cell[7], cell[11]); }
	static mat4 FromColumnMajor(const mat4& T)
	{
		mat4 M;
		M.cell[0] = T.cell[0], M.cell[1] = T.cell[4], M.cell[2] = T.cell[8], M.cell[3] = T.cell[12];
		M.cell[4] = T.cell[1], M.cell[5] = T.cell[5], M.cell[6] = T.cell[9], M.cell[7] = T.cell[13];
		M.cell[8] = T.cell[2], M.cell[9] = T.cell[6], M.cell[10] = T.cell[10], M.cell[11] = T.cell[14];
		M.cell[12] = T.cell[3], M.cell[13] = T.cell[7], M.cell[14] = T.cell[11], M.cell[15] = T.cell[15];
		return M;
	}
	constexpr static mat4 Identity() { return mat4{}; }
	static mat4 ZeroMatrix() { mat4 r; memset(r.cell, 0, 64); return r; }
	static mat4 RotateX(const float a) { mat4 r; r.cell[5] = cosf(a); r.cell[6] = -sinf(a); r.cell[9] = sinf(a); r.cell[10] = cosf(a); return r; };
	static mat4 RotateY(const float a) { mat4 r; r.cell[0] = cosf(a); r.cell[2] = sinf(a); r.cell[8] = -sinf(a); r.cell[10] = cosf(a); return r; };
	static mat4 RotateZ(const float a) { mat4 r; r.cell[0] = cosf(a); r.cell[1] = -sinf(a); r.cell[4] = sinf(a); r.cell[5] = cosf(a); return r; };
};

inline float3 operator*(const float3& b, const mat4& a) {
	return float3(a.cell[0] * b.x + a.cell[1] * b.y + a.cell[2] * b.z + a.cell[3] * 1.f,
		a.cell[4] * b.x + a.cell[5] * b.y + a.cell[6] * b.z + a.cell[7] * 1.f,
		a.cell[8] * b.x + a.cell[9] * b.y + a.cell[10] * b.z + a.cell[11] * 1.f);
}

class Ray
{
public:
	Ray() = default;
	Ray(float3 O, float3 D) : O(O), D(D), t(1e34f), hitObjIdx(-1) {};

public:
	float3 GetIntersectionPoint() { return O + t * D; }

public:
	float3 O, D;
	float t, u, v;
	int hitObjIdx;
};

struct ShadingInfo
{
	float3 albedo;

	ShadingInfo() = default;
	ShadingInfo(float3 albedo) : albedo(albedo) {};
};

struct Vertex
{
	float3 position;
	float3 normal;
	std::vector<int> faces;

	Vertex() = default;
	Vertex(float3 v) : position(v) {};

	const float3 GetPosition() const { return position; }
	void	AddFace(int faceIdx) { faces.push_back(faceIdx); }
};

struct Triangle
{
	int id;
	std::vector<float3> verticesPos;
	std::vector<int> verIndices;
	float3 normal, centroid;
	ShadingInfo shadingInfo;

	Triangle() = default;
	Triangle(int id, float3 v0, float3 v1, float3 v2, int vIdx1, int vIdx2, int vIdx3, float3 n)
		: id(id), normal(n)
	{
		verIndices.reserve(3);
		verIndices.push_back(vIdx1);
		verIndices.push_back(vIdx2);
		verIndices.push_back(vIdx3);
		verticesPos.reserve(3);
		verticesPos.push_back(v0);
		verticesPos.push_back(v1);
		verticesPos.push_back(v2);
		centroid = (v0 + v1 + v2) * 0.333f;
		shadingInfo = ShadingInfo(float3(1.f, 0.f, 1.f));
	};
	Triangle(int id, float3 v0, float3 v1, float3 v2, float3 n)
		: id(id), normal(n)
	{
		verticesPos.reserve(3);
		verticesPos.push_back(v0);
		verticesPos.push_back(v1);
		verticesPos.push_back(v2);
	};

	void Intersect(Ray& ray)
	{
		float3 v0 = verticesPos[0];
		float3 v1 = verticesPos[1];
		float3 v2 = verticesPos[2];
		float3 edge1 = v1 - v0;
		float3 edge2 = v2 - v0;
		float3 h = cross(ray.D, edge2);
		float a = dot(edge1, h);
		if (a > -EPSILON && a < EPSILON) return; // the ray is parallel to the triangle	
		float f = 1.0 / a;
		float3 s = ray.O - v0;
		float u = f * dot(s, h);
		if (u < 0.0 || u > 1.0) return;
		float3 q = cross(s, edge1);
		float v = f * dot(ray.D, q);
		if (v < 0.0 || u + v > 1.0) return;
		float t = f * dot(edge2, q);
		if (t > EPSILON && t < ray.t)
		{
			ray.t = t;
			ray.hitObjIdx = id;
			ray.u = u;
			ray.v = v;
			return;
		}
		return;
	}
};

struct Edge
{
	int vertexIdxA, vertexIdxB;

	Edge() = default;
	Edge(int vIdxA, int vIdxB) : vertexIdxA(vIdxA), vertexIdxB(vIdxB) {};

	bool const operator==(const Edge& o) const { return std::tie(vertexIdxA, vertexIdxB) == std::tie(o.vertexIdxA, o.vertexIdxB); }
	bool const operator<(const Edge& o) const { return std::tie(vertexIdxA, vertexIdxB) < std::tie(o.vertexIdxA, o.vertexIdxB); }
};

// Axis-aligned bounding box class
class AABB
{
public:
	AABB() = default;
	AABB(float3 a, float3 b) { bmin[0] = a.x, bmin[1] = a.y, bmin[2] = a.z, bmin[3] = 0, bmax[0] = b.x, bmax[1] = b.y, bmax[2] = b.z, bmax[3] = 0; }

	void Grow(const AABB& bb) {
		for (int i = 0; i < 4; i++)
		{
			bmin[i] = std::min(bmin[i], bb.bmin[i]);
			bmax[i] = std::max(bmax[i], bb.bmax[i]);
		}
	}
	void Grow(const float3& p)
	{ 
		float p4[4];
		p4[0] = p.x; p4[1] = p.y; p4[2] = p.z; p4[3] = 0.f;
		for (int i = 0; i < 4; i++)
		{
			bmin[i] = std::min(bmin[i], p4[i]);
			bmax[i] = std::max(bmax[i], p4[i]);
		}
	}

	float Area() const
	{
		float e[4];
		return std::max(0.0f, e[0] * e[1] + e[0] * e[2] + e[1] * e[2]);
	}
	public:
		float bmin[4], bmax[4];
};

#include "Parser.h"
#include "Renderer.h"
#include "Scene.h"
#include "Bvh.h"