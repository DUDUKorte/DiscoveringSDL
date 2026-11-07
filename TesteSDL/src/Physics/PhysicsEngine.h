#pragma once

#include <Utils/LeandroMath.h>
#include <Physics/BoundingBox.h>
#include <EditorCore/BaseMesh.h>

struct WindowProperties;

class PhysicsEngine {
public:
	static void UpdateAndCheckCollision(const std::vector<BaseMesh*>& meshList);

	static void ResolvePhysics(const std::vector<BaseMesh*>& meshList);
	
	static CollisionHit* CheckBBCollision(const BoundingBox& a, const BoundingBox& b);

	static void DrawDebugBoxes(const std::vector<BaseMesh*>& meshList, const WindowProperties* mainWindow, bool usePerspective);

	static void UpdateWorldBB(BaseMesh* mesh);
	
private:
};
