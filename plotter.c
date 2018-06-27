#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plotter.h"

#define UNIFORM "uniform_"

void set_vertex_shader(struct plotter* plotter, const char* v_shader)
{
    plotter->v_shader = v_shader;
}

void set_fragment_shader(struct plotter* plotter, const char* f_shader)
{
    plotter->f_shader = f_shader;
}

void setup_plotter(struct plotter* plotter)
{
	printf("Plotter setup started\n");
    plotter->window = initalize_glfw_window(plotter);

    GLuint vs = create_vertex_shader(plotter);
    GLuint fs = create_fragment_shader(plotter);
    
    printf("Shaders: \nVertex -> %d\nFragment -> %d\n", vs, fs);
    plotter->program = create_program(vs, fs);
    printf("Program created: %d\n", plotter->program);
    char* attributes[] = { "vertex2d", "v_color" };
    set_attributes(plotter, 2, attributes);

    GLuint buffers[3];
    create_buffers(plotter, sizeof(buffers)/sizeof(GLuint), buffers);
    
    generate_time_scale(plotter);
    generate_millivolts_scale(plotter);
    printf("Plotter setup finished!\n");
}

void set_attributes(struct plotter* plotter, size_t num_attributes, char* attributes[])
{
	plotter->attributes = (GLint*)calloc(num_attributes, sizeof(GLint));
	for(size_t i = 0; i < num_attributes; i++)
    {
		//~ plotter->attributes[i] = create_attribute(plotter->program, attributes[i]);
		*(plotter->attributes + i) = create_attribute(plotter->program, attributes[i]);
    }
    printf("%d attributes created!\n", num_attributes);
}

GLFWwindow* initalize_glfw_window(struct plotter* plotter)
{
    if (!glfwInit())
        fprintf(stderr, "Could not init glfw\n");
        
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    
    plotter->window_width = mode->width;
    plotter->window_height = mode->height;
    
    printf("Width: %d Height: %d\n", mode->width, mode->height);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "ECG plot", glfwGetPrimaryMonitor(), NULL);
    if (!window)
    {
        glfwTerminate();
			fprintf(stderr, "Could not create glfw window\n");
    }
    glfwMakeContextCurrent(window);
    
    printf("Range: %d Tick value: %f Tick size: %d\n", plotter->max_voltage_range, (float)plotter->voltage_tick_value, plotter->tick_size);
    int voltage_scale_height_pixels = (int)((plotter->max_voltage_range / plotter->voltage_tick_value) * plotter->tick_size);
    int offset_bottom = (plotter->window_height - voltage_scale_height_pixels)/2;
    printf("Height pixels: %d Offset: %d\n", voltage_scale_height_pixels, offset_bottom);
    glViewport(0, offset_bottom, mode->width, voltage_scale_height_pixels);
    glScissor(0, offset_bottom, mode->width, voltage_scale_height_pixels);
    //~ glViewport(0, 0, mode->width, mode->height);
    
    // Set keyboard callback for input keyboard input handling
    glfwSetKeyCallback(window, handle_input);   

    return window;
}

void handle_input(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		exit(EXIT_SUCCESS);
	}
}

void on_render(struct plotter* plotter)
{
    while (!glfwWindowShouldClose(plotter->window))
    {
		render_func(plotter);
	}
}

static void render_func(struct plotter* plotter)
{
	//~ printf("Rendering for program %d, window %d", plotter->program, plotter->window);
    int window_width = plotter->window_width;
	int window_height = plotter->window_height;

	glUseProgram(plotter->program);

	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Set the color to black
	GLfloat black[4] = { 0, 0, 0, 1 };
	glUniform4fv(plotter->attributes[1], 1, black);
	//~ printf("Width: %d Height: %d Pixel size(plotter): %f sizeof(pixel_size): %d\n", plotter->window_width, plotter->window_height, pixel_x, sizeof pixel_x);
	
	glEnableVertexAttribArray(plotter->attributes[0]);
	glEnableVertexAttribArray(plotter->attributes[1]);

	//~ printf("Buffer: %p Size: %d, Address: %p Num of elements: %d\n", plotter->buffers[0].address, plotter->buffers[0].size_bytes, plotter->buffers[0].data, plotter->buffers[0].num_elements);
	glBindBuffer(GL_ARRAY_BUFFER, plotter->buffers[0].address);
    glBufferData(GL_ARRAY_BUFFER, plotter->buffers[0].size_bytes, plotter->buffers[0].data, GL_DYNAMIC_DRAW);
    //~ glVertexAttribPointer(plotter->attributes[0], 2, GL_FLOAT, GL_FALSE, 0, 0);
    
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
		//~ (GLvoid*) (2 * sizeof(GLfloat))     // offset of first element
		(GLvoid*) offsetof(struct point, color)  // offset
	);

    
    glDrawArrays(GL_LINES, 0, plotter->buffers[0].num_elements);
    
	glBindBuffer(GL_ARRAY_BUFFER, plotter->buffers[1].address);
    glBufferData(GL_ARRAY_BUFFER, plotter->buffers[1].size_bytes, plotter->buffers[1].data, GL_DYNAMIC_DRAW);
    //~ glVertexAttribPointer(plotter->attributes[0], 2, GL_FLOAT, GL_FALSE, 0, 0);
    
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

    glfwPollEvents();
    // put the stuff we've been drawing onto the display
    glfwSwapBuffers(plotter->window);
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

GLuint create_vertex_shader(struct plotter* plotter)
{
    return create_shader(plotter->v_shader, GL_VERTEX_SHADER);
}

GLuint create_fragment_shader(struct plotter* plotter)
{
    return create_shader(plotter->f_shader, GL_FRAGMENT_SHADER);
}

GLint get_attribute(GLuint program, const char *name)
{
	GLint attribute = glGetAttribLocation(program, name);
	if(attribute == -1)
		fprintf(stderr, "Could not bind attribute %s\n", name);
	return attribute;
}

GLint get_uniform(GLuint program, const char *name)
{
	GLint uniform = glGetUniformLocation(program, name);
	if(uniform == -1)
		fprintf(stderr, "Could not bind uniform %s\n", name);
	return uniform;
}

void free_resources(struct plotter* plotter)
{
    glDeleteProgram(plotter->program);
    glDeleteBuffers(1, &((struct buffer*)plotter->buffers)->address);
    glfwDestroyWindow(plotter->window);
	glfwTerminate();
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
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

GLint create_attribute(GLuint program, char* attribute_name)
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

void create_buffers(struct plotter* plotter, size_t num_buffers, GLuint* buffers)
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

void pass_static_data_buffer(GLuint* buffer, size_t index, GLvoid* data)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer[index]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
}

void pass_dynamic_data_buffer(GLuint* buffer, size_t index, size_t size, GLvoid* data)
{
	printf("buffer: %p\n", buffer[index]);
    glBindBuffer(GL_ARRAY_BUFFER, buffer[index]);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

struct plotter* get_plotter(void)
{
	struct plotter* new_plotter = (struct plotter*)malloc(sizeof(struct plotter));
	printf("Address allocated for new plotter: %p\n", new_plotter);
	struct plotter plotter;
	*new_plotter = plotter;
	return new_plotter;
}

void set_window_size(struct plotter* plotter, int width, int height)
{
    plotter->window_width = width;
    plotter->window_height = height;
}

static int starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

void generate_time_scale(struct plotter* plotter)
{
    float pixel_weight_x = 2.0/plotter->window_width;
    int num_of_ticks = plotter->window_width/plotter->tick_size + 1;
    float tick_width_in_opengl_coord = plotter->tick_size * pixel_weight_x;
    plotter->buffers[0].num_elements = num_of_ticks*2;
    printf("Pixel weight: %f, Tick width: %f. Num of ticks: %d\n", pixel_weight_x, tick_width_in_opengl_coord, num_of_ticks);
    
    struct point ticks[num_of_ticks*2];
    plotter->buffers[0].size_bytes = sizeof ticks;
    plotter->buffers[0].data = (struct point*)calloc(num_of_ticks*2,sizeof(struct point));
    //~ plotter->buffers[0].data = ticks;

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

void generate_millivolts_scale(struct plotter* plotter)
{
	float pixel_weight_y = 2.0/500;  //plotter->window_height;
    int num_of_ticks = 50;//plotter->window_height/plotter->tick_size+1;
    float tick_width_in_opengl_coord = plotter->tick_size * pixel_weight_y;
    plotter->buffers[1].num_elements = num_of_ticks*2;
    printf("Pixel weight: %f, Tick width: %f. Num of ticks: %d\n", pixel_weight_y, tick_width_in_opengl_coord, num_of_ticks);
    
    struct point ticks[num_of_ticks*2];
    plotter->buffers[1].size_bytes = sizeof ticks;
    plotter->buffers[1].data = (struct point*)calloc(num_of_ticks*2,sizeof(struct point));
    //~ printf("Width: %d Height: %d Pixel size(plotter): %f sizeof(pixel_size): %d\n", WINDOWS_WIDTH, WINDOWS_HEIGHT, pixel_x, sizeof pixel_x);

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
