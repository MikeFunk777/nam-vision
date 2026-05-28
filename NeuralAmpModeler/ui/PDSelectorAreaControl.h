#pragma once

// Included from NeuralAmpModeler.cpp inside its anonymous namespace.
// Keep these UI building blocks header-only until the Xcode project is split into compiled sources.

class SelectorAreaControl : public IControl
{
public:
  SelectorAreaControl(const IRECT& bounds, const ISVG& leftArrow, const ISVG& rightArrow)
  : IControl(bounds)
  , mLeftArrow(leftArrow)
  , mRightArrow(rightArrow)
  {
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(kPDBackground, mRECT);

    IRECT section = mRECT.GetFromLeft(kInputMeterWidth);
    section = IRECT(section.R, mRECT.T, section.R + kAmpImageWidth, mRECT.B);
    DrawSelector(g, section, GetLabel(SelectorKind::Amp).c_str());

    section = IRECT(section.R, mRECT.T, section.R + kMainAreaSpacerWidth, mRECT.B);
    section = IRECT(section.R, mRECT.T, section.R + kCabImageWidth, mRECT.B);
    DrawSelector(g, section, GetLabel(SelectorKind::Cab).c_str());
    section = IRECT(section.R, mRECT.T, section.R + kOutputMeterWidth, mRECT.B);
    g.FillRect(kPDBackground, section);

    const auto borderBounds = IRECT(mRECT.L, mRECT.B - kSelectorAreaBorderSize, mRECT.R, mRECT.B);
    g.FillRect(kPDLightGrey, borderBounds);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    switch (msgTag)
    {
      case kMsgTagClearModel: ClearSelection(SelectorKind::Amp); break;
      case kMsgTagClearIR: ClearSelection(SelectorKind::Cab); break;
      case kMsgTagLoadedModel:
        if (pData != nullptr)
          SetSelectedFile(SelectorKind::Amp, reinterpret_cast<const char*>(pData));
        break;
      case kMsgTagLoadedIR:
        if (pData != nullptr)
          SetSelectedFile(SelectorKind::Cab, reinterpret_cast<const char*>(pData));
        break;
      default: break;
    }
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    SelectorKind kind;
    SelectorGeometry geometry;
    if (!GetGeometryAtPoint(x, y, kind, geometry))
      return;

    if (geometry.leftArrow.Contains(x, y))
    {
      Cycle(kind, -1);
      return;
    }

    if (geometry.rightArrow.Contains(x, y))
    {
      Cycle(kind, 1);
      return;
    }

    if (geometry.label.Contains(x, y))
    {
      ShowMenu(kind, geometry.label);
      return;
    }
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu == nullptr)
      return;

    const auto* item = pSelectedMenu->GetChosenItem();
    if (item == nullptr)
      return;

    SelectIndex(mPopupKind, item->GetTag());
  }

private:
  enum class SelectorKind
  {
    Amp,
    Cab
  };

  struct SelectorState
  {
    std::string extension;
    std::string emptyLabel;
    std::vector<std::string> files;
    int selectedIndex = -1;
    std::string selectedPath;
    std::vector<std::string> pendingFiles;
  };

  struct SelectorGeometry
  {
    IRECT section;
    IRECT leftArrow;
    IRECT label;
    IRECT rightArrow;
  };

public:
  void SetPendingAmpFiles(std::vector<std::string> files) { SetPendingFiles(SelectorKind::Amp, std::move(files)); }
  void SetPendingCabFiles(std::vector<std::string> files) { SetPendingFiles(SelectorKind::Cab, std::move(files)); }

private:

  static std::string EllipsizeToFit(IGraphics& g, const IText& textStyle, const std::string& text, float maxWidth)
  {
    if (text.empty() || maxWidth <= 0.f)
      return "";

    IRECT measured;
    g.MeasureText(textStyle, text.c_str(), measured);
    if (measured.W() <= maxWidth)
      return text;

    const std::string suffix = "...";
    g.MeasureText(textStyle, suffix.c_str(), measured);
    if (measured.W() > maxWidth)
      return "";

    size_t low = 0;
    size_t high = text.size();
    while (low < high)
    {
      const size_t mid = (low + high + 1) / 2;
      const std::string candidate = text.substr(0, mid) + suffix;
      g.MeasureText(textStyle, candidate.c_str(), measured);

      if (measured.W() <= maxWidth)
        low = mid;
      else
        high = mid - 1;
    }

    return text.substr(0, low) + suffix;
  }

  void DrawSelector(IGraphics& g, const IRECT& bounds, const char* label)
  {
    const IText selectorText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);
    const std::string actualLabel = label && label[0] != '\0' ? label : "Select...";
    const float maxTextWidth = std::max(0.f, bounds.W() - (2.f * kSelectorArrowSize) - (2.f * kSelectorGap));
    const std::string displayLabel = EllipsizeToFit(g, selectorText, actualLabel, maxTextWidth);

    IRECT measured;
    g.MeasureText(selectorText, displayLabel.c_str(), measured);
    const float textWidth = std::min(measured.W(), maxTextWidth);
    const float totalWidth = (2.f * kSelectorArrowSize) + (2.f * kSelectorGap) + textWidth;
    float x = bounds.MW() - (totalWidth / 2.f);
    const float rowHeight = std::max(kSelectorArrowSize, measured.H());
    const float rowTop = bounds.T;
    const float rowBottom = rowTop + rowHeight;
    const float arrowTop = rowTop + ((rowHeight - kSelectorArrowSize) / 2.f);

    const auto leftArrowBounds = IRECT(x, arrowTop, x + kSelectorArrowSize, arrowTop + kSelectorArrowSize);
    g.DrawSVG(mLeftArrow, leftArrowBounds, &mBlend);
    x = leftArrowBounds.R + kSelectorGap;

    const auto labelBounds = IRECT(x, rowTop, x + textWidth, rowBottom);
    g.DrawText(selectorText, displayLabel.c_str(), labelBounds, &mBlend);
    x = labelBounds.R + kSelectorGap;

    const auto rightArrowBounds = IRECT(x, arrowTop, x + kSelectorArrowSize, arrowTop + kSelectorArrowSize);
    g.DrawSVG(mRightArrow, rightArrowBounds, &mBlend);
  }

  SelectorState& State(SelectorKind kind) { return kind == SelectorKind::Amp ? mAmpState : mCabState; }
  const SelectorState& State(SelectorKind kind) const { return kind == SelectorKind::Amp ? mAmpState : mCabState; }

  std::string GetLabel(SelectorKind kind) const
  {
    const auto& state = State(kind);
    if (state.selectedIndex >= 0 && state.selectedIndex < static_cast<int>(state.files.size()))
      return FileStem(std::filesystem::u8path(state.files[static_cast<size_t>(state.selectedIndex)]));

    return state.emptyLabel;
  }

  bool GetGeometryAtPoint(float x, float y, SelectorKind& kind, SelectorGeometry& geometry)
  {
    const auto amp = GetAmpSectionBounds();
    if (amp.Contains(x, y))
    {
      kind = SelectorKind::Amp;
      geometry = GetSelectorGeometry(amp, GetLabel(kind));
      return true;
    }

    const auto cab = GetCabSectionBounds();
    if (cab.Contains(x, y))
    {
      kind = SelectorKind::Cab;
      geometry = GetSelectorGeometry(cab, GetLabel(kind));
      return true;
    }

    return false;
  }

  IRECT GetAmpSectionBounds() const
  {
    const auto input = mRECT.GetFromLeft(kInputMeterWidth);
    return IRECT(input.R, mRECT.T, input.R + kAmpImageWidth, mRECT.B);
  }

  IRECT GetCabSectionBounds() const
  {
    const auto amp = GetAmpSectionBounds();
    const auto spacer = IRECT(amp.R, mRECT.T, amp.R + kMainAreaSpacerWidth, mRECT.B);
    return IRECT(spacer.R, mRECT.T, spacer.R + kCabImageWidth, mRECT.B);
  }

  SelectorGeometry GetSelectorGeometry(const IRECT& bounds, const std::string& label)
  {
    const IText selectorText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);
    const float maxTextWidth = std::max(0.f, bounds.W() - (2.f * kSelectorArrowSize) - (2.f * kSelectorGap));
    const std::string displayLabel = EllipsizeToFit(*GetUI(), selectorText, label, maxTextWidth);

    IRECT measured;
    GetUI()->MeasureText(selectorText, displayLabel.c_str(), measured);
    const float textWidth = std::min(measured.W(), maxTextWidth);
    const float totalWidth = (2.f * kSelectorArrowSize) + (2.f * kSelectorGap) + textWidth;
    float x = bounds.MW() - (totalWidth / 2.f);
    const float rowHeight = std::max(kSelectorArrowSize, measured.H());
    const float rowTop = bounds.T;
    const float arrowTop = rowTop + ((rowHeight - kSelectorArrowSize) / 2.f);

    SelectorGeometry geometry;
    geometry.section = bounds;
    geometry.leftArrow = IRECT(x, arrowTop, x + kSelectorArrowSize, arrowTop + kSelectorArrowSize);
    x = geometry.leftArrow.R + kSelectorGap;
    geometry.label = IRECT(x, rowTop, x + textWidth, rowTop + rowHeight);
    x = geometry.label.R + kSelectorGap;
    geometry.rightArrow = IRECT(x, arrowTop, x + kSelectorArrowSize, arrowTop + kSelectorArrowSize);
    return geometry;
  }

  void SetSelectedFile(SelectorKind kind, const char* filePath)
  {
    auto& state = State(kind);
    state.selectedPath = NormalizePath(filePath);
    state.files.clear();
    state.selectedIndex = -1;

    try
    {
      const auto path = std::filesystem::u8path(state.selectedPath);
      const auto directory = path.parent_path();
      const auto normalized = PathString(path);

      if (!state.pendingFiles.empty() && ContainsFile(state.pendingFiles, normalized))
      {
        state.files = std::move(state.pendingFiles);
      }
      else
      {
        state.pendingFiles.clear();
        state.files = FindFiles(directory, state.extension);
      }

      for (size_t i = 0; i < state.files.size(); ++i)
      {
        if (state.files[i] == normalized)
        {
          state.selectedIndex = static_cast<int>(i);
          break;
        }
      }
    }
    catch (...)
    {
      state.files.clear();
      state.selectedIndex = -1;
    }

    SetDirty(false);
  }

  void ClearSelection(SelectorKind kind)
  {
    auto& state = State(kind);
    state.files.clear();
    state.selectedIndex = -1;
    state.selectedPath.clear();
    state.pendingFiles.clear();
    SetDirty(false);
  }

  void SetPendingFiles(SelectorKind kind, std::vector<std::string> files)
  {
    NormalizeAndSortFiles(files);
    State(kind).pendingFiles = std::move(files);
  }

  static void NormalizeAndSortFiles(std::vector<std::string>& files)
  {
    for (auto& file : files)
      file = NormalizePath(file.c_str());

    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return ToLower(a) < ToLower(b); });
    files.erase(std::unique(files.begin(), files.end()), files.end());
  }

  static bool ContainsFile(const std::vector<std::string>& files, const std::string& file)
  {
    return std::find(files.begin(), files.end(), file) != files.end();
  }

  void Cycle(SelectorKind kind, int direction)
  {
    auto& state = State(kind);
    if (state.files.empty())
    {
      OpenSelectorScreen(kind);
      return;
    }

    const int nFiles = static_cast<int>(state.files.size());
    state.selectedIndex = (state.selectedIndex + direction + nFiles) % nFiles;
    LoadSelected(kind);
  }

  void ShowMenu(SelectorKind kind, const IRECT& menuBounds)
  {
    const auto& state = State(kind);
    if (state.files.empty())
    {
      OpenSelectorScreen(kind);
      return;
    }

    mPopupKind = kind;
    mPopupMenu.Clear();
    for (size_t i = 0; i < state.files.size(); ++i)
      mPopupMenu.AddItem(new IPopupMenu::Item(FileStem(std::filesystem::u8path(state.files[i])).c_str(),
                                              IPopupMenu::Item::kNoFlags, static_cast<int>(i)));
    mPopupMenu.SetChosenItemIdx(state.selectedIndex);
    GetUI()->CreatePopupMenu(*this, mPopupMenu, menuBounds);
  }

  void SelectIndex(SelectorKind kind, int index)
  {
    auto& state = State(kind);
    if (index < 0 || index >= static_cast<int>(state.files.size()))
      return;

    state.selectedIndex = index;
    LoadSelected(kind);
  }

  void LoadSelected(SelectorKind kind)
  {
    auto& state = State(kind);
    if (state.selectedIndex < 0 || state.selectedIndex >= static_cast<int>(state.files.size()))
      return;

    WDL_String path(state.files[static_cast<size_t>(state.selectedIndex)].c_str());
    if (kind == SelectorKind::Amp)
      PLUG()->LoadNAMFile(path);
    else
      PLUG()->LoadIRFile(path);
  }

  void OpenSelectorScreen(SelectorKind kind)
  {
    if (auto* ui = GetUI())
    {
      if (auto* screen = ui->GetControlWithTag(kind == SelectorKind::Amp ? kCtrlTagAmpSelectorScreen
                                                                         : kCtrlTagCabSelectorScreen))
        screen->Hide(false);
      ui->SetAllControlsDirty();
    }
  }

  static std::vector<std::string> FindFiles(const std::filesystem::path& directory, const std::string& extension)
  {
    std::vector<std::string> files;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, ec),
         end;
         !ec && it != end; it.increment(ec))
    {
      std::error_code entryError;
      if (it->is_regular_file(entryError) && HasExtension(it->path(), extension))
        files.push_back(PathString(it->path()));
    }

    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return ToLower(a) < ToLower(b); });
    return files;
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

  static std::string PathString(const std::filesystem::path& path)
  {
    return path.lexically_normal().string();
  }

  static std::string FileName(const std::filesystem::path& path)
  {
    const std::string name = path.filename().string();
    return name.empty() ? path.string() : name;
  }

  static std::string FileStem(const std::filesystem::path& path)
  {
    const std::string stem = path.stem().string();
    return stem.empty() ? FileName(path) : stem;
  }

  static std::string ToLower(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
  }

  static bool HasExtension(const std::filesystem::path& path, const std::string& extension)
  {
    return ToLower(path.extension().string()) == "." + extension;
  }

  ISVG mLeftArrow;
  ISVG mRightArrow;
  SelectorState mAmpState {"nam", "Select amp...", {}, -1, "", {}};
  SelectorState mCabState {"wav", "Select cab...", {}, -1, "", {}};
  SelectorKind mPopupKind = SelectorKind::Amp;
  IPopupMenu mPopupMenu {"Files"};
};
