Ned Text Editor Notes: bugs, features, plans
Questions:
What is the end state for ned? Ai features?? Popularity? Personal use?? Bug free? What feature set?
Goal: create an editor that I can use for the future.   Maybe other people will like it too?
AI slop?? Probably not, maybe see if I can put some feature in tho, such as lsp aware auto complete

—————————————————————————————————————
Features/Bugs:



— Multi Selection— half complete needs shift tab multi selection support.

— Fix performance on large files when inserting text
	
— Fix line numbers, to be more to the left, and then also fix line numbers over 10k are cut off…

— Fix screen curvature making click areas different then Imgui. 

—possible add more lsp support, add rename symbol support, add go, and react lsp’s

—Selection key bind for current scope, () {} []

— Fix LSP info box sometimes appearing randomly above where it should like 5 lines above… seems to happen randomly.

— Expand icon support ~ currently working on it added a bunch of icons, need txt icon and others still, currently: still add more icons, txt icon needed, resizing icons is a pain, and finding ones that look good jeesh.  Will do later

—————————————————————————————————————
Staging


—————————————————————————————————————
Merged in pr #53 https://github.com/nealmick/ned/pull/53
X-  Improved Readme
X-  multi cursor makes line numbers rainbow when active cursors on line…
X-  Improved find/replace ctrl enter for file content search now spawns multi cursor at each found location
X-  Make main cursor red if multi cursor active
X-  Spawn cursor line above/below with cmd option up/down
X-  Multi Cursor
X-  Multi Selection— half complete needs shift tab multi selection support.
X-  Multi Selection/Cursor enter key
X-  Multi Selection/Cursor char input
X-  Multi Selection/Cursor delete key
X-  Multi Selection/Cursor backspace key
X-  Persistent undo/redo states
X-  Fix editor tile for long file names overflowing settings cog
X-  Add border to window controls
X-  Make window controls appear briefly on launch
X-  Rasterization pixelation…
X- add custom window controls
X- Round window corners
X- add window border
X-  Remove top title bar, custom title bar, dragging window, resizing
X- fix text color not updating on change…
X- fix file finder initializing late
X- debounce undo redo save states
X- make undo redo saved states also save cursor index and restore cursor index.
X- Fix tree sitter c++ syntax weird coloring for std::cout colors std green sometimes fixed was identifier coloring.

—————————————————————————————————————
Merged in pr #51 https://github.com/nealmick/ned/pull/51
X- Fix right click menu is too skinny cutting off option details.
X-  reverse terminal scroll direction 
X- Fix shift scroll for horizontal scrolling, possible broke when added scroll speed reduction in ned class
X- fix terminal size scrolling at 2/3 screen, fix is use stop then go to bottom but need to resize on terminal init
X- Shader Settings, make it so shaders have settings page… for different effects. Currently just below other settings, but could make a dedicated page aswell.
X- Tree sitter syntax sometimes messes up when cutting a line, I think it stops highlighting everything after the edit, which could miss some syntax color changes beyond the edit..  crtl z causes syntax colors mismatch it seems…. Seems to fixed by querying ful tree and apply full colors every time
X-  Tree sitter syntax sometimes no highlight first node, in python and jsx tsx… was reusing parse tree now fixed
X-  Fix tree sitter to not use c highlight for unknown file types.. just don’t highlight at all..
X-  add burn in effect shader
X-  Fix settings window scroll bar to always display..
X-  Settings closes when picking color outside of popup registering as click close outside
X-  File tree explorer takes a second to load when first selected folder… 
X-  C grammar appears to be crashing, issue fixed, was re using parse tree for some reason.
X-  Fix file finder bug when switching quickly… debounce loading files too quick
X-  Line Move, hold down option with up down to move line… 
X-  Fix side bar divider hover color to blue.. currently black. Possible increase size..
X-  fix file tree explorer hover effect to not get cut off
X-  Fix terminal emulator character width to match font size and update when font size changes… how is font size being tracked, how character width set???
X-  Fix settings ex Botton right side padding issue
X-  Fix mouse hover on file finder popup
X-  Fix scroll speed, slow scrolling down

————
And much more undocumented



