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
	//get pointers to leg for convenience:
	// for (auto &transform : scene.transforms) {
	// 	if (transform.name == "Hip.FL") hip = &transform;
	// 	else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
	// 	else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	// }
	for (auto &transform : scene.transforms) {
		std::cout << "transform.name: " << transform.name << std::endl;
		// if (transform.name == "Hip.FL") hip = &transform;
		if (transform.name == "Rotator") knob = &transform;
	}
	// if (hip == nullptr) throw std::runtime_error("Hip not found.");
	if (knob == nullptr) throw std::runtime_error("Knob not found.");
	// if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	// if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");
	knob_rotation = knob->rotation;
	knob_position = knob->position;
	std::cout << "knob_position.x,y,z: " << knob_position.x << " " << knob_position.y << " " << knob_position.z << std::endl; 
	// hip_base_rotation = hip->rotation;
	// upper_leg_base_rotation = upper_leg->rotation;
	// lower_leg_base_rotation = lower_leg->rotation;

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
		} 
	// else if (evt.key.keysym.sym == SDLK_a) {
	// 		left.downs += 1;
	// 		left.pressed = true;
	// 		return true;
	// 	} else if (evt.key.keysym.sym == SDLK_d) {
	// 		right.downs += 1;
	// 		right.pressed = true;
	// 		return true;
	// 	} else if (evt.key.keysym.sym == SDLK_w) {
	// 		up.downs += 1;
	// 		up.pressed = true;
	// 		return true;
	// 	} else if (evt.key.keysym.sym == SDLK_s) {
	// 		down.downs += 1;
	// 		down.pressed = true;
	// 		return true;
	// 	}
	// } else if (evt.type == SDL_KEYUP) {
	// 	if (evt.key.keysym.sym == SDLK_a) {
	// 		left.pressed = false;
	// 		return true;
	// 	} else if (evt.key.keysym.sym == SDLK_d) {
	// 		right.pressed = false;
	// 		return true;
	// 	} else if (evt.key.keysym.sym == SDLK_w) {
	// 		up.pressed = false;
	// 		return true;
	// 	} else if (evt.key.keysym.sym == SDLK_s) {
	// 		down.pressed = false;
	// 		return true;
	// 	}
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

void PlayMode::update(float elapsed) {

	// int window_width, window_height;
	// SDL_GetWindowSize(window, &window_width, &window_height);
	// int window_width = 1280;
	// int window_height = 720;

	// from this documentation:
	// https://wiki.libsdl.org/SDL2/SDL_GetMouseState

	// code taken from this stackexchange post:
	// https://stackoverflow.com/questions/8491247/c-opengl-convert-world-coords-to-screen2d-coords
	// begins here:
	glm::vec4 objectpos(knob_position.x, knob_position.y, knob_position.z, 1.0f);	
	glm::vec4 clip_space_pos = camera->make_projection() * glm::mat4(camera->transform->make_world_to_local()) * objectpos;
	// transform this position from clip-space to normalized device coordinate space
	glm::vec3 ndcSpacePos = glm::vec3(clip_space_pos.x, clip_space_pos.y, clip_space_pos.z)/clip_space_pos.w;
	// The next step is to transform from this [-1, 1] space to window-relative coordinate
	glm::vec2 windowSpacePos = (glm::vec2((ndcSpacePos.x+1.0)/2.0, (ndcSpacePos.y+1.0)/2.0)) * glm::vec2(1280, 720);
	// code taken from the above stackexchange post ends here

	for (int i = 0; i < 2; ++i) {
       std::cout << windowSpacePos[i] << std::endl;
    }

	// mygl_Position = OBJECT_TO_CLIP * objectpos
	// for (int i = 0; i < 4; ++i) {
    //    std::cout << mygl_Position[i] << std::endl;
    // }
	//  std::cout << "====" << std::endl;
	// for (int i = 0; i < 4; ++i) {
    //  	for (int j = 0; j < 4; ++j) {
	// 		std::cout << OBJECT_TO_CLIP_mat4[i][j] << std::endl;
    // 	}
    // }

	// for (auto const &drawable : drawables) {
	// 	assert(drawable.transform);
	// }

	int mouse_x, mouse_y;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	

	// if we are moving the camera
	if (SDL_GetRelativeMouseMode()){
		std::cout << "knob_position.x,y,z: " << knob_position.x << " " << knob_position.y << " " << knob_position.z << std::endl; 
	}
	// if not
	else{
		std::cout << "mouse_x " << mouse_x << " mouse_y " << mouse_y << std::endl;
	}
	
	// std::cout << knob_rotation << std::endl;
	// std::cout << "knob->position: " << knob->position << std::endl;
	// using this stack exchange post to experiment with ray casting:
	// https://stackoverflow.com/questions/40276068/opengl-raycasting-with-any-object		
	
	// float x = (2.0f * mouse_x) / window_width - 1.0f;
    // float y = 1.0f;
    // float z = 1.0f;
	// std::cout << "x " << x << " y " << y << " z " << z << std::endl;
	
	// //slowly rotates through [0,1):
	// wobble += elapsed / 10.0f;
	// wobble -= std::floor(wobble);

	// hip->rotation = hip_base_rotation * glm::angleAxis(
	// 	glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 1.0f, 0.0f)
	// );
	// upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
	// 	glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 0.0f, 1.0f)
	// );
	// lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
	// 	glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
	// 	glm::vec3(0.0f, 0.0f, 1.0f)
	// );

	// //move camera:
	// {

	// 	//combine inputs into a move:
	// 	constexpr float PlayerSpeed = 30.0f;
	// 	glm::vec2 move = glm::vec2(0.0f);
	// 	if (left.pressed && !right.pressed) move.x =-1.0f;
	// 	if (!left.pressed && right.pressed) move.x = 1.0f;
	// 	if (down.pressed && !up.pressed) move.y =-1.0f;
	// 	if (!down.pressed && up.pressed) move.y = 1.0f;

	// 	//make it so that moving diagonally doesn't go faster:
	// 	if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

	// 	glm::mat4x3 frame = camera->transform->make_local_to_parent();
	// 	glm::vec3 frame_right = frame[0];
	// 	//glm::vec3 up = frame[1];
	// 	glm::vec3 frame_forward = -frame[2];

	// 	camera->transform->position += move.x * frame_right + move.y * frame_forward;
	// }

	// //reset button press counters:
	// left.downs = 0;
	// right.downs = 0;
	// up.downs = 0;
	// down.downs = 0;
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
