#include "Scene.hpp"

struct Goal {
	bool isGoal;
	Scene::Transform *transform;
	bool isCollected = false;
};
