/*
 * Shell backend used for testing
 *
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "shell_fbdev.h"
#include <init.h>
#include <drivers/display.h>

SHELL_fbdev_DEFINE(shell_transport_fbdev);
SHELL_DEFINE(shell_fbdev, "fbev:~$ ", &shell_transport_fbdev, 
			CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_SIZE,
	     	CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_TIMEOUT, SHELL_FLAG_OLF_CRLF);


#if DT_NODE_HAS_STATUS(DT_INST(0, ilitek_ili9340), okay)
#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, ilitek_ili9340))
#endif


extern void setupScroll(const struct device *display_dev, uint16_t tfa, uint16_t bfa) ;
extern void printString(const struct device *display_dev, const char *str);
extern void clearscreen(const struct device *display_dev, int16_t color);


static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_fbdev *sh_fbdev = (struct shell_fbdev *)transport->ctx;

	if (sh_fbdev->initialized) {
		return -EINVAL;
	}
	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	struct shell_fbdev *sh_fbdev = (struct shell_fbdev *)transport->ctx;

	if (!sh_fbdev->initialized) {
		return -ENODEV;
	}

	sh_fbdev->initialized = false;

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	struct shell_fbdev *sh_fbdev = (struct shell_fbdev *)transport->ctx;

	if (!sh_fbdev->initialized) {
		return -ENODEV;
	}

	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	struct shell_fbdev *sh_fbdev = (struct shell_fbdev *)transport->ctx;

	const uint8_t *data8 = (const uint8_t *)data;
	if (!sh_fbdev->initialized) {

		sh_fbdev->display_dev = device_get_binding(DISPLAY_DEV_NAME);

		if (sh_fbdev->display_dev == NULL) {
			return -ENODEV;
		}
		display_blanking_off(sh_fbdev->display_dev);
		display_set_pixel_format(sh_fbdev->display_dev, PIXEL_FORMAT_RGB_565);
		clearscreen(sh_fbdev->display_dev,0x0000);
		setupScroll(sh_fbdev->display_dev,0,0);
		sh_fbdev->initialized = true;
	}
	printString(sh_fbdev->display_dev, data8);
	*cnt = length;

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_fbdev *sh_fbdev = (struct shell_fbdev *)transport->ctx;

	if (!sh_fbdev->initialized) {
		return -ENODEV;
	}
	*cnt = 0;

	return 0;
}

const struct shell_transport_api shell_fbdev_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read
};

int enable_shell_fbdev(const struct device *arg)
{
	ARG_UNUSED(arg);
	shell_init(&shell_fbdev, NULL, true, true, LOG_LEVEL_INF);
	return 0;
}
//CONFIG_SHELL_BACKEND_FBDEV_INIT_PRIORITY=1
SYS_INIT(enable_shell_fbdev, APPLICATION, 96);


const struct shell *shell_backend_fbdev_get_ptr(void)
{
	return &shell_fbdev;
}



void shell_backend_fbdev_clear_output(const struct shell *shell)
{
	struct shell_fbdev *sh_fbdev = (struct shell_fbdev *)shell->iface->ctx;
	clearscreen(sh_fbdev->display_dev,0x0000);
}
