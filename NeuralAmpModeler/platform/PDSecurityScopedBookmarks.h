#pragma once

#include "IPlug_include_in_plug_hdr.h"

#ifdef OS_MAC
bool CreateSecurityScopedBookmark(const char* path, WDL_String& bookmark);
bool StartAccessingSecurityScopedBookmark(const char* bookmark, WDL_String& resolvedPath);
void StopAccessingSecurityScopedBookmarks();
#endif
