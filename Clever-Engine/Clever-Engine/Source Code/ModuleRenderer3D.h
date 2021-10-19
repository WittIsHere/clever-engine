#pragma once
#include "Module.h"
#include "Globals.h"
#include "glmath.h"
#include "Light.h"

#define MAX_LIGHTS 8
#define CHECKERS_HEIGHT 124
#define CHECKERS_WIDTH 124

struct SceneData;
struct MeshData;

class ModuleRenderer3D : public Module
{
public:
	ModuleRenderer3D(Application* app, bool start_enabled = true);
	~ModuleRenderer3D();

	bool Init();
	update_status PreUpdate(float dt);
	update_status PostUpdate(float dt);
	bool CleanUp();

	void OnResize(int width, int height);

	void PrepareDrawScene(SceneData* scene);

	bool GetVSync() const;
	void SetVSync(bool vsync);
	uint* GetOpenGLVersion() const;

private:
	void PrepareDrawMesh(MeshData* mesh);
	void DrawScene();
	void DrawMesh(MeshData* mesh);

public:
	Light lights[MAX_LIGHTS];
	SDL_GLContext context;
	mat3x3 NormalMatrix;
	mat4x4 ModelMatrix, ViewMatrix, ProjectionMatrix;

	// Draw Cube vars
	void DrawCube();
	void DMPlane();
	void TestPlane();
	void CreateCheckerTex();
	void BindCheckerTex();

	uint checker_Buffer;
	bool defaultTexture = false;

    bool vsync = true;

private:
	SceneData* currentScene = nullptr;

	//test meshes buffers
	uint vertex_Buffer;
	uint indices_Buffer;
	uint texCoords_Buffer;
};