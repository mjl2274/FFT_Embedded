#ifndef _AUDIO_BUS_H
#define _AUDIO_BUS_H

#include <linux/ioctl.h>

typedef struct {
	unsigned char data;
} audio_data_t;
  
typedef struct {
	unsigned char red, green, blue, x, y;
} vga_ball_color_t;

typedef struct {
  vga_ball_color_t background1;
} vga_ball_arg_t;  

typedef struct {
  audio_data_t background;
} audio_data_arg_t;

//typedef struct {
//} audio_data_t;

#define AUDIO_DISPLAY_MAGIC 'q'

/* ioctls and their arguments */
#define AUDIO_DATA_WRITE _IOW(AUDIO_DISPLAY_MAGIC, 1, audio_data_arg_t *)
#define AUDIO_DATA_READ _IOR(AUDIO_DISPLAY_MAGIC, 2, audio_data_arg_t *)
//#define AUDIO_DATA_VISUALIZE _IOW(AUDIO_DISPLAY_MAGIC, 3, audio_data_arg_t *) // New IOCTL command

#endif
