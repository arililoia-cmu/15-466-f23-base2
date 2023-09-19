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
	glm::vec2 calculate_ws_epos(glm::vec3 object_position);
	glm::vec2 calculate_ws_epos2(Scene::Transform *object);
	bool check_if_inside(glm::uvec2 check, glm::uvec2 point1, glm::uvec2 point2, glm::uvec2 point3, glm::uvec2 point4);
	int generate_random_angle();

	//----- game state -----


	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;


	// knob stuff
	Scene::Transform *knob = nullptr;

	Scene::Transform *top_left = nullptr;
	Scene::Transform *top_right = nullptr;
	Scene::Transform *bottom_left = nullptr;
	Scene::Transform *bottom_right = nullptr;


	// Scene::Drawable *top_left_drawable = nullptr;
	// Scene::Drawable *top_right_drawable = nullptr;
	// Scene::Drawable *bottom_left_drawable = nullptr;
	// Scene::Drawable *bottom_right_drawable = nullptr;


	glm::quat knob_base_rotation;
	glm::vec3 knob_position;

	glm::vec3 tl_wpos;
	glm::vec3 tr_wpos;
	glm::vec3 bl_wpos;
	glm::vec3 br_wpos;

	glm::vec2 tl_pos;
	glm::vec2 tr_pos;
	glm::vec2 bl_pos;
	glm::vec2 br_pos;

	int goal_angle = 100;
	int current_angle;
	int points = 0;

	float wobble = 0.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
