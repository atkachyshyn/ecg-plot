#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
//~ #include "adc.h"
#include "plotter.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// Defines for scales
#define TIME_SCALE_MAX_VISIBLE_RANGE_SECONDS 6
#define VOLTAGE_SCALE_MAX_VISIBLE_RANGE_MILLIVOLTS 5
#define TICK_SPACE_PIXELS 10;
#define TIME_SCALE_TICK_VALUE_SECONDS 0.04
#define VOLTAGE_SCALE_TICK_VALUE_MILLIVOLTS 0.1
#define DELAY 3906250L

static float time_val = 0; 
static float voltage_val = 0;

void read_ecg_simulation(void);
static FILE* open_file(char* path);
static void close_file(FILE* fp);
static int read_next(float* time, float* voltage, FILE* fp);
void *threadFunc(void *arg);

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
    
    // read file with frequency 256HZ in another thread
    pthread_t pth;
	pthread_create(&pth,NULL,threadFunc,NULL);

    // Call render function
    on_render(new_plotter);

	// Free resources
    free_resources(new_plotter);
    
    pthread_join(pth,NULL);
    
    return 0;
}

static FILE* open_file(char* path)
{
    FILE* fp = fopen(path, "r");
    if (fp == NULL)
        fprintf( stderr, "Can't open given file", 30);
    return fp;
}

static void close_file(FILE* fp)
{
    fclose(fp);
}

static int read_next(float* time, float* voltage, FILE* fp)
{
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    read = getline(&line, &len, fp);

    if (read == -1)
    {
        *time = 0;
        *voltage = 0;
        return 0;
    }

    const char* x = strtok(line, " ");
    const char* y = strtok (NULL, " ");

    if (x != NULL && y != NULL)
    {
        *time = atof(x);
        *voltage = atof(y);
    }

    printf("x: %f y: %f\n", *time, *voltage);

    return 1;
}

void *threadFunc(void *arg)
{
	
	struct timespec ts = {0, 3906250L };
    FILE* fp = open_file("../ecgsyn.dat");
    while (read_next(&time_val, &voltage_val, fp)) 
    {
        nanosleep (&ts, NULL);
    }
    close_file(fp);
    
    return NULL;
}
