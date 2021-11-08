#pragma once
#include "Module.h"
#include "Globals.h"

#include <vector>

class GameObject;

class ModuleUI : public Module
{
public:
	ModuleUI(Application* app, bool start_enabled = true);
	~ModuleUI();

	bool Start();
	update_status PreUpdate(float dt);
	update_status Update(float dt);
	update_status PostUpdate(float dt);
	void Render();
	bool CleanUp();

private:
	void DrawConsoleSpace(bool* active);
	void DrawConfigurationSpace(bool* active);
	void DrawHierarchySpace(bool* active);
	void ShowChildData(GameObject* GO);

	void ShowDockingDisabledMessage();

public:
	void ConsoleLog(const char* text);
	void AddLogFPS(float fps, float ms);

public:
	bool showDemoWindow;
	bool usingKeyboard;
	bool usingMouse;

	bool changeFPSlimit = false;
	bool changeTitleName = false;
	int max_fps = 60;

private:
	//Bool variables to activate the different windows
	bool activeConsole = true;
	bool activeConfiguration = false;
	bool activeDockingSpace = false;
	bool activeHierarchy = true;

	//Configuration window variables
	bool need_scroll = false;
	std::vector<float> fps_log;
	std::vector<float> ms_log;

	//Console window variables
	std::vector<char*>	buffer;
	bool scrollToBottom;
};
