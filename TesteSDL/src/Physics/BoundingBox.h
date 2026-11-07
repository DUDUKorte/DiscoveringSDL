#pragma once
#include <Utils/LeandroMath.h>

class BaseMesh; // n tira isso daqui pelo amor de deus

// TODO: Implementar depois, esses canais não significam basicamente nada por enquanto
enum CollisionChannel { WorldStatic = 0, PlayerMesh, TriggerMesh };

struct RaycastHit{
    bool outHit = false;
    float distance = 0.0f;
    glm::vec3 impactPoint;
    glm::vec3 normal;
    BaseMesh* other;
};

struct CollisionHit {
    BaseMesh* other;
    glm::vec3 normal;
    float penetration;
    CollisionChannel channel;
};

struct BoundingBox {
    glm::vec3 localMin = { -0.5f, -0.5f, -0.5f };
    glm::vec3 localMax = { 0.5f, 0.5f, 0.5f };

    glm::vec3 worldMin = { 0.0f, 0.0f, 0.0f };
    glm::vec3 worldMax = { 0.0f, 0.0f, 0.0f };

    glm::vec3 offset = { 0.0f, 0.0f, 0.0f };
    glm::vec3 sizeScale = { 1.0f, 1.0f, 1.0f };

    glm::vec3 localCenter = { 0.0f, 0.0f, 0.0f };
    glm::vec3 worldCenter = { 0.0f, 0.0f, 0.0f };

    // axes X eY e Z
    Axis axes;
    glm::vec3 obbPoints[8] = {};
    glm::vec3 halfExtends = { 0.0f, 0.0f, 0.0f };

    bool ShowDebug = true;
    CollisionChannel Channel = WorldStatic;
    std::vector<CollisionHit> OutHits;
};