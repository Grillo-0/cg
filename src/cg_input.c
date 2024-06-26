/*
 * Copyright Arthur Grillo (c) 2024
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <SDL.h>

#include "cg_core.h"
#include "cg_input.h"
#include "cg_math.h"

extern struct cg_contex cg_ctx;

bool cg_keycode_is_down(enum cg_keycode code) {
	return cg_ctx.keys[code];
}

struct cg_vec2f cg_mouse_pos() {
	return cg_ctx.mouse_pos;
}

struct cg_vec2f cg_mouse_rel_pos() {
	struct cg_vec2f res = cg_ctx.mouse_rel_pos;
	cg_ctx.mouse_rel_pos = (struct cg_vec2f){0};
	return res;
}
