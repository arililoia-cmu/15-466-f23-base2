#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("knob.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("knob.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
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

	for (auto &transform : scene.transforms) {
		std::cout << "transform.name: " << transform.name << std::endl;
		// if (transform.name == "Hip.FL") hip = &transform;
		if (transform.name == "Rotator") knob = &transform;
		else if (transform.name == "TopLeft") top_left = &transform;
		else if (transform.name == "TopRight") top_right = &transform;
		else if (transform.name == "BottomLeft") bottom_left = &transform;
		else if (transform.name == "BottomRight") bottom_right = &transform;
		

	}
	// if (hip == nullptr) throw std::runtime_error("Hip not found.");
	if (knob == nullptr) throw std::runtime_error("Knob not found.");
	// if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	// if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");
	knob_base_rotation = knob->rotation;
	knob_position = knob->position;

	tl_wpos = top_left->position;
	tr_wpos = top_right->position;
	bl_wpos = bottom_left->position;
	br_wpos = bottom_right->position;

	// std::cout << "knob_position.x,y,z: " << knob_position.x << " " << knob_position.y << " " << knob_position.z << std::endl; 
	// hip_base_rotation = hip->rotation;
	// upper_leg_base_rotation = upper_leg->rotation;
	// lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

int PlayMode::generate_random_angle(){
	// inspired by u/Walter's answer:
	// https://stackoverflow.com/questions/5008804/generating-a-random-integer-from-a-range
	std::random_device rd;     // Only used once to initialise (seed) engine
	std::mt19937 rng(rd());    // Random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(0,360); // Guaranteed unbiased
	auto random_integer = uni(rng);
	return random_integer;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} 
		else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			right.downs += 1;
			right.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			right.pressed = false;
			return true;
		}
	} else 
	if (evt.type == SDL_MOUSEBUTTONDOWN) {
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

	// return false;
	return false;
}

glm::vec2 PlayMode::calculate_ws_epos(glm::vec3 object_position){
	// code heavily inspired by this stackexchange post:
	// https://stackoverflow.com/questions/8491247/c-opengl-convert-world-coords-to-screen2d-coords
	// begins here:
	glm::vec4 objectpos(object_position.x, object_position.y, object_position.z, 1.0f);	
	glm::vec4 clip_space_pos = camera->make_projection() * glm::mat4(camera->transform->make_world_to_local()) * objectpos;
	// transform this position from clip-space to normalized device coordinate space
	glm::vec3 ndcSpacePos = glm::vec3(clip_space_pos.x, clip_space_pos.y, clip_space_pos.z)/clip_space_pos.w;
	// The next step is to transform from this [-1, 1] space to window-relative coordinate
	glm::vec2 windowSpacePos = (glm::vec2((ndcSpacePos.x+1.0)/2.0, (ndcSpacePos.y+1.0)/2.0)) * glm::vec2(1280, 720);
	// code taken from the above stackexchange post ends here
	return windowSpacePos;

}

void PlayMode::update(float elapsed) {

	// int window_width, window_height;
	// SDL_GetWindowSize(window, &window_width, &window_height);
	// int window_width = 1280;
	// int window_height = 720;

	// from this documentation:
	// https://wiki.libsdl.org/SDL2/SDL_GetMouseState


	tl_pos = calculate_ws_epos(tl_wpos);

	std::cout << "tl_pos (" << tl_pos[0] << ", " << tl_pos[1] << ")" << std::endl;

	int mouse_x, mouse_y;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	

	// // if we are moving the camera
	// if (SDL_GetRelativeMouseMode()){
	// 	std::cout << "tl_pos.x,y " << tl_pos.x << " " << tl_pos.y << std::endl; 
	// }
	// // if not
	// else{
	// 	std::cout << "mouse_x " << mouse_x << " mouse_y " << mouse_y << std::endl;
	// }


	// std::cout << knob_rotation << std::endl;
	// std::cout << "knob->position: " << knob->position << std::endl;
	// using this stack exchange post to experiment with ray casting:
	// https://stackoverflow.com/questions/40276068/opengl-raycasting-with-any-object		
	
	// float x = (2.0f * mouse_x) / window_width - 1.0f;
    // float y = 1.0f;
    // float z = 1.0f;
	// std::cout << "x " << x << " y " << y << " z " << z << std::endl;
	
	
	// wobble += elapsed * 0;
	if (left.pressed && !right.pressed){
		wobble += elapsed;
		std::cout << "left pressed" << std::endl;
		knob->rotation = knob_base_rotation *  glm::angleAxis(
		glm::radians(20.0f * wobble * 2.0f * float(M_PI)),
		glm::vec3(0.0f, 0.0f, 1.0f)
		);
	}
	if (!left.pressed && right.pressed){
		wobble -= elapsed;
		std::cout << "right pressed" << std::endl;
		knob->rotation = knob_base_rotation *  glm::angleAxis(
		glm::radians(20.0f * wobble * 2.0f * float(M_PI)),
		glm::vec3(0.0f, 0.0f, 1.0f)
		);
	}

	float angle = 2.0f * acos(knob->rotation.w); // Calculate the angle in radians
    // glm::vec3 axis = glm::normalize(glm::vec3(quaternion)); // Normalize the axis
	current_angle = (int)(glm::degrees(angle));
	

	if ((current_angle == goal_angle) && !left.pressed && !right.pressed){
		points++;
		goal_angle = generate_random_angle();
	}

	// //reset button press counters:
	// left.downs = 0;
	// right.downs = 0;
	// up.downs = 0;
	// down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	// string to stream conversion taken u/pb_overflow's response here:
	// https://www.experts-exchange.com/questions/21195748/itoa'-undeclared-first-use-this-function.html

	std::ostringstream o_ca;	
	o_ca << current_angle;
	std::string str_ca = o_ca.str();

	std::ostringstream o_ga;	
	o_ga << goal_angle;
	std::string str_ga = o_ga.str();

	std::ostringstream o_p;	
	o_p << points;
	std::string str_p = o_p.str();


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

		
		lines.draw_text(str_p + ": Current Angle = " + str_ca + ", Goal Angle = " + str_ga,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H*2, 0.0f, 0.0f), glm::vec3(0.0f, H*2, 0.0f),

			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		
		
	}
}
