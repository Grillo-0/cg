/*
 * Copyright Arthur Grillo (c) 2024
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <SDL2/SDL.h>

#include <GL/glew.h>

#define BED_FUNC_PREFIX cg
#include "external/bed.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "external/tinyobj_loader_c.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#include "cg_core.h"
#include "cg_gfx.h"
#include "cg_input.h"
#include "cg_math.h"
#include "cg_util.h"

#define DEFAULT_TEX_SIZE 32

extern struct cg_contex cg_ctx;

static const char* shader_attrib_names[] = {
	[CG_SATTRIB_LOC_VERTEX_POSITION] = "position",
	[CG_SATTRIB_LOC_VERTEX_UV] = "uv",
	[CG_SATTRIB_LOC_VERTEX_NORMAL] = "normal",
};

static const char* shader_uniform_names[] = {
	[CG_SUNIFORM_MATRIX_MODEL] = "model",
	[CG_SUNIFORM_MATRIX_VIEW] = "view",
	[CG_SUNIFORM_MATRIX_PROJECTION] = "projection",
	[CG_SUNIFORM_DIFFUSE_COLOR] = "diffuse_color",
	[CG_SUNIFORM_DIFFUSE_TEXTURE] = "diffuse_tex",
	[CG_SUNIFORM_DIFFUSE_TEXTURE_PROVIDED] = "diffuse_tex_provided",
};

void cg_start_render(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	cg_assert(!cg_check_gl());
}

void cg_end_render(void) {
	SDL_GL_SwapWindow(cg_ctx.window.base);
}

void cg_set_fill(bool fill) {
	cg_ctx.fill = fill;
	if (fill) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	cg_assert(!cg_check_gl());
}

bool cg_get_fill() {
	return cg_ctx.fill;
}

struct cg_mesh cg_mesh_create(const float *verts, const size_t num_verts,
			      const int *indices, const size_t num_indices,
			      const float *normals, const size_t num_normals,
			      const float *uvs, const size_t num_uvs) {
	cg_assert(verts != NULL);

	struct cg_mesh mesh = {0};

	mesh.verts = malloc(sizeof(*mesh.verts) * num_verts * 3);
	assert(mesh.verts);

	memcpy(mesh.verts, verts, sizeof(*mesh.verts) * num_verts * 3);

	mesh.num_verts = num_verts;

	if (indices != NULL) {
		mesh.indices = malloc(sizeof(*mesh.indices) * num_indices);
		assert(mesh.indices);

		memcpy(mesh.indices, indices, sizeof(*mesh.indices) * num_indices);
		mesh.num_indices = num_indices;
	}

	if (normals != NULL) {
		mesh.normals = malloc(sizeof(*mesh.normals) * num_normals * 3);
		assert(mesh.normals);

		memcpy(mesh.normals, normals, sizeof(*mesh.normals) * num_normals * 3);
		mesh.num_normals = num_normals;
	}

	if (uvs != NULL) {
		mesh.uvs = malloc(sizeof(*mesh.uvs) * num_uvs * 2);
		assert(mesh.uvs);

		memcpy(mesh.uvs, uvs, sizeof(*mesh.uvs) * num_uvs * 2);
		mesh.num_uvs = num_uvs;
	}

	cg_info("Mesh loaded:\n");
	cg_info("\tnumber of vertices: %zu\n", num_verts);
	if (mesh.indices != NULL)
		cg_info("\tnumber of indicies: %zu\n", num_indices);
	if (mesh.normals != NULL)
		cg_info("\tnumber of normals: %zu\n", num_normals);
	if (mesh.uvs != NULL)
		cg_info("\tnumber of uvs: %zu\n", num_uvs);

	glGenVertexArrays(1, &mesh.vao);
	cg_assert(mesh.vao > 0);

	glBindVertexArray(mesh.vao);
	cg_assert(!cg_check_gl());

	glGenBuffers(1, &mesh.vbo);
	cg_assert(mesh.vbo > 0);

	if (mesh.indices != NULL) {
		glGenBuffers(1, &mesh.ebo);
	}

	if (mesh.normals != NULL) {
		glGenBuffers(1, &mesh.nbo);
	}

	if (mesh.uvs != NULL) {
		glGenBuffers(1, &mesh.tbo);
	}

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	cg_assert(!cg_check_gl());

	glBufferData(GL_ARRAY_BUFFER, sizeof(*mesh.verts) * mesh.num_verts * 3, mesh.verts,
		     GL_STATIC_DRAW);
	cg_assert(!cg_check_gl());

	glVertexAttribPointer(CG_SATTRIB_LOC_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE,
			      sizeof(*mesh.verts) * 3, 0);
	cg_assert(!cg_check_gl());

	glEnableVertexAttribArray(CG_SATTRIB_LOC_VERTEX_POSITION);
	cg_assert(!cg_check_gl());

	if (mesh.indices != NULL) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
		cg_assert(!cg_check_gl());

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*mesh.indices) * num_indices,
			     mesh.indices, GL_STATIC_DRAW);
		cg_assert(!cg_check_gl());
	}

	if (mesh.uvs != NULL) {
		glBindBuffer(GL_ARRAY_BUFFER, mesh.tbo);
		cg_assert(!cg_check_gl());

		glBufferData(GL_ARRAY_BUFFER, sizeof(*mesh.uvs) * mesh.num_uvs * 2,
			     mesh.uvs, GL_STATIC_DRAW);
		cg_assert(!cg_check_gl());

		glVertexAttribPointer(CG_SATTRIB_LOC_VERTEX_UV, 2, GL_FLOAT, GL_FALSE,
				      sizeof(*mesh.uvs) * 2, 0);
		cg_assert(!cg_check_gl());

		glEnableVertexAttribArray(CG_SATTRIB_LOC_VERTEX_UV);
		cg_assert(!cg_check_gl());
	}

	if (mesh.normals != NULL) {
		glBindBuffer(GL_ARRAY_BUFFER, mesh.nbo);
		cg_assert(!cg_check_gl());

		glBufferData(GL_ARRAY_BUFFER, sizeof(*mesh.normals) * mesh.num_normals * 3,
			     mesh.normals, GL_STATIC_DRAW);
		cg_assert(!cg_check_gl());

		glVertexAttribPointer(CG_SATTRIB_LOC_VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE,
				      sizeof(*mesh.normals) * 3, 0);
		cg_assert(!cg_check_gl());

		glEnableVertexAttribArray(CG_SATTRIB_LOC_VERTEX_NORMAL);
		cg_assert(!cg_check_gl());
	}

	return mesh;
}

static unsigned int create_shader(const char *src, int length, GLenum type) {
	unsigned int shader = glCreateShader(type);
	cg_assert(shader != 0);

	if (length < 1)
		glShaderSource(shader, 1, &src, NULL);
	else
		glShaderSource(shader, 1, &src, &length);
	cg_assert(!cg_check_gl());

	glCompileShader(shader);
	cg_assert(!cg_check_gl());

	int status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	cg_assert(!cg_check_gl());

	char info_log[1024];
	if (status == false) {
		glGetShaderInfoLog(shader, 1024, NULL, info_log);
		cg_error("Shader compilation error: %s", info_log);
		cg_assert(0);
	}

	return shader;
}

void cg_shader_prg_builder_add_shader(struct cg_shader_prg_builder *builder, const char *src,
				      int length,
				      GLenum type) {
	unsigned int shader = create_shader(src, length, type);
	cg_da_append(&builder->shaders, shader);
}

static void bind_loc(unsigned int prg, enum cg_shader_attrib_loc loc) {
	glBindAttribLocation(prg, loc, shader_attrib_names[loc]);
	cg_assert(!cg_check_gl());
}

struct cg_shader_prg cg_shader_prg_builder_build(struct cg_shader_prg_builder *builder) {
	struct cg_shader_prg prg = {0};

	prg.id = glCreateProgram();
	cg_assert(prg.id != 0);

	for (size_t i = 0; i < builder->shaders.len; i++) {
		unsigned int shader = builder->shaders.items[i];

		glAttachShader(prg.id, shader);
		cg_assert(!cg_check_gl());
	}

	bind_loc(prg.id, CG_SATTRIB_LOC_VERTEX_POSITION);
	bind_loc(prg.id, CG_SATTRIB_LOC_VERTEX_UV);

	glLinkProgram(prg.id);
	cg_assert(!cg_check_gl());

	int status;
	char info_log[1024];
	glGetProgramiv(prg.id, GL_LINK_STATUS, &status);
	cg_assert(!cg_check_gl());

	if (status == false) {
		glGetProgramInfoLog(prg.id, 1024, NULL, info_log);
		cg_error("Shader program linking error: %s\n", info_log);
		cg_assert(0);
	}

	for (size_t i = 0; i < CG_SUNIFORM_SIZE; i++) {
		prg.uniform_locs[i] = glGetUniformLocation(prg.id, shader_uniform_names[i]);
		cg_assert(!cg_check_gl());
	}

	for (size_t i = 0; i < builder->shaders.len; i++) {
		unsigned int shader = builder->shaders.items[i];

		glDeleteShader(shader);
		cg_assert(!cg_check_gl());
	}

	return prg;
}

struct cg_shader_prg cg_shader_prg_default() {
	static struct cg_shader_prg default_shader_prg = {0};

	if (default_shader_prg.id == 0) {
		struct cg_shader_prg_builder builder = {0};

		size_t vert_shader_len;
		const char *vert_shader_src = (char*)cg_bed_get("../resources/shaders/vert.glsl",
								   &vert_shader_len);
		cg_assert(vert_shader_src != NULL);

		cg_shader_prg_builder_add_shader(&builder,
						 vert_shader_src,
						 vert_shader_len,
						 GL_VERTEX_SHADER);

		size_t frag_shader_len;
		const char *frag_shader_src = (char*)cg_bed_get("../resources/shaders/frag.glsl",
								&frag_shader_len);
		cg_assert(frag_shader_src != NULL);

		cg_shader_prg_builder_add_shader(&builder,
						 frag_shader_src,
						 frag_shader_len,
						 GL_FRAGMENT_SHADER);

		default_shader_prg = cg_shader_prg_builder_build(&builder);
	}

	return default_shader_prg;
}

struct cg_texture cg_texture_create_2d(const unsigned char *data, size_t width, size_t height,
				       int internal_format, int format) {
	struct cg_texture tex = { .type = CG_TEXTURE_2D };

	glGenTextures(1, &tex.gl_tex);
	cg_assert(!cg_check_gl());

	glBindTexture(GL_TEXTURE_2D, tex.gl_tex);
	cg_assert(!cg_check_gl());

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE,
		     data);
	cg_assert(!cg_check_gl());

	glGenerateMipmap(GL_TEXTURE_2D);
	cg_assert(!cg_check_gl());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	cg_assert(!cg_check_gl());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	cg_assert(!cg_check_gl());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	cg_assert(!cg_check_gl());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	cg_assert(!cg_check_gl());

	return tex;
}

struct cg_texture cg_texture_from_file_2d(const char *file_path) {
	cg_info("Loading file %s\n", file_path);
	size_t file_size;
	unsigned char *file = cg_ctx.file_read(file_path, &file_size);
	if (file == NULL) {
		cg_error("File %s nor found\n", file_path);
	}
	cg_assert(file != NULL);

	int width, height, channels;
	unsigned char *data = stbi_load_from_memory(file, file_size, &width, &height, &channels, 0);
	cg_assert(data != NULL);

	int internal_format = -1;
	int format = -1;

	switch (channels) {
		case 1:
			internal_format = GL_RED;
			format = GL_RED;
			break;
		case 2:
			internal_format = GL_RG;
			format = GL_RG;
			break;
		case 3:
			internal_format = GL_RGB;
			format = GL_RGB;
			break;
		case 4:
			internal_format = GL_RGBA;
			format = GL_RGBA;
			break;
	}

	cg_assert(internal_format != -1);
	cg_assert(format != -1);

	struct cg_texture tex = cg_texture_create_2d(data, width, height, internal_format, format);

	stbi_image_free(data);

	return tex;
}

struct cg_texture cg_texture_default() {
	static struct cg_texture default_tex = {0};

	if (default_tex.gl_tex == 0) {
		unsigned char default_tex_data[DEFAULT_TEX_SIZE * 4 * DEFAULT_TEX_SIZE] = {0};

		for (size_t y = 0; y < DEFAULT_TEX_SIZE; y++) {
			for (size_t x = 0; x < DEFAULT_TEX_SIZE; x++) {
				if ((x + y % 2) % 2 == 0) {
					default_tex_data[(x + y * DEFAULT_TEX_SIZE) * 4 + 0] = 0xff;
					default_tex_data[(x + y * DEFAULT_TEX_SIZE) * 4 + 1] = 0x00;
					default_tex_data[(x + y * DEFAULT_TEX_SIZE) * 4 + 2] = 0xff;
				}
			}
		}

		default_tex = cg_texture_create_2d(default_tex_data,
						   DEFAULT_TEX_SIZE, DEFAULT_TEX_SIZE,
						   GL_RGBA, GL_RGBA);
		glBindTexture(GL_TEXTURE_2D, default_tex.gl_tex);
		cg_assert(!cg_check_gl());
		cg_assert(!cg_check_gl());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		cg_assert(!cg_check_gl());
	}

	return default_tex;
}

struct cg_material cg_material_default() {
	struct cg_material ret = {
		.shader = cg_shader_prg_default(),
		.enable_color = true,
		.tex_diffuse = cg_texture_default(),
	};

	return ret;
}
static void find_coord_min_max(const float *vertices, const size_t vert_len,
			       float *x_min, float *x_max,
			       float *y_min, float *y_max,
			       float *z_min, float *z_max) {
	*x_min = vertices[0];
	*x_max = vertices[0];
	*y_min = vertices[1];
	*y_max = vertices[1];
	*z_min = vertices[2];
	*z_max = vertices[2];

	for (size_t i = 0; i < vert_len; i += 1) {
		*x_min = CG_MIN(vertices[i * 3 + 0], *x_min);
		*x_max = CG_MAX(vertices[i * 3 + 0], *x_max);
		*y_min = CG_MIN(vertices[i * 3 + 1], *y_min);
		*y_max = CG_MAX(vertices[i * 3 + 1], *y_max);
		*z_min = CG_MIN(vertices[i * 3 + 2], *z_min);
		*z_max = CG_MAX(vertices[i * 3 + 2], *z_max);
	}
}


struct cg_model cg_model_create(const struct cg_mesh *meshes, const size_t num_meshes,
				const struct cg_material *materials, const size_t num_materials,
				const size_t *mesh_to_material) {
	struct cg_material default_material = cg_material_default();
	size_t default_mesh_to_material[] = {0};

	struct cg_model ret = {
		.num_meshes = num_meshes,
		.num_materials = num_materials,
		.scale = (struct cg_vec3f){1, 1, 1},
	};

	if (materials == NULL) {
		ret.num_materials = 1;
		materials = &default_material;
		mesh_to_material = default_mesh_to_material;
	}

	ret.meshes = malloc(ret.num_meshes * sizeof(*ret.meshes));
	cg_assert(ret.meshes != NULL);
	memcpy(ret.meshes, meshes, ret.num_meshes * sizeof(*ret.meshes));

	ret.materials = malloc(ret.num_materials * sizeof(*ret.materials));
	cg_assert(ret.materials != NULL);
	memcpy(ret.materials, materials, ret.num_materials * sizeof(*ret.materials));

	ret.mesh_to_material = malloc(ret.num_meshes * sizeof(*ret.mesh_to_material));
	cg_assert(ret.mesh_to_material != NULL);
	memcpy(ret.mesh_to_material, mesh_to_material, ret.num_meshes * sizeof(*ret.mesh_to_material));

	struct cg_box bounding_box = {0};
	for (size_t i = 0; i < ret.num_meshes; i++) {
		float x_min, x_max;
		float y_min, y_max;
		float z_min, z_max;
		find_coord_min_max(ret.meshes[i].verts, ret.meshes[i].num_verts,
				   &x_min, &x_max,
				   &y_min, &y_max,
				   &z_min, &z_max);
		bounding_box.min.x = CG_MIN(bounding_box.min.x, x_min);
		bounding_box.max.x = CG_MAX(bounding_box.max.x, x_max);
		bounding_box.min.y = CG_MIN(bounding_box.min.y, y_min);
		bounding_box.max.y = CG_MAX(bounding_box.max.y, y_max);
		bounding_box.min.z = CG_MIN(bounding_box.min.z, z_min);
		bounding_box.max.z = CG_MAX(bounding_box.max.z, z_max);
	}

	ret.bounding_box = bounding_box;

	return ret;
}

static void tn_read_file_callback(void *ctx, const char *filename, int is_mtl,
			          const char *obj_filename, char **buf, size_t *len) {
	(void) ctx;
	(void) is_mtl;
	(void) obj_filename;

	*buf = (char*)cg_ctx.file_read(filename, len);
}

static struct cg_texture load_tex_relative(const char *model_path, const char *image_path) {
	char *dir_end = strrchr(model_path, '/');
	if (dir_end != NULL) {
		char *image_dir_end = strrchr(image_path, '/');
		if (image_dir_end != NULL) {
			*image_dir_end = '\0';
			if (strstr(image_path, "..") == NULL) {
				return cg_texture_from_file_2d(image_path);
			}
			*image_dir_end = '/';
		}

		int model_dir_len;

		if (dir_end == NULL) {
			model_dir_len = 0;
		} else {
			model_dir_len  = dir_end - model_path + 1;
		}

		char *abs_image_path = calloc(model_dir_len + strlen(image_path) + 1, sizeof(char));
		sprintf(abs_image_path, "%.*s%s", model_dir_len, model_path, image_path);

		struct cg_texture ret = cg_texture_from_file_2d(abs_image_path);

		free(abs_image_path);

		return ret;

	} else {
		return cg_texture_from_file_2d(image_path);
	}
}

struct cg_model cg_model_from_obj_file(const char *file_path) {
	cg_assert(file_path != NULL);

	tinyobj_attrib_t tn_attrib = {0};
	tinyobj_attrib_init(&tn_attrib);
	tinyobj_shape_t *tn_shapes;
	size_t tn_num_shapes;
	tinyobj_material_t *tn_materials;
	size_t tn_num_materials;
	int ret = tinyobj_parse_obj(&tn_attrib, &tn_shapes, &tn_num_shapes,
				    &tn_materials, &tn_num_materials,
				    file_path, tn_read_file_callback, NULL,
				    0);
	cg_assert(ret == TINYOBJ_SUCCESS);

	float x_min, x_max;
	float y_min, y_max;
	float z_min, z_max;
	find_coord_min_max(tn_attrib.vertices, tn_attrib.num_vertices,
			   &x_min, &x_max,
			   &y_min, &y_max,
			   &z_min, &z_max);

	float x_size = x_max - x_min;
	float y_size = y_max - y_min;
	float z_size = z_max - z_min;

	for (size_t i = 0; i < tn_attrib.num_vertices * 3; i += 3) {
		float *x = &tn_attrib.vertices[i + 0];
		float *y = &tn_attrib.vertices[i + 1];
		float *z = &tn_attrib.vertices[i + 2];

		*x = (*x - x_size / 2 - x_min) / x_size;
		*y = (*y - y_size / 2 - y_min) / x_size;
		*z = (*z - z_size / 2 - z_min) / x_size;
	}

	float *ex_verts = tn_attrib.num_vertices == 0 ? NULL : malloc(sizeof(*ex_verts) *
								      tn_attrib.num_faces * 3);
	float *ex_uvs = tn_attrib.num_texcoords == 0 ? NULL : malloc(sizeof(*ex_uvs) *
								     tn_attrib.num_faces * 2);
	float *ex_norms = tn_attrib.num_normals == 0 ? NULL : malloc(sizeof(*ex_uvs) *
								     tn_attrib.num_faces * 3);

	for (size_t i = 0; i < tn_attrib.num_faces; i++) {
		if (ex_verts != NULL) {
			ex_verts[i * 3 + 0] = tn_attrib.vertices[tn_attrib.faces[i].v_idx * 3 + 0];
			ex_verts[i * 3 + 1] = tn_attrib.vertices[tn_attrib.faces[i].v_idx * 3 + 1];
			ex_verts[i * 3 + 2] = tn_attrib.vertices[tn_attrib.faces[i].v_idx * 3 + 2];
		}

		if (ex_uvs != NULL) {
			ex_uvs[i * 2 + 0] = tn_attrib.texcoords[tn_attrib.faces[i].vt_idx * 2 + 0];
			ex_uvs[i * 2 + 1] = tn_attrib.texcoords[tn_attrib.faces[i].vt_idx * 2 + 1];
		}

		if (ex_norms != NULL) {
			ex_norms[i * 3 + 0] = tn_attrib.normals[tn_attrib.faces[i].vn_idx * 3 + 0];
			ex_norms[i * 3 + 1] = tn_attrib.normals[tn_attrib.faces[i].vn_idx * 3 + 1];
			ex_norms[i * 3 + 2] = tn_attrib.normals[tn_attrib.faces[i].vn_idx * 3 + 2];
		}
	}

	struct CG_DA(struct cg_mesh) meshes = {0};

	size_t *mesh_to_material = calloc(tn_num_shapes, sizeof(*mesh_to_material));

	for (size_t i = 0;  i < tn_num_shapes; i++) {
		mesh_to_material[i] = i;
	}

	bool add_default_material = false;

	for (size_t i = 0;  i < tn_num_shapes; i++) {
		size_t num_indices = tn_shapes[i].length * 3;
		size_t indices_offset = tn_shapes[i].face_offset * 3;
		struct cg_mesh m = cg_mesh_create(ex_verts + indices_offset * 3, num_indices,
						  NULL, 0,
						  ex_norms + indices_offset * 3, num_indices,
						  ex_uvs + indices_offset * 2, num_indices);
		cg_da_append(&meshes, m);

		int mesh_to_mat = tn_attrib.material_ids[tn_shapes[i].face_offset];
		if (mesh_to_mat == -1) {
			add_default_material = true;
			mesh_to_mat = tn_num_materials;
		}

		mesh_to_material[i] = mesh_to_mat;
	}

	struct CG_DA(struct cg_material) materials = {0};

	for (size_t i = 0;  i < tn_num_materials; i++) {
		struct cg_material m = {0};

		m.shader = cg_shader_prg_default();

		m.color_ambient = cg_vec3f_from_array(tn_materials[i].ambient);
		m.color_diffuse = cg_vec3f_from_array(tn_materials[i].diffuse);
		m.color_specular = cg_vec3f_from_array(tn_materials[i].specular);
		m.color_transmittance = cg_vec3f_from_array(tn_materials[i].transmittance);
		m.color_emission = cg_vec3f_from_array(tn_materials[i].emission);

		m.specular_exponent = tn_materials[i].shininess;
		m.index_of_refraction = tn_materials[i].ior;
		m.opacity = tn_materials[i].dissolve;

		m.enable_color = true;

		if (tn_materials[i].ambient_texname)
			m.tex_ambient = load_tex_relative(file_path,
							  tn_materials[i].ambient_texname);
		if (tn_materials[i].diffuse_texname)
			m.tex_diffuse = load_tex_relative(file_path,
							  tn_materials[i].diffuse_texname);
		if (tn_materials[i].specular_texname)
			m.tex_specular = load_tex_relative(file_path,
							   tn_materials[i].specular_texname);
		if (tn_materials[i].specular_highlight_texname)
			m.tex_specular_highlight =
				load_tex_relative(file_path,
						  tn_materials[i].specular_highlight_texname);
		if (tn_materials[i].bump_texname)
			m.tex_bump = load_tex_relative(file_path, tn_materials[i].bump_texname);
		if (tn_materials[i].displacement_texname)
			m.tex_displacement =
				load_tex_relative(file_path, tn_materials[i].displacement_texname);
		if (tn_materials[i].alpha_texname)
			m.tex_alpha = load_tex_relative(file_path, tn_materials[i].alpha_texname);

		cg_da_append(&materials, m);
	}

	if (add_default_material) {
		cg_da_append(&materials, cg_material_default());
	}

	struct cg_model model = cg_model_create(meshes.items, meshes.len,
						materials.items, materials.len, mesh_to_material);

	free(materials.items);
	free(mesh_to_material);
	free(meshes.items);
	free(ex_verts);
	free(ex_uvs);
	free(ex_norms);
	tinyobj_attrib_free(&tn_attrib);
	tinyobj_shapes_free(tn_shapes, tn_num_shapes);
	tinyobj_materials_free(tn_materials, tn_num_materials);

	return model;
}

void cg_model_set_position(struct cg_model *model, struct cg_vec3f position) {
	model->position = position;
}

void cg_model_move(struct cg_model *model, struct cg_vec3f ds) {
	model->position = cg_vec3f_add(model->position, ds);
}

void cg_model_set_rotation(struct cg_model *model, struct cg_vec3f rotation) {
	model->rotation = rotation;
}

void cg_model_rotate(struct cg_model *model, struct cg_vec3f dr) {
	model->rotation = cg_vec3f_add(model->rotation, dr);
}

void cg_model_set_scale(struct cg_model *model, struct cg_vec3f scale) {
	model->scale = scale;
}

void cg_model_scale(struct cg_model *model, struct cg_vec3f ds) {
	model->scale = cg_vec3f_mul(model->scale, ds);
}

struct cg_box cg_model_get_bounding_box(struct cg_model *model) {
	struct cg_box ret = {0};

	struct cg_mat4f m = cg_mat4f_model(model->position, model->scale, model->rotation);
	ret.min = cg_vec3f_mat4f_multiply(model->bounding_box.min, m);
	ret.max = cg_vec3f_mat4f_multiply(model->bounding_box.max, m);

	return ret;
}

void cg_model_draw(struct cg_model *model) {
	for (size_t i = 0; i < model->num_meshes; i++) {
		struct cg_material *material = &model->materials[model->mesh_to_material[i]];
		struct cg_shader_prg *shader = &material->shader;
		struct cg_mat4f m = cg_mat4f_model(model->position, model->scale, model->rotation);

		glUseProgram(material->shader.id);
		cg_assert(!cg_check_gl());

		glUniformMatrix4fv(material->shader.uniform_locs[CG_SUNIFORM_MATRIX_MODEL],
				   1, false, m.d);
		cg_assert(!cg_check_gl());

		glUniformMatrix4fv(material->shader.uniform_locs[CG_SUNIFORM_MATRIX_VIEW],
				1, false, cg_ctx.view_matrix.d);
		cg_assert(!cg_check_gl());

		glUniformMatrix4fv(material->shader.uniform_locs[CG_SUNIFORM_MATRIX_PROJECTION],
				1, false, cg_ctx.projection_matrix.d);
		cg_assert(!cg_check_gl());

		if (material->tex_diffuse.gl_tex != 0 &&
		    shader->uniform_locs[CG_SUNIFORM_DIFFUSE_TEXTURE] != -1) {
			glUniform1i(shader->uniform_locs[CG_SUNIFORM_DIFFUSE_TEXTURE_PROVIDED], 1);
			glUniform1i(shader->uniform_locs[CG_SUNIFORM_DIFFUSE_TEXTURE], 0);
			cg_assert(!cg_check_gl());
			glActiveTexture(GL_TEXTURE0);
			cg_assert(!cg_check_gl());
			glBindTexture(GL_TEXTURE_2D, material->tex_diffuse.gl_tex);
			cg_assert(!cg_check_gl());
		} else {
			glUniform1i(shader->uniform_locs[CG_SUNIFORM_DIFFUSE_TEXTURE_PROVIDED], 0);
			glUniform3f(material->shader.uniform_locs[CG_SUNIFORM_DIFFUSE_COLOR],
				    material->color_diffuse.x,
				    material->color_diffuse.y,
				    material->color_diffuse.z);
			cg_assert(!cg_check_gl());
		}

		struct cg_mesh *mesh = &model->meshes[i];
		glBindVertexArray(mesh->vao);
		cg_assert(!cg_check_gl());

		if (mesh->indices == NULL)
			glDrawArrays(GL_TRIANGLES, 0, mesh->num_verts);
		else
			glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, 0);

		cg_assert(!cg_check_gl());
	}
}

static struct cg_mesh mesh_cube() {
	static bool created = false;
	static float box_verts[8 * 3];

	if (!created) {
		created = true;

		for (size_t z = 0; z <= 1; z++) {
			for (size_t y = 0; y <= 1; y++) {
				for (size_t x = 0; x <= 1; x++) {
					size_t index = z * 4 * 3 + y * 3 * 2 + x * 3;
					box_verts[index + 0] = x - 0.5;
					box_verts[index + 1] = y - 0.5;
					box_verts[index + 2] = z - 0.5;
				}
			}
		}

	}


	static int box_indices[6 * 2 * 3] = {
		0, 1, 2,
		2, 1, 3,

		4, 5, 6,
		6, 5, 7,

		2, 6, 3,
		6, 7, 3,

		0, 1, 4,
		4, 5, 1,

		1, 3, 7,
		5, 1, 7,

		0, 6, 2,
		0, 4, 6,
	};

	return cg_mesh_create(box_verts, CG_ARRAY_LEN(box_verts) / 3,
			      box_indices, CG_ARRAY_LEN(box_indices),
			      NULL, 0,
			      NULL, 0);
}

void cg_model_draw_bounding_box(struct cg_model *model) {
	struct cg_box bounding_box = model->bounding_box;

	struct cg_material material = cg_material_default();
	material.tex_diffuse = (struct cg_texture){0};
	material.color_diffuse = (struct cg_vec3f){1.0, 0.0, 0.0};

	struct cg_mesh cube_mesh = mesh_cube();
	struct cg_model box = cg_model_create(&cube_mesh, 1, &material, 1, (size_t[]){0});

	struct cg_vec3f sizes = cg_vec3f_sub(bounding_box.max, bounding_box.min);

	cg_model_set_scale(&box, sizes);

	cg_model_move(&box, model->position);
	cg_model_scale(&box, model->scale);
	cg_model_rotate(&box, model->rotation);

	bool fill = cg_get_fill();
	cg_set_fill(false);
	cg_model_draw(&box);
	cg_set_fill(fill);
}

struct cg_camera cg_camera_create(const struct cg_vec3f pos,
				  const float fov,
				  const float near_plane,
				  const float far_plane) {

	float aspect = cg_ctx.window.width / cg_ctx.window.height;

	float_t depth = near_plane - far_plane;

	cg_ctx.projection_matrix = (struct cg_mat4f) {{
		1.0 / (tanf(fov / 2) * aspect), 0.0, 0.0, 0.0,
		0.0, 1.0 / tanf(fov / 2), 0.0 , 0.0,
		0.0, 0.0, (near_plane + far_plane) / depth, 2 * near_plane * far_plane / depth,
		0.0, 0.0, -1.0, 0.0,
	}};

	return (struct cg_camera) {
		.pos = pos,
		.rotation = cg_mat4f_identity(),
		.fov = fov,
		.near_plane = near_plane,
		.far_plane = far_plane,
	};
}

void cg_camera_update_FPS(struct cg_camera *camera) {
	struct cg_vec3f ds = {0};

	if (cg_keycode_is_down(CG_KEY_W))
		ds.z -= 0.1;

	if (cg_keycode_is_down(CG_KEY_S))
		ds.z += 0.1;

	if (cg_keycode_is_down(CG_KEY_A))
		ds.x -= 0.1;

	if (cg_keycode_is_down(CG_KEY_D))
		ds.x += 0.1;

	struct cg_vec2f rel_pos = cg_mouse_rel_pos();

	rel_pos.x /= cg_ctx.window.width;
	rel_pos.y /= cg_ctx.window.height;

	rel_pos.x *= 10.0;
	rel_pos.y *= 10.0;

	camera->rotation = cg_mat4f_multiply(cg_mat4f_rotate_y(rel_pos.x), camera->rotation);
	camera->rotation = cg_mat4f_multiply(camera->rotation, cg_mat4f_rotate_x(rel_pos.y));

	float pitch, yaw, roll;
	cg_mat4f_rotation_to_angles(camera->rotation, &pitch, &yaw, &roll);
	ds = cg_vec3f_mat4f_multiply(ds, cg_mat4f_rotate_y(yaw));
	camera->pos = cg_vec3f_add(camera->pos, ds);

	struct cg_mat4f translation = cg_mat4f_translate(-camera->pos.x,
							 -camera->pos.y,
							 -camera->pos.z);

	cg_ctx.view_matrix = cg_mat4f_multiply(translation, camera->rotation);
}
