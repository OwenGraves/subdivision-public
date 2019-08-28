#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <random>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// OpenGL library includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <debuggl.h>
#include "subdivision.h"
#include "camera.h"

using namespace std::chrono;
using namespace std;
using namespace glm;

int window_width = 800, window_height = 600;

// VBO and VAO descriptors.
enum { kVertexBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos][kNumVbos];  // These will store VBO descriptors.

const char* vertex_shader =
        R"zzz(#version 430 core
in vec4 vertex_position;
uniform mat4 view;
uniform vec4 light_position;
out vec4 vs_light;
out vec4 world_position;
void main()
{
    world_position = vertex_position;
	gl_Position = view * vertex_position;
	vs_light = -gl_Position + view * light_position;
}
)zzz";

const char* geometry_shader =
        R"zzz(#version 430 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 viewi;
in vec4 world_position[];
in vec4 vs_light[];
flat out vec4 normal;
out vec4 light_direction;
out vec4 world;
out vec3 bary;
void main()
{

	int n = 0;
    vec4 side1 = viewi * gl_in[1].gl_Position - viewi * gl_in[0].gl_Position;
    vec4 side2 = viewi * gl_in[2].gl_Position - viewi * gl_in[0].gl_Position;
    normal = vec4(normalize(cross(side1.xyz,side2.xyz)), 1.0f);
	for (n = 0; n < gl_in.length(); n++) {
		bary = vec3(n == 0, n == 1, n == 2);
        world = world_position[n];
		light_direction = viewi * vs_light[n];
		gl_Position = projection * gl_in[n].gl_Position;

		EmitVertex();
	}
	EndPrimitive();
}
)zzz";

const char* fragment_shader =
        R"zzz(#version 430 core
flat in vec4 normal;
in vec4 light_direction;
out vec4 fragment_color;
void main()
{
//	vec3 color = vec3(-normal.x, normal.y, normal.z);
//	float dot_nl = dot(normalize(light_direction.xyz), normalize(normal.xyz));
//	dot_nl = clamp(dot_nl, 0.0, 1.0);
//	fragment_color = vec4(clamp(dot_nl * color, 0.0, 1.0),1.0);
    fragment_color = vec4(.5,.5,.5,1);
}
)zzz";

void
ErrorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << description << "\n";
}

std::shared_ptr<subdivision> g_subdivision;
Camera g_camera;
float scalefactor = 1.0f;
bool drawframe = false;
bool resetgeometry = true;
bool loopmode = true; // false is catmull-clark
int recursion_level = 0;

void
KeyCallback(GLFWwindow* window,
            int key,
            int scancode,
            int action,
            int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	} else if (key == GLFW_KEY_EQUAL && mods == GLFW_MOD_SHIFT && action != GLFW_RELEASE) {
		scalefactor *= 2;
	} else if (key == GLFW_KEY_MINUS && mods == GLFW_MOD_SHIFT && action != GLFW_RELEASE) {
		scalefactor *= 0.5f;
	} else if (key == GLFW_KEY_W && action != GLFW_RELEASE) {
	    g_camera.zoom(scalefactor * 1.0f);
	} else if (key == GLFW_KEY_S && mods != GLFW_MOD_CONTROL && action != GLFW_RELEASE) {
	    g_camera.zoom(scalefactor * -1.0f);
    } else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
	    g_camera.strafe(scalefactor * -1.0f);
	} else if (key == GLFW_KEY_D && action != GLFW_RELEASE) {
	    g_camera.strafe(scalefactor * 1.0f);
	} else if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE) {
		g_camera.rotateLeft();
	} else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE) {
		g_camera.rotateRight();
	} else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) {
		g_camera.strafeVert(scalefactor * -1.0f);
	} else if (key == GLFW_KEY_UP && action != GLFW_RELEASE) {
		g_camera.strafeVert(scalefactor * 1.0f);
    } else if (key == GLFW_KEY_F && action != GLFW_RELEASE) {
        drawframe = !drawframe;
    } else if (key == GLFW_KEY_R && action != GLFW_RELEASE) {
	    recursion_level = 0;
        resetgeometry = true;
    } else if (key == GLFW_KEY_MINUS && action != GLFW_RELEASE) {
	    resetgeometry = true;
	    recursion_level = std::max(0,recursion_level - 1);
    } else if (key == GLFW_KEY_EQUAL && action != GLFW_RELEASE) {
	    resetgeometry = true;
        recursion_level++;
	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		loopmode = !loopmode;
		resetgeometry = true;
	} else if (key == GLFW_KEY_Z && action != GLFW_RELEASE) {
        g_camera.lookDown();
    }
}

int g_current_button;
bool g_mouse_pressed;

void
MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	if (!g_mouse_pressed) {
		// reset mouse
		g_camera.updateRotate = false;
		return;
	}
	if (g_current_button == GLFW_MOUSE_BUTTON_LEFT) {
		if (g_camera.updateRotate) {
			g_camera.rotateMouse(g_camera.x - mouse_x, g_camera.y - mouse_y);
		}
		g_camera.x = mouse_x;
		g_camera.y = mouse_y;
		g_camera.updateRotate = true;
	} else if (g_current_button == GLFW_MOUSE_BUTTON_RIGHT) {
	} else if (g_current_button == GLFW_MOUSE_BUTTON_MIDDLE) {
	}
}

void
MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	g_mouse_pressed = (action == GLFW_PRESS);
	g_current_button = button;
}

string trim(string str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last-first+1));
}

int main(int argc, char* argv[])
{
	std::string window_title = "Subdivision";
	// Load geometry from file
	bool loaded_geometry = false;
    std::vector<glm::vec3> loaded_vertices_3;
    std::vector<glm::vec4> loaded_vertices;
    std::vector<glm::uvec3> loaded_faces;
	if (argc > 1 && string(argv[1]).length() > 0) {
        loaded_geometry = true;
        string line;
        ifstream file;
        file.open(argv[1]);
        bool readingpoints = true;
        if (file.is_open()) {
            while (getline(file, line)) {
//                cout << "line" << line << endl;
                if (line.find("points") != string::npos)
                    readingpoints = true;
                if (line.find("faces") != string::npos)
                    readingpoints = false;
                // nothing specified
                if (line.find("(") == string::npos || line.find(")") == string::npos)
                    continue;
                double dx,dy,dz;
                uint ix, iy, iz;
                int fcommapos = line.find(",");
                int scommapos = line.find(",",fcommapos + 1);
                if (readingpoints) {
//                    cout << trim(line.substr(line.find("(") + 1, line.find(",") - (line.find("(") + 1)));
//                    cout << trim(line.substr(fcommapos + 1, scommapos - (fcommapos + 1)));
//                    cout << trim(line.substr(scommapos + 1, line.find(")") - (scommapos + 1))) << endl;
                    dx = stod(trim(line.substr(line.find("(") + 1, line.find(",") - (line.find("(") + 1))));
                    dy = stod(trim(line.substr(fcommapos + 1, scommapos - (fcommapos + 1))));
                    dz = stod(trim(line.substr(scommapos + 1, line.find(")") - (scommapos + 1))));
                    loaded_vertices_3.push_back(vec3(dx,dy,dz));
//                    cout << dx << " " << dy << " " << dz << endl;
                } else {
                    ix = stod(trim(line.substr(line.find("(") + 1, line.find(",") - (line.find("(") + 1))));
                    iy = stod(trim(line.substr(fcommapos + 1, scommapos - (fcommapos + 1))));
                    iz = stod(trim(line.substr(scommapos + 1, line.find(")") - (scommapos + 1))));
                    loaded_faces.push_back(uvec3(ix,iy,iz));
                }
            }
        } else
            cout << "could not open file " << argv[1] << endl;
	}
	//
	if (!glfwInit()) exit(EXIT_FAILURE);
	g_subdivision = std::make_shared<subdivision>();
	glfwSetErrorCallback(ErrorCallback);

	// Ask an OpenGL 4.1 core profile context
	// It is required on OSX and non-NVIDIA Linux
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(window_width, window_height,
			&window_title[0], nullptr, nullptr);
	CHECK_SUCCESS(window != nullptr);
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;

	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MousePosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	// Setup vertex shader.
	GLuint vertex_shader_id = 0;
	const char* vertex_source_pointer = vertex_shader;
	CHECK_GL_ERROR(vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, nullptr));
	glCompileShader(vertex_shader_id);
	CHECK_GL_SHADER_ERROR(vertex_shader_id);

	// Setup geometry shader.
	GLuint geometry_shader_id = 0;
	const char* geometry_source_pointer = geometry_shader;
	CHECK_GL_ERROR(geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER));
	CHECK_GL_ERROR(glShaderSource(geometry_shader_id, 1, &geometry_source_pointer, nullptr));
	glCompileShader(geometry_shader_id);
	CHECK_GL_SHADER_ERROR(geometry_shader_id);

	// Setup fragment shader.
	GLuint fragment_shader_id = 0;
	const char* fragment_source_pointer = fragment_shader;
	CHECK_GL_ERROR(fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(fragment_shader_id, 1, &fragment_source_pointer, nullptr));
	glCompileShader(fragment_shader_id);
	CHECK_GL_SHADER_ERROR(fragment_shader_id);

    // subdivision program.
    GLuint program_id = 0;
    CHECK_GL_ERROR(program_id = glCreateProgram());
    CHECK_GL_ERROR(glAttachShader(program_id, vertex_shader_id));
    CHECK_GL_ERROR(glAttachShader(program_id, fragment_shader_id));
    CHECK_GL_ERROR(glAttachShader(program_id, geometry_shader_id));
    CHECK_GL_ERROR(glBindAttribLocation(program_id, 0, "vertex_position"));
    CHECK_GL_ERROR(glBindFragDataLocation(program_id, 0, "fragment_color"));
    glLinkProgram(program_id);
    CHECK_GL_PROGRAM_ERROR(program_id);
    GLint projection_matrix_location = 0;
    CHECK_GL_ERROR(projection_matrix_location =
                           glGetUniformLocation(program_id, "projection"));
    GLint view_matrix_location = 0;
    CHECK_GL_ERROR(view_matrix_location =
                           glGetUniformLocation(program_id, "view"));
    GLint viewi_matrix_location = 0;
    CHECK_GL_ERROR(viewi_matrix_location =
                           glGetUniformLocation(program_id, "viewi"));
    GLint light_position_location = 0;
    CHECK_GL_ERROR(light_position_location =
                           glGetUniformLocation(program_id, "light_position"));

    std::vector<glm::vec3> obj_vertices_3;
    std::vector<glm::vec4> obj_vertices;
    std::vector<glm::uvec3> obj_faces;

	std::vector<glm::uvec4> obj_quad_faces;

	glm::vec4 light_position = glm::vec4(-10.0f, 10.0f, 0.0f, 1.0f);
	float aspect = 0.0f;
	float theta = 0.0f;
    high_resolution_clock::time_point start = high_resolution_clock::now();
	while (!glfwWindowShouldClose(window)) {
		// Setup some basic window stuff.
		if (drawframe)
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
		else
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LESS);
        high_resolution_clock::time_point curr = high_resolution_clock::now();
        double t = duration_cast<milliseconds>(curr - start).count();

		// Switch to the Geometry VAO.
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

		// Compute the projection matrix.
		aspect = static_cast<float>(window_width) / window_height;
		glm::mat4 projection_matrix =
			glm::perspective(glm::radians(45.0f), aspect, 0.1f, 500.0f);

		// Compute the view matrix
		glm::mat4 view_matrix = g_camera.get_view_matrix();
		glm::mat4 viewi_matrix = glm::inverse(view_matrix);

		// Use our program.
		CHECK_GL_ERROR(glUseProgram(program_id));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE,
					&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE,
					&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(viewi_matrix_location, 1, GL_FALSE,
										  &viewi_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(light_position_location, 1, &light_position[0]));

		if (loopmode) {
			if (resetgeometry) {
			    if (loaded_geometry) {
                    obj_vertices_3.clear();
                    obj_faces.clear();
                    for (auto i : loaded_vertices_3)
                        obj_vertices_3.push_back(i);
                    for (auto i : loaded_faces)
                        obj_faces.push_back(i);
			    }
			    else
				    g_subdivision->generate_geometry(obj_vertices_3, obj_faces);

				obj_vertices.clear();
				for (auto i : obj_vertices_3) {
					obj_vertices.emplace_back(vec4(i, 1.0f));
				}

				for (int i = 0; i < recursion_level; i++) {
					g_subdivision->loop(obj_vertices_3, obj_faces);
				}

				obj_vertices.clear();
				for (auto i : obj_vertices_3) {
					obj_vertices.emplace_back(vec4(i, 1.0f));
				}

				// Setup our VAO array.
				CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[0]));

				CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

				// Generate buffer objects
				CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kGeometryVao][0]));

				// Setup vertex data in a VBO.
				CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
				// NOTE: We do not send anything right now, we just describe it to OpenGL.
				CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
											sizeof(float) * obj_vertices.size() * 4, obj_vertices.data(),
											GL_STATIC_DRAW));
				CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
				CHECK_GL_ERROR(glEnableVertexAttribArray(0));

				// Setup element array buffer.
				CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
				CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
											sizeof(uint32_t) * obj_faces.size() * 3,
											obj_faces.data(), GL_STATIC_DRAW));
				resetgeometry = false;
			}

			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));
		} else {
			if (resetgeometry) {

				g_subdivision->generate_quad_geometry(obj_vertices_3, obj_quad_faces);

				obj_vertices.clear();
				for (auto i : obj_vertices_3) {
					obj_vertices.emplace_back(vec4(i, 1.0f));
				}

				for (int i = 0; i < recursion_level; i++) {
					g_subdivision->catmull(obj_vertices_3, obj_quad_faces);
				}

				obj_vertices.clear();
				for (auto i : obj_vertices_3) {
					obj_vertices.emplace_back(vec4(i, 1.0f));
				}

				obj_faces.clear();
				for (auto f : obj_quad_faces) {
					obj_faces.emplace_back(uvec3(f.x,f.y,f.z));
					obj_faces.emplace_back(uvec3(f.z,f.w,f.x));
				}

				// Setup our VAO array.
				CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[0]));

				CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

				// Generate buffer objects
				CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kGeometryVao][0]));

				// Setup vertex data in a VBO.
				CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
				// NOTE: We do not send anything right now, we just describe it to OpenGL.
				CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
											sizeof(float) * obj_vertices.size() * 4, obj_vertices.data(),
											GL_STATIC_DRAW));
				CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
				CHECK_GL_ERROR(glEnableVertexAttribArray(0));

				// Setup element array buffer.
				CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
				CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
											sizeof(uint32_t) * obj_faces.size() * 3,
											obj_faces.data(), GL_STATIC_DRAW));
				resetgeometry = false;
			}

			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));
		}

		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}