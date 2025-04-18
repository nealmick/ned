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
	static void printColors();

	static bool colorsNeedUpdate;	 // settings theme changed
	static ThemeColors cachedColors; // cached settings theme colors, so we dont read from disk

	static const ImVec4 &convertNodeTypeCPP(const std::string &nodeType,
											const std::string &parentNodeType,
											const std::string &nodeText);
	static const ImVec4 &convertNodeTypeJS(const std::string &nodeType,
										   const std::string &parentNodeType,
										   const std::string &nodeText);

	static const ImVec4 &convertNodeTypePython(const std::string &nodeType,
											   const std::string &parentNodeType,
											   const std::string &nodeText);

	static const ImVec4 &convertNodeTypeCSharp(const std::string &nodeType,
											   const std::string &parentNodeType,
											   const std::string &nodeText);

	static void setColors(const std::string &fileContent,
						  std::vector<ImVec4> &fileColors,
						  int start,
						  int end,
						  const ImVec4 &color);
	static TSParser *getParser();
	static void cleanupParser();

  private:
	static TSParser *parser;
	static std::mutex parserMutex;
	static void traverse(const std::string &extension,
						 const std::string &fileContent,
						 std::vector<ImVec4> &fileColors,
						 TSNode node,
						 int depth,
						 bool is_last,
						 const std::vector<bool> &hierarchy,
						 const std::string &parentNodeType = "");
};