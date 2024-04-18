/*
 * Copyright Arthur Grillo (c) 2024
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "cg_core.h"
#include "cg_gfx.h"
#include "cg_math.h"
#include "cg_util.h"

#include "external/bed.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

float angle_x, angle_y;

int main(void) {
	cg_window_create("OBJ Loading Example", 400 , 400);

	cg_set_file_read_callback(bed_get);

	struct cg_model model = cg_model_from_obj_file("../examples/resources/suzzanne.obj");

	size_t suzzanne_tex_len;
	unsigned char *suzzanne_tex = bed_get("../examples/resources/suzzanne_tex.png",
					      &suzzanne_tex_len);
	cg_assert(suzzanne_tex != NULL);

	int width, height, channels;
	unsigned char *data = stbi_load_from_memory(suzzanne_tex, suzzanne_tex_len,
						    &width, &height, &channels, 3);
	cg_assert(data != NULL);
	cg_assert(channels == 4);

	struct cg_texture tex = cg_texture_create_2d(data, width, height, GL_RGBA, GL_RGB);

	model.materials[0].tex_diffuse = tex;

	stbi_image_free(data);

	while (!cg_window_should_close()) {
		cg_start_render();

		glClearColor(0.1, 0.1, 0.1, 1.0);

		cg_model_draw(&model);

		cg_end_render();
	}
}
