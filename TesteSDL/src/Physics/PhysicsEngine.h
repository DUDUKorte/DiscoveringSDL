#pragma once

#include <Physics/BoundingBox.h>
#include <EditorCore/BaseMesh.h>

struct WindowProperties;

class PhysicsEngine {
public:
	static void UpdateAndCheckCollision(const std::vector<BaseMesh*>& meshList);

	static void ResolvePhysics(const std::vector<BaseMesh*>& meshList);
	
	static RaycastHit Raycast(glm::vec3 origin, glm::vec3 direction, float maxDist, BaseMesh* ignoreMesh, bool drawDebug = false);
	
	static CollisionHit* CheckBBCollision(const BoundingBox& a, const BoundingBox& b);

	static void DrawRayDebug(glm::vec3 origin, glm::vec3 direction, float maxDist, const RaycastHit& hit, const WindowProperties* mainWindow, bool usePerspective);
	
	static void DrawDebugBoxes(const std::vector<BaseMesh*>& meshList, const WindowProperties* mainWindow, bool usePerspective);
	
	static void UpdateWorldBB(BaseMesh* mesh);
	
private:
};
