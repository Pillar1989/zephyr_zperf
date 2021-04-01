/*
 * Shell backend used for testing
 *
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_fbdev_H__
#define SHELL_fbdev_H__

#include <shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_fbdev_transport_api;

struct shell_fbdev {
	bool initialized;
	const struct device *display_dev;
};

#define SHELL_fbdev_DEFINE(_name)					\
	static struct shell_fbdev _name##_shell_fbdev;			\
	struct shell_transport _name = {				\
		.api = &shell_fbdev_transport_api,			\
		.ctx = (struct shell_fbdev *)&_name##_shell_fbdev	\
	}


/**
 * @brief This function shall not be used directly. It provides pointer to shell
 *	  fbdev backend instance.
 *
 * Function returns pointer to the shell fbdev instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_fbdev_get_ptr(void);

void shell_backend_fbdev_clear_output(const struct shell *shell);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_fbdev_H__ */
