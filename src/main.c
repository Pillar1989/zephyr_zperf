/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zperf sample.
 */
#include <usb/usb_device.h>
#include <net/net_config.h>
#include "shell_fbdev.h"
extern void btn_init();
extern void btn_loop(const struct shell *sh);


void main(void)
{
	const struct shell *sh = (const struct shell *)shell_backend_fbdev_get_ptr();
	btn_init();
	shell_help(sh);
	shell_backend_fbdev_clear_output(sh);
	btn_loop(sh);

#if defined(CONFIG_USB)
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0) {
		printk("usb enable error %d\n", ret);
	}

	(void)net_config_init_app(NULL, "Initializing network");
#endif /* CONFIG_USB */
}
