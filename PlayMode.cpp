#include "PlayMode.hpp"

#include "GL.hpp"
#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
GLuint hexapod_meshes_for_unlit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("sub.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	hexapod_meshes_for_unlit_color_texture_program = ret->make_vao_for_program(color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("sub.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		if(transform->name == "SonarParent"
				|| transform->name == "SonarArm"){
			drawable.pipeline = color_texture_program_pipeline;
			drawable.pipeline.vao = hexapod_meshes_for_unlit_color_texture_program;
		}else{
			drawable.pipeline = lit_color_texture_program_pipeline;
			drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		}

		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "SubParent") subparent = &transform;
		if (transform.name == "Sub") sub = &transform;
		if (transform.name == "Floor") floor = &transform;
		if (transform.name == "SonarArm") sonararm = &transform;
	}
	if (sub == nullptr) throw std::runtime_error("sub not found.");
	if (subparent == nullptr) throw std::runtime_error("subparent not found.");
	if (floor == nullptr) throw std::runtime_error("floor not found.");
	if (sonararm == nullptr) throw std::runtime_error("sonararm not found.");

	subRotation = sub->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, glm::vec3(0), 10.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			forward.downs += 1;
			forward.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			back.downs += 1;
			back.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_q) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			forward.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			back.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_q) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			/* glm::vec2 motion = glm::vec2( */
			/* 	evt.motion.xrel / float(window_size.y), */
			/* 	-evt.motion.yrel / float(window_size.y) */
			/* ); */
			/* camera->transform->rotation = glm::normalize( */
			/* 	camera->transform->rotation */
			/* 	* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f)) */
			/* 	* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f)) */
			/* ); */
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	//move sound to follow leg tip position:
	//
	//floor->position = glm::vec3(sub->position.x, sub->position.y, 0.0f);

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (back.pressed && !up.pressed) move.y =-1.0f;
		if (!back.pressed && forward.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = subparent->make_local_to_world();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		subparent->position += move.y * -frame_right * elapsed * 1000.0f;
		subparent->rotation = glm::normalize(
			subparent->rotation
			* glm::angleAxis(-move.x * 10.0f * elapsed, glm::vec3(0.0f, 0.0f, 1.0f))
		);

		if(subparent->position.z <= 1.2f){
			subparent->position.z = 1.2f;
		}

		float sonarSpeed = 1.0f;

		sonararm->rotation = glm::normalize(
			sonararm->rotation
			* glm::angleAxis(sonarSpeed * elapsed, glm::vec3(1.0f, 0.0f, 0.0f))
		);

		float newSonarAngle = currentSonarAngle + sonarSpeed * elapsed;
		if(newSonarAngle >= 2 * glm::pi<float>()){
			newSonarAngle -= 2 * glm::pi<float>();
		}

		if(currentSonarAngle > newSonarAngle){
			// we crossed 0
			printf("crossed 0 angle\n");
		}

		currentSonarAngle = newSonarAngle;

		if(up.pressed && down.pressed){
		}else
		if(up.pressed) {
			subparent->position += glm::vec3(0, 0, 1) * elapsed * 10.0f;
			sub->rotation = glm::normalize(
				subRotation
				* glm::angleAxis(-20.0f, glm::vec3(0.0f, 1.0f, 0.0f))
			);
		}else if(down.pressed){
			subparent->position -= glm::vec3(0, 0, 1) * elapsed * 10.0f;
			sub->rotation = glm::normalize(
				subRotation
				* glm::angleAxis(20.0f, glm::vec3(0.0f, 1.0f, 0.0f))
			);
		}else{
			sub->rotation = glm::normalize(
				subRotation
			);
		}
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	forward.downs = 0;
	back.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	glm::vec4 fog_color(0, 0, 0.6f, 1.0f);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUniform4fv(lit_color_texture_program->FOG_COLOR_vec4, 1, glm::value_ptr(fog_color));
	glUseProgram(0);

	glClearColor(fog_color.x, fog_color.y, fog_color.z, fog_color.w);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}
