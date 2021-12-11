#pragma once

#include "Globals.h"
#include "Component.h"

class TextureData;
class ParsonNode;

class c_Material : public Component
{
public:
	c_Material(GameObject* parent, COMPONENT_TYPE type);
	c_Material(GameObject* parent, ComponentData* data);
	~c_Material();

	bool Enable();
	bool Update();
	bool Disable();

	bool SaveState(ParsonNode& root) const override;
	bool LoadState(ParsonNode& root) override;

	bool AssignNewData(MaterialData* data);

public:
	const char* getPath();
	const uint getTextureID();

	void setPath(const char* path);
	void setTextureID(uint id);

	void changeTextureData(TextureData* data);

private:
	TextureData* textureData = nullptr;
	
};
