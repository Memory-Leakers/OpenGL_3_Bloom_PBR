//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
//#include <stb_image.h>
//#include <stb_image_write.h>
//#include "Globals.h"
#include "ModelLoaderFuncs.h"

#define MIPMAP_BASE_LEVEL 0
#define MIPMAP_MAX_LEVEL 4

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
	GLchar  infoLogBuffer[1024] = {};
	GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
	GLsizei infoLogSize;
	GLint   success;

	char versionString[] = "#version 430\n";
	char shaderNameDefine[128];
	sprintf(shaderNameDefine, "#define %s\n", shaderName);
	char vertexShaderDefine[] = "#define VERTEX\n";
	char fragmentShaderDefine[] = "#define FRAGMENT\n";

	const GLchar* vertexShaderSource[] = {
		versionString,
		shaderNameDefine,
		vertexShaderDefine,
		programSource.str
	};
	const GLint vertexShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(vertexShaderDefine),
		(GLint)programSource.len
	};
	const GLchar* fragmentShaderSource[] = {
		versionString,
		shaderNameDefine,
		fragmentShaderDefine,
		programSource.str
	};
	const GLint fragmentShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(fragmentShaderDefine),
		(GLint)programSource.len
	};

	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
	glCompileShader(vshader);
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
	glCompileShader(fshader);
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint programHandle = glCreateProgram();
	glAttachShader(programHandle, vshader);
	glAttachShader(programHandle, fshader);
	glLinkProgram(programHandle);
	glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	glUseProgram(0);

	glDetachShader(programHandle, vshader);
	glDetachShader(programHandle, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);

	return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
	String programSource = ReadTextFile(filepath);

	Program program = {};
	program.handle = CreateProgramFromSource(programSource, programName);
	program.filepath = filepath;
	program.programName = programName;
	program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);

	GLint attributeCount = 0;
	glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

	for (GLuint i = 0; i < attributeCount; i++)
	{
		GLsizei length = 256; // GLsizei length = 0;
		GLint size = 0;
		GLenum type = 0;
		GLchar name[256];
		glGetActiveAttrib(program.handle, i,
			ARRAY_COUNT(name),
			&length,
			&size,
			&type,
			name);

		u8 location = glGetAttribLocation(program.handle, name);
		program.shaderLayout.attributes.push_back(VertexShaderAttribute{ location, (u8)size });
	}

	app->programs.push_back(program);

	return app->programs.size() - 1;
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
	GLuint ReturnValue = 0;

	SubMesh& Submesh = mesh.submeshes[submeshIndex];
	for (u32 i = 0; i < (u32)Submesh.vaos.size(); ++i)
	{
		if (Submesh.vaos[i].programHandle == program.handle)
		{
			ReturnValue = Submesh.vaos[i].handle;
			break;
		}
	}

	if (ReturnValue == 0)
	{
		glGenVertexArrays(1, &ReturnValue);
		glBindVertexArray(ReturnValue);

		glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

		auto& ShaderLayout = program.shaderLayout.attributes;
		for (auto ShaderIt = ShaderLayout.cbegin(); ShaderIt != ShaderLayout.cend(); ++ShaderIt)
		{
			bool attributeWasLinked = false;
			auto SubmeshLayout = Submesh.vertexBufferLayout.attributes;
			for (auto SubmeshIt = SubmeshLayout.cbegin(); SubmeshIt != SubmeshLayout.cend(); ++SubmeshIt)
			{
				if (ShaderIt->location == SubmeshIt->location)
				{
					const u32 index = SubmeshIt->location;
					const u32 ncomp = SubmeshIt->componentCount;
					const u32 offset = SubmeshIt->offset + Submesh.vertexOffset;
					const u32 stride = Submesh.vertexBufferLayout.stride;

					glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)(offset));
					glEnableVertexAttribArray(index);

					attributeWasLinked = true;
					break;
				}
			}

			assert(attributeWasLinked);
		}
		glBindVertexArray(0);

		VAO vao = { ReturnValue, program.handle };
		Submesh.vaos.push_back(vao);
	}

	return ReturnValue;
}

glm::mat4 TransformPositionScale(const vec3& position, const vec3& scaleFactors)
{
	glm::mat4 toReturn = glm::translate(position);
	toReturn = glm::scale(toReturn, scaleFactors);
	return toReturn;
}

glm::mat4 TranformScale(const vec3& scaleFactors)
{
	return glm::scale(scaleFactors);
}

void CreateLight(App* app, Light light)
{
	app->lights.push_back(light);

	//If point light, create and relate a light
	if (light.type == LightType_Point)
	{
		app->entities.push_back({ light.position, vec3(0.1), app->SphereModelIndex, 0, 0 });
		app->lights[app->lights.size() - 1].sphere = app->entities.size() - 1;
	}
}

void Init(App* app)
{
	// TODO: Initialize your resources here!
	// - vertex buffers
	// - element/index buffers
	// - vaos
	// - programs (and retrieve uniform indices)
	// - textures

	//Get OPENGL info.
	app->openglDebugInfo += "Open GL version:\n" + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));

	//Vertex Buffer Object (VBO)
	glGenBuffers(1, &app->embeddedVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Element Buffer Objects (EBO)
	glGenBuffers(1, &app->embeddedElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Vertex Array Object (VAO)
	glGenVertexArrays(1, &app->vao);
	glBindVertexArray(app->vao);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	app->renderToBackBufferShader = LoadProgram(app, "Shaders/RENDER_TO_BB.glsl", "RENDER_TO_BB"); //Forward
	app->renderToFrameBufferShader = LoadProgram(app, "Shaders/RENDER_TO_FB.glsl", "RENDER_TO_FB"); //Deferred
	app->framebufferToQuadShader = LoadProgram(app, "Shaders/FB_TO_BB.glsl", "FB_TO_BB");
	//app->framebufferToQuadShader = LoadProgram(app, "Shaders/FB_TO_QUAD.glsl", "FB_TO_QUAD");

	// Load bloom shaders
	app->blitBrightestPixelsShader = LoadProgram(app, "Shaders/PASS_BLIT_BRIGHT.glsl", "PASS_BLIT_BRIGHT");
	app->blurShader = LoadProgram(app, "Shaders/BLUR.glsl", "BLUR");
	app->bloomShader = LoadProgram(app, "Shaders/BLOOM.glsl", "BLOOM");

	const Program& texturedMeshProgram = app->programs[app->renderToBackBufferShader];
	app->texturedMeshProgram_uTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");
	app->texturedMeshProgram_uMetallic = glGetUniformLocation(texturedMeshProgram.handle, "uMetallic");
	app->texturedMeshProgram_uRoughness = glGetUniformLocation(texturedMeshProgram.handle, "uRoughness");
	app->texturedMeshProgram_uAO = glGetUniformLocation(texturedMeshProgram.handle, "uAO");
	app->texturedMeshProgram_uNormal = glGetUniformLocation(texturedMeshProgram.handle, "uNormal");
	app->texturedMeshProgram_uEmissive = glGetUniformLocation(texturedMeshProgram.handle, "uEmissive");

	//u32 PatrickModelIndex = ModelLoader::LoadModel(app, "Models/Patrick/Patrick.obj");
	u32 GroundModelIndex = ModelLoader::LoadModel(app, "Models/Ground/ground.obj");
	//u32 GoombaModelIndex = ModelLoader::LoadModel(app, "Models/Goomba/goomba.obj");
	app->SphereModelIndex = ModelLoader::LoadModel(app, "Models/Sphere/sphere.obj");
	u32 ChestModelIndex = ModelLoader::LoadModel(app, "Models/Chest/Chest.obj");

	u32 CarModelIndex = ModelLoader::LoadModel(app, "Models/Car/Car.obj");

	VertexBufferLayout vertexBufferLayout = {};
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, 3 * sizeof(float) });
	vertexBufferLayout.stride = 5 * sizeof(float);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

	app->localUniformBuffer = CreateConstantBuffer(app->maxUniformBufferSize);

	//app->entities.push_back({ vec3(0.0, 0.0, 0.0), vec3(0.45), PatrickModelIndex, 0, 0});
	//app->entities.push_back({ vec3(2.35, 0.0, 0.0), vec3(0.45), PatrickModelIndex, 0, 0 });
	//app->entities.push_back({ vec3(-2.35, 0.0, 0.0), vec3(0.45), PatrickModelIndex, 0, 0 });

	app->entities.push_back({ vec3(0.0, -1.0, 0.0), vec3(0.01), CarModelIndex, 0, 0 });

	app->entities.push_back({ vec3(0, -1.55, 0.0), vec3(5, 5, 5), GroundModelIndex, 0, 0 });

	// app->entities.push_back({ vec3(2.5, -1, 2.5), vec3(0.03), GoombaModelIndex, 0, 0 });

	app->entities.push_back({ vec3(-2.5, -1.5, 2.5), vec3(2), ChestModelIndex, 0, 0 });

	CreateLight(app, { LightType::LightType_Directional, vec3(1.0, 1.0, 1.0), vec3(-0.70, 0.0, -0.2), vec3(0.0, 0.0, 0.0), 2.5f });
	//CreateLight(app, { LightType::LightType_Directional, vec3(1.0, 0.0, 1.0), vec3(-1.0, 1.0, -1.0), vec3(0.0, 0.0, 0.0), 1.0f});
	//CreateLight(app, { LightType::LightType_Point, vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0), vec3(0.0, 3.0, 0.0), 1.0f });
	//CreateLight(app, { LightType::LightType_Point, vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 1.0), vec3(6.0, 0.0, 4.0), 1.0f });
	CreateLight(app, { LightType::LightType_Point, vec3(0.05, 0.74, 0.97), vec3(1.0, 1.0, 1.0), vec3(-1.30, -0.1, 2.0), 87.0f });

	app->ConfigureFrameBuffer(app->defferedFrameBuffer);

	app->mode = Mode_Forward;

	app->cam.Init(app->displaySize);

	app->InitBloomEffect();

	app->bloom.active = false;
}

void Gui(App* app)
{
	ImGui::Begin("Others");

	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Camera Position");
	ImGui::DragFloat3("", &app->cam.position.x, 0.1f);
	ImGui::Dummy(ImVec2(10, 10));
	ImGui::DragFloat("Speed", &app->cam.cameraSpeed, 0.1f, 0.0f);
	ImGui::Dummy(ImVec2(10, 10));
	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Direction");
	ImGui::Text("(%.3f, %.3f, %.3f)", app->cam.direction.x, app->cam.direction.y, app->cam.direction.z);

	ImGui::End();

	ImGui::Begin("Info");
	ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
	ImGui::Text("%s", app->openglDebugInfo.c_str());

	ImGui::Checkbox("Bloom", &app->bloom.active);
	ImGui::Checkbox("Bloom test", &app->bloom.test);
	ImGui::SliderFloat("Bloom threshold", &app->bloom.threshold, 0, 1);
	ImGui::Image((ImTextureID)app->prefinalTextureID, ImVec2(320, 180), ImVec2(0, 1), ImVec2(1, 0));

	const char* RenderModes[] = { "FORWARD", "DEFERRED", "DEPTH", "ALBEDO", "NORMALS", "POSITION", "VIEW DIRECTION", "METALLIC", "ROUGHNESS", "AMBIENT OCCLUSSION", "EMISSIVE" };
	if (ImGui::BeginCombo("Render Mode", RenderModes[app->mode]))
	{
		for (size_t i = 0; i < ARRAY_COUNT(RenderModes); ++i)
		{
			bool isSelected = (i == app->mode);
			if (ImGui::Selectable(RenderModes[i], isSelected))
			{
				app->mode = static_cast<Mode>(i);
			}
		}
		ImGui::EndCombo();
	}

	if (app->mode != Mode::Mode_Forward)
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "G-Buffer textures");
		ImGui::Dummy(ImVec2(20.0f, 20.0f));
		for (size_t i = 0; i < app->defferedFrameBuffer.colorAttachment.size(); ++i)
		{
			ImGui::Text(RenderModes[i + 3]);
			ImGui::Image((ImTextureID)app->defferedFrameBuffer.colorAttachment[i], ImVec2(320, 180), ImVec2(0, 1), ImVec2(1, 0));
			ImGui::Dummy(ImVec2(7.5f, 7.5f));
		}
		ImGui::Text("DEPTH");
		ImGui::Image((ImTextureID)app->defferedFrameBuffer.depthHandle, ImVec2(320, 180), ImVec2(0, 1), ImVec2(1, 0));
	}

	ImGui::End();

	// LIGHTS

	static int auxIndex = -1;
	static std::string auxSelectedName = "";
	std::string name = "";

	ImGui::Begin("Lights");

	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Click Lights to modify them.");
	ImGui::Dummy(ImVec2(10, 10));
	for (int i = 0; i < app->lights.size(); ++i)
	{
		if (app->lights[i].type == LightType_Directional)
		{
			name = "Directional Light ";
		}
		else
		{
			name = "Point Light ";
		}
		name += std::to_string(i);

		if (ImGui::Selectable(name.c_str(), &app->lights[i].selected))
		{
			auxIndex = i;
			auxSelectedName = name + " is selected!";
		}
	}

	ImGui::Dummy(ImVec2(10, 10));
	if (auxIndex > -1)
	{
		if (app->lights[auxIndex].selected)
		{
			//Deselect others
			for (int k = 0; k < app->lights.size(); k++) {
				app->lights[k].selected = (k == auxIndex);
			}

			//Modify
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), auxSelectedName.c_str());
			ImGui::Spacing();

			if (ImGui::DragFloat3("Position", &app->lights[auxIndex].position.x, 0.1f))
			{
				if (app->lights[auxIndex].sphere != -1)
				{
					app->entities[app->lights[auxIndex].sphere].position = app->lights[auxIndex].position;
				}
			}
			ImGui::DragFloat3("Direction", &app->lights[auxIndex].direction.x, 0.1f, -1.0f, 1.0f);
			ImGui::DragFloat("Intensity", &app->lights[auxIndex].intensity);
			ImGui::ColorPicker3("Color", &app->lights[auxIndex].color.x);
		}
	}

	ImGui::End();
}

void Update(App* app)
{
	// CAMERA MOVEMENT
	float cameraSpeed = app->cam.cameraSpeed * app->deltaTime;
	if (app->input.keys[K_W] == BUTTON_PRESSED)
	{
		app->cam.position += cameraSpeed * app->cam.front;
	}
	if (app->input.keys[K_S] == BUTTON_PRESSED)
	{
		app->cam.position -= cameraSpeed * app->cam.front;
	}
	if (app->input.keys[K_A] == BUTTON_PRESSED)
	{
		app->cam.position -= glm::normalize(glm::cross(app->cam.front, app->cam.up)) * cameraSpeed;
	}
	if (app->input.keys[K_D] == BUTTON_PRESSED)
	{
		app->cam.position += glm::normalize(glm::cross(app->cam.front, app->cam.up)) * cameraSpeed;
	}

	//CAMERA LOOK AROUND

	if (app->input.mouseButtons[RIGHT] == BUTTON_PRESSED)
	{
		app->cam.LookAround(app->input.mousePos.x, app->input.mousePos.y);
	}
	else
	{
		app->cam.lastX = app->input.mousePos.x;
		app->cam.lastY = app->input.mousePos.y;
	}

	// Apply
	app->cam.UpdateViewProjection();
}

void Render(App* app)
{
	app->UpdateEntityBuffer();

	GLubyte* pixels = new GLubyte[app->displaySize.x * app->displaySize.y * 4];

	switch (app->mode)
	{
	case  Mode_Forward:
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, app->displaySize.x, app->displaySize.y);

		const Program& forwardProgram = app->programs[app->renderToBackBufferShader];
		glUseProgram(forwardProgram.handle);

		//BufferManager::BindBuffer(app->localUnfiromBuffer);
		app->RenderGeometry(forwardProgram);
		//BufferManager::UnmapBuffer(app->localUnfiromBuffer);
	}
	break;
	case  Mode_Albedo:
	case  Mode_Deferred:
	case  Mode_Normals:
	case  Mode_Position:
	case  Mode_Depth:
	case  Mode_ViewDirection:
	case  Mode_Metallic:
	case  Mode_Roughness:
	case  Mode_Ao:
	case  Mode_Emissive:
	{
		//Render to FB ColorAtt.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, app->displaySize.x, app->displaySize.y);
		glBindFramebuffer(GL_FRAMEBUFFER, app->defferedFrameBuffer.fbHandle);
		glDrawBuffers(app->defferedFrameBuffer.colorAttachment.size(), app->defferedFrameBuffer.colorAttachment.data());
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const Program& deferredProgram = app->programs[app->renderToFrameBufferShader];
		glUseProgram(deferredProgram.handle);
		app->RenderGeometry(deferredProgram);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//Render to BB from ColorAtt.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, app->displaySize.x, app->displaySize.y);

		const Program& FBToBB = app->programs[app->framebufferToQuadShader];
		glUseProgram(FBToBB.handle);

		//Render Quad
		glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->localUniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[0]);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "uAlbedo"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[1]);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "uNormals"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[2]);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "uPosition"), 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[3]);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "uViewDir"), 3);

		// Metallic
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[4]);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "uMetallic"), 4);

		// Roughness
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[5]);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "uRoughness"), 5);

		// AO
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[6]);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "uAO"), 6);

		// Emissive
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.colorAttachment[7]);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "uEmissive"), 7);

		// Depth
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, app->defferedFrameBuffer.depthHandle);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "uDepth"), 8);

		glUniform1i(glGetUniformLocation(FBToBB.handle, "showAlbedo"), app->mode == Mode_Albedo ? 1 : 0);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "showNormals"), app->mode == Mode_Normals ? 1 : 0);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "showPosition"), app->mode == Mode_Position ? 1 : 0);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "showViewDir"), app->mode == Mode_ViewDirection ? 1 : 0);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "showDepth"), app->mode == Mode_Depth ? 1 : 0);

		glUniform1i(glGetUniformLocation(FBToBB.handle, "showMetallic"), app->mode == Mode_Metallic ? 1 : 0);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "showRoughness"), app->mode == Mode_Roughness ? 1 : 0);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "showAo"), app->mode == Mode_Ao ? 1 : 0);
		glUniform1i(glGetUniformLocation(FBToBB.handle, "showEmissive"), app->mode == Mode_Emissive ? 1 : 0);

		glBindVertexArray(app->vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(0);
		glUseProgram(0);
	}
	break;
	default:;
	}

	// get final render and save in texture
	glReadPixels(0, 0, app->displaySize.x, app->displaySize.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	//Clean previous
	if (app->prefinalTextureID != 0)
	{
		glDeleteTextures(1, &app->prefinalTextureID);
		app->prefinalTextureID = 0;
	}

	glGenTextures(1, &app->prefinalTextureID);

	glBindTexture(GL_TEXTURE_2D, app->prefinalTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// bloom efect
	if (app->bloom.active)
	{
		const vec2 horizontal(1.0, 0.0);
		const vec2 vertical(0.0, 1.0);

		app->PassBlitBrightPixels(app->bloom.fbBloom[0], app->prefinalTextureID, app->bloom.threshold);

		/*
		glBindTexture(GL_TEXTURE_2D, app->bloom.rtBright);
		glGenerateMipmap(GL_TEXTURE_2D);

		app->PassBlur(app->bloom.fbBloom[0], vec2(app->displaySize.x / 2, app->displaySize.y / 2), GL_COLOR_ATTACHMENT1, app->bloom.rtBright, 2, horizontal);
		app->PassBlur(app->bloom.fbBloom[1], vec2(app->displaySize.x / 4, app->displaySize.y / 4), GL_COLOR_ATTACHMENT1, app->bloom.rtBright, 4, horizontal);
		app->PassBlur(app->bloom.fbBloom[2], vec2(app->displaySize.x / 8, app->displaySize.y / 8), GL_COLOR_ATTACHMENT1, app->bloom.rtBright, 8, horizontal);
		app->PassBlur(app->bloom.fbBloom[3], vec2(app->displaySize.x / 16, app->displaySize.y / 16), GL_COLOR_ATTACHMENT1, app->bloom.rtBright, 16, horizontal);
		app->PassBlur(app->bloom.fbBloom[4], vec2(app->displaySize.x / 32, app->displaySize.y / 32), GL_COLOR_ATTACHMENT1, app->bloom.rtBright, 32, horizontal);

		app->PassBlur(app->bloom.fbBloom[0], vec2(app->displaySize.x / 2, app->displaySize.y / 2), GL_COLOR_ATTACHMENT0, app->bloom.rtBloomH, 2, vertical);
		app->PassBlur(app->bloom.fbBloom[1], vec2(app->displaySize.x / 4, app->displaySize.y / 4), GL_COLOR_ATTACHMENT0, app->bloom.rtBloomH, 4, vertical);
		app->PassBlur(app->bloom.fbBloom[2], vec2(app->displaySize.x / 8, app->displaySize.y / 8), GL_COLOR_ATTACHMENT0, app->bloom.rtBloomH, 8, vertical);
		app->PassBlur(app->bloom.fbBloom[3], vec2(app->displaySize.x / 16, app->displaySize.y / 16), GL_COLOR_ATTACHMENT0, app->bloom.rtBloomH, 16, vertical);
		app->PassBlur(app->bloom.fbBloom[4], vec2(app->displaySize.x / 32, app->displaySize.y / 32), GL_COLOR_ATTACHMENT0, app->bloom.rtBloomH, 32, vertical);
		/*
		app->PassBloom(GL_COLOR_ATTACHMENT3, app->bloom.rtBright, 4);
		*/
	}

	delete[] pixels;
}

void App::UpdateEntityBuffer()
{
	BufferManager::MapBuffer(localUniformBuffer, GL_WRITE_ONLY);

	PushVec3(localUniformBuffer, cam.position);
	PushUInt(localUniformBuffer, lights.size());

	//Light
	for (int i = 0; i < lights.size(); ++i)
	{
		BufferManager::AlignHead(localUniformBuffer, sizeof(vec4));

		Light& light = lights[i];
		PushUInt(localUniformBuffer, light.type);
		PushVec3(localUniformBuffer, light.color);

		PushVec3(localUniformBuffer, light.direction);
		PushVec3(localUniformBuffer, light.position);

		PushFloat(localUniformBuffer, light.intensity);
	}
	globalParamsSize = localUniformBuffer.head - globalParamsOffset;

	//Entities
	u32 iteration = 0;
	for (auto it = entities.begin(); it != entities.end(); ++it)
	{
		glm::mat4 world = TransformPositionScale(it->position, it->scale);
		glm::mat4 WVP = cam.projection * cam.view * world;

		Buffer& localBuffer = localUniformBuffer;
		BufferManager::AlignHead(localBuffer, uniformBlockAlignment);
		it->localParamsOffset = localBuffer.head;
		PushMat4(localBuffer, world);
		PushMat4(localBuffer, WVP);
		it->localParamsSize = localBuffer.head - it->localParamsOffset;

		++iteration;
	}
	BufferManager::UnmapBuffer(localUniformBuffer);
}

void App::ConfigureFrameBuffer(FrameBuffer& aConfigFB)
{
	aConfigFB.colorAttachment.push_back(CreateTexture());
	aConfigFB.colorAttachment.push_back(CreateTexture(true));
	aConfigFB.colorAttachment.push_back(CreateTexture(true));
	aConfigFB.colorAttachment.push_back(CreateTexture(true));

	aConfigFB.colorAttachment.push_back(CreateTexture(true)); // oMetallic
	aConfigFB.colorAttachment.push_back(CreateTexture(true)); // oRoughness
	aConfigFB.colorAttachment.push_back(CreateTexture(true)); // oAo
	aConfigFB.colorAttachment.push_back(CreateTexture(true)); // oEmissive

	glGenTextures(1, &aConfigFB.depthHandle);
	glBindTexture(GL_TEXTURE_2D, aConfigFB.depthHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, displaySize.x, displaySize.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &aConfigFB.fbHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, aConfigFB.fbHandle);

	std::vector<GLuint> drawBuffers;
	for (size_t i = 0; i < aConfigFB.colorAttachment.size(); ++i)
	{
		GLuint position = GL_COLOR_ATTACHMENT0 + i;
		glFramebufferTexture(GL_FRAMEBUFFER, position, aConfigFB.colorAttachment[i], 0);
		drawBuffers.push_back(position);
	}

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, aConfigFB.depthHandle, 0);
	glDrawBuffers(drawBuffers.size(), drawBuffers.data());

	GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		int i = 0;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void App::RenderGeometry(const Program aBindedProgram)
{
	glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), localUniformBuffer.handle, globalParamsOffset, globalParamsSize);
	for (auto it = entities.begin(); it != entities.end(); ++it)
	{
		glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), localUniformBuffer.handle, it->localParamsOffset, it->localParamsSize);

		Model& model = models[it->modelIndex];
		Mesh& mesh = meshes[model.meshIdx];

		for (u32 i = 0; i < mesh.submeshes.size(); ++i)
		{
			GLuint vao = FindVAO(mesh, i, aBindedProgram);
			glBindVertexArray(vao);

			u32 subMeshmaterialIdx = model.materialIdx[i];
			Material& subMeshMaterial = materials[subMeshmaterialIdx];

			// Albedo
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[subMeshMaterial.albedoTextureIdx].handle);
			glUniform1i(texturedMeshProgram_uTexture, 0);

			// Normal
			bool useNormalTexture = subMeshMaterial.bumpTextureIdx != 0 ? 1 : 0;
			glUniform1i(glGetUniformLocation(aBindedProgram.handle, "useNormalTexture"), useNormalTexture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, textures[subMeshMaterial.bumpTextureIdx].handle);
			glUniform1i(texturedMeshProgram_uNormal, 1);

			// Metallic
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, textures[subMeshMaterial.specularTextureIdx].handle);
			glUniform1i(texturedMeshProgram_uMetallic, 2);

			//// Roughness
			//glActiveTexture(GL_TEXTURE3);
			//glBindTexture(GL_TEXTURE_2D, textures[subMeshMaterial.shininessTextureIdx].handle);
			//glUniform1i(texturedMeshProgram_uRoughness, 3);

			// AO
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, textures[subMeshMaterial.aoTextureIdx].handle);
			glUniform1i(texturedMeshProgram_uAO, 4);

			// Turn On/Off emissive
			bool useEmissive = subMeshMaterial.emissiveTextureIdx != 0 ? 1 : 0;
			glUniform1i(glGetUniformLocation(aBindedProgram.handle, "useEmissive"), useEmissive);

			// Emissive
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, textures[subMeshMaterial.emissiveTextureIdx].handle);
			glUniform1i(texturedMeshProgram_uEmissive, 5);

			SubMesh& submesh = mesh.submeshes[i];
			glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
		}
	}
}

const GLuint App::CreateTexture(const bool isFloatingPoint)
{
	GLuint texturehandle = 0;

	GLenum internalFormat = isFloatingPoint ? GL_RGBA16F : GL_RGBA8;
	GLenum format = GL_RGBA;
	GLenum dataType = isFloatingPoint ? GL_FLOAT : GL_UNSIGNED_BYTE;

	glGenTextures(1, &texturehandle);
	glBindTexture(GL_TEXTURE_2D, texturehandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, displaySize.x, displaySize.y, 0, format, dataType, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texturehandle;
}

void App::InitBloomEffect()
{
	// Vertical
	if (bloom.rtBright != 0)
	{
		glDeleteTextures(1, &bloom.rtBright);
	}

	glGenTextures(1, &bloom.rtBright);
	glBindTexture(GL_TEXTURE_2D, bloom.rtBright);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, displaySize.x / 2, displaySize.y / 2, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, displaySize.x / 4, displaySize.y / 4, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, displaySize.x / 8, displaySize.y / 8, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, displaySize.x / 16, displaySize.y / 16, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, displaySize.x / 32, displaySize.y / 32, 0, GL_RGBA, GL_FLOAT, nullptr);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Horizontal
	if (bloom.rtBloomH != 0)
	{
		glDeleteTextures(1, &bloom.rtBloomH);
	}

	glGenTextures(1, &bloom.rtBloomH);
	glBindTexture(GL_TEXTURE_2D, bloom.rtBloomH);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, displaySize.x / 2, displaySize.y / 2, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, displaySize.x / 4, displaySize.y / 4, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, displaySize.x / 8, displaySize.y / 8, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, displaySize.x / 16, displaySize.y / 16, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, displaySize.x / 32, displaySize.y / 32, 0, GL_RGBA, GL_FLOAT, nullptr);
	glGenerateMipmap(GL_TEXTURE_2D);

	bloom.fbBloom.resize(5);

	for (int i = 0; i < 5; ++i)
	{
		bloom.fbBloom[i].colorAttachment.push_back(CreateTexture(true));
		bloom.fbBloom[i].colorAttachment.push_back(CreateTexture(true));

		glGenTextures(1, &bloom.fbBloom[i].depthHandle);
		glBindTexture(GL_TEXTURE_2D, bloom.fbBloom[i].depthHandle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, displaySize.x, displaySize.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenFramebuffers(2, &bloom.fbBloom[i].fbHandle);
		glBindFramebuffer(GL_FRAMEBUFFER, bloom.fbBloom[i].fbHandle);

		std::vector<GLuint> drawBuffers;
		for (size_t j = 0; j < bloom.fbBloom[i].colorAttachment.size(); ++j)
		{
			GLuint position = GL_COLOR_ATTACHMENT0 + j;
			glFramebufferTexture2D(GL_FRAMEBUFFER, position, GL_TEXTURE_2D, bloom.fbBloom[i].colorAttachment[j], 0);
			drawBuffers.push_back(position);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bloom.fbBloom[i].depthHandle, 0);
		glDrawBuffers(drawBuffers.size(), drawBuffers.data());

		GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
		{
			printf_s("bloom framebuffer %d is not complete\n", i);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void App::PassBlitBrightPixels(FrameBuffer& fb, GLuint inputTexture, float threshold)
{
	if (bloom.test)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fb.fbHandle);
	}
	
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloom.rtBright, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glViewport(-displaySize.x, -displaySize.y, displaySize.x * 2, displaySize.y * 2);

	const Program& blitBrightestProgram = programs[blitBrightestPixelsShader];
	glUseProgram(blitBrightestProgram.handle);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glUniform1i(glGetUniformLocation(blitBrightestProgram.handle, "colorTexture"), 0);
	glUniform1f(glGetUniformLocation(blitBrightestProgram.handle, "threshold"), threshold);

	// Render the square
	glDrawArrays(GL_TRIANGLES, 0, 4);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glUseProgram(0);

	// Debug code
	/*
	int width = displaySize.x;
	int height = displaySize.y;
	std::vector<float> pixels(width * height * 4); // Assuming RGBA16F (4 channels, float)

	glBindTexture(GL_TEXTURE_2D, fb.colorAttachment[0]);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());

	int widtsh = displaySize.x;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	*/
}

void App::PassBlur(FrameBuffer& fb, vec2 viewportSize, GLenum colorAttachment, GLuint inputTexture, GLuint lod, vec2 direction)
{
	//glBindFramebuffer(GL_FRAMEBUFFER, fb.fbHandle);
	//glDrawBuffer(colorAttachment);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glViewport(0, 0, viewportSize.x, viewportSize.y);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	const Program& blurProgram = programs[blurShader];
	glUseProgram(blurProgram.handle);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexture);

	glUniform1i(glGetUniformLocation(blurProgram.handle, "colorMap"), 0);
	glUniform1i(glGetUniformLocation(blurProgram.handle, "inputLod"), lod);
	glUniform2f(glGetUniformLocation(blurProgram.handle, "direction"), direction.x, direction.y);

	// Render the square
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glEnable(GL_DEPTH_TEST);

	glUseProgram(0);
}

void App::PassBloom(GLenum colorAttachment, GLuint inputTexture, GLuint maxLod)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(colorAttachment);

	glViewport(0, 0, displaySize.x, displaySize.y);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	const Program& bloom = programs[bloomShader];
	glUseProgram(bloom.handle);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexture);

	glUniform1i(glGetUniformLocation(bloom.handle, "colorMap"), 0);
	glUniform1i(glGetUniformLocation(bloom.handle, "maxLod"), maxLod);

	RenderGeometry(bloom);

	glBlendFunc(GL_ONE, GL_ZERO);
	glDisable(GL_BLEND);

	glUseProgram(0);
}

void Camera::Init(ivec2 displaySize)
{
	cameraSpeed = 2.5f;
	aspectRatio = (float)displaySize.x / (float)displaySize.y;
	projection = glm::perspective(glm::radians(60.0f), aspectRatio, zNear, zFar);
	front = glm::vec3(0.0f, 0.0f, -1.0f);
	up = glm::vec3(0.0f, 1.0f, 0.0f);

	direction = glm::vec3(0.562, -0.358, -0.746);
}

void Camera::UpdateViewProjection()
{
	projection = glm::perspective(glm::radians(60.0f), aspectRatio, zNear, zFar);
	view = glm::lookAt(position, position + front, up);
}

void Camera::LookAround(float mouseX, float mouseY)
{
	float xoffset = mouseX - lastX;
	float yoffset = lastY - mouseY;
	lastX = mouseX;
	lastY = mouseY;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(direction);
}