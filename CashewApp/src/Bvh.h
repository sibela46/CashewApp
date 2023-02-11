#pragma once
#define BINS 100

// 32-bytes BVHNode, half a cache line - pure beauty
struct BVHNode
{
    float3 aabbMin, aabbMax;
    int leftFirst, triCount;
    bool isLeaf() { return triCount > 0; };
};

struct Bin { AABB bounds; int priCount = 0; };

class Bvh
{
public:
	Bvh();

    void BuildBVH(std::vector<Triangle> tri);

    void Subdivide(int nodeIdx);
    void UpdateNodeBounds(int nodeIdx);

    void IntersectBVH(Ray& ray, const int nodeIdx);
    bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax);

private:
    int N = 0;
    int nodesUsed = 1;
    static const int m_rootNodeIdx = 0;
    BVHNode* m_BvhNodes;
    std::vector<Triangle> m_triangles;
    std::vector<int> m_triIndices;
};