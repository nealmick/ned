/*
File: globals.cpp
Description: Global variable definitions for the NED editor.
This file defines all the global variables used throughout the application.
*/

#include "ai/ai_agent.h"
#include "editor/editor.h"
#include "editor/editor_bookmarks.h"
#include "files/file_tree.h"
#include "files/files.h"
#include "util/font.h"
#include "util/keybinds.h"
#include "util/settings.h"
#include "util/splitter.h"
#include "util/terminal.h"
#include "util/welcome.h"
#include "util/window_resize.h"

// Define all global variables
// Note: gBookmarks and gAIAgent are already defined in ned.cpp for the main app
// But we need them for the embeddable library too
Bookmarks gBookmarks;
AIAgent gAIAgent;
Font gFont;
Settings gSettings;
Editor gEditor;
FileExplorer gFileExplorer;
Terminal gTerminal;
KeybindsManager gKeybinds;

// gWelcome is already defined in welcome.cpp

// updateMacOSWindowProperties is already defined in app.cpp