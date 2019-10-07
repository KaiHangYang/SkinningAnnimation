#include <imgui.h>
#include <cstdio>
#include <iostream>

#include "common/logging.h"
#include "common/shader.h"
#include "gui/imgui_impl_glfw.h"
#include "gui/imgui_impl_opengl3.h"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

static void glfw_error_callback(int error, const char* desc) {
	LOG(WARNING) << "GLFW Error: " << error << ", " << desc;
}

int main(int argc, char** argv) {
	cv::VideoCapture video_cap = cv::VideoCapture("C:/Users/kaihang/Desktop/test_video.mp4");
	cv::Mat img;
	video_cap.read(img);
	int wnd_height = video_cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	int wnd_width = video_cap.get(CV_CAP_PROP_FRAME_WIDTH);

	glfwSetErrorCallback(glfw_error_callback);

	CHECK(glfwInit()) << "GLFW Error: Failed to initialize GLFW!";

	// Use OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);		   // 3.0+ only
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);					   // fix window size
	const char* glsl_version = "#version 330";

	GLFWwindow* window = glfwCreateWindow(wnd_width, wnd_height, "Skinning Animation", nullptr, nullptr);

	if (!window) return 1;

	glfwMakeContextCurrent(window);
	// set v-sync, make the gpu frequency the same as 
	// the screen refresh frequency
	glfwSwapInterval(1);

	bool success = gl3wInit() == 0;
	CHECK(success) << "GL3W: Failed to initialize OpenGL loader!";

	// Setup gui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'misc/fonts/README.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	GLuint img_texture_id;
	glGenTextures(1, &img_texture_id);
	glBindTexture(GL_TEXTURE_2D, img_texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	LOG(INFO) << img.cols << " " << img.rows << " " << img_texture_id;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols, img.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Only vbo binding, vertexAttribPointer VertexAttribArray are related to vao.
	GLuint bg_vao;
	glGenVertexArrays(1, &bg_vao);
	glBindVertexArray(bg_vao);
	
	std::vector<GLfloat> bg_vertices{-1.f, 1.f, 0.f, 0.f, // x, y, ux, uy
							  -1.f, -1.f, 0.f, 1.f,
							  1.f, -1.f, 1.f, 1.f,
							  1.f, 1.f, 1.f, 0.f };
	GLintptr vertex_position_offset = 0 * sizeof(GLfloat);
	GLintptr vertex_uv_offset = 2 * sizeof(GLfloat);

	std::vector<GLushort> bg_indices{ 0, 1, 2,
									2, 3, 0 };
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
	glBindVertexArray(0);

	// May try to use pbo to accelerate the reading process.

	Shader bg_shader("../res/shader/bg.vs", "../res/shader/bg.fs");

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	bool show_canvas = false;
	// Main Loop
	while (!glfwWindowShouldClose(window)) {
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Controller
		{
			ImGui::Begin("Controller");
			ImGui::Checkbox("Canvas", &show_canvas);
			ImGui::ColorEdit3("clear color", (float*)(&clear_color));
			ImGui::End();
		}

		//// Canvas
		//if (show_canvas) {
		//	ImGui::Begin("Canvas", &show_canvas);
		//	int test = 3;
		//	ImGui::Image(&test, ImVec2(img.cols, img.rows));
		//	ImGui::End();
		//}
		ImGui::Render();

		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);

		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw my thing for test
		bg_shader.Use();
		glBindVertexArray(bg_vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, img_texture_id);
		bg_shader.Set("tex", 0);
		glDrawElements(GL_TRIANGLES, bg_indices.size(), GL_UNSIGNED_SHORT, 0);
		glBindVertexArray(0);

		// Try to read framebuffer data
		std::vector<float> frame_img(display_w * display_h * 3);

		// Frame buffer coord in opengl is 
		// |
		// |_______
		// but in image coordinate is 
		// ________
		// |
		// |
		// so I need to flip vertically

		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, display_w, display_h, GL_BGR, GL_FLOAT, &frame_img[0]);
		cv::Mat tmp_img(wnd_height, wnd_width, CV_32FC3, &frame_img[0]);
		cv::flip(tmp_img, tmp_img, 0);
		cv::imshow("test", tmp_img);
		cv::waitKey(1);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}


	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}