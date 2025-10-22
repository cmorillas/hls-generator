// Copyright (c) 2014 Marshall A. Greenblatt. All rights reserved.
// Minimal cef_win.h for dynamic loading

#ifndef CEF_INCLUDE_INTERNAL_CEF_WIN_H_
#define CEF_INCLUDE_INTERNAL_CEF_WIN_H_
#pragma once

#include "include/internal/cef_types_win.h"
#include "include/internal/cef_types_wrappers.h"

// Handle types for Windows
#define CefCursorHandle cef_cursor_handle_t
#define CefEventHandle cef_event_handle_t
#define CefWindowHandle cef_window_handle_t
#define CefTextInputContext cef_text_input_context_t

#define kNullCursorHandle NULL
#define kNullEventHandle NULL
#define kNullWindowHandle NULL
#define kNullTextInputContext NULL

#if defined(OS_WIN) || defined(PLATFORM_WINDOWS)

// CefMainArgs wrapper for Windows
class CefMainArgs : public cef_main_args_t {
 public:
  CefMainArgs() : cef_main_args_t{} {}
  CefMainArgs(const cef_main_args_t& r) : cef_main_args_t(r) {}
  CefMainArgs(HINSTANCE hInstance) : cef_main_args_t{hInstance} {}
};

// CefWindowInfo wrapper
struct CefWindowInfoTraits {
  typedef cef_window_info_t struct_type;

  static inline void init(struct_type* s) {}
  static inline void clear(struct_type* s) {}

  static inline void set(const struct_type* src,
                          struct_type* target,
                          bool copy) {
    *target = *src;
  }
};

// CefWindowInfo with helper methods
class CefWindowInfo : public CefStructBase<CefWindowInfoTraits> {
 public:
  CefWindowInfo() : CefStructBase<CefWindowInfoTraits>() {}
  CefWindowInfo(const cef_window_info_t& r) : CefStructBase<CefWindowInfoTraits>(r) {}

  void SetAsWindowless(CefWindowHandle parent) {
    windowless_rendering_enabled = true;
    parent_window = parent;
    runtime_style = CEF_RUNTIME_STYLE_ALLOY;
  }
};

#endif  // OS_WIN || PLATFORM_WINDOWS

#endif  // CEF_INCLUDE_INTERNAL_CEF_WIN_H_
