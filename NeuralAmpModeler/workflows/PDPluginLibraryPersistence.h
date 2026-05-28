#pragma once

// Included from NeuralAmpModeler.cpp after the NeuralAmpModeler class is declared.
// These workflow files split the implementation without changing Xcode target membership.

void NeuralAmpModeler::_RefreshLibrarySidebar()
{
  if (auto* pGraphics = GetUI())
  {
    if (auto* sidebar = pGraphics->GetControlWithTag(kCtrlTagLibrarySidebar))
      sidebar->As<NAMLibrarySidebarControl>()->SetRoots(mNAMRootDirectory.Get(), mIRRootDirectory.Get());
    if (auto* ampSelector = pGraphics->GetControlWithTag(kCtrlTagAmpSelectorScreen))
      ampSelector->As<PDAmpSelectorScreenControl>()->SetRootDirectory(mNAMRootDirectory.Get());
    if (auto* cabSelector = pGraphics->GetControlWithTag(kCtrlTagCabSelectorScreen))
      cabSelector->As<PDAmpSelectorScreenControl>()->SetRootDirectory(mIRRootDirectory.Get());
    if (auto* thumbnail = pGraphics->GetControlWithTag(kCtrlTagModelThumbnail))
      thumbnail->As<NAMModelThumbnailControl>()->SetNAMRootDirectory(mNAMRootDirectory.Get());
  }
}

void NeuralAmpModeler::_LoadLibrarySettings()
{
  std::filesystem::path path;
  if (!GetLibrarySettingsPath(path))
    return;

  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec))
    return;

  std::ifstream stream(path);
  if (!stream.good())
    return;

  const auto settings = nlohmann::json::parse(stream, nullptr, false);
  if (settings.is_discarded() || !settings.is_object())
    return;

  const auto loadString = [&settings](const char* key, WDL_String& target) {
    const auto it = settings.find(key);
    if (it != settings.end() && it->is_string())
      target.Set(it->get<std::string>().c_str());
  };

  loadString(kNAMRootDirectoryKey, mNAMRootDirectory);
  loadString(kIRRootDirectoryKey, mIRRootDirectory);

#ifdef OS_MAC
  loadString(kNAMRootDirectoryBookmarkKey, mNAMRootDirectoryBookmark);
  loadString(kIRRootDirectoryBookmarkKey, mIRRootDirectoryBookmark);

  WDL_String resolvedPath;
  if (StartAccessingSecurityScopedBookmark(mNAMRootDirectoryBookmark.Get(), resolvedPath) && resolvedPath.GetLength())
    mNAMRootDirectory.Set(resolvedPath.Get());

  resolvedPath.Set("");
  if (StartAccessingSecurityScopedBookmark(mIRRootDirectoryBookmark.Get(), resolvedPath) && resolvedPath.GetLength())
    mIRRootDirectory.Set(resolvedPath.Get());
#endif
}

void NeuralAmpModeler::_SaveLibrarySettings() const
{
  std::filesystem::path path;
  if (!GetLibrarySettingsPath(path))
    return;

  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec)
    return;

  nlohmann::json settings;
  settings[kNAMRootDirectoryKey] = std::string(mNAMRootDirectory.Get());
  settings[kIRRootDirectoryKey] = std::string(mIRRootDirectory.Get());
#ifdef OS_MAC
  settings[kNAMRootDirectoryBookmarkKey] = std::string(mNAMRootDirectoryBookmark.Get());
  settings[kIRRootDirectoryBookmarkKey] = std::string(mIRRootDirectoryBookmark.Get());
#endif

  std::ofstream stream(path, std::ios::trunc);
  if (stream.good())
    stream << settings.dump(2) << '\n';
}
