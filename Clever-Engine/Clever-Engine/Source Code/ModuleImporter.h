#pragma once
#include "Module.h"
#include "Globals.h"

struct aiMesh;

struct MeshData
{
	uint id_vertex = 0;       // Vertex Buffer
	uint num_vertex = 0;      // num of Vertex
	float* vertex = nullptr;  // Vertex Array

	uint id_index = 0;        // Index Buffer
	uint num_index = 0;       // num of Index
	uint* index = nullptr;    // Index Array
};

struct SceneData
{
	MeshData myMeshes[10];
	//std::vector<MeshData*> myMeshes; 
};

class ModuleImporter : public Module
{
public:

	ModuleImporter(Application* app, bool start_enabled = true);
	~ModuleImporter();

	bool Init();
	update_status PreUpdate(float dt);
	bool CleanUp();

	void ImportScene(const char* file_path);
	void ImportMesh(aiMesh* mesh, MeshData myMesh);

	SceneData myScene;

};