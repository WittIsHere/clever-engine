#pragma once

#include "Globals.h"
#include "Component.h"

struct c_Material;

class c_Mesh : public Component
{
public:
	c_Mesh();
	~c_Mesh();

	bool Enable();
	bool Update();
	bool Disable();


public:
	uint vertexCount = 0;				// Number of Vertex

	uint vPosID = 0;					// Vertex Positions Buffer ID
	float* vPosData = nullptr;			// Vertex Positions Array

	uint vTexCoordsID = 0;				// Vertex Texture Coords Buffer ID
	float* vTexCoordsData = nullptr;	// Vertex Texture Coords Array

	//uint vNormID = 0;					// Vertex Normals Buffer ID
	//float* vNormData = nullptr;		// Vertex Normals Array

	uint indicesID = 0;					// Indices Buffer ID
	uint indicesCount = 0;				// Number of Indices
	uint* indicesData = nullptr;		// Indices Array

	c_Material* texture = 0;			// Pointer to acess the texture inside myTexures() vector
};