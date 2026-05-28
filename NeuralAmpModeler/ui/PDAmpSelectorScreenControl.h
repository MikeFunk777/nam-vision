#pragma once

// Included from NeuralAmpModeler.cpp inside its anonymous namespace.
// Keep these UI building blocks header-only until the Xcode project is split into compiled sources.

class PDAmpSelectorScreenControl : public IControl
{
public:
  enum class Target
  {
    Amp,
    Cab
  };

  PDAmpSelectorScreenControl(const IRECT& bounds, const ISVG& backIcon, const IBitmap& fallbackImage,
                             const IBitmap& noSelectionImage, const char* title, const char* extension, Target target)
  : IControl(bounds)
  , mBackIcon(backIcon)
  , mFallbackImage(fallbackImage)
  , mNoSelectionImage(noSelectionImage)
  , mTitle(title)
  , mExtension(extension)
  , mTarget(target)
  {
  }

  void SetNAMRootDirectory(const char* namRootDirectory)
  {
    SetRootDirectory(namRootDirectory);
  }

  void SetRootDirectory(const char* rootDirectory)
  {
    const std::string root = NormalizePath(rootDirectory);
    if (root == mRootDirectory)
      return;

    mRootDirectory = root;
    Rescan();
    SetDirty(false);
  }

  void Draw(IGraphics& g) override
  {
    const auto headerBounds = IRECT(mRECT.L, mRECT.T, mRECT.R, mRECT.T + kMainHeaderHeight);
    const auto borderBounds =
      IRECT(headerBounds.L, headerBounds.B - kMainHeaderBorderSize, headerBounds.R, headerBounds.B);
    const auto leftSlot = GetBackButtonBounds();
    const IText titleText(kSelectorTextSize, kPDForeground, kPDFontBold, EAlign::Center, EVAlign::Middle);

    g.FillRect(kPDBackground, mRECT);
    g.FillRect(kPDBackground, headerBounds);
    g.FillRect(kPDLightGrey, borderBounds);
    g.DrawSVG(mBackIcon, leftSlot, &mBlend);
    g.DrawText(titleText, mTitle.c_str(), headerBounds, &mBlend);
    DrawGrid(g);
    DrawSubfolderPrompt(g);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mShowingSubfolderPrompt)
    {
      HandleSubfolderPromptMouseDown(x, y);
      return;
    }

    if (GetBackButtonBounds().Contains(x, y))
    {
      CloseScreen();
      return;
    }

    SelectFolderAtPoint(x, y);
  }

  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    if (mShowingSubfolderPrompt)
      return;

    const auto scrollArea = GetScrollArea();
    if (!scrollArea.Contains(x, y))
      return;

    const float contentHeight = GetContentHeight();
    if (contentHeight <= scrollArea.H())
      return;

    mScroll -= d * kAmpSelectorWheelStep;
    ClampScroll();
    SetDirty(false);
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    IControl::OnMouseOver(x, y, mod);
  }

  void OnMouseOut() override
  {
    IControl::OnMouseOut();
  }

private:
  struct Folder
  {
    std::string name;
    std::string path;
    std::vector<std::string> directFiles;
    std::vector<std::string> recursiveFiles;
    bool hasDirectSubfolders = false;
    std::string thumbnailPath;
    IBitmap thumbnail;
    std::vector<std::string> titleLines;
    float titleLinesWidth = 0.f;
  };

  IRECT GetBackButtonBounds() const
  {
    return IRECT(mRECT.L + kMainHeaderPadding, mRECT.T + kMainHeaderPadding,
                 mRECT.L + kMainHeaderPadding + kMainHeaderSlotSize,
                 mRECT.T + kMainHeaderPadding + kMainHeaderSlotSize);
  }

  IRECT GetScrollArea() const { return IRECT(mRECT.L, mRECT.T + kAmpSelectorContentTop, mRECT.R, mRECT.B); }

  float GetContentHeight() const
  {
    const int rows = (GetCardCount() + kAmpSelectorColumns - 1) / kAmpSelectorColumns;
    return rows * kAmpSelectorCardHeight;
  }

  void ClampScroll()
  {
    const float maxScroll = std::max(0.f, GetContentHeight() - GetScrollArea().H());
    mScroll = std::clamp(mScroll, 0.f, maxScroll);
  }

  void DrawGrid(IGraphics& g)
  {
    const auto scrollArea = GetScrollArea();
    const IText titleText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);

    ClampScroll();
    g.PathClipRegion(scrollArea);

    for (int i = 0; i < GetCardCount(); ++i)
    {
      const int column = i % kAmpSelectorColumns;
      const int row = i / kAmpSelectorColumns;
      const float cardLeft = scrollArea.L + (column * kAmpSelectorCardWidth);
      const float cardTop = scrollArea.T + (row * kAmpSelectorCardHeight) - mScroll;
      const auto cardBounds =
        IRECT(cardLeft, cardTop, cardLeft + kAmpSelectorCardWidth, cardTop + kAmpSelectorCardHeight);

      if (cardBounds.B < scrollArea.T || cardBounds.T > scrollArea.B)
        continue;

      const auto imageBounds =
        IRECT(cardBounds.L + kAmpSelectorCardPadding, cardBounds.T + kAmpSelectorCardPadding,
              cardBounds.L + kAmpSelectorCardPadding + kAmpSelectorImageSize,
              cardBounds.T + kAmpSelectorCardPadding + kAmpSelectorImageSize);
      DrawCardBorder(g, cardBounds);

      if (i == 0)
      {
        DrawContainedBitmap(g, mNoSelectionImage.IsValid() ? mNoSelectionImage : mFallbackImage, imageBounds, 1.f);
        DrawCardTitleText(g, titleText, NoSelectionLabel(), imageBounds, cardBounds);
        continue;
      }

      auto& folder = mFolders[static_cast<size_t>(i - 1)];
      LoadFolderThumbnail(folder);
      DrawContainedBitmap(g, folder.thumbnail.IsValid() ? folder.thumbnail : mFallbackImage, imageBounds, 1.f);
      DrawCardTitle(g, titleText, folder, imageBounds, cardBounds);
    }

    g.PathClipRegion(IRECT());
  }

  void DrawCardBorder(IGraphics& g, const IRECT& bounds)
  {
    g.FillRect(kPDLightGrey, IRECT(bounds.R - 2.f, bounds.T, bounds.R, bounds.B));
    g.FillRect(kPDLightGrey, IRECT(bounds.L, bounds.B - 2.f, bounds.R, bounds.B));
  }

  static void DrawContainedBitmap(IGraphics& g, const IBitmap& bitmap, const IRECT& bounds, float opacity)
  {
    if (!bitmap.IsValid() || bitmap.W() <= 0 || bitmap.H() <= 0)
      return;

    const float scale = std::min(bounds.W() / static_cast<float>(bitmap.W()), bounds.H() / static_cast<float>(bitmap.H()));
    const float drawWidth = bitmap.W() * scale;
    const float drawHeight = bitmap.H() * scale;
    const auto imageBounds = bounds.GetCentredInside(drawWidth, drawHeight);
    const IBlend blend(EBlend::Default, opacity);

    g.DrawFittedBitmap(bitmap, imageBounds, &blend);
  }

  void DrawCardTitle(IGraphics& g, const IText& textStyle, Folder& folder, const IRECT& imageBounds,
                     const IRECT& cardBounds)
  {
    const auto textBounds = IRECT(cardBounds.L + kAmpSelectorCardPadding, imageBounds.B,
                                  cardBounds.R - kAmpSelectorCardPadding, cardBounds.B - kAmpSelectorCardPadding);
    const auto& lines = GetTitleLines(g, textStyle, folder, textBounds.W());
    DrawCardTitleLines(g, textStyle, lines, textBounds);
  }

  void DrawCardTitleText(IGraphics& g, const IText& textStyle, const std::string& label, const IRECT& imageBounds,
                         const IRECT& cardBounds)
  {
    const auto textBounds = IRECT(cardBounds.L + kAmpSelectorCardPadding, imageBounds.B,
                                  cardBounds.R - kAmpSelectorCardPadding, cardBounds.B - kAmpSelectorCardPadding);
    const auto lines = WrapTextToFit(g, textStyle, label, textBounds.W());
    DrawCardTitleLines(g, textStyle, lines, textBounds);
  }

  void DrawCardTitleLines(IGraphics& g, const IText& textStyle, const std::vector<std::string>& lines,
                          const IRECT& textBounds)
  {
    if (lines.empty())
      return;

    const float lineHeight = textStyle.mSize + 2.f;
    const int maxLines = static_cast<int>(std::max(1.f, std::floor((textBounds.H() - (2.f * kAmpSelectorTitlePaddingY))
                                                                   / lineHeight)));
    const int lineCount = std::min(static_cast<int>(lines.size()), maxLines);
    const float totalTextHeight = static_cast<float>(lineCount) * lineHeight;
    const auto titleBounds = IRECT(textBounds.L, textBounds.T, textBounds.R,
                                   std::max(textBounds.T + kAmpSelectorTitleMinHeight, textBounds.B));
    const IText lineText = textStyle.WithVAlign(EVAlign::Middle).WithAlign(EAlign::Center);

    g.FillRect(kPDBackground, titleBounds);

    float y = titleBounds.MH() - (totalTextHeight / 2.f);
    for (int i = 0; i < lineCount; ++i)
    {
      const IRECT lineBounds(titleBounds.L, y, titleBounds.R, y + lineHeight);
      g.DrawText(lineText, lines[static_cast<size_t>(i)].c_str(), lineBounds, &mBlend);
      y += lineHeight;
    }
  }

  const std::vector<std::string>& GetTitleLines(IGraphics& g, const IText& textStyle, Folder& folder, float maxWidth)
  {
    if (folder.titleLines.empty() || std::abs(folder.titleLinesWidth - maxWidth) > 0.5f)
    {
      folder.titleLines = WrapTextToFit(g, textStyle, folder.name, maxWidth);
      folder.titleLinesWidth = maxWidth;
    }

    return folder.titleLines;
  }

  static std::vector<std::string> WrapTextToFit(IGraphics& g, const IText& textStyle, const std::string& text,
                                                float maxWidth)
  {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word;
    std::string currentLine;

    while (stream >> word)
    {
      const std::string candidate = currentLine.empty() ? word : currentLine + " " + word;
      if (TextFits(g, textStyle, candidate, maxWidth))
      {
        currentLine = candidate;
        continue;
      }

      if (!currentLine.empty())
      {
        lines.push_back(currentLine);
        currentLine.clear();
      }

      if (TextFits(g, textStyle, word, maxWidth))
      {
        currentLine = word;
        continue;
      }

      SplitLongWord(g, textStyle, word, maxWidth, lines, currentLine);
    }

    if (!currentLine.empty())
      lines.push_back(currentLine);

    return lines;
  }

  static bool TextFits(IGraphics& g, const IText& textStyle, const std::string& text, float maxWidth)
  {
    IRECT measured;
    g.MeasureText(textStyle, text.c_str(), measured);
    return measured.W() <= maxWidth;
  }

  static void SplitLongWord(IGraphics& g, const IText& textStyle, const std::string& word, float maxWidth,
                            std::vector<std::string>& lines, std::string& currentLine)
  {
    size_t start = 0;
    while (start < word.size())
    {
      size_t length = 1;
      while (start + length <= word.size() && TextFits(g, textStyle, word.substr(start, length), maxWidth))
        length++;

      if (length > 1)
        length--;

      const std::string part = word.substr(start, length);
      if (start + length >= word.size())
        currentLine = part;
      else
        lines.push_back(part);

      start += length;
    }
  }

  IRECT GetSubfolderPromptCardBounds() const
  {
    return mRECT.GetCentredInside(kSubfolderPromptCardWidth, kSubfolderPromptCardHeight);
  }

  IRECT GetSubfolderPromptTextBounds() const
  {
    const auto card = GetSubfolderPromptCardBounds();
    return IRECT(card.L + kSubfolderPromptCardPadding, card.T + kSubfolderPromptCardPadding,
                 card.R - kSubfolderPromptCardPadding,
                 card.T + kSubfolderPromptCardPadding + kSubfolderPromptTextHeight);
  }

  IRECT GetSubfolderSelectButtonBounds() const
  {
    const auto card = GetSubfolderPromptCardBounds();
    const float totalButtonWidth = kSubfolderPromptSelectButtonWidth + kSubfolderPromptButtonGap +
                                   kSubfolderPromptLoadAllButtonWidth;
    const float left = card.MW() - (totalButtonWidth / 2.f);
    const float top = GetSubfolderPromptTextBounds().B + kSubfolderPromptTextButtonGap;
    return IRECT(left, top, left + kSubfolderPromptSelectButtonWidth, top + kSettingsButtonHeight);
  }

  IRECT GetSubfolderLoadAllButtonBounds() const
  {
    const auto select = GetSubfolderSelectButtonBounds();
    return IRECT(select.R + kSubfolderPromptButtonGap, select.T,
                 select.R + kSubfolderPromptButtonGap + kSubfolderPromptLoadAllButtonWidth, select.B);
  }

  void DrawSubfolderPrompt(IGraphics& g)
  {
    if (!mShowingSubfolderPrompt)
      return;

    static constexpr const char* kPromptText =
      "These captures are organised into sub-folders on your system. Select the sub-folder you'd like to load "
      "or load all files found in all sub-folders.";

    const auto card = GetSubfolderPromptCardBounds();
    const auto textBounds = GetSubfolderPromptTextBounds();
    const IText text(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);
    const auto lines = WrapTextToFit(g, text, kPromptText, textBounds.W());
    const float lineHeight = text.mSize + 2.f;
    const float textHeight = static_cast<float>(lines.size()) * lineHeight;
    float y = textBounds.MH() - (textHeight / 2.f);

    g.FillRect(kPDForeground.WithOpacity(0.8f), mRECT);
    g.FillRoundRect(kPDBackground, card, kSubfolderPromptCardRadius, &mBlend);

    for (const auto& line : lines)
    {
      const IRECT lineBounds(textBounds.L, y, textBounds.R, y + lineHeight);
      g.DrawText(text, line.c_str(), lineBounds, &mBlend);
      y += lineHeight;
    }

    DrawGeneralButton(g, GetSubfolderSelectButtonBounds(), "Select sub-folder");
    DrawGeneralButton(g, GetSubfolderLoadAllButtonBounds(), "Load all");
  }

  void DrawGeneralButton(IGraphics& g, const IRECT& bounds, const char* label)
  {
    const IText text(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);
    g.DrawRoundRect(kPDForeground, bounds, kSettingsButtonRadius, &mBlend, kSettingsButtonBorderSize);
    g.DrawText(text, label, bounds, &mBlend);
  }

  void HandleSubfolderPromptMouseDown(float x, float y)
  {
    if (GetSubfolderSelectButtonBounds().Contains(x, y))
    {
      PromptForSubfolder();
      return;
    }

    if (GetSubfolderLoadAllButtonBounds().Contains(x, y))
    {
      LoadFolderFiles(mSubfolderPromptFolder.recursiveFiles);
      return;
    }

    if (!GetSubfolderPromptCardBounds().Contains(x, y))
      HideSubfolderPrompt();
  }

  void ShowSubfolderPrompt(const Folder& folder)
  {
    mSubfolderPromptFolder = Folder {};
    mSubfolderPromptFolder.name = folder.name;
    mSubfolderPromptFolder.path = folder.path;
    mSubfolderPromptFolder.directFiles = folder.directFiles;
    mSubfolderPromptFolder.recursiveFiles = folder.recursiveFiles;
    mSubfolderPromptFolder.hasDirectSubfolders = folder.hasDirectSubfolders;
    mShowingSubfolderPrompt = true;
    SetDirty(false);
  }

  void HideSubfolderPrompt()
  {
    mShowingSubfolderPrompt = false;
    mSubfolderPromptFolder = Folder {};
    SetDirty(false);
  }

  void PromptForSubfolder()
  {
    WDL_String directory(mSubfolderPromptFolder.path.c_str());
    mShowingSubfolderPrompt = false;
    SetDirty(false);

    GetUI()->PromptForDirectory(directory, [this](const WDL_String& fileName, const WDL_String& path) {
      if (!path.GetLength())
        return;

      bool readError = false;
      Folder folder;
      if (!BuildRecursiveLoadableFolder(std::filesystem::u8path(path.Get()), folder, readError))
        return;

      LoadFolderFiles(folder.hasDirectSubfolders || folder.directFiles.empty() ? folder.recursiveFiles
                                                                               : folder.directFiles);
    });
  }

  void SelectFolderAtPoint(float x, float y)
  {
    const auto scrollArea = GetScrollArea();
    if (!scrollArea.Contains(x, y))
      return;

    const int cardIndex = GetCardIndexAtPoint(x, y);
    if (cardIndex < 0)
      return;

    if (cardIndex == 0)
    {
      ClearSelection();
      return;
    }

    const int folderIndex = cardIndex - 1;
    if (folderIndex < 0 || folderIndex >= static_cast<int>(mFolders.size()))
      return;

    const auto& folder = mFolders[static_cast<size_t>(folderIndex)];
    if (folder.hasDirectSubfolders)
    {
      ShowSubfolderPrompt(folder);
      return;
    }

    LoadFolderFiles(folder.directFiles);
  }

  void LoadFolderFiles(std::vector<std::string> files)
  {
    NormalizeAndSortFiles(files);
    if (files.empty())
      return;

    WDL_String path(files.front().c_str());
    if (auto* ui = GetUI())
    {
      if (auto* selector = ui->GetControlWithTag(kCtrlTagSelectorArea))
      {
        if (mTarget == Target::Amp)
          selector->As<SelectorAreaControl>()->SetPendingAmpFiles(files);
        else
          selector->As<SelectorAreaControl>()->SetPendingCabFiles(files);
      }
    }

    if (mTarget == Target::Amp)
      PLUG()->LoadNAMFile(path);
    else
      PLUG()->LoadIRFile(path);

    CloseScreen();
  }

  void ClearSelection()
  {
    const int clearMsgTag = mTarget == Target::Amp ? kMsgTagClearModel : kMsgTagClearIR;

    if (auto* delegate = GetDelegate())
      delegate->SendArbitraryMsgFromUI(clearMsgTag);

    if (auto* ui = GetUI())
    {
      if (auto* mainArea = ui->GetControlWithTag(kCtrlTagMainArea))
        mainArea->OnMsgFromDelegate(clearMsgTag, 0, nullptr);

      if (auto* selectorArea = ui->GetControlWithTag(kCtrlTagSelectorArea))
        selectorArea->OnMsgFromDelegate(clearMsgTag, 0, nullptr);
    }

    CloseScreen();
  }

  void CloseScreen()
  {
    mShowingSubfolderPrompt = false;
    mSubfolderPromptFolder = Folder {};
    ReleaseThumbnails();
    Hide(true);
    if (auto* ui = GetUI())
      ui->SetAllControlsDirty();
  }

  int GetCardIndexAtPoint(float x, float y) const
  {
    const auto scrollArea = GetScrollArea();
    const float localY = y - scrollArea.T + mScroll;
    if (localY < 0.f)
      return -1;

    const int row = static_cast<int>(localY / kAmpSelectorCardHeight);
    const int column = static_cast<int>((x - scrollArea.L) / kAmpSelectorCardWidth);
    if (column < 0 || column >= kAmpSelectorColumns)
      return -1;

    const int index = row * kAmpSelectorColumns + column;
    if (index < 0 || index >= GetCardCount())
      return -1;

    return index;
  }

  int GetCardCount() const { return static_cast<int>(mFolders.size()) + 1; }

  std::string NoSelectionLabel() const { return mTarget == Target::Amp ? "No amp" : "No cab"; }

  static std::vector<std::string> FindFiles(const std::filesystem::path& directory, const std::string& extension)
  {
    std::vector<std::string> files;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, ec),
         end;
         !ec && it != end; it.increment(ec))
    {
      std::error_code entryError;
      if (it->is_regular_file(entryError) && HasExtension(it->path(), extension.c_str()))
        files.push_back(PathString(it->path()));
    }

    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return ToLower(a) < ToLower(b); });
    return files;
  }

  static void NormalizeAndSortFiles(std::vector<std::string>& files)
  {
    for (auto& file : files)
      file = NormalizePath(file.c_str());

    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return ToLower(a) < ToLower(b); });
    files.erase(std::unique(files.begin(), files.end()), files.end());
  }

  static void FindFilesRecursive(const std::filesystem::path& directory, const std::string& extension,
                                 std::vector<std::string>& files, bool& readError)
  {
    std::vector<std::filesystem::directory_entry> entries;
    std::error_code ec;

    for (std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied,
                                                ec),
         end;
         !ec && it != end; it.increment(ec))
    {
      entries.push_back(*it);
    }

    if (ec)
    {
      readError = true;
      return;
    }

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
      return ToLower(a.path().filename().string()) < ToLower(b.path().filename().string());
    });

    for (const auto& entry : entries)
    {
      std::error_code entryError;
      if (entry.is_regular_file(entryError) && HasExtension(entry.path(), extension.c_str()))
        files.push_back(PathString(entry.path()));
    }

    for (const auto& entry : entries)
    {
      std::error_code entryError;
      if (entry.is_symlink(entryError))
        continue;

      entryError.clear();
      if (entry.is_directory(entryError))
        FindFilesRecursive(entry.path(), extension, files, readError);
    }
  }

  void LoadFolderThumbnail(Folder& folder)
  {
    if (folder.thumbnail.IsValid() || folder.thumbnailPath.empty())
      return;

    std::error_code ec;
    const auto path = std::filesystem::u8path(folder.thumbnailPath);
    if (std::filesystem::is_regular_file(path, ec) && IsJPG(path))
      folder.thumbnail = LoadJPGBitmapFromFile(path);

    if (folder.thumbnail.IsValid() && (folder.thumbnail.W() <= 0 || folder.thumbnail.H() <= 0))
      folder.thumbnail = IBitmap();
  }

  void Rescan()
  {
    ReleaseThumbnails();
    mFolders.clear();
    mScroll = 0.f;

    if (!DirectoryExists(mRootDirectory))
      return;

    bool readError = false;
    if (mTarget == Target::Amp)
      RescanAmpFolders(readError);
    else
      RescanTopLevelFolders(readError);
  }

  void ReleaseThumbnails()
  {
    for (auto& folder : mFolders)
    {
#ifdef OS_MAC
      if (auto* ui = GetUI(); ui != nullptr && folder.thumbnail.IsValid())
        ui->ReleaseBitmap(folder.thumbnail);
#endif
      folder.thumbnail = IBitmap();
    }
  }

  void RescanAmpFolders(bool& readError)
  {
    Folder rootFolder;
    if (BuildLoadableFolder(std::filesystem::u8path(mRootDirectory), rootFolder, readError))
      mFolders.push_back(std::move(rootFolder));

    std::vector<std::filesystem::directory_entry> entries;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(std::filesystem::u8path(mRootDirectory),
                                                std::filesystem::directory_options::skip_permission_denied,
                                                ec),
         end;
         !ec && it != end; it.increment(ec))
    {
      entries.push_back(*it);
    }

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
      return ToLower(a.path().filename().string()) < ToLower(b.path().filename().string());
    });

    for (const auto& entry : entries)
    {
      std::error_code entryError;
      if (entry.is_symlink(entryError))
        continue;

      entryError.clear();
      if (!entry.is_directory(entryError))
        continue;

      Folder folder;
      if (BuildRecursiveLoadableFolder(entry.path(), folder, readError))
        mFolders.push_back(std::move(folder));
    }
  }

  void RescanTopLevelFolders(bool& readError)
  {
    std::vector<std::filesystem::directory_entry> entries;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(std::filesystem::u8path(mRootDirectory),
                                                std::filesystem::directory_options::skip_permission_denied,
                                                ec),
         end;
         !ec && it != end; it.increment(ec))
    {
      entries.push_back(*it);
    }

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
      return ToLower(a.path().filename().string()) < ToLower(b.path().filename().string());
    });

    for (const auto& entry : entries)
    {
      std::error_code entryError;
      if (entry.is_symlink(entryError))
        continue;

      entryError.clear();
      if (entry.is_directory(entryError))
      {
        Folder folder;
        if (BuildRecursiveLoadableFolder(entry.path(), folder, readError))
          mFolders.push_back(std::move(folder));
      }
    }
  }

  bool BuildRecursiveLoadableFolder(const std::filesystem::path& directory, Folder& folder, bool& readError)
  {
    std::vector<std::string> files;
    FindFilesRecursive(directory, mExtension, files, readError);
    NormalizeAndSortFiles(files);
    if (files.empty())
      return false;

    folder.name = FileName(directory);
    folder.path = PathString(directory);
    folder.directFiles = FindFiles(directory, mExtension);
    folder.recursiveFiles = std::move(files);
    folder.hasDirectSubfolders = HasDirectSubfolders(directory, readError);

    std::filesystem::path thumbnailPath;
    if (FindFirstJPG(directory, thumbnailPath))
      folder.thumbnailPath = PathString(PrepareThumbnailFile(thumbnailPath));

    return true;
  }

  bool BuildLoadableFolder(const std::filesystem::path& directory, Folder& folder, bool& readError)
  {
    folder.directFiles = FindFiles(directory, mExtension);
    if (folder.directFiles.empty())
      return false;

    folder.name = FileName(directory);
    folder.path = PathString(directory);
    folder.hasDirectSubfolders = HasDirectSubfolders(directory, readError);
    if (folder.hasDirectSubfolders)
    {
      FindFilesRecursive(directory, mExtension, folder.recursiveFiles, readError);
      NormalizeAndSortFiles(folder.recursiveFiles);
    }
    else
    {
      folder.recursiveFiles = folder.directFiles;
    }

    std::filesystem::path thumbnailPath;
    if (FindFirstJPG(directory, thumbnailPath))
      folder.thumbnailPath = PathString(PrepareThumbnailFile(thumbnailPath));

    return true;
  }

  static bool HasDirectSubfolders(const std::filesystem::path& directory, bool& readError)
  {
    std::error_code ec;
    std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec)
    {
      readError = true;
      return false;
    }

    for (std::filesystem::directory_iterator end; it != end; it.increment(ec))
    {
      if (ec)
      {
        readError = true;
        return false;
      }

      std::error_code entryError;
      if (!it->is_symlink(entryError))
      {
        entryError.clear();
        if (it->is_directory(entryError))
          return true;
      }
    }

    return false;
  }

  static bool CountDirectFilesWithExtension(const std::filesystem::path& directory, const char* extension, bool& readError)
  {
    std::error_code ec;
    std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec)
    {
      readError = true;
      return false;
    }

    for (std::filesystem::directory_iterator end; it != end; it.increment(ec))
    {
      if (ec)
      {
        readError = true;
        return false;
      }

      std::error_code entryError;
      if (it->is_regular_file(entryError) && HasExtension(it->path(), extension))
        return true;
    }

    return false;
  }

  static bool FindFirstJPG(const std::filesystem::path& directory, std::filesystem::path& result)
  {
    if (FindFirstGeneratedThumbnailJPG(directory, result, kAmpSelectorThumbnailPixelSize))
      return true;

    if (FindFirstSourceJPG(directory, result))
      return true;

    return FindFirstGeneratedThumbnailJPG(directory, result, 181);
  }

  static bool FindFirstGeneratedThumbnailJPG(const std::filesystem::path& directory, std::filesystem::path& result,
                                             int pixelSize)
  {
    return FindFirstJPGMatching(directory, result, true, pixelSize);
  }

  static bool FindFirstSourceJPG(const std::filesystem::path& directory, std::filesystem::path& result)
  {
    return FindFirstJPGMatching(directory, result, false, 0);
  }

  static bool FindFirstJPGMatching(const std::filesystem::path& directory, std::filesystem::path& result,
                                   bool generatedOnly, int generatedPixelSize)
  {
    std::vector<std::filesystem::directory_entry> entries;
    std::error_code ec;

    for (std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied,
                                                ec),
         end;
         !ec && it != end; it.increment(ec))
    {
      entries.push_back(*it);
    }

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
      return ToLower(a.path().filename().string()) < ToLower(b.path().filename().string());
    });

    for (const auto& entry : entries)
    {
      std::error_code entryError;
      if (!entry.is_regular_file(entryError) || !IsJPG(entry.path()))
        continue;

      const bool isGeneratedThumbnail = IsGeneratedThumbnailJPG(entry.path());
      const bool isRequestedGeneratedThumbnail =
        generatedOnly && IsGeneratedThumbnailJPGForSize(entry.path(), generatedPixelSize);

      if (isRequestedGeneratedThumbnail || (!generatedOnly && !isGeneratedThumbnail))
      {
        result = entry.path();
        return true;
      }
    }

    for (const auto& entry : entries)
    {
      std::error_code entryError;
      if (entry.is_symlink(entryError))
        continue;

      entryError.clear();
      if (entry.is_directory(entryError)
          && FindFirstJPGMatching(entry.path(), result, generatedOnly, generatedPixelSize))
        return true;
    }

    return false;
  }

  static std::filesystem::path PrepareThumbnailFile(const std::filesystem::path& source)
  {
    if (IsGeneratedThumbnailJPG(source))
      return source;

    const auto target = GeneratedThumbnailPath(source);
    std::error_code ec;
    if (std::filesystem::is_regular_file(target, ec))
      return target;

#ifdef OS_MAC
    if (CreateThumbnailJPG(source, target))
      return target;
#endif

    return source;
  }

  static std::filesystem::path GeneratedThumbnailPath(const std::filesystem::path& source)
  {
    return source.parent_path() / (source.stem().string() + "@" +
                                   std::to_string(kAmpSelectorThumbnailPixelSize) + "px.jpg");
  }

  static std::string ToLower(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
  }

  static bool HasExtension(const std::filesystem::path& path, const char* extension)
  {
    return ToLower(path.extension().string()) == std::string(".") + extension;
  }

  static bool IsJPG(const std::filesystem::path& path)
  {
    return ToLower(path.extension().string()) == ".jpg";
  }

  static bool IsGeneratedThumbnailJPG(const std::filesystem::path& path)
  {
    return IsGeneratedThumbnailJPGForSize(path, kAmpSelectorThumbnailPixelSize)
           || IsGeneratedThumbnailJPGForSize(path, 181);
  }

  static bool IsGeneratedThumbnailJPGForSize(const std::filesystem::path& path, int pixelSize)
  {
    const std::string stem = ToLower(path.stem().string());
    const std::string suffix = "@" + std::to_string(pixelSize);
    return IsJPG(path) && (stem.ends_with(suffix + "px") || stem.ends_with(suffix));
  }

  IBitmap LoadJPGBitmapFromFile(const std::filesystem::path& path)
  {
    std::vector<uint8_t> data;
    if (!ReadFileBytes(path, data))
      return IBitmap();

    const std::string cacheName = ThumbnailCacheName(path);
    return GetUI()->LoadBitmap(cacheName.c_str(), data.data(), static_cast<int>(data.size()), 1, false, 1);
  }

  static bool ReadFileBytes(const std::filesystem::path& path, std::vector<uint8_t>& data)
  {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.good())
      return false;

    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size <= 0 || size > std::numeric_limits<int>::max())
      return false;

    stream.seekg(0, std::ios::beg);
    data.resize(static_cast<size_t>(size));
    stream.read(reinterpret_cast<char*>(data.data()), size);
    return stream.good();
  }

  static std::string ThumbnailCacheName(const std::filesystem::path& path)
  {
    uint64_t hash = 1469598103934665603ull;
    for (unsigned char c : PathString(path))
    {
      hash ^= c;
      hash *= 1099511628211ull;
    }

    return "pd-selector-thumbnail-" + std::to_string(hash) + ".jpg";
  }

#ifdef OS_MAC
  static bool CreateThumbnailJPG(const std::filesystem::path& sourcePath, const std::filesystem::path& targetPath)
  {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(sourcePath, ec))
      return false;

    const std::string pathString = sourcePath.string();
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                           reinterpret_cast<const UInt8*>(pathString.c_str()),
                                                           static_cast<CFIndex>(pathString.size()),
                                                           false);
    if (url == nullptr)
      return false;

    CGImageSourceRef source = CGImageSourceCreateWithURL(url, nullptr);
    CFRelease(url);

    if (source == nullptr)
      return false;

    int maxPixelSize = kAmpSelectorThumbnailPixelSize;
    CFNumberRef maxPixelSizeValue = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &maxPixelSize);
    if (maxPixelSizeValue == nullptr)
    {
      CFRelease(source);
      return false;
    }

    const void* thumbnailKeys[] = {kCGImageSourceCreateThumbnailFromImageAlways,
                                   kCGImageSourceThumbnailMaxPixelSize,
                                   kCGImageSourceCreateThumbnailWithTransform};
    const void* thumbnailValues[] = {kCFBooleanTrue, maxPixelSizeValue, kCFBooleanTrue};
    CFDictionaryRef thumbnailOptions =
      CFDictionaryCreate(kCFAllocatorDefault, thumbnailKeys, thumbnailValues, 3, &kCFTypeDictionaryKeyCallBacks,
                         &kCFTypeDictionaryValueCallBacks);
    CFRelease(maxPixelSizeValue);

    if (thumbnailOptions == nullptr)
    {
      CFRelease(source);
      return false;
    }

    CGImageRef thumbnail = CGImageSourceCreateThumbnailAtIndex(source, 0, thumbnailOptions);
    CFRelease(thumbnailOptions);
    CFRelease(source);

    if (thumbnail == nullptr || CGImageGetWidth(thumbnail) == 0 || CGImageGetHeight(thumbnail) == 0)
    {
      if (thumbnail != nullptr)
        CGImageRelease(thumbnail);
      return false;
    }

    const std::string targetString = targetPath.string();
    CFURLRef targetURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                                 reinterpret_cast<const UInt8*>(targetString.c_str()),
                                                                 static_cast<CFIndex>(targetString.size()),
                                                                 false);
    if (targetURL == nullptr)
    {
      CGImageRelease(thumbnail);
      return false;
    }

    CGImageDestinationRef destination = CGImageDestinationCreateWithURL(targetURL, CFSTR("public.jpeg"), 1, nullptr);
    CFRelease(targetURL);

    if (destination == nullptr)
    {
      CGImageRelease(thumbnail);
      return false;
    }

    float quality = 0.82f;
    CFNumberRef qualityValue = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &quality);
    CFDictionaryRef properties = nullptr;
    if (qualityValue != nullptr)
    {
      const void* propertyKeys[] = {kCGImageDestinationLossyCompressionQuality};
      const void* propertyValues[] = {qualityValue};
      properties = CFDictionaryCreate(kCFAllocatorDefault, propertyKeys, propertyValues, 1,
                                      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
      CFRelease(qualityValue);
    }

    CGImageDestinationAddImage(destination, thumbnail, properties);
    const bool ok = CGImageDestinationFinalize(destination);
    if (properties != nullptr)
      CFRelease(properties);
    CFRelease(destination);
    CGImageRelease(thumbnail);

    return ok && std::filesystem::is_regular_file(targetPath, ec);
  }
#endif

  static std::string PathString(const std::filesystem::path& path)
  {
    return path.lexically_normal().string();
  }

  static std::string NormalizePath(const char* path)
  {
    if (!CStringHasContents(path))
      return "";

    try
    {
      return PathString(std::filesystem::u8path(path));
    }
    catch (...)
    {
      return path;
    }
  }

  static bool DirectoryExists(const std::string& path)
  {
    if (path.empty())
      return false;

    std::error_code ec;
    return std::filesystem::is_directory(std::filesystem::u8path(path), ec);
  }

  static std::string FileName(const std::filesystem::path& path)
  {
    const std::string name = path.filename().string();
    return name.empty() ? path.string() : name;
  }

  ISVG mBackIcon;
  IBitmap mFallbackImage;
  IBitmap mNoSelectionImage;
  std::string mTitle;
  std::string mExtension;
  Target mTarget;
  std::string mRootDirectory;
  std::vector<Folder> mFolders;
  Folder mSubfolderPromptFolder;
  bool mShowingSubfolderPrompt = false;
  float mScroll = 0.f;
};
