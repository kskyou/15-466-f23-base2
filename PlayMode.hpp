#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	std::vector<Scene::Transform*> dirt_trans;
	std::vector<Scene::Transform*> tree_trans;
	std::vector<Scene::Transform*> bush_trans;

	std::array<int, 1764> height_grid;
	std::array<int, 1600> content_grid ;

	glm::vec3 player_pos = glm::vec3(20.5f, 20.5f, 15.0f);
	float z_vel = 0.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
