// editor_tree_sitter.h
#pragma once
#include "../util/settings.h"
#include "imgui.h"
#include <iostream>
#include <string>
#include <tree_sitter/api.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct ThemeColors
{
	ImVec4 keyword;
	ImVec4 string;
	ImVec4 number;
	ImVec4 comment;
	ImVec4 text;
	ImVec4 function;
	ImVec4 type;
	ImVec4 variable;
};

class TreeSitter
{
  public:
	static void parse(const std::string &fileContent,
					  std::vector<ImVec4> &fileColors,
					  const std::string &extension);

	static void updateThemeColors();
	static void refreshColors() { colorsNeedUpdate = true; };

	static bool colorsNeedUpdate;	 // settings theme changed
	static ThemeColors cachedColors; // <--- ADDED STATIC HERE!

	static void setColors(const std::string &fileContent,
						  std::vector<ImVec4> &fileColors,
						  int start,
						  int end,
						  const ImVec4 &color);
	static TSParser *getParser();

  private:
	static TSParser *parser;
	static std::mutex parserMutex;
	static std::unordered_map<std::string, TSQuery *> queryCache;
	// incremental parsing
	static TSTree *previousTree;
	static std::string previousContent;
};