#pragma once

extern GLFWwindow* window;

struct point {
	GLfloat vertex2d[2];
	GLfloat color[3];
};

struct plotter
{
    const char* v_shader;
    const char* f_shader;
    int tick_size;
    float time_tick_value;
    float voltage_tick_value;
    float max_voltage_range;
    size_t num_attributes;
    GLuint program;
    GLFWwindow* window;
    int window_height;
    int window_width;
    GLint* attributes;
	struct buffer* buffers;
};

struct buffer
{
	GLuint address;
	size_t size_bytes;
	size_t num_elements;
	struct point* data;
};

// Plotter
struct plotter* get_plotter(void);
void setup_plotter(struct plotter* plotter);

// GLFW
static GLFWwindow* initalize_glfw_window(struct plotter* plotter);
static void handle_input(GLFWwindow* window, int key, int scancode, int action, int mods);
void on_render(struct plotter* plotter);

// OpenGL
static GLuint create_program(GLuint vertex_shader, GLuint fragment_shader);
void set_vertex_shader(struct plotter* plotter, const char* v_shader);
void set_fragment_shader(struct plotter* plotter, const char* f_shader);
static GLuint create_vertex_shader(struct plotter* plotter);
static GLuint create_fragment_shader(struct plotter* plotter);
static GLuint create_shader(const char* shader_source, GLenum type);
static GLint get_attribute(GLuint program, const char *name);
static GLint get_uniform(GLuint program, const char *name);
static GLint create_attribute(GLuint program, char* attribute_name);
static void set_attributes(struct plotter* plotter, size_t num_attributes, char* attributes[]);
static void create_buffers(struct plotter* plotter, size_t num_buffers, GLuint* buffers);
void free_resources(struct plotter* plotter);

// Render
static void generate_time_scale(struct plotter* plotter);
static void generate_millivolts_scale(struct plotter* plotter);
static void render_func(struct plotter* plotter);

// Utility
static int starts_with(const char *pre, const char *str);
