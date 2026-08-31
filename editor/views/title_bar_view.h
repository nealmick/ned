/*
	File: views/title_bar_view.h
	Description: Editor title bar (path, git changes).
	Reads git public data as a const leaf edge.
*/

#pragma once

#include "imgui.h"
#include <string>

struct ImFont;
class EditorGit;
class Icons;
class Settings;

class TitleBarView
{
  public:
	TitleBarView(const EditorGit &gitService, Icons &iconSet, Settings &appSettings)
		: git(&gitService), icons(&iconSet), settings(&appSettings)
	{
	}

	void render(ImFont *font, const std::string &filePath, bool showGitChanges);

  private:
	const EditorGit *git;
	Icons *icons;
	Settings *settings;

	std::string truncateFilePath(const std::string &path, float maxWidth);
	std::string normalizePathForDisplay(const std::string &path);
};
