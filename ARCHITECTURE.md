# ned architecture

One page. Every structural change gets checked against this file — if a
change disagrees with a rule here, either change the design or change this
file first. Nothing else.

## Layering

```
┌────────────────────────────────────────────────────────────┐
│ host/imgui/            GLFW host: window, GL loop, shaders │
│   ned.cpp, workbench, settings_view, welcome,              │
│   ned_embed (ImGui-embedding facade), ned_imgui_config.h   │
│   host/qt/             Qt host (planned)                   │
├────────────────────────────────────────────────────────────┤
│ <module>/views/imgui/  ImGui backend for that module       │
│   editor/views/imgui/  editor surface (see roles below)    │
│   files/views/imgui/   sidebar, tree rows, Ctrl+P finder   │
│   lsp/views/imgui/     dashboard, symbol info, picker      │
├────────────────────────────────────────────────────────────┤
│ <module>/              model + logic, no widgets           │
│   editor/              buffer, state, ops, commands,       │
│                        events, services (highlight/git/    │
│                        diagnostics/save), util helpers     │
│   lsp/                 client (session/transport), sync    │
│   files/               scan/filter/open logic, tree model  │
│   util/                settings persistence, keybinds,     │
│                        icons, terminal, platform windows   │
├────────────────────────────────────────────────────────────┤
│ ned_core (CMake target)  the gate: core sources compiled   │
│                          with imgui ABSENT from the        │
│                          include path. New ImGui usage in  │
│                          these files breaks the build.     │
└────────────────────────────────────────────────────────────┘
```

## Rules

1. **Core is UI-free — mechanically enforced.** Anything linkable from
   `ned_core` must not include `imgui.h` (directly or transitively).
   Notable current exceptions, by design, until Phase 2 lands:
   `editor_commands` (clipboard), `editor_view_state` (scroll),
   `highlight_service`/`tree_sitter` (ImVec4 colors), `editor_api`,
   `git_service` (reads Settings, which owns the ImGui-bound Font).

2. **Views live with the module whose state they render.**
   `files/views/imgui/` renders `files/` state. A view never reaches
   across to own another module's rendering.

3. **A module either contains logic or calls a UI toolkit — never both.**
   `<module>/` = state + logic. `<module>/views/imgui/` = ImGui code.
   `util/` currently bends this rule for font/icons/terminal (legacy
   ImGui glue pending Phase 2 abstraction); don't add new cases.

4. **Hosts own chrome.** Window, title bar, dock/tab layout, settings
   window, welcome screen, shaders — anything that renders *the app*
   rather than *a module's data* lives in `host/<backend>/`.

5. **Three roles inside a backend, don't blur them:**
   - **model** — state + logic, testable headless (editor core, FileFinder scan)
   - **presenter** — composes views, owns layout/input/focus policy for a
     surface (`editor_frame`, `editor_input`, `LspImGuiView`)
   - **view** — dumb drawing of given state (`text_view`, `gutter_view`,
     `file_tree_view`, ...)
   One view (or presenter) per file. Never merge several views into one
   file.

6. **Backend-neutral code goes in the module, not a view folder.**
   Pure helpers that both backends will use (`hover_markdown`,
   `hover_trigger`, `utf8`) live in `<module>/util/`.

7. **Errors flow toward the user.** Logic layers report failure via return
   values/state; only views/hosts render notifications (today:
   `Settings::showNotification` state + `SettingsView` draws it).

## Planned (not yet true)

- `editor/platform/` — backend-neutral interfaces (NedKey, NedColor,
  clipboard, font metrics, input events, textures). ImGui implements them;
  the Qt backend will implement them too. This is what finally lets rule
  1 cover the four exception files above.
- `host/qt/` — Qt host reusing `configureMacOSNSWindow` for the same
  native title bar treatment. Drops initially: terminal, LSP dashboard,
  CRT shaders.
- On-demand rendering (dirty-flag frame pacing) in the ImGui host.

## Analogy check (VSCode)

The shape is deliberately VSCode-like: UI-free core ↔ `vs/editor/common`;
`<module>/ + <module>/views/imgui/` ↔ `workbench/contrib/<feature>/`;
`host/` ↔ workbench shell. The one deviation is two UI backends
(ImGui today, Qt planned) where VSCode has one (DOM) — that choice is the
reason the rules above exist.
