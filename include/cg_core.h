/*
 * Copyright Arthur Grillo (c) 2024
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef __CG_CORE_H__
#define __CG_CORE_H__

#include <stdbool.h>
#include <stddef.h>

#include <GL/glew.h>

#include "cg_input.h"
#include "cg_math.h"
#include "cg_util.h"

typedef unsigned char* (*cg_file_reader_callback_t)(const char *file_path, size_t *file_size);

struct cg_window {
	void *base;
	size_t width, height;
};

struct cg_contex {
	struct cg_window window;
	bool window_should_close;

	bool keys[CG_KEY_LEN];
	struct cg_vec2f mouse_pos;
	struct cg_vec2f mouse_rel_pos;

	struct cg_mat4f view_matrix;
	struct cg_mat4f projection_matrix;

	bool fill;

	cg_file_reader_callback_t file_read;
};

void cg_window_create(const char *window_name, size_t width, size_t height);
bool cg_window_should_close(void);

void cg_enable_cursor(void);
void cg_disable_cursor(void);

void cg_set_file_read_callback(cg_file_reader_callback_t func);
void cg_reset_file_read_callback();

#endif // __CG_CORE_H__
