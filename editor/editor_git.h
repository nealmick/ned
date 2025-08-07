#pragma once
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class EditorGit
{
  public:
	void init();
	void gitEditedLines();
	void printGitEditedLines();
	bool isLineEdited(const std::string &filePath, int lineNumber) const;
	std::string gitPlusMinus(const std::string &filePath);
	std::map<std::string, std::vector<int>> editedLines;
	std::string currentGitChanges;		 // Store the current git changes string
	std::set<std::string> modifiedFiles; // Store paths of modified files

  private:
	bool isGitInitialized();
	void backgroundTask();
	void updateModifiedFiles();

	std::atomic<bool> git_enabled{false};
	std::thread backgroundThread;
};

extern EditorGit gEditorGit;
