#include "Pch.h"
#include "Geometry.h"

void Renderer::Geometry::GenerateCubeGeometry(std::vector<Vertex1Pos1UV1Norm>& outVertices, std::vector<uint32_t>& outIndices, const float width)
{
    const float halfWidth = width / 2.0f;

    outVertices =
    {
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, -halfWidth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,   halfWidth, -halfWidth), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth, -halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth, -halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth,  halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth, -halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth, -halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth, -halfWidth) , glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth, -halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, -halfWidth), glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, -halfWidth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,   halfWidth, -halfWidth), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,   halfWidth, -halfWidth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  halfWidth,  halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  halfWidth,  halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, halfWidth) , glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth,  halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth,  halfWidth) , glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    outIndices =
    {
        0,1,2,0,2,3,10,8,4,10,4,5,22,20,6,22,6,7,19,16,14,19,14,12,15,17,21,15,21,9,11,23,13,23,18,13
    };
}

void Renderer::Geometry::GenerateSphereGeometry(std::vector<Vertex1Pos1UV1Norm>& outVertices, std::vector<uint32_t>& outIndices,
    const float radius, const int32_t sectors, const int32_t stacks)
{
    constexpr float PI = glm::pi<float>();

    float sectorStep = 2.f * PI / static_cast<float>(sectors);
    float stackStep = PI / stacks;
    float sectorAngle = 0.f;
    float stackAngle = 0.f;

    float xy = 0.f;
    float lengthInv = 1.0f / radius;

    for (int i = stacks; i >= 0; i--)
    {
        stackAngle = PI / 2 - i * stackStep;
        xy = radius * glm::cos(stackAngle);

        for (int j = 0; j <= sectors; j++)
        {
            Renderer::Vertex1Pos1UV1Norm vert;

            sectorAngle = j * sectorStep;

            vert.Position.x = xy * glm::cos(sectorAngle);
            vert.Position.y = xy * glm::sin(sectorAngle);
            vert.Position.z = radius * glm::sin(stackAngle);

            vert.UV.x = static_cast<float>(j) / sectors;
            vert.UV.y = static_cast<float>(i) / stacks;

            vert.Normal.x = vert.Position.x * lengthInv;
            vert.Normal.y = vert.Position.y * lengthInv;
            vert.Normal.z = vert.Position.z * lengthInv;

            outVertices.push_back(vert);
        }
    }

    unsigned int k1 = 0;
    unsigned int k2 = 0;

    for (int i = 0; i < stacks; i++)
    {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; j++, k1++, k2++)
        {
            if (i != 0)
            {
                outIndices.push_back(k1);
                outIndices.push_back(k2);
                outIndices.push_back(k1 + 1);
            }

            if (i != (stacks - 1))
            {
                outIndices.push_back(k1 + 1);
                outIndices.push_back(k2);
                outIndices.push_back(k2 + 1);
            }
        }
    }

    std::reverse(outIndices.begin(), outIndices.end());
}
