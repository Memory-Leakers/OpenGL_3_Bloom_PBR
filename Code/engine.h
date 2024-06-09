//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include "BufferSuppFuncs.h"
//#include "ModelLoaderFuncs.h"
#include "Globals.h"

const VertexV3V2 vertices[] = {
    {glm::vec3(-1.0,-1.0,0.0), glm::vec2(0.0,0.0)},
    {glm::vec3(1.0,-1.0,0.0), glm::vec2(1.0,0.0)},
    {glm::vec3(1.0,1.0,0.0), glm::vec2(1.0,1.0)},
    {glm::vec3(-1.0,1.0,0.0), glm::vec2(0.0,1.0)},
};

const u16 indices[] =
{
    0,1,2,
    0,2,3
};

struct Bloom
{
    bool active = false;
    bool test = false;
    float threshold = 1.0;
    GLuint rtBright; // For blitting brightest pixels and vertical blur
    GLuint rtBloomH; // For first pass horizontal blur
    std::vector<FrameBuffer>fbBloom;
};

struct Camera
{
    float cameraSpeed;
    float aspectRatio;
    float zNear = 0.1f;
    float zFar = 1000.0f;
    glm::mat4 projection;

    vec3 position = vec3(-0.095, -2.492, 4.473);
    vec3 direction;

    glm::mat4 view;

    float lastX = 400;
    float lastY = 300;

    float yaw = -90.0;
    float pitch = 0.0f;

    glm::vec3 front;
    glm::vec3 up;

    void Init(ivec2 displaySize);

    void UpdateViewProjection();

    void LookAround(float xoffset, float yoffset);
};

struct App
{
    void UpdateEntityBuffer();

    void ConfigureFrameBuffer(FrameBuffer& aConfigFB);

    void RenderGeometry(const Program aBindedProgram);

    const GLuint CreateTexture(const bool isFloatingPoint = false);

    void InitBloomEffect();

    void PassBlitBrightPixels(FrameBuffer& fb, GLuint inputTexture, float threshold);

    void PassBlur(FrameBuffer& fb,vec2 viewportSize, GLenum colorAttachment, GLuint inputTexture, GLuint lod, vec2 direction);

    void PassBloom(GLenum colorAttachment, GLuint inputTexture,  GLuint inputTexture2, GLuint maxLod);

    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    //Camera
    Camera cam;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>    textures;
    std::vector<Material>   materials;
    std::vector<Mesh>       meshes;
    std::vector<Model>      models;
    std::vector<Program>    programs;

    // program indices
    GLuint renderToBackBufferShader;
    GLuint renderToFrameBufferShader;
    GLuint framebufferToQuadShader;

    GLuint renderToBackBufferShaderNoPBR;
    GLuint renderToFrameBufferShaderNoPBR;
    GLuint framebufferToQuadShaderNoPBR;
    
    // for bloom
    GLuint blitBrightestPixelsShader;
    GLuint blurShader;
    GLuint bloomShader;

    //u32 patricioModel = 0;
    GLuint texturedMeshProgram_uTexture;
    GLuint texturedMeshProgram_uMetallic;
    GLuint texturedMeshProgram_uRoughness;
    GLuint texturedMeshProgram_uAO;
    GLuint texturedMeshProgram_uNormal;
    GLuint texturedMeshProgram_uEmissive;

    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Models
    u32 SphereModelIndex;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    std::string openglDebugInfo;

    GLint maxUniformBufferSize;
    GLint uniformBlockAlignment;
    Buffer localUniformBuffer;
    std::vector<Entity> entities;
    std::vector<Light> lights;

    FrameBuffer defferedFrameBuffer;

    GLuint globalParamsOffset;
    GLuint globalParamsSize;

    GLuint prefinalTextureID = 0;

    Bloom bloom;

    bool pbr = false;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);
