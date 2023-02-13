#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "document.h"
#include "filereadstream.h"

// constants
#define EPSILON			0.0000001
#define PI				3.14159265358979323846264f

// Colours
inline uint32_t ConvertToRGBA(const glm::vec3& colour)
{
	uint8_t r = colour.x * 255.0f;
	uint8_t g = colour.y * 255.0f;
	uint8_t b = colour.z * 255.0f;
	uint8_t a = 255.0f;
	uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
	return result;
}

inline glm::vec3 fminf(const glm::vec3& a, const glm::vec3& b) { return glm::vec3(fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z)); }
inline glm::vec3 fmaxf(const glm::vec3& a, const glm::vec3& b) { return glm::vec3(fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z)); }
inline float Area(float a, float b, float c)
{
	float inv2 = (1.f / 2.f);
	float s = (a + b + c) * inv2;
	return sqrtf(s * (s - a) * (s - b) * (s - c));
}


// Classes and structs
class Ray
{
public:
	Ray() : t(1e34f), hitObjIdx(-1) {};
	Ray(glm::vec3 O, glm::vec3 D) : O(O), D(D), t(1e34f), hitObjIdx(-1) {};

public:
	glm::vec3 GetIntersectionPoint() { return O + t * D; }

public:
	glm::vec3 O, D;
	glm::vec3 faceNormal;
	float t, u, v;
	int hitObjIdx;
};

struct ShadingInfo
{
	glm::vec3 albedo;

	ShadingInfo() = default;
	ShadingInfo(glm::vec3 albedo) : albedo(albedo) {};
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	std::vector<int> faces;

	Vertex() = default;
	Vertex(glm::vec3 v) : position(v) {};

	const glm::vec3 GetPosition() const { return position; }
	void	AddFace(int faceIdx) { faces.push_back(faceIdx); }
};

struct Triangle
{
	int id;
	std::vector<glm::vec3> verticesPos;
	std::vector<int> verIndices;
	glm::vec3 normal, centroid;
	glm::vec3 colour;

	Triangle() = default;
	Triangle(int id, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, int vIdx1, int vIdx2, int vIdx3, glm::vec3 n, glm::vec3 colour)
		: id(id), normal(n), colour(colour)
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
	};
	Triangle(int id, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 n)
		: id(id), normal(n)
	{
		verticesPos.reserve(3);
		verticesPos.push_back(v0);
		verticesPos.push_back(v1);
		verticesPos.push_back(v2);
	};

	void Intersect(Ray& ray)
	{
		glm::vec3 v0 = verticesPos[0];
		glm::vec3 v1 = verticesPos[1];
		glm::vec3 v2 = verticesPos[2];
		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		glm::vec3 h = glm::cross(ray.D, edge2);
		float a = glm::dot(edge1, h);
		if (a > -EPSILON && a < EPSILON) return; // the ray is parallel to the triangle	
		float f = 1.0 / a;
		glm::vec3 s = ray.O - v0;
		float u = f * glm::dot(s, h);
		if (u < 0.0 || u > 1.0) return;
		glm::vec3 q = glm::cross(s, edge1);
		float v = f * glm::dot(ray.D, q);
		if (v < 0.0 || u + v > 1.0) return;
		float t = f * glm::dot(edge2, q);
		if (t > EPSILON && t < ray.t)
		{
			ray.t = t;
			ray.hitObjIdx = id;
			ray.u = u;
			ray.v = v;
			ray.faceNormal = normal;
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
	AABB(glm::vec3 a, glm::vec3 b) { bmin[0] = a.x, bmin[1] = a.y, bmin[2] = a.z, bmin[3] = 0, bmax[0] = b.x, bmax[1] = b.y, bmax[2] = b.z, bmax[3] = 0; }

	void Grow(const AABB& bb) {
		for (int i = 0; i < 4; i++)
		{
			bmin[i] = std::min(bmin[i], bb.bmin[i]);
			bmax[i] = std::max(bmax[i], bb.bmax[i]);
		}
	}
	void Grow(const glm::vec3& p)
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
#include "Camera.h"
#include "Scene.h"
#include "Bvh.h"