#pragma once

#include <EditorCore/BaseMesh.h>

struct WindowProperties;

class CollisionEngine {
public:
	static void UpdateAndCheckCollision(const std::vector<BaseMesh*>& meshList);

	static bool CheckBBCollision(const BoundingBox& a, const BoundingBox& b);

	static void DrawDebugBoxes(const std::vector<BaseMesh*>& meshList, const WindowProperties* mainWindow, bool usePerspective);

private:
	static void UpdateWorldBB(BaseMesh* mesh);
};
