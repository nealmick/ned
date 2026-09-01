#include "clipboard.h"

namespace {
IClipboard *gClipboard = nullptr;
}

void setEditorClipboard(IClipboard *clipboard) { gClipboard = clipboard; }

IClipboard *editorClipboard() { return gClipboard; }
