#pragma once

#include "graphic/shader.h"
#include <GL/gl3w.h>
#include <opencv2/core.hpp>

// Scene class:
// Handle background rendering and 
// frame caputre

// Must be initialized after the glfw ininted
class Scene {
public:
    Scene();
	Scene(const Scene& rhs) = delete;
	Scene& operator()(const Scene& rhs) = delete;

    ~Scene();
	void Draw(const cv::Mat& bg_img);

private:
    Shader shader_;
	int bg_n_;
    GLuint vao_ = 0;
	GLuint bg_tex_id_ = 0;
};