#include "utils.h"

Bvh::Bvh()
{

}

void Bvh::BuildBVH(std::vector<Triangle> triangles)
{
    N = triangles.size();
    m_triangles = triangles;
    m_triIndices.resize(N);
    m_BvhNodes = (BVHNode*)_aligned_malloc(sizeof(BVHNode) * N * 2, 64);

    for (int i = 0; i < N; i++) m_triIndices[i] = i;

    // Assign all triangles to root node
    BVHNode& root = m_BvhNodes[m_rootNodeIdx];
    root.leftFirst = 0;
    root.triCount = N;
    nodesUsed = 1;
    UpdateNodeBounds(m_rootNodeIdx);

    // Subdivide recursively
    Subdivide(m_rootNodeIdx);
}

void Bvh::Subdivide(int nodeIdx)
{
    // terminate recursion
    BVHNode& node = m_BvhNodes[nodeIdx];
    if (node.triCount <= 2) return;
    // determine split axis and position
    glm::vec3 extent = node.aabbMax - node.aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
    // in-place partition
    int i = node.leftFirst;
    int j = i + node.triCount - 1;
    while (i <= j)
    {
        if (m_triangles[m_triIndices[i]].centroid[axis] < splitPos)
            i++;
        else
            std::swap(m_triIndices[i], m_triIndices[j--]);
    }
    // abort split if one of the sides is empty
    int leftCount = i - node.leftFirst;
    if (leftCount == 0 || leftCount == node.triCount) return;
    // create child nodes
    int leftChildIdx = nodesUsed++;
    int rightChildIdx = nodesUsed++;
    m_BvhNodes[leftChildIdx].leftFirst = node.leftFirst;
    m_BvhNodes[leftChildIdx].triCount = leftCount;
    m_BvhNodes[rightChildIdx].leftFirst = i;
    m_BvhNodes[rightChildIdx].triCount = node.triCount - leftCount;
    node.leftFirst = leftChildIdx;
    node.triCount = 0;
    UpdateNodeBounds(leftChildIdx);
    UpdateNodeBounds(rightChildIdx);
    // recurse
    Subdivide(leftChildIdx);
    Subdivide(rightChildIdx);
}

void Bvh::UpdateNodeBounds(int nodeIdx)
{
    BVHNode& node = m_BvhNodes[nodeIdx];
    node.aabbMin = glm::vec3(1e30f);
    node.aabbMax = glm::vec3(-1e30f);
    for (int first = node.leftFirst, i = 0; i < node.triCount; i++)
    {
        int leafTriIdx = m_triIndices[first + i];
        Triangle& leafTri = m_triangles[leafTriIdx];
        node.aabbMin = fminf(node.aabbMin, leafTri.verticesPos[0]),
        node.aabbMin = fminf(node.aabbMin, leafTri.verticesPos[1]),
        node.aabbMin = fminf(node.aabbMin, leafTri.verticesPos[2]),
        node.aabbMax = fmaxf(node.aabbMax, leafTri.verticesPos[0]),
        node.aabbMax = fmaxf(node.aabbMax, leafTri.verticesPos[1]),
        node.aabbMax = fmaxf(node.aabbMax, leafTri.verticesPos[2]);
    }
}

void Bvh::IntersectBVH(Ray& ray, const int nodeIdx)
{
    BVHNode& node = m_BvhNodes[nodeIdx];
    if (!IntersectAABB(ray, node.aabbMin, node.aabbMax)) return;
    if (node.isLeaf())
    {
        for (int i = 0; i < node.triCount; i++)
            m_triangles[m_triIndices[node.leftFirst + i]].Intersect(ray);
    }
    else
    {
        IntersectBVH(ray, node.leftFirst);
        IntersectBVH(ray, node.leftFirst + 1);
    }
}

bool Bvh::IntersectAABB(const Ray& ray, const glm::vec3 bmin, const glm::vec3 bmax)
{
    float tx1 = (bmin.x - ray.O.x) / ray.D.x, tx2 = (bmax.x - ray.O.x) / ray.D.x;
    float tmin = std::min(tx1, tx2), tmax = std::max(tx1, tx2);
    float ty1 = (bmin.y - ray.O.y) / ray.D.y, ty2 = (bmax.y - ray.O.y) / ray.D.y;
    tmin = std::max(tmin, std::min(ty1, ty2)), tmax = std::min(tmax, std::max(ty1, ty2));
    float tz1 = (bmin.z - ray.O.z) / ray.D.z, tz2 = (bmax.z - ray.O.z) / ray.D.z;
    tmin = std::max(tmin, std::min(tz1, tz2)), tmax = std::min(tmax, std::max(tz1, tz2));
    return tmax >= tmin && tmin < ray.t&& tmax > 0;
}