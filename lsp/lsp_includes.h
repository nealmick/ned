#pragma once

// Platform-specific LSP framework includes
#ifdef _WIN32
#include "../build/lib/lsp-framework/generated/lsp/messages.h"
#include "../build/lib/lsp-framework/generated/lsp/types.h"
#else
#include "../.build/lib/lsp-framework/generated/lsp/messages.h"
#include "../.build/lib/lsp-framework/generated/lsp/types.h"
#endif

// Common LSP framework includes
#include "../lib/lsp-framework/lsp/connection.h"
#include "../lib/lsp-framework/lsp/error.h"
#include "../lib/lsp-framework/lsp/messagehandler.h"
#include "../lib/lsp-framework/lsp/process.h"

// Common project includes used by LSP classes
#include "../editor/editor.h"
#include "../editor/editor_scroll.h"
#include "../files/files.h"
#include "lsp_client.h"

// Standard includes commonly used by LSP classes
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>