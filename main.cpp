#include "load_save_png.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <chrono>
#include <iostream>
#include <fstream>

static GLuint compile_shader(GLenum type, std::string const &source);
static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader);

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game1: Text/Tiles";
		glm::uvec2 size = glm::uvec2(640, 480);
	} config;

	//------------  initialization ------------

	//Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);

	//Ask for an OpenGL context version 3.3, core profile, enable debug:
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	//create window:
	SDL_Window *window = SDL_CreateWindow(
		config.title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		config.size.x, config.size.y,
		SDL_WINDOW_OPENGL /*| SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI*/
	);

	if (!window) {
		std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
		return 1;
	}

	//Create OpenGL context:
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!context) {
		SDL_DestroyWindow(window);
		std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
		return 1;
	}

	#ifdef _WIN32
	//On windows, load OpenGL extensions:
	if (!init_gl_shims()) {
		std::cerr << "ERROR: failed to initialize shims." << std::endl;
		return 1;
	}
	#endif

	//Set VSYNC + Late Swap (prevents crazy FPS):
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		std::cerr << "NOTE: couldn't set vsync + late swap tearing (" << SDL_GetError() << ")." << std::endl;
		if (SDL_GL_SetSwapInterval(1) != 0) {
			std::cerr << "NOTE: couldn't set vsync (" << SDL_GetError() << ")." << std::endl;
		}
	}

	//Hide mouse cursor (note: showing can be useful for debugging):
	SDL_ShowCursor(SDL_DISABLE);

	//------------ opengl objects / game assets ------------

	//texture:
	GLuint tex = 0;
	glm::uvec2 tex_size = glm::uvec2(0, 0);

	{ //load texture 'tex':
		std::vector< uint32_t > data;
		if (!load_png("assets.png", &tex_size.x, &tex_size.y, &data, LowerLeftOrigin)) {
			std::cerr << "Failed to load texture." << std::endl;
			exit(1);
		}
		//create a texture object:
		glGenTextures(1, &tex);
		//bind texture object to GL_TEXTURE_2D:
		glBindTexture(GL_TEXTURE_2D, tex);
		//upload texture data from data:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_size.x, tex_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		//set texture sampling parameters:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	//shader program:
	GLuint program = 0;
	GLuint program_Position = 0;
	GLuint program_TexCoord = 0;
	GLuint program_Color = 0;
	GLuint program_mvp = 0;
	GLuint program_tex = 0;
	{ //compile shader program:
		GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER,
			"#version 330\n"
			"uniform mat4 mvp;\n"
			"in vec4 Position;\n"
			"in vec2 TexCoord;\n"
			"in vec4 Color;\n"
			"out vec2 texCoord;\n"
			"out vec4 color;\n"
			"void main() {\n"
			"	gl_Position = mvp * Position;\n"
			"	color = Color;\n"
			"	texCoord = TexCoord;\n"
			"}\n"
		);

		GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER,
			"#version 330\n"
			"uniform sampler2D tex;\n"
			"in vec4 color;\n"
			"in vec2 texCoord;\n"
			"out vec4 fragColor;\n"
			"void main() {\n"
			"	fragColor = texture(tex, texCoord) * color;\n"
			"}\n"
		);

		program = link_program(fragment_shader, vertex_shader);

		//look up attribute locations:
		program_Position = glGetAttribLocation(program, "Position");
		if (program_Position == -1U) throw std::runtime_error("no attribute named Position");
		program_TexCoord = glGetAttribLocation(program, "TexCoord");
		if (program_TexCoord == -1U) throw std::runtime_error("no attribute named TexCoord");
		program_Color = glGetAttribLocation(program, "Color");
		if (program_Color == -1U) throw std::runtime_error("no attribute named Color");

		//look up uniform locations:
		program_mvp = glGetUniformLocation(program, "mvp");
		if (program_mvp == -1U) throw std::runtime_error("no uniform named mvp");
		program_tex = glGetUniformLocation(program, "tex");
		if (program_tex == -1U) throw std::runtime_error("no uniform named tex");
	}

	//vertex buffer:
	GLuint buffer = 0;
	{ //create vertex buffer
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
	}

	struct Vertex {
		Vertex(glm::vec2 const &Position_, glm::vec2 const &TexCoord_, glm::u8vec4 const &Color_) :
			Position(Position_), TexCoord(TexCoord_), Color(Color_) { }
		glm::vec2 Position;
		glm::vec2 TexCoord;
		glm::u8vec4 Color;
	};
	static_assert(sizeof(Vertex) == 20, "Vertex is nicely packed.");

	//vertex array object:
	GLuint vao = 0;
	{ //create vao and set up binding:
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glVertexAttribPointer(program_Position, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLbyte *)0);
		glVertexAttribPointer(program_TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLbyte *)0 + sizeof(glm::vec2));
		glVertexAttribPointer(program_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLbyte *)0 + sizeof(glm::vec2) + sizeof(glm::vec2));
		glEnableVertexAttribArray(program_Position);
		glEnableVertexAttribArray(program_TexCoord);
		glEnableVertexAttribArray(program_Color);
	}

	//------------ sprite info ------------
	struct SpriteInfo {
		int object_id;
		glm::vec2 origin = glm::vec2(0.0, 0.0);
		glm::vec2 min_uv = glm::vec2(0.0, 0.0);
		glm::vec2 max_uv = glm::vec2(0.0, 0.0);
	};


	//----------- game objects -----------
	//The idea I think is to make a vector of objects
	//make a grid of positions on the map
	//A occupancy value will tell if the tile can be traversed
	struct Object {
		glm::vec2 pos;
		std::vector<SpriteInfo>::iterator sprite;
	};

	struct Tile {
		glm::vec2 pos;
		bool occupied; //traversal value
		Object* sprite; //Texture
		Object* object; //the object that is occupying this tile
	};


	Tile tiles[100][100]; //background tiles
	
	//------- SpriteInfo loader ----------
	struct Header {
		int text_size_x;
		int text_size_y;
		int num_textures;
	};

	Header header;

	std::vector<SpriteInfo> sprites;

	std::ifstream asset_locations;

	asset_locations.open("textures.blob", std::ios::in | std::ios::binary);
	asset_locations.read(reinterpret_cast<char *> (&header), sizeof(header));
	if (!asset_locations.good()) {
		throw std::runtime_error("Unable to open textures");
	}

	sprites.resize(header.num_textures);
	if (!asset_locations.read(reinterpret_cast< char * >(&sprites[0]), sprites.size() * sizeof(SpriteInfo))) {
		throw std::runtime_error("Failed to read chunk data.");
	}

	//---------- objects -----------------
	Object player_down;
	Object player_right;
	Object player_left;
	Object player_up;
	Object floor;
	Object wall;
	Object wire_vert;
	Object wire_hori;
	Object wire_up_right;
	Object wire_up_left;
	Object wire_down_right;
	Object wire_down_left;
	Object sweeper;

	for (auto it = sprites.begin(); it < sprites.end(); it++) {
		switch(it->object_id) {
		case 1: 
			player_down.sprite = it;
			break;
		case 2:
			player_right.sprite = it;
			break;
		case 3:
			player_left.sprite = it;
			break;
		case 4:
			player_up.sprite = it;
			break;
		case 11:
			floor.sprite = it;
			break;
		case 12:
			wall.sprite = it;
			break;
		}
	}

	for (int i = 0; i < 100; i++) {
		for (int j = 0; j < 100; j++) {
			tiles[i][j].sprite = &floor;
			tiles[i][j].pos = glm::vec2(i,j);
		}
	}

	Tile player;
	player.sprite = &player_down;

	player.pos = glm::vec2(5.0f, 5.0f);
	

	//------------ game state ------------

	struct {
		glm::vec2 at = glm::vec2(5.0f, 5.0f);
		glm::vec2 radius = glm::vec2(15.0f, 15.0f);
	} camera;
	//correct radius for aspect ratio:
	camera.radius.x = camera.radius.y * (float(config.size.x) / float(config.size.y));

	//------------ game loop ------------

	bool should_quit = false;
	while (true) {
		static SDL_Event evt;
		while (SDL_PollEvent(&evt) == 1) {
			//handle input:
			if (evt.type == SDL_KEYDOWN) {
				switch(evt.key.keysym.sym) {
				case SDLK_ESCAPE:
					should_quit = true;
					break;
				case SDLK_UP:
					camera.at.y += 1;
					player.pos.y += 1;
					player.sprite = &player_up;
					break;
				case SDLK_RIGHT:
					camera.at.x += 1;
					player.pos.x += 1;
					player.sprite = &player_right;
					break;
				case SDLK_LEFT:
					camera.at.x -= 1;
					player.pos.x -= 1;
					player.sprite = &player_left;
					break;
				case SDLK_DOWN:
					camera.at.y -= 1;
					player.pos.y -= 1;
					player.sprite = &player_down;
					break;
				}
			} else if (evt.type == SDL_QUIT) {
				should_quit = true;
				break;
			}
		}
		if (should_quit) break;

		auto current_time = std::chrono::high_resolution_clock::now();
		static auto previous_time = current_time;
		float elapsed = std::chrono::duration< float >(current_time - previous_time).count();
		previous_time = current_time;

		{ //update game state:
			(void)elapsed;
		}

		//draw output:
		glClearColor(0.5, 0.5, 0.5, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


		{ //draw game state:
			std::vector< Vertex > verts;

			auto draw_sprite = [&verts, &header](SpriteInfo const &sprite, glm::vec2 const &at) {
				glm::vec2 min_uv;
				min_uv.x = sprite.min_uv.x / header.text_size_x;
				min_uv.y = 1.0f - sprite.min_uv.y / header.text_size_y;
				//TODO: max_uv needs to be filled in
				glm::vec2 max_uv;
				max_uv.x = sprite.max_uv.x / header.text_size_x;
				max_uv.y = 1.0f - sprite.max_uv.y / header.text_size_y;
				//glm::vec2 max_uv = glm::vec2(1.0, 1.0);
				//TODO: change how the rad is still 0, 0
				//glm::vec2 rad = sprite.rad;
				glm::vec2 top;
				glm::vec2 bottom;
				bottom.x = at.x - (sprite.origin.x - sprite.min_uv.x) / 8;
				bottom.y = at.y - (sprite.max_uv.y - sprite.origin.y) / 8;
				top.x = at.x + (sprite.max_uv.x - sprite.origin.x) / 8;
				top.y = at.y + (sprite.origin.y - sprite.min_uv.y) / 8;
				
				glm::u8vec4 tint = glm::u8vec4(0xff, 0xff, 0xff, 0xff);
				verts.emplace_back(glm::vec2(bottom.x,bottom.y), glm::vec2(min_uv.x, max_uv.y), tint);
				verts.emplace_back(verts.back());
				verts.emplace_back(glm::vec2(bottom.x, top.y), glm::vec2(min_uv.x, min_uv.y), tint);
				verts.emplace_back(glm::vec2(top.x,bottom.y), glm::vec2(max_uv.x, max_uv.y), tint);
				verts.emplace_back(glm::vec2(top.x, top.y), glm::vec2(max_uv.x, min_uv.y), tint);
				verts.emplace_back(verts.back());
			};


			//Draw a sprite "player" at position (5.0, 2.0):
			//stddatic SpriteInfo player; //TODO: hoist
			//draw_sprite(player, glm::vec2(0.5, 0.5));
			
			for (int i = 0; i < 100; i++) {
				for (int j = 0; j < 100; j++) {
					draw_sprite(*tiles[i][j].sprite->sprite, glm::vec2(i, j));
				}
			}
			
			//draw_sprite(*tiles[0][0].sprite->sprite, glm::vec2(0, 0));
			draw_sprite(*player.sprite->sprite, player.pos);
			
			//rect(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f), glm::u8vec4(0xff, 0x00, 0x00, 0xff));
			//rect(mouse * camera.radius + camera.at, glm::vec2(1.0f, 1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));


			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * verts.size(), &verts[0], GL_STREAM_DRAW);

			glUseProgram(program);
			glUniform1i(program_tex, 0);
			glm::vec2 scale = 1.0f / camera.radius;
			glm::vec2 offset = scale * -camera.at;
			glm::mat4 mvp = glm::mat4(
				glm::vec4(scale.x, 0.0f, 0.0f, 0.0f),
				glm::vec4(0.0f, scale.y, 0.0f, 0.0f),
				glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
				glm::vec4(offset.x, offset.y, 0.0f, 1.0f)
			);
			glUniformMatrix4fv(program_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

			glBindTexture(GL_TEXTURE_2D, tex);
			glBindVertexArray(vao);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, verts.size());
		}


		SDL_GL_SwapWindow(window);
	}


	//------------  teardown ------------

	SDL_GL_DeleteContext(context);
	context = 0;

	SDL_DestroyWindow(window);
	window = NULL;

	return 0;
}



static GLuint compile_shader(GLenum type, std::string const &source) {
	GLuint shader = glCreateShader(type);
	GLchar const *str = source.c_str();
	GLint length = source.size();
	glShaderSource(shader, 1, &str, &length);
	glCompileShader(shader);
	GLint compile_status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE) {
		std::cerr << "Failed to compile shader." << std::endl;
		GLint info_log_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
		std::vector< GLchar > info_log(info_log_length, 0);
		GLsizei length = 0;
		glGetShaderInfoLog(shader, info_log.size(), &length, &info_log[0]);
		std::cerr << "Info log: " << std::string(info_log.begin(), info_log.begin() + length);
		glDeleteShader(shader);
		throw std::runtime_error("Failed to compile shader.");
	}
	return shader;
}

static GLuint link_program(GLuint fragment_shader, GLuint vertex_shader) {
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	GLint link_status = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &link_status);
	if (link_status != GL_TRUE) {
		std::cerr << "Failed to link shader program." << std::endl;
		GLint info_log_length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
		std::vector< GLchar > info_log(info_log_length, 0);
		GLsizei length = 0;
		glGetProgramInfoLog(program, info_log.size(), &length, &info_log[0]);
		std::cerr << "Info log: " << std::string(info_log.begin(), info_log.begin() + length);
		throw std::runtime_error("Failed to link program");
	}
	return program;
}
