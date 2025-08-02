/*
File: globals.h
Description: Global variable declarations for the NED editor.
This file declares all the global variables used throughout the application.
*/

#pragma once

// Forward declarations
class Bookmarks;
class AIAgent;
class Font;
class Settings;
class Editor;
class FileExplorer;
class Terminal;
class KeybindsManager;
class Welcome;

// Declare all global variables
extern Bookmarks gBookmarks;
extern AIAgent gAIAgent;
extern Font gFont;
extern Settings gSettings;
extern Editor gEditor;
extern FileExplorer gFileExplorer;
extern Terminal gTerminal;
extern KeybindsManager gKeybinds;
extern Welcome &gWelcome;