# NED Editor Embedding Plan

## Current Understanding

The NED editor is a 45k line project that currently:
- Creates its own GLFW window
- Has a main `Ned` class that manages the application
- Uses ImGui for the UI
- Has complex initialization with shaders, fonts, etc.

## Goal

Create an embeddable version that can be used within other ImGui projects by:
- **Removing GLFW window management** - let it work as an ImGui window/child window
- **Reusing ALL existing functionality** - terminal, keybinds, settings, tree-sitter, file explorer, etc.
- **Keeping all features intact** - no feature changes, just rendering context changes
- **Working as a submodule** - people include it in their project and use it

## Key Insights from Code Analysis

### 1. **How the current `Ned` class works:**
- `initialize()` calls `Init::initializeAllComponents()` which sets up:
  - Graphics system (GLFW, OpenGL, ImGui)
  - Window management
  - Components (settings, UI, etc.)
  - Shader quad
- `run()` calls `app.runMainLoop()` which:
  - Polls events
  - Handles window management
  - Calls `render.renderFrame()` for the main rendering
  - Handles frame timing and font reloading

### 2. **Core editor components (ALL important):**
- **Essential**: Editor, FileExplorer, AIAgent, Settings, Splitter, WindowResize, Terminal, Keybinds, Tree-sitter
- **Window-specific**: GLFW window creation, shader management, framebuffers
- **Rendering**: `Render::renderMainWindow()` contains the main ImGui UI code

### 3. **What initialization is required:**
- **Must have**: ALL components (Settings, Font, FileExplorer, Editor, AIAgent, Splitter, WindowResize, Terminal, Keybinds, Tree-sitter)
- **Can skip**: GLFW window creation, shader initialization, framebuffers
- **Global state**: Uses many global variables (gSettings, gFont, gEditor, etc.) - user can only create one instance

### 4. **How rendering works:**
- `Render::renderMainWindow()` contains the main ImGui UI code
- It calls `gEditor.renderEditor()`, `gFileExplorer.renderFileExplorer()`, etc.
- The actual editor UI is in `Editor::renderEditor()`

### 5. **Dependencies:**
- **Tightly coupled to GLFW**: Window creation, event polling, scroll callbacks
- **Optional for embedding**: Shaders, framebuffers, complex graphics
- **Global state**: Many components use global variables

## Proposed Approach

### 1. **Create an embeddable wrapper class**
```cpp
class NedEmbed {
    // Similar to Ned but without window management
    bool initialize();  // Initialize everything except GLFW window
    void render(float width, float height);  // Render as ImGui window/child
    void handleInput();  // Handle input through ImGui
    void cleanup();
};
```

### 2. **Extract the editor UI rendering**
- Use `Render::renderMainWindow()` as a template
- Remove window-specific ImGui calls (ImGui::SetNextWindowPos, etc.)
- Keep ALL the core editor rendering logic
- Make it work as an ImGui window/child window

### 3. **Handle initialization properly**
- Initialize ALL components (Settings, Font, Editor, FileExplorer, Terminal, Keybinds, etc.)
- Skip GLFW window creation and shader initialization
- Handle global state appropriately (one instance per application)

### 4. **Create a minimal interface**
```cpp
// Usage in another ImGui project:
NedEmbed nedEditor;
nedEditor.initialize();
// In render loop:
nedEditor.render(availableWidth, availableHeight);
```

## Key Challenges

1. **Global state management**: Many components use global variables - user can only create one instance
2. **GLFW dependency**: Need to handle input through ImGui instead of GLFW window
3. **ImGui context**: Need to work within existing ImGui context
4. **Initialization order**: Components depend on each other

## Next Steps

1. **Study the rendering pipeline more closely**
   - Look at `Render::renderMainWindow()` implementation
   - Understand what ImGui calls are made
   - Identify what can be extracted for embedding

2. **Identify minimal initialization requirements**
   - What components are absolutely necessary? (ALL of them)
   - What can be made optional? (Just GLFW window and shaders)
   - How to handle global state? (One instance per app)

3. **Create a proof of concept**
   - Start with just the editor rendering as ImGui window
   - Add minimal initialization (everything except GLFW window)
   - Test within a simple ImGui project

## Questions

- What parts of the editor are most important for embedding? **ALL of them**
- Should we support all features or just core editing? **ALL features, no changes**
- How should we handle the global state variables? **One instance per application**
- What's the minimal set of dependencies needed? **Same as current project (submodule)**
- How do we handle input without GLFW window management? **Through ImGui's input system** 

---

## Progress & Notes

### What We've Done So Far
- Created the `NedEmbed` wrapper class to allow embedding the NED editor as an ImGui window, without its own GLFW window.
- Set up a local development workflow using a separate ImGui demo project (`ImGui_Ned_Embed`) that includes the NED project via a local path (not as a remote submodule).
- Modified CMake to build the NED project as a static library (`ned_embed`) for embedding, and link it into the demo app.
- Simplified initialization in the embeddable wrapper to avoid window and shader setup.
- Removed/disabled shader and framebuffer dependencies in the embeddable build.
- Demo app attempts to render the NED editor as an ImGui window, using the embeddable wrapper.

### Changes in Plan / Direction
- Originally planned to support all features immediately, but linker errors due to missing global variables/functions (e.g., `gAIAgent`, `gBookmarks`) and macOS-specific functions (`updateMacOSWindowProperties`) require us to stub or disable some features for now.
- The embeddable build currently stubs or disables some features (like the AI agent pane) to get a minimal version working.
- Long-term, we will refactor to reduce reliance on global state and make all features available in both standalone and embedded modes.

### TODO List (Running)
- [x] Stub or define missing globals/functions in the embeddable build so it links and runs (for dev/test).
- [x] Fix ImGui version mismatch between demo app and NED library.
- [x] Copy necessary resource files (fonts, settings) to demo app.
- [ ] Debug segmentation fault in demo app during NED editor initialization.
- [ ] Test the editor in the demo app and verify basic editing, file explorer, and settings work.
- [ ] Iteratively re-enable more features (AI agent, terminal, etc.) by resolving their dependencies.
- [ ] Refactor the codebase for better modularity (reduce global state, use dependency injection/context objects).
- [ ] Document the embedding process and any limitations for users who want to include NED in their own ImGui apps. 