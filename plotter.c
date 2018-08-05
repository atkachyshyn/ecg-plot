#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plotter.h"

#define UNIFORM "uniform_"

// Setup plotter ///////////////////////////////////////////////////////////////////////////////////////////////////

struct plotter* get_plotter(void)
{
	struct plotter* new_plotter = (struct plotter*)malloc(sizeof(struct plotter));
	printf("Address allocated for new plotter: %p\n", new_plotter);
	struct plotter plotter;
	*new_plotter = plotter;
	return new_plotter;
}

void setup_plotter(struct plotter* plotter)
{
	printf("Plotter setup started\n");
    plotter->window = initalize_glfw_window(plotter);

    GLuint vs = create_vertex_shader(plotter);
    GLuint fs = create_fragment_shader(plotter);

    plotter->program = create_program(vs, fs);
    char* attributes[] = { "vertex2d", "v_color" };
    set_attributes(plotter, 2, attributes);

    GLuint buffers[3];
    create_buffers(plotter, sizeof(buffers)/sizeof(GLuint), buffers);

    generate_time_scale(plotter);
    generate_millivolts_scale(plotter);
}

// GLFW region /////////////////////////////////////////////////////////////////////////////////////////////////

// Setup window instance
static GLFWwindow* initalize_glfw_window(struct plotter* plotter)
{
    if (!glfwInit())
        fprintf(stderr, "Could not initialize GLFW\n");

    // Get primary monitor configuration
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    plotter->window_width = mode->width;
    plotter->window_height = mode->height;

    printf("Width: %d Height: %d\n", mode->width, mode->height);

    // Set OpenGL ES 2.0 environment
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);

    // Create window
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "ECG plot", glfwGetPrimaryMonitor(), NULL);
    if (!window)
    {
        glfwTerminate();
			fprintf(stderr, "Could not create GLFW window\n");
    }
    glfwMakeContextCurrent(window);

    // Configure view port for ECG graph (5mV high and X seconds width)
    int voltage_scale_height_pixels = (int)((plotter->max_voltage_range / plotter->voltage_tick_value) * plotter->tick_size);
    int offset_bottom = (plotter->window_height - voltage_scale_height_pixels)/2;
    glViewport(0, offset_bottom, mode->width, voltage_scale_height_pixels);
    glScissor(0, offset_bottom, mode->width, voltage_scale_height_pixels);

    // Set keyboard callback for input keyboard input handling
    glfwSetKeyCallback(window, handle_input);

    return window;
}

// Callback for keyboard interactions
static void handle_input(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		exit(EXIT_SUCCESS);
	}
}

// Render method
void on_render(struct plotter* plotter)
{
    while (!glfwWindowShouldClose(plotter->window))
    {
		render_func(plotter);

		glfwPollEvents();
        // put the stuff we've been drawing onto the display
        glfwSwapBuffers(plotter->window);
	}
}

// GLFW end region /////////////////////////////////////////////////////////////////////////////////////////////////

// OpenGL Program setup ////////////////////////////////////////////////////////////////////////////////////////////

// Program
static GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLint link_ok = GL_FALSE;
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok) {
        fprintf(stderr, "Error during program linking!/n");
        return 0;
    }

    return program;
}

// Shaders
void set_vertex_shader(struct plotter* plotter, const char* v_shader)
{
    plotter->v_shader = v_shader;
}

void set_fragment_shader(struct plotter* plotter, const char* f_shader)
{
    plotter->f_shader = f_shader;
}

static GLuint create_vertex_shader(struct plotter* plotter)
{
    return create_shader(plotter->v_shader, GL_VERTEX_SHADER);
}

static GLuint create_fragment_shader(struct plotter* plotter)
{
    return create_shader(plotter->f_shader, GL_FRAGMENT_SHADER);
}

static GLuint create_shader(const char* shader_source, GLenum type)
{
    GLint compile_ok = GL_FALSE, link_ok = GL_FALSE;

    GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &shader_source, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_ok);
	if (!compile_ok) {
		fprintf(stderr, "Error in vertex shader\n");
		return 0;
	}

	return shader;
}

// Attributes
static GLint get_attribute(GLuint program, const char *name)
{
	GLint attribute = glGetAttribLocation(program, name);
	if(attribute == -1)
		fprintf(stderr, "Could not bind attribute %s\n", name);
	return attribute;
}

static GLint get_uniform(GLuint program, const char *name)
{
	GLint uniform = glGetUniformLocation(program, name);
	if(uniform == -1)
		fprintf(stderr, "Could not bind uniform %s\n", name);
	return uniform;
}

static GLint create_attribute(GLuint program, char* attribute_name)
{
	int is_uniform = starts_with(UNIFORM, attribute_name);
	printf("Creating attribute -> %s of type %s for program -> %d\n", attribute_name, is_uniform == 0 ? "regular" : "uniform", program);
    GLint attribute = is_uniform == 0 ? glGetAttribLocation(program, attribute_name) : glGetUniformLocation(program, attribute_name);
    if (attribute == -1) {
        fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
        return 0;
    }
	printf("Created attribute -> %d\n", attribute);
    return attribute;
}

static void set_attributes(struct plotter* plotter, size_t num_attributes, char* attributes[])
{
	plotter->attributes = (GLint*)calloc(num_attributes, sizeof(GLint));
	for(size_t i = 0; i < num_attributes; i++)
    {
		*(plotter->attributes + i) = create_attribute(plotter->program, attributes[i]);
    }
}

// Buffers
static void create_buffers(struct plotter* plotter, size_t num_buffers, GLuint* buffers)
{
	plotter->buffers = (struct buffer*)malloc(num_buffers*sizeof(struct buffer));

    glGenBuffers(num_buffers, buffers);
    for(size_t i = 0;i<num_buffers;i++)
    {
		plotter->buffers[i].address = buffers[i];
		plotter->buffers[i].size_bytes = 0;
		plotter->buffers[i].data = NULL;
		plotter->buffers[i].num_elements = 0;
		printf("Buffer[%d]: %p Size: %d\n", i, plotter->buffers[i].address, plotter->buffers[i].size_bytes);
	}
}

void free_resources(struct plotter* plotter)
{
    glDeleteProgram(plotter->program);
    glDeleteBuffers(1, &((struct buffer*)plotter->buffers)->address);
    glfwDestroyWindow(plotter->window);
	glfwTerminate();
}

// OpenGL Program setup end /////////////////////////////////////////////////////////////////////////////////////////

// Rendering section ////////////////////////////////////////////////////////////////////////////////////////////

static void generate_time_scale(struct plotter* plotter)
{
    float pixel_weight_x = 2.0/plotter->window_width;
    int num_of_ticks = plotter->window_width/plotter->tick_size + 1;
    float tick_width_in_opengl_coord = plotter->tick_size * pixel_weight_x;
    plotter->buffers[0].num_elements = num_of_ticks*2;

    struct point ticks[num_of_ticks*2];
    plotter->buffers[0].size_bytes = sizeof ticks;
    plotter->buffers[0].data = (struct point*)calloc(num_of_ticks*2,sizeof(struct point));

	for (int i = 0; i < num_of_ticks; i++)
	{
		float x = -1 + i * tick_width_in_opengl_coord;

		GLfloat* color = (GLfloat[3]){0.69, 0.4, 0.35};
		if (i % 5 == 0){
			color = ((GLfloat[3]){1.0, 0.0, 0.0});
		};

		plotter->buffers[0].data[i * 2].vertex2d[0] = x;
		plotter->buffers[0].data[i * 2].vertex2d[1] = -1.0;
		plotter->buffers[0].data[i * 2].color[0] = color[0];
		plotter->buffers[0].data[i * 2].color[1] = color[1];
		plotter->buffers[0].data[i * 2].color[2] = color[2];
		plotter->buffers[0].data[i * 2 + 1].vertex2d[0] = x;
		plotter->buffers[0].data[i * 2 + 1].vertex2d[1] = 1.0;
		plotter->buffers[0].data[i * 2 + 1].color[0] = color[0];
		plotter->buffers[0].data[i * 2 + 1].color[1] = color[1];
		plotter->buffers[0].data[i * 2 + 1].color[2] = color[2];

		printf("tick[%d].x = %f tick[%d].y = %f\n", i*2, plotter->buffers[0].data[i * 2].vertex2d[0], i*2, plotter->buffers[0].data[i * 2].vertex2d[1]);
		printf("tick[%d].x = %f tick[%d].y = %f\n", i*2+1, plotter->buffers[0].data[i * 2+1].vertex2d[0], i*2+1, plotter->buffers[0].data[i * 2+1].vertex2d[1]);
	}

	printf("Buffer-> data size: %d, data address: %p address: %p\n", sizeof ticks, ticks);
}

static void generate_millivolts_scale(struct plotter* plotter)
{
	float pixel_weight_y = 2.0/500;  //plotter->window_height;
    int num_of_ticks = 50;//plotter->window_height/plotter->tick_size+1;
    float tick_width_in_opengl_coord = plotter->tick_size * pixel_weight_y;
    plotter->buffers[1].num_elements = num_of_ticks*2;

    struct point ticks[num_of_ticks*2];
    plotter->buffers[1].size_bytes = sizeof ticks;
    plotter->buffers[1].data = (struct point*)calloc(num_of_ticks*2,sizeof(struct point));

	for (int i = 0; i < num_of_ticks; i++) {
		float y = -1 + i * tick_width_in_opengl_coord;

		GLfloat* color = (GLfloat[3]){0.69, 0.4, 0.35};
		if ((i % 5) == 0){
			color = ((GLfloat[3]){1.0, 0.0, 0.0});
		};

		plotter->buffers[1].data[i * 2].vertex2d[0] = -1.0;
		plotter->buffers[1].data[i * 2].vertex2d[1] = y;
		plotter->buffers[1].data[i * 2].color[0] = color[0];
		plotter->buffers[1].data[i * 2].color[1] = color[1];
		plotter->buffers[1].data[i * 2].color[2] = color[2];
		plotter->buffers[1].data[i * 2 + 1].vertex2d[0] = 1.0;
		plotter->buffers[1].data[i * 2 + 1].vertex2d[1] = y;
		plotter->buffers[1].data[i * 2 + 1].color[0] = color[0];
		plotter->buffers[1].data[i * 2 + 1].color[1] = color[1];
		plotter->buffers[1].data[i * 2 + 1].color[2] = color[2];

		printf("tick[%d].x = %f tick[%d].y = %f\n", i*2, plotter->buffers[0].data[i * 2].vertex2d[0], i*2, plotter->buffers[0].data[i * 2].vertex2d[1]);
		printf("tick[%d].x = %f tick[%d].y = %f\n", i*2+1, plotter->buffers[0].data[i * 2+1].vertex2d[0], i*2+1, plotter->buffers[0].data[i * 2+1].vertex2d[1]);
	}
}

void set_data(struct plotter* plotter, float* data)
{
	size_t num_elements = sizeof(data)/sizeof(data[0])/2;
	plotter->buffers[2].num_elements = num_elements;
    plotter->buffers[2].size_bytes = num_elements * sizeof(struct point);
    plotter->buffers[2].data = (struct point*)calloc(num_elements,sizeof(struct point));
	printf("Buffer[2]: num_elements: %d, size_bytes: %p address: %p\n",plotter->buffers[2].num_elements, plotter->buffers[2].size_bytes,plotter->buffers[2]);
	for(int i = 0; i++; i < num_elements)
	{
		plotter->buffers[2].data[i].vertex2d[0] = data[i * 2];
		plotter->buffers[2].data[i].vertex2d[1] = data[i * 2 + 1];
		plotter->buffers[2].data[i].color[0] = 0.0;
		plotter->buffers[2].data[i].color[1] = 0.0;
		plotter->buffers[2].data[i].color[2] = 0.0;
		printf("data[%d].x = %f data[%d].y = %f\n", i, plotter->buffers[2].data[i].vertex2d[0], i, plotter->buffers[2].data[i].vertex2d[1]);
	}
}

static void render_func(struct plotter* plotter)
{
    int window_width = plotter->window_width;
	int window_height = plotter->window_height;

	glUseProgram(plotter->program);

	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	// Set the color to black
	GLfloat black[4] = { 0, 0, 0, 1 };
	glUniform4fv(plotter->attributes[1], 1, black);

	glEnableVertexAttribArray(plotter->attributes[0]);
	glEnableVertexAttribArray(plotter->attributes[1]);

	glBindBuffer(GL_ARRAY_BUFFER, plotter->buffers[0].address);
    glBufferData(GL_ARRAY_BUFFER, plotter->buffers[0].size_bytes, plotter->buffers[0].data, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(
		plotter->attributes[0],   // attribute
		2,                   // number of elements per vertex, here (x,y)
		GL_FLOAT,            // the type of each element
		GL_FALSE,            // take our values as-is
		sizeof(struct point),  // next coord2d appears every 5 floats
		0                    // offset of first element
	);
	glVertexAttribPointer(
		plotter->attributes[1],      // attribute
		3,                      // number of elements per vertex, here (r,g,b)
		GL_FLOAT,               // the type of each element
		GL_FALSE,               // take our values as-is
		sizeof(struct point),  // stride
		(GLvoid*) offsetof(struct point, color)  // offset
	);

    glDrawArrays(GL_LINES, 0, plotter->buffers[0].num_elements);

	glBindBuffer(GL_ARRAY_BUFFER, plotter->buffers[1].address);
    glBufferData(GL_ARRAY_BUFFER, plotter->buffers[1].size_bytes, plotter->buffers[1].data, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(
		plotter->attributes[0],   // attribute
		2,                   // number of elements per vertex, here (x,y)
		GL_FLOAT,            // the type of each element
		GL_FALSE,            // take our values as-is
		sizeof(struct point),  // next coord2d appears every 5 floats
		0                    // offset of first element
	);
	glVertexAttribPointer(
		plotter->attributes[1],      // attribute
		3,                      // number of elements per vertex, here (r,g,b)
		GL_FLOAT,               // the type of each element
		GL_FALSE,               // take our values as-is
		sizeof(struct point),  // stride
		(GLvoid*) offsetof(struct point, color)  // offset
	);

    glDrawArrays(GL_LINES, 0, plotter->buffers[1].num_elements);

	glBindBuffer(GL_ARRAY_BUFFER, plotter->buffers[2].address);
    glBufferData(GL_ARRAY_BUFFER, plotter->buffers[2].size_bytes, plotter->buffers[2].data, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(
		plotter->attributes[0],   // attribute
		2,                   // number of elements per vertex, here (x,y)
		GL_FLOAT,            // the type of each element
		GL_FALSE,            // take our values as-is
		sizeof(struct point),  // next coord2d appears every 5 floats
		0                    // offset of first element
	);
	glVertexAttribPointer(
		plotter->attributes[1],      // attribute
		3,                      // number of elements per vertex, here (r,g,b)
		GL_FLOAT,               // the type of each element
		GL_FALSE,               // take our values as-is
		sizeof(struct point),  // stride
		(GLvoid*) offsetof(struct point, color)  // offset
	);

    glDrawArrays(GL_LINES, 0, plotter->buffers[2].num_elements);
}

// Utility functions //////////////////////////////////////////////////////////////////////////////

static int starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}
