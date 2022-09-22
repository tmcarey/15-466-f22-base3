#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "Particle.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <array>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	bool DidPassLocation(float prevAngle, float newAngle, glm::vec3 position);

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, back, forward, up, down;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *sub = nullptr;
	Scene::Transform *sub2 = nullptr;
	glm::quat subRotation;
	Scene::Transform *allparent = nullptr;
	Scene::Transform *subparent = nullptr;
	Scene::Transform *camparent = nullptr;
	Scene::Transform *floor = nullptr;
	Scene::Transform *sonararm = nullptr;
	std::array<Particle*, 25> particles;
	float currentSonarAngle = 0.0f;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > leg_tip_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
