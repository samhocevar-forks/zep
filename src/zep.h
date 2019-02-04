#pragma once

#ifdef ZEP_SINGLE_HEADER_BUILD
#include "buffer.cpp"
#include "commands.cpp"
#include "editor.cpp"
#include "mode.cpp"
#include "mode_standard.cpp"
#include "mode_vim.cpp"
#include "scroller.cpp"
#include "splits.cpp"
#include "syntax.cpp"
#include "syntax_providers.cpp"
#include "syntax_rainbow_brackets.cpp"
#include "tab_window.cpp"
#include "theme.cpp"
#include "window.cpp"
#include "mcommon/file/file.cpp"
#include "mcommon/file/archive.cpp"

#ifdef ZEP_FEATURE_FILE_WATCHER
#include "mcommon/FileWatcher/FileWatcher.cpp"
#if defined(_WIN32)
#include "mcommon/FileWatcher/FileWatcherWin32.cpp"
#elif defined(__APPLE_CC__) || defined(BSD)
#include "mcommon/FileWatcher/FileWatcherOSX.cpp"
#elif defined(__linux__)
#include "mcommon/FileWatcher/FileWatcherLinux.cpp"
#endif
#endif

#include "mcommon/string/stringutils.cpp"
#include "mcommon/animation/timer.cpp"
#ifdef ZEP_QT
#include "imgui/display_qt.cpp"
#include "imgui/editor_qt.cpp"
#else
#include "imgui/display_imgui.cpp"
#include "imgui/editor_imgui.cpp"
#endif
#else
#include "editor.h"
#include "syntax.h"
#include "buffer.h"
#include "tab_window.h"
#include "window.h"
#include "mode.h"
#ifdef ZEP_QT
#include "imgui/display_qt.h"
#include "imgui/editor_qt.h"
#else
#include "imgui/display_imgui.h"
#include "imgui/editor_imgui.h"
#include "imgui/console_imgui.h"
#endif
#endif

