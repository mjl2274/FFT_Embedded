/* * Device driver for the audio visualizer 
 *
 * A Platform device implemented using the misc subsystem
 *
 * Max Lavey mjl2274 
 * Columbia University
 *
 * References:
 * Linux source: Documentation/driver-model/platform.txt
 *               drivers/misc/arm-charlcd.c
 * http://www.linuxforu.com/tag/linux-device-drivers/
 * http://free-electrons.com/docs/
 *
 * "make" to build
 * insmod vga_ball.ko
 *
 * Check code style with
 * checkpatch.pl --file --no-tree vga_ball.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "audio_ioctl.h"
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


// Define constants
#define SAMPLE_RATE 48000
#define CHUNK_SIZE 4096
#define MIN_SEGMENT_DURATION_SEC 2
#define MAX_SEGMENT_DURATION_SEC 44
#define FILENAME "HCB.wav"
#define FRACTIONAL_BITS 14

//Definte Driver
#define DRIVER_NAME "audio_data"

/* Device registers */
#define BG_DATA(x) (x)

#define BG_RED(x) (x)
#define BG_GREEN(x) ((x)+1)
#define BG_BLUE(x) ((x)+2)
#define BG_X(x) ((x)+3)
#define BG_Y(x) ((x)+4)

/*
 * Information about our device
 */
struct audio_data_dev {
	struct resource res; /* Resource: our registers */
	void __iomem *virtbase; /* Where registers can be accessed in memory */
        audio_data_t background;
        vga_ball_color_t background1;
} dev;

/*
 * Write segments of a single digit
 * Assumes digit is in range and the device information has been set up
 */
static void write_background_audio(audio_data_t *background)
{
	iowrite32(background->data, BG_DATA(dev.virtbase) );
	dev.background = *background;
}

static void read_background_audio(audio_data_t *background)
{
	ioread32(background->data, BG_DATA(dev.virtbase) );
	dev.background = *background;
}


static void write_background_video(vga_ball_color_t *background1)
{
	iowrite8(background1->red, BG_RED(dev.virtbase) );
	iowrite8(background1->green, BG_GREEN(dev.virtbase) );
	iowrite8(background1->blue, BG_BLUE(dev.virtbase) );
	iowrite8(background1->x, BG_X(dev.virtbase) );
	iowrite8(background1->y, BG_Y(dev.virtbase) );
	dev.background1 = *background1;
}


/*
 * Handle ioctl() calls from userspace:
 * Read or write the segments on single digits.
 * Note extensive error checking of arguments
 Here is the example for help later 
 */
/*static long audio_data_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	audio_data_arg_t vla;

	switch (cmd) {
	case :
		if (copy_from_user(&vla, (audio_data_arg_t *) arg,
				   sizeof(audio_data_arg_t)))
			return -EACCES;
		write_background(&vla.background);
		break;

	case audio_data_READ_BACKGROUND:
	  	vla.background = dev.background;
		if (copy_to_user((audio_data_arg_t *) arg, &vla,
				 sizeof(audio_data_arg_t)))
			return -EACCES;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}
*/

/*
static void visualize_frequency_content(uint32_t frequency_content)
{
    // Process the preprocessed frequency content to generate visual patterns/effects
    // Map frequency content to visual parameters (e.g., color, position)

    // Example: Use frequency content to set the intensity of the background color
    uint8_t intensity = (uint8_t)(frequency_content & 0xFF);
    audio_data_color_t color = {intensity, intensity, intensity, 0x00, 0x00};

    // Write the visual output to the VGA display
    write_background(&color);
}
*/

static long audio_data_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	audio_data_arg_t vla;
	vga_ball_arg_t vla2;
    switch (cmd) {
    	case AUDIO_DATA_WRITE:
			if (copy_from_user(&vla, (audio_data_t *) arg,
				   	sizeof(audio_data_arg_t)))
				return -EACCES;
			write_background_audio(&vla.background);
			write_background_video(&vla2.background1);
		break;

		case AUDIO_DATA_READ:
			vla2.background1 = dev.background1;
	  		vla.background = dev.background;
			if (copy_to_user((audio_data_arg_t *) arg, &vla,
					sizeof(audio_data_arg_t)))
				return -EACCES;
			else if (copy_to_user((vga_ball_arg_t *) arg, &vla2,
					sizeof(audio_data_arg_t)))
				return -EACCES;
			break;

      //  case AUDIO_DATA_VISUALIZE:
            // Start receiving preprocessed frequency content from hardware
            // Continuously visualize frequency content on VGA display
            //while (/* condition for continuous processing */) {
              //  uint32_t frequency_content = /* read frequency content from hardware */;
                //visualize_frequency_content(frequency_content);
           // }
           // break; 
        // Other cases for existing ioctl commands
        default:
            return -EINVAL;
    }
    return 0;
}




/* The operations our device knows how to do */
static const struct file_operations audio_data_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = audio_data_ioctl,
};

/* Information about our device for the "misc" framework -- like a char dev */
static struct miscdevice audio_data_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &audio_data_fops,
};

/*
 * Initialization code: get resources (registers) and display
 * a welcome message
 */

 

static int __init audio_data_probe(struct platform_device *pdev)
{
        audio_data_t beige = {00000000000000000000000000000000};
        vga_ball_color_t beige1 = { 0xf9, 0xe4, 0xb7, 0x00, 0x00 };
	int ret;

	/* Register ourselves as a misc device: creates /dev/vga_ball */
	ret = misc_register(&audio_data_misc_device);

	/* Get the address of our registers from the device tree */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
	if (ret) {
		ret = -ENOENT;
		goto out_deregister;
	}

	/* Make sure we can use these registers */
	if (request_mem_region(dev.res.start, resource_size(&dev.res),
			       DRIVER_NAME) == NULL) {
		ret = -EBUSY;
		goto out_deregister;
	}

	/* Arrange access to our registers */
	dev.virtbase = of_iomap(pdev->dev.of_node, 0);
	if (dev.virtbase == NULL) {
		ret = -ENOMEM;
		goto out_release_mem_region;
	}
        
	/* Set an initial color */
        write_background_audio(&beige);
        write_background_video(&beige1);

	return 0;

out_release_mem_region:
	release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
	misc_deregister(&audio_data_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int audio_data_remove(struct platform_device *pdev)
{
	iounmap(dev.virtbase);
	release_mem_region(dev.res.start, resource_size(&dev.res));
	misc_deregister(&audio_data_misc_device);
	return 0;
}

/* Which "compatible" string(s) to search for in the Device Tree */
#ifdef CONFIG_OF
static const struct of_device_id audio_data_of_match[] = {
	{ .compatible = "csee4840,audio_data-1.0" },
	{},
};
MODULE_DEVICE_TABLE(of, audio_data_of_match);
#endif

/* Information for registering ourselves as a "platform" driver */
static struct platform_driver audio_data_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(audio_data_of_match),
	},
	.remove	= __exit_p(audio_data_remove),
};

/* Called when the module is loaded: set things up */
static int __init audio_data_init(void)
{
	pr_info(DRIVER_NAME ": init\n");
	return platform_driver_probe(&audio_data_driver, audio_data_probe);
}

/* Calball when the module is unloaded: release resources */
static void __exit audio_data_exit(void)
{
	platform_driver_unregister(&audio_data_driver);
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(audio_data_init);
module_exit(audio_data_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Lavey, Columbia University");
MODULE_DESCRIPTION("Audio Driver");
