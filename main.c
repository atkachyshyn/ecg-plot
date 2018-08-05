#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
//~ #include "adc.h"
#include "plotter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Defines for scales
#define TIME_SCALE_MAX_VISIBLE_RANGE_SECONDS 6
#define VOLTAGE_SCALE_MAX_VISIBLE_RANGE_MILLIVOLTS 5
#define TICK_SPACE_PIXELS 10;
#define TIME_SCALE_TICK_VALUE_SECONDS 0.04
#define VOLTAGE_SCALE_TICK_VALUE_MILLIVOLTS 0.1

void read_ecg_simulation(void);

float ECG_SAMPLE[] = {0.123,0.3445,0.656,0.23,0.4565,0.111,0.87,0.9873,0.456,0.44,0.7943,0.666,0.14,0.654,0.7124,0.545,0.123,0.3445,0.656,0.23,0.4565,0.111,0.87,0.9873,0.456,0.44,0.7943,0.666,0.14,0.654,0.7124,0.545,0.123,0.3445,0.656,0.23,0.4565,0.111,0.87,0.9873,0.456,0.44,0.7943,0.666,0.14,0.654,0.7124,0.545,0.123,0.3445,0.656,0.23,0.4565,0.111,0.87,0.9873,0.456,0.44,0.7943,0.666,0.14,0.654,0.7124,0.545,0.123,0.3445,0.656,0.23,0.4565,0.111,0.87,0.9873,0.456,0.44,0.7943,0.666,0.14,0.654,0.7124,0.545};

int main(void)
{
	// Create new plotter
    struct plotter* new_plotter = get_plotter();

	// Source code of vertex shaders
    const char* v_shader_source =
        "#version 100\n"  // OpenGL ES 2.0
		"attribute highp vec2 vertex2d;"
		"attribute lowp vec3 v_color;"
		"varying lowp vec3 f_color;"
        "void main(void) {                        "
		"  gl_Position = vec4(vertex2d, 0.0, 1.0); "
		"  f_color = v_color;                      "
		"}";

	// Source code of fragment shaders
    const char* f_shader_source =
        "#version 100\n"  // OpenGL ES 2.0
        "varying lowp vec3 f_color;"
		"void main(void) {        "
		"  gl_FragColor = vec4(f_color, 1.0);"
		"}";

	// Pass shaders to plotter
    set_vertex_shader(new_plotter, v_shader_source);
    set_fragment_shader(new_plotter, f_shader_source);
    
    // Setup graph scales
	new_plotter->tick_size = TICK_SPACE_PIXELS;
    new_plotter->time_tick_value = TIME_SCALE_TICK_VALUE_SECONDS;
    new_plotter->voltage_tick_value = VOLTAGE_SCALE_TICK_VALUE_MILLIVOLTS;
    new_plotter->max_voltage_range = VOLTAGE_SCALE_MAX_VISIBLE_RANGE_MILLIVOLTS;
	
	// Setup plotter (Create window, compile shaders, generate VBOs)
    setup_plotter(new_plotter);

    read_ecg_simulation();
    set_data(new_plotter, ECG_SAMPLE);

	// Call render function
    on_render(new_plotter);

	// Free resources
    free_resources(new_plotter);
    
    return 0;
}

void read_ecg_simulation()
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("ecgsyn.dat", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        printf("Retrieved line of length %zu :\n", read);
        printf("%s", line);
    }

    fclose(fp);
    if (line)
        free(line);
}