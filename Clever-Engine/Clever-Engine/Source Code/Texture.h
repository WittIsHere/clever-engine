#pragma once

#include "Globals.h"
#include "Component.h"


class TextureData : public Component
{
public:
	TextureData();
	~TextureData();

	bool Enable();
	bool Update();
	bool Disable();

public:
	const char* path;
	uint textureID;
};
