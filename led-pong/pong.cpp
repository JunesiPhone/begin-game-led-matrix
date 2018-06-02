#include "pong.h"
#include <fcntl.h>
#include <stdio.h>
#include <linux/joystick.h>
#include <curses.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <map>
#include <algorithm>
#include <cstring>
#include "rpi-rgb-led-matrix/include/led-matrix.h"
#include "rpi-rgb-led-matrix/include/graphics.h"
#include "rpi-rgb-led-matrix/include/canvas.h"
#include "rpi-rgb-led-matrix/include/threaded-canvas-manipulator.h"

volatile bool interrupt = false;

bool gameOver = false;
bool moveLeft = false;
bool moveRight = false;
bool moveUp = false;
bool moveDown = false;

int speed = 160;

RGBMatrix *canvas = NULL;

static void InterruptHandler(int signo) {
    interrupt = true;
}

/**
 * Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event){
    ssize_t bytes;
    bytes = read(fd, event, sizeof(*event));
    if (bytes == sizeof(*event))
        return 0;
    /* Error, could not read full event. */
    return -1;
}

/**
 * Returns the number of axes on the controller or 0 if an error occurs.
 */
size_t get_axis_count(int fd){
    __u8 axes;
    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;
    return axes;
}

/**
 * Returns the number of buttons on the controller or 0 if an error occurs.
 */
size_t get_button_count(int fd){
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;
    return buttons;
}

/**
 * Current state of an axis.
 */
struct axis_state {
    short x, y;
};

/**
 * Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
size_t get_axis_state(struct js_event *event, struct axis_state axes[3]){
    size_t axis = event->number / 2;
    if (axis < 3){
        if (event->number % 2 == 0){
            axes[axis].x = event->value;
        }
        else{
            axes[axis].y = event->value;
        }
    }
    return axis;
}


/**
 * setup game
 */
void setup(){
    gameOver = false;
}

/**
 * draw game
 */

int height = 64;
int width = 32;
int p1Height = height - 2;
int startCenter = (width/2) - 1;

void draw(){
    canvas->Fill(0,0,0);

    DrawLine(canvas, 0, 0, height-1, 0, GREY); //right
    DrawLine(canvas, 0, width-1, height-1, width-1, GREY); //left
    DrawLine(canvas, 0, 0, 0, width-1, GREY); //top
    DrawLine(canvas, height-1, 0, height-1, width-1, GREY); //bottom

    //y,x
    if(moveUp){
        p1Height--;
        moveUp = false;
    }
    if(moveDown){
        p1Height++;
        moveDown = false;
    }

    if(moveLeft){
        startCenter++;
        moveLeft = false;
    }
    if(moveRight){
        startCenter--;
        moveRight = false;
    }

    canvas->SetPixel(p1Height, startCenter, 0, 0, 255);

}

int main(int argc, char** argv){
    setup();
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    RGBMatrix::Options defaults;
    defaults.chain_length = 2;
    defaults.led_rgb_sequence = "RBG";
    defaults.hardware_mapping = "adafruit-hat";
    defaults.pixel_mapper_config="Rotate:360";
    rgb_matrix::RuntimeOptions runtime_opt;
    runtime_opt.drop_privileges = 0;
    canvas = CreateMatrixFromFlags(&argc, &argv, &defaults, &runtime_opt);

    const char *device;
    int js;
    struct js_event event;
    struct axis_state axes[3] = {0};
    size_t axis;
    device = "/dev/input/js0";

    js = open(device, O_RDONLY);

    if (js == -1)
        perror("Could not open joystick");


    while(!gameOver && !interrupt){
        draw();
        if(read_event(js, &event) == 0){
        switch (event.type){
            case JS_EVENT_BUTTON:
                if(event.number && event.value){
                    //no button events
                }
                break;
            case JS_EVENT_AXIS:
                axis = get_axis_state(&event, axes);
                if (axis < 3){
                    if(axes[axis].x == -32767){
                        moveLeft = true;
                    }
                    if(axes[axis].x == 32767){
                        moveRight = true;
                    }
                    if(axes[axis].y == -32767){
                        moveUp = true;
                    }
                    if(axes[axis].y == 32767){
                        moveDown = true;
                    }
                }
                break;
            default:
                break;
        }
        }
        usleep(10*1000);
    }    
}
