#pragma once
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
	std::map<std::string, std::vector<int>> editedLines;
	std::string currentGitChanges;		 // Store the current git changes string
	std::set<std::string> modifiedFiles; // Store paths of modified files

  private:
	bool isGitInitialized();
	void backgroundTask();
	void updateModifiedFiles();
	void
	updateLineAnimations(const std::map<std::string, std::vector<int>> &newEditedLines);
	void cleanupCompletedAnimations();

	// Filesystem watcher methods
	void startFileWatcher();
	void stopFileWatcher();
	void onFileChanged();

#ifdef PLATFORM_MACOS
	static void fsEventsCallback(ConstFSEventStreamRef streamRef,
								 void *clientCallBackInfo,
								 size_t numEvents,
								 void *eventPaths,
								 const FSEventStreamEventFlags eventFlags[],
								 const FSEventStreamEventId eventIds[]);
	FSEventStreamRef fsEventStream = nullptr;
#elif defined(PLATFORM_LINUX)
	int inotifyFd = -1;
	int gitDirWatch = -1;
	void processInotifyEvents();
#endif

	std::atomic<bool> git_enabled{false};
	std::atomic<bool> filesChanged{false};
	std::thread backgroundThread;
	std::thread watcherThread;
	std::chrono::steady_clock::time_point lastUpdate;

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
