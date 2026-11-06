#pragma once

#include <Utils/LeandroMath.h>
#include <EditorCore/BaseMesh.h>

struct WindowProperties;

class CollisionEngine {
public:
	static void UpdateAndCheckCollision(std::vector<BaseMesh*>& meshList);

	static bool CheckBBCollision(const BoundingBox& a, const BoundingBox& b);

	static void DrawDebugBoxes(const std::vector<BaseMesh*>& meshList, WindowProperties* mainWindow, bool usePerspective);

private:
	static void UpdateWorldBB(BaseMesh* mesh);
};
