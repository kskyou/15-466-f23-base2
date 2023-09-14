#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("real.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("real.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		//std::cout << transform.name << "\n";
		if (transform.name.substr(0, 4) == "Dirt"){
			dirt_trans.push_back(&transform);
		} else if (transform.name.substr(0, 4) == "Tree"){
			tree_trans.push_back(&transform);
		} else if (transform.name.substr(0, 4) == "Bush"){
			bush_trans.push_back(&transform);
		}
	}

	srand(time(NULL));
	auto randf = [] () {return static_cast<float> (rand()) / (static_cast<float> (RAND_MAX));};

	for (uint32_t i=0; i<1600; i++){
		dirt_trans[i]->position = glm::vec3((i / 40) * 1.0f + 0.5f, (i % 40) * 1.0f + 0.5f, 0.0f);
		int turn = 3 * (((i / 40) & 1) == 1) + (((i % 40) & 1) == 1) - 2 * (((i / 40) & 1) == 1) * (((i % 40) & 1) == 1);
		dirt_trans[i]->rotation = glm::angleAxis(glm::radians(-turn * 90.0f),glm::vec3(0.0f, 0.0f, 1.0f));
		content_grid[i] = 0;
	}

	std::vector<glm::vec2> biome_attractor;
	for (uint32_t k=0; k<16; k++){
		biome_attractor.push_back(glm::vec2(randf() * 40.0f, randf() * 40.0f)); 
		std::cout << biome_attractor[k].x << biome_attractor[k].y << "\n";
	}

	for (uint32_t i=0; i<200; i++){
		bool found_placement = false;
		for (uint32_t j=0; j<4; j++){
			glm::vec2 attempt = glm::vec2(randf() * 40.0f, randf() * 40.0f); 
			std::vector<std::pair<float, bool>> biome_distances;
			for (uint32_t k=0; k<16; k++){
				biome_distances.push_back(std::make_pair(glm::length(attempt - biome_attractor[k]), (k >= 10))); 
			}
			sort(biome_distances.begin(), biome_distances.end());
			int xgrid = floor(attempt.x);
			int ygrid = floor(attempt.y);
			if ((biome_distances[0].first && biome_distances[1].second) || randf() < 0.05f){
				if (content_grid[xgrid * 40 + ygrid] == 0){
					found_placement = true;
					content_grid[xgrid * 40 + ygrid] = 1;
					tree_trans[i]->position = glm::vec3(xgrid * 1.0f + 0.5f, ygrid * 1.0f + 0.5f, 2.0f);
					break;
				}
			}
		}
		if (!found_placement){
			std::cout << i << "\n";
			tree_trans[i]->position = glm::vec3(20.0f, 20.0f, -10.0f);
		}
	}

	for (uint32_t i=0; i<200; i++){
		bool found_placement = false;
		for (uint32_t j=0; j<4; j++){
			glm::vec2 attempt = glm::vec2(randf() * 40.0f, randf() * 40.0f); 
			std::vector<std::pair<float, bool>> biome_distances;
			for (uint32_t k=0; k<16; k++){
				biome_distances.push_back(std::make_pair(glm::length(attempt - biome_attractor[k]), (k >= 10))); 
			}
			sort(biome_distances.begin(), biome_distances.end());
			int xgrid = floor(attempt.x);
			int ygrid = floor(attempt.y);
			if (biome_distances[0].first || randf() < 0.05f){
				if (content_grid[xgrid * 40 + ygrid] == 0){
					found_placement = true;
					content_grid[xgrid * 40 + ygrid] = 1;
					bush_trans[i]->position = glm::vec3(xgrid * 1.0f + 0.5f, ygrid * 1.0f + 0.5f, 0.5f);
					break;
				}
			}
		}
		if (!found_placement){
			std::cout << i << "\n";
			bush_trans[i]->position = glm::vec3(20.0f, 20.0f, -10.0f);
		}
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
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
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
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
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
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
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):

	/*
	hip->rotation = // hip_base_rotation * 
		glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	upper_leg->rotation = // upper_leg_base_rotation * 
		glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = // lower_leg_base_rotation * 
		glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	*/
	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

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
}
