/*
	File: editor.h
	Description: Thin composition root — owns subsystems; all edges ctor-bound.
	Outside code talks only through EditorApi (friend). Guts stay private.
	Undo history lives in ProjectUndo (app/project service), not here.
*/
#pragma once
#include "imgui.h"

#include "editor_api.h"
#include "editor_commands.h"
#include "editor_events.h"
#include "editor_frame.h"
#include "editor_input.h"
#include "editor_operations.h"
#include "editor_state.h"
#include "editor_view_state.h"
#include "services/git/git_service.h"
#include "services/highlight/highlight_service.h"
#include "services/save_service.h"
#include "util/editor_finder.h"
#include "util/editor_line_jump.h"

#include <string>

class Icons;
class ProjectUndo;
class Settings;

class Editor
{
	friend class EditorApi;

  public:
	Editor(Settings &settings,
		   std::string &projectRoot,
		   Icons &icons,
		   ProjectUndo &projectUndo);
	void renderEditor(ImFont *font, float editorWidth);

	// Public interface for the shell (ned, embed, file explorer).
	EditorApi api;

  private:
	void setContent(const std::string &raw);

	// Shell refs (declaration order = init order)
	Settings &settings;
	std::string &projectRoot;
	Icons &icons;
	ProjectUndo &projectUndo;

	// Document core
	EditorState state;
	EditorEvents events;
	EditorOperations operations;
	EditorViewState viewState;

	// Services
	EditorSave save;
	EditorHighlight highlight;
	EditorGit git;

	// Interaction / presentation
	EditorCommands commands;
	EditorInput input;
	EditorFrame frame;
	EditorLineJump lineJump;
	EditorFinder finder;
};
