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

	static std::pair<TSLanguage *, std::string>
	detectLanguageAndQuery(const std::string &extension);
	static void
	computeEditRange(const std::string &newContent, size_t &start, size_t &newEnd, size_t &oldEnd);
	static TSInputEdit createEdit(size_t start, size_t oldEnd, size_t newEnd);
	static TSInput createInput(const std::string &content);
	static TSTree *createNewTree(TSParser *parser, bool initialParse, const std::string &content);
	static TSQuery *loadQueryFromCacheOrFile(TSLanguage *lang, const std::string &query_path);
	static void executeQueryAndHighlight(TSQuery *query,
										 TSTree *tree,
										 const std::string &content,
										 std::vector<ImVec4> &colors,
										 bool initialParse,
										 size_t start,
										 size_t end);
};