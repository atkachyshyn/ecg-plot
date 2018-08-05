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

float ECG_SAMPLE[] = {123,3445,656,23,4565,111,87,9873,456,44,7943,666,14,654,7124,545,123,3445,656,23,4565,111,87,9873,456,44,7943,666,14,654,7124,545,123,3445,656,23,4565,111,87,9873,456,44,7943,666,14,654,7124,545,123,3445,656,23,4565,111,87,9873,456,44,7943,666,14,654,7124,545,123,3445,656,23,4565,111,87,9873,456,44,7943,666,14,654,7124,545,123,3445,656,23,4565,111,87,9873,456,44,7943,666,14,654,7124,545};

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

    set_data(new_plotter, ECG_SAMPLE);

	// Call render function
    on_render(new_plotter);

	// Free resources
    free_resources(new_plotter);
    
    return 0;
}



