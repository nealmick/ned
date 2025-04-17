/*
	util/close_popper.cpp
	This utility closses all popup windows and is used to prevent conflicts
	with opening multiple popups windows. if settings window is opened then it
   triggers both linejump and bookmarks popup to close.
*/
#include "close_popper.h"
#include "../editor/editor_bookmarks.h"
#include "../editor/editor_line_jump.h"
#include "../files/file_finder.h"
#include "settings.h"

void ClosePopper::closeAllExcept(Type keepOpen)
{
	switch (keepOpen)
	{
	case Type::Settings:
		gBookmarks.showBookmarksWindow = false;
		gLineJump.showLineJumpWindow = false;
		gFileFinder.showFFWindow = false;
		break;

	case Type::Bookmarks:
		gSettings.showSettingsWindow = false;
		gLineJump.showLineJumpWindow = false;
		gFileFinder.showFFWindow = false;
		break;

	case Type::LineJump:
		gSettings.showSettingsWindow = false;
		gBookmarks.showBookmarksWindow = false;
		gFileFinder.showFFWindow = false;
		break;

	case Type::FileFinder:
		gSettings.showSettingsWindow = false;
		gBookmarks.showBookmarksWindow = false;
		gLineJump.showLineJumpWindow = false;
		break;
	}
}

void ClosePopper::closeAll()
{
	gSettings.showSettingsWindow = false;
	gBookmarks.showBookmarksWindow = false;
	gLineJump.showLineJumpWindow = false;
	gFileFinder.showFFWindow = false;
}
