#pragma once

// Included from NeuralAmpModeler.cpp after the NeuralAmpModeler class is declared.
// These workflow files split the implementation without changing Xcode target membership.

void NeuralAmpModeler::SetNAMRootDirectory(const WDL_String& directory)
{
  mNAMRootDirectory.Set(directory.Get());
#ifdef OS_MAC
  mNAMRootDirectoryBookmark.Set("");
  CreateSecurityScopedBookmark(mNAMRootDirectory.Get(), mNAMRootDirectoryBookmark);
#endif
  _SaveLibrarySettings();
  _RefreshLibrarySidebar();

}

void NeuralAmpModeler::SetIRRootDirectory(const WDL_String& directory)
{
  mIRRootDirectory.Set(directory.Get());
#ifdef OS_MAC
  mIRRootDirectoryBookmark.Set("");
  CreateSecurityScopedBookmark(mIRRootDirectory.Get(), mIRRootDirectoryBookmark);
#endif
  _SaveLibrarySettings();
  _RefreshLibrarySidebar();

}
