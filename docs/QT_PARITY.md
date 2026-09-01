# Qt backend parity audit

Honest status of every feature vs the ImGui backend (the reference).
Process per agreement: one feature at a time, verified by the user against
ImGui behavior before moving on. Update this file when a status changes.

Status legend: ✅ verified by user · 🟡 partial (works, known gaps) ·
🟥 not started · ⚠️ half-done with known debt (called out explicitly).

## 1. Editor surface (foundation)

| Area | ImGui reference | Qt status | Gaps |
|---|---|---|---|
| Text rendering (grid) | per-glyph via ImDrawList | 🟡 | per-cell grid in place; needs verification pass on mixed tabs/UTF-8 |
| Tabs | 4-cell stops, `kTabSize` | 🟡 | expansion model in place; hit-testing on wrapped rows ignores segment (see ⚠️ below) |
| Caret | 1px line, blink; rainbow mode | 🟡 | rainbow dropped by request; blink works |
| Selection rendering | filled rects per row | 🟡 | wrapped-row selection rects use segment-of(0) — wrong for multi-segment rows (⚠️) |
| Scroll-follow caret | `revealCursor` every edit | 🟥 | `commands.requestEnsureVisible()` is a NO-OP in Qt — caret can leave the viewport while typing |
| Word wrap | `WrapLayout` (shared) | 🟡 | shared engine wired; visual-line hit-testing incomplete (⚠️ segment not resolved from y) |
| Gutter | line numbers, git tint + marks | 🟡 | wired; git tint shows only after workspace-root fix — needs user verify |
| Minimap | full (drag, density) | 🟡 | density runs ported (per-span colors dimmed 72%, visible-window strip, continuous slider, wheel); scrollbar hides when minimap on — needs user verify |
| Editor title bar | icon + path + git ±N | 🟡 | present; icon from ned SVG set |

## 2. Keyboard input

| Key | ImGui behavior | Qt status |
|---|---|---|
| Plain arrows | move, no mod | ✅ parity code |
| Option+←/→ | word move | ✅ parity code |
| Option+↑/↓ | add caret above/below | ✅ parity code |
| Cmd/Ctrl+←/→ | line start/end | ✅ parity code |
| Cmd/Ctrl+↑/↓ | jump ±5 lines | ✅ parity code |
| Alt+Backspace/Delete | **word** delete (`deleteLeft(KeyAlt)`) | 🟥 Qt uses Cmd/Ctrl — wrong modifier |
| Tab / Shift+Tab | indent / outdent | ✅ (event() interception) |
| Enter, plain Backspace/Delete | insert/delete | ✅ |
| Cmd+A/Z/Y/C/X/V/S | expected | ✅ |
| Cmd +/- font zoom | app shortcut | ✅ |
| Escape | collapse selection | 🟥 not wired |
| Printable text | UTF-8 typing | ✅ |
| IME | n/a on ImGui | ✅ Qt-native (advantage) |

## 3. Mouse input

| Interaction | ImGui | Qt status |
|---|---|---|
| Click / drag select | yes | 🟡 works; no UTF-8 boundary snap |
| Shift+click | extend from anchor | ✅ parity code |
| Double-click | `selectWordAt` | 🟥 |
| Right-click menu | context menu (cut/copy/paste/select-all) | 🟥 |
| Wheel scroll | child-window scroll | 🟡 scrollbar + wheel |
| Cmd+scroll | zoom? (check) | 🟥 |

## 4. Features

| Feature | ImGui | Qt status |
|---|---|---|
| Find/replace | in-editor bar, match count, multi-select all | 🟡 own bar; no highlight-all, replace emits N dirty signals |
| Go-to-line | in-editor overlay | ✅ parity (overlay) |
| File finder (Ctrl+P) | FileFinder shared model | 🟡 Qt-local scan + own fuzzy (⚠️ not the shared matcher) |
| File sidebar | FileTree shared, git dirty tint | 🟡 tree shared; ⚠️ dirty-file tint not rendered |
| Settings window | full (all profile keys, live apply) | 🟡 11 controls; many profile keys missing |
| Git gutter/±N | shared EditorGit | 🟡 DidEdit subscribed (ImGui parity); interact-verified dirty lines + ±N; needs user verify |
| LSP (goto/ref/hover) | EditorApi-bound popups | 🟥 blocked: LSPClient binds to EditorApi (ImGui presenter seam) |
| Multi-cursor rendering | all carets + selections | ✅ renders |
| Multi-cursor edits | per-caret reverse order | ✅ via shared commands |
| Terminal | imgui-terminal | 🟥 intentionally out of scope |
| CRT shaders | GL post pass | 🟥 intentionally out of scope |
| Window chrome | transparent titlebar, blur, SF buttons, native title | 🟡 no vibrancy blur (needs non-reparenting approach) |
| Tab management | 1-9 switch, W close, dirty markers | ✅ |

## 5. Known half-done debt (⚠️ explicit list)

0. (resolved) wrapped-row hit-test, selection rects, width unification — fixed.
1. `qt_editor_view.cpp` has grown by accretion (~800 lines): paintEvent
   mixes concerns (title bar, gutter, git, text, overlays) — should be
   split into small paint helpers when the foundation is verified.
2. (resolved) Qt finder doesn't reuse the shared FileFinder fuzzy model.
3. (resolved) Replace All title update via repaintAndFollow emit.
Remaining: 1 (split paintEvent), 4 (finder matcher), 7 (sidebar tint),
8 resolved (context menu + Escape shipped).
7. Sidebar has no git dirty-file tinting (`isFileModified` unused).
8. No context menu; Escape doesn't collapse selection.

## Working order (agreed)

1. Foundation: scroll-follow, Alt-delete, Escape, double-click, context
   menu, wrapped-row hit-test — then USER VERIFICATION of §1+§2+§3.
2. Minimap.
3. Find/replace polish (highlight-all, shared matcher where sensible).
4. LSP seam (EditorApi abstraction) then popups.
5. Settings completion, sidebar dirty tint, chrome vibrancy.
