// close_popper.cpp
#include "close_popper.h"
#include "editor.h"
#include "settings.h"
#include "bookmarks.h"
#include "line_jump.h"

void ClosePopper::closeAllExcept(Type keepOpen) {
    switch(keepOpen) {
        case Type::Settings:
            gBookmarks.showBookmarksWindow = false;
            gLineJump.showLineJumpWindow = false;
            break;

        case Type::Bookmarks:
            gSettings.showSettingsWindow = false;
            gLineJump.showLineJumpWindow = false;
            break;

        case Type::LineJump:
            gSettings.showSettingsWindow = false;
            gBookmarks.showBookmarksWindow = false;
            break;
    }
}
void ClosePopper::closeAll() {
    gSettings.showSettingsWindow = false;
    gBookmarks.showBookmarksWindow = false;
    gLineJump.showLineJumpWindow = false;
}