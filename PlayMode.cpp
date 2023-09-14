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
		if (transform.name.substr(0, 5) == "Dirt."){
			dirt_trans.push_back(&transform);
		} else if (transform.name.substr(0, 5) == "Tree."){
			tree_trans.push_back(&transform);
		} else if (transform.name.substr(0, 5) == "Bush."){
			bush_trans.push_back(&transform);
		} else if (transform.name.substr(0, 5) == "Tall."){
			tall_trans.push_back(&transform);
		} else if (transform.name.substr(0, 5) == "Flowe"){
			flower_trans.push_back(&transform);
		}
	}

	srand(time(NULL));
	auto randf = [] () {return static_cast<float> (rand()) / (static_cast<float> (RAND_MAX));};

	// Height generation via diamond-square algorithm
	std::vector<std::vector<float>> height(65, std::vector<float> (65, 0));
	height[0][0] = 1.5 * randf(); 
	height[0][64] = 1.5 * randf(); 
	height[64][0] =  1.5 * randf(); 
	height[64][64] = 1.5 * randf(); 

	int step = 64;
	int half = 32;
	float eps = 1.0f; 
	while (step >= 2){
		for (int i=0; i<64/step; i++){
			for (int j=0; j<64/step; j++){
				height[half+i*step][half+j*step] = (height[i*step][j*step] + height[(i+1)*step][j*step] + height[i*step][(j+1)*step] + height[(i+1)*step][(j+1)*step]) / 4.0f + (randf() - 0.5f) * eps; 
			}
		}
		for (int j=0; j<64/step; j++){
			height[half+j*step][0]= (height[j*step][0] + height[(j+1)*step][0] + height[half+j*step][half]) / 3.0f + (randf() - 0.5f) * eps; 
			height[half+j*step][64] = (height[j*step][64] + height[(j+1)*step][64] + height[half+j*step][64-half]) / 3.0f + (randf() - 0.5f) * eps; 
		}
		for (int j=0; j<64/step; j++){
			height[0][half+j*step] = (height[0][j*step] + height[0][(j+1)*step] + height[half][half+j*step]) / 3.0f + (randf() - 0.5f) * eps; 
			height[64][half+j*step] = (height[64][j*step] + height[64][(j+1)*step] + height[64-half][half+j*step]) / 3.0f + (randf() - 0.5f) * eps; 
		}
		for (int i=0; i<64/step-1; i++){
			for (int j=0; j<64/step; j++){
				height[half+half+i*step][half+j*step] = (height[half+i*step][half+j*step] + height[half+(i+1)*step][half+j*step] + height[half+half+i*step][j*step] + height[half+half+i*step][(j+1)*step]) / 4.0f + (randf() - 0.5f) * eps; 
			}
		}
		for (int i=0; i<64/step; i++){
			for (int j=0; j<64/step-1; j++){
				height[half+i*step][half+half+j*step] = (height[i*step][half+half+j*step] + height[(i+1)*step][half+half+j*step] + height[half+i*step][half+j*step] + height[half+i*step][half+(j+1)*step]) / 4.0f + (randf() - 0.5f) * eps; 
			}
		}
		step = step / 2;
		half = half / 2;
		eps = eps / 3;
	}
	for (int i=0; i<42; i++){
		for (int j=0; j<42; j++){
			if (i == 0 || j == 0 || i == 41 || j == 41){
				height_grid[42*i + j] = 30;
			} else {
				height_grid[42*i + j] = floor(15.0f * height[i+10][j+10]);
			}
		}
	}


	for (uint32_t i=0; i<1600; i++){
		dirt_trans[i]->position = glm::vec3((i / 40) * 1.0f + 0.5f, (i % 40) * 1.0f + 0.5f, 0.5f * height_grid[(i / 40 + 1) * 42 + (i % 40 + 1)] - 0.5f);
		int turn = 3 * (((i / 40) & 1) == 1) + (((i % 40) & 1) == 1) - 2 * (((i / 40) & 1) == 1) * (((i % 40) & 1) == 1);
		dirt_trans[i]->rotation = glm::angleAxis(glm::radians(-turn * 90.0f),glm::vec3(0.0f, 0.0f, 1.0f));
		content_grid[i] = 0;
	}


	// Funky biome generation I made
	std::vector<glm::vec2> biome_attractor;
	for (uint32_t k=0; k<16; k++){
		biome_attractor.push_back(glm::vec2(randf() * 40.0f, randf() * 40.0f)); 
	}


	for (uint32_t i=0; i<10; i++){
		bool found_placement = false;
		for (uint32_t j=0; j<20; j++){
			glm::vec2 attempt = glm::vec2(randf() * 40.0f, randf() * 40.0f); 
			std::vector<std::pair<float, bool>> biome_distances;
			for (uint32_t k=0; k<16; k++){
				biome_distances.push_back(std::make_pair(glm::length(attempt - biome_attractor[k]), (k >= 6))); 
			}
			sort(biome_distances.begin(), biome_distances.end());
			int xgrid = floor(attempt.x);
			int ygrid = floor(attempt.y);
			if (!biome_distances[0].second || !biome_distances[1].second  || randf() < 0.05f){
				if (content_grid[40 * xgrid + ygrid] == 0){
					found_placement = true;
					content_grid[40 * xgrid + ygrid] = 5;
					flower_trans[i]->position = glm::vec3(xgrid * 1.0f + 0.5f, ygrid * 1.0f + 0.5f, 0.1f + 0.5f * height_grid[42*(xgrid+1) + (ygrid+1)]);
					flower_trans[i]->rotation = glm::angleAxis(glm::radians((biome_distances[3].second) * 90.0f),glm::vec3(0.0f, 0.0f, 1.0f));
					break;
				}
			}
		}
		if (!found_placement){
			flower_trans[i]->position = glm::vec3(20.0f, 20.0f, -10.0f);
		}
	}

	for (uint32_t i=0; i<200; i++){
		bool found_placement = false;
		for (uint32_t j=0; j<4; j++){
			glm::vec2 attempt = glm::vec2(randf() * 40.0f, randf() * 40.0f); 
			std::vector<std::pair<float, bool>> biome_distances;
			for (uint32_t k=0; k<16; k++){
				biome_distances.push_back(std::make_pair(glm::length(attempt - biome_attractor[k]), (k >= 6))); 
			}
			sort(biome_distances.begin(), biome_distances.end());
			int xgrid = floor(attempt.x);
			int ygrid = floor(attempt.y);
			if ((biome_distances[0].second && biome_distances[1].second) || randf() < 0.05f){
				if (content_grid[40 * xgrid + ygrid] == 0){
					found_placement = true;
					content_grid[40 * xgrid + ygrid] = 1;
					tree_trans[i]->position = glm::vec3(xgrid * 1.0f + 0.5f, ygrid * 1.0f + 0.5f, 1.0f + 0.5f * biome_distances[2].second + 0.5f * height_grid[42*(xgrid+1) + (ygrid+1)]);
					break;
				}
			}
		}
		if (!found_placement){
			tree_trans[i]->position = glm::vec3(20.0f, 20.0f, -10.0f);
		}
	}

	for (uint32_t i=0; i<200; i++){
		bool found_placement = false;
		for (uint32_t j=0; j<4; j++){
			glm::vec2 attempt = glm::vec2(randf() * 40.0f, randf() * 40.0f); 
			std::vector<std::pair<float, bool>> biome_distances;
			for (uint32_t k=0; k<16; k++){
				biome_distances.push_back(std::make_pair(glm::length(attempt - biome_attractor[k]), (k >= 6))); 
			}
			sort(biome_distances.begin(), biome_distances.end());
			int xgrid = floor(attempt.x);
			int ygrid = floor(attempt.y);
			if (biome_distances[0].second || randf() < 0.05f){
				if (content_grid[40 * xgrid + ygrid] == 0){
					found_placement = true;
					content_grid[40 * xgrid + ygrid] = 2;
					bush_trans[i]->position = glm::vec3(xgrid * 1.0f + 0.5f, ygrid * 1.0f + 0.5f, 0.5f * height_grid[42*(xgrid+1) + (ygrid+1)]);
					break;
				}
			}
		}
		if (!found_placement){
			bush_trans[i]->position = glm::vec3(20.0f, 20.0f, -10.0f);
		}
	}

	for (uint32_t i=0; i<200; i++){
		bool found_placement = false;
		for (uint32_t j=0; j<4; j++){
			glm::vec2 attempt = glm::vec2(randf() * 40.0f, randf() * 40.0f); 
			std::vector<std::pair<float, bool>> biome_distances;
			for (uint32_t k=0; k<16; k++){
				biome_distances.push_back(std::make_pair(glm::length(attempt - biome_attractor[k]), (k >= 6))); 
			}
			sort(biome_distances.begin(), biome_distances.end());
			int xgrid = floor(attempt.x);
			int ygrid = floor(attempt.y);
			if (!biome_distances[0].second || !biome_distances[1].second  || randf() < 0.05f){
				if (content_grid[40 * xgrid + ygrid] == 0){
					found_placement = true;
					content_grid[40 * xgrid + ygrid] = 3;
					tall_trans[i]->position = glm::vec3(xgrid * 1.0f + 0.5f, ygrid * 1.0f + 0.5f, 0.5f * height_grid[42*(xgrid+1) + (ygrid+1)]);
					tall_trans[i]->rotation = glm::angleAxis(glm::radians((biome_distances[2].second) * 90.0f),glm::vec3(0.0f, 0.0f, 1.0f));
					break;
				}
			}
		}
		if (!found_placement){
			tall_trans[i]->position = glm::vec3(20.0f, 20.0f, -10.0f);
		}
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
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
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_f) {
			ef.downs += 1;
			ef.pressed = true;
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
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_f) {
			ef.pressed = false;
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
		constexpr float PlayerSpeed = 3.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x = -1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_forward = -frame[2];

		glm::vec3 player_move = move.x * frame_right + move.y * frame_forward;
		player_move.z = 0;
		if (player_move != glm::vec3(0.0f)) player_move = glm::normalize(player_move) * PlayerSpeed;

		z_vel -= 9.8 * elapsed;
		int xgrid = floor(player_pos.x);
		int ygrid = floor(player_pos.y);

		float tile_height = height_grid[42*(xgrid+1)+(ygrid+1)] * 0.5f;
		if (player_pos.z < tile_height){
			z_vel = 0;
		}
		if (player_pos.z < tile_height - 0.05f){
			if (player_pos.x - float(xgrid) < 0.1f){
				player_move.x = (player_move.x > 0) ? 0 : player_move.x;
			} else if (player_pos.x - float(xgrid) > 0.9f){
				player_move.x = (player_move.x < 0) ? 0 : player_move.x;
			} 
			if (player_pos.y - float(ygrid) < 0.1f){
				player_move.y = (player_move.y > 0) ? 0 : player_move.y;
			} else if (player_pos.y - float(ygrid) > 0.9f){
				player_move.y = (player_move.y < 0) ? 0 : player_move.y;
			} 
		}

		if (space.pressed && player_pos.z <tile_height + 0.1f && z_vel < 0.1f){
			z_vel = 3.0f;
		}

		if (ef.pressed){
			for (uint32_t i=0; i<10; i++){
				if (glm::length(player_pos - flower_trans[i]->position) < 1.0f) {
					found ++;
					flower_trans[i]->position = glm::vec3(20.0f, 20.0f, -10.0f);
				}
			}
		}
		

		player_pos += (player_move + glm::vec3(0.0f, 0.0f, z_vel)) * elapsed;

		//std::cout << player_pos.x << " " << player_pos.y << "\n";

		camera->transform->position = player_pos + glm::vec3(0.0f, 0.0f, 1.5f);
		// move.x * frame_right + move.y * frame_forward;
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
		//lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
		//	glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
		//	glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
		//	glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Use WASD, space and mouse to move, and F to pick flowers. You have found " + std::to_string(found) + "/10 flowers",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

	}
}
