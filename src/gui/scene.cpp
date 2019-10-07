#include "common/logging.h"
#include "gui/scene.h"
#include <string>

Scene::~Scene() {}

Scene::Scene() {
    const std::string vs_str = 
            "#version 330 core\n"
            "layout(location = 0) in vec2 in_position;\n"
            "layout(location = 1) in vec2 in_tex_coord;\n"
            "out vec2 tex_coord;\n"
            "void main(void) {\n"
                "tex_coord = in_tex_coord;\n"
                "gl_Position = vec4(in_position, -0.99999, 1.0);\n"
            "}\n";
    const std::string fs_str = 
            "#version 330 core\n"
            "in vec2 tex_coord;\n"
            "uniform sampler2D tex;\n"
            "out vec4 color;\n"
            "void main(void) {\n"
                "color = texture(tex, tex_coord);\n"
            "}\n";
    shader_.InitFromString(vs_str, fs_str);

    // Only vbo binding, vertexAttribPointer VertexAttribArray are related to vao.
	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);

    // x, y, ux, uy
	std::vector<GLfloat> bg_vertices{
                             -1.f, 1.f, 0.f, 0.f, 
							  -1.f, -1.f, 0.f, 1.f,
							  1.f, -1.f, 1.f, 1.f,
							  1.f, 1.f, 1.f, 0.f };

	GLintptr vertex_position_offset = 0 * sizeof(GLfloat);
	GLintptr vertex_uv_offset = 2 * sizeof(GLfloat);

	std::vector<GLushort> bg_indices{ 0, 1, 2,
									2, 3, 0 };
	bg_n_ = bg_indices.size();

	GLuint bg_vbo, bg_ebo;
	glGenBuffers(1, &bg_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, bg_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * bg_vertices.size(), bg_vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (GLvoid*)vertex_position_offset);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (GLvoid*)vertex_uv_offset);
	
	glGenBuffers(1, &bg_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * bg_indices.size(), bg_indices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenTextures(1, &bg_tex_id_);
	glBindTexture(GL_TEXTURE_2D, bg_tex_id_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindVertexArray(0);
}

void Scene::Draw(const cv::Mat& bg_img) {
	glBindVertexArray(vao_);
	shader_.Use();
	glBindTexture(GL_TEXTURE_2D, bg_tex_id_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bg_img.rows, bg_img.cols, 0, GL_RGB, GL_UNSIGNED_BYTE, bg_img.data);
	glActiveTexture(GL_TEXTURE0);
	shader_.Set("tex", 0);
	glDrawElements(GL_TRIANGLES, bg_n_, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
}