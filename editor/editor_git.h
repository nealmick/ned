#pragma once
#include "git_libgit2.h"
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef PLATFORM_MACOS
#include <CoreServices/CoreServices.h>
#elif defined(PLATFORM_LINUX)
#include <sys/inotify.h>
#endif

class EditorGit
{
  public:
	~EditorGit(); // Destructor to clean up resources
	void init();
	void gitEditedLines();
	void printGitEditedLines();
	bool isLineEdited(const std::string &filePath, int lineNumber) const;
	std::string gitPlusMinus(const std::string &filePath);
	float getLineAnimationAlpha(const std::string &filePath,
								int lineNumber) const; // Get animation alpha for line

	// Simple function for file tree to get modified file paths
	std::set<std::string> getModifiedFilePaths();

	// Trigger immediate git update for current file (non-blocking)
	void triggerImmediateUpdate();

	std::map<std::string, std::vector<int>> editedLines;
	std::string currentGitChanges; // Store the current git changes string

  private:
	bool isGitInitialized();
	void backgroundTask();
	void
	updateLineAnimations(const std::map<std::string, std::vector<int>> &newEditedLines);
	void cleanupCompletedAnimations();

	std::atomic<bool> git_enabled{false};
	std::atomic<bool> immediateUpdateRequested{false};
	std::thread backgroundThread;
	std::chrono::steady_clock::time_point lastUpdate;

	// libgit2 wrapper for lock-free operations
	GitLibgit2 gitWrapper;

	// Animation tracking
	struct LineAnimation
	{
		std::chrono::steady_clock::time_point startTime;
		bool fadingIn; // true = fading in, false = fading out
	};
	std::map<std::string, std::map<int, LineAnimation>>
		lineAnimations; // filepath -> line -> animation
};

extern EditorGit gEditorGit;
