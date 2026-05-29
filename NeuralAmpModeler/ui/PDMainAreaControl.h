#pragma once

// Included from NeuralAmpModeler.cpp inside its anonymous namespace.
// Keep these UI building blocks header-only until the Xcode project is split into compiled sources.

class PDHeaderLoadingBarControl : public IControl
{
public:
  PDHeaderLoadingBarControl(const IRECT& bounds)
  : IControl(bounds)
  {
    SetIgnoreMouse(true);
  }

  void Draw(IGraphics& g) override
  {
    if (!mLoading)
      return;

    const float fillWidth = mRECT.W() * mProgress;
    if (fillWidth <= 0.f)
      return;

    g.FillRect(kPDForeground, IRECT(mRECT.L, mRECT.T, mRECT.L + fillWidth, mRECT.B));
  }

  void SetLoading(bool loading)
  {
    if (loading && !mLoading)
    {
      mStartedAt = std::chrono::steady_clock::now();
      mLastPaintAt = mStartedAt;
      mProgress = 0.02f;
      mLastPaintWidth = -1.f;
    }

    if (!loading)
    {
      mProgress = 0.f;
      mLastPaintWidth = -1.f;
    }

    if (loading != mLoading)
    {
      mLoading = loading;
      SetDirty(false);
    }
  }

  void Tick()
  {
    if (!mLoading)
      return;

    const auto now = std::chrono::steady_clock::now();
    const auto frameElapsed = std::chrono::duration<float, std::milli>(now - mLastPaintAt).count();
    if (frameElapsed < kHeaderLoadingBarFrameTimeMs)
      return;

    const auto elapsed = std::chrono::duration<float, std::milli>(now - mStartedAt).count();
    const float progress = std::min(0.95f, elapsed / kHeaderLoadingBarFillTimeMs);
    const float paintWidth = std::floor(mRECT.W() * progress);

    if (paintWidth != mLastPaintWidth)
    {
      mProgress = progress;
      mLastPaintWidth = paintWidth;
      mLastPaintAt = now;
      SetDirty(false);
    }
  }

private:
  bool mLoading = false;
  float mProgress = 0.f;
  float mLastPaintWidth = -1.f;
  std::chrono::steady_clock::time_point mStartedAt = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point mLastPaintAt = std::chrono::steady_clock::now();
};

class PDMeterControl : public IVPeakAvgMeterControl<>
{
public:
  static constexpr float kMeterMinDB = -70.f;
  static constexpr float kMeterMaxDB = -0.01f;

  PDMeterControl(const IRECT& bounds)
  : IVPeakAvgMeterControl<>(bounds, "", DEFAULT_STYLE.WithShowValue(false).WithDrawFrame(false).WithWidgetFrac(1.f),
                            EDirection::Vertical, {}, 0, kMeterMinDB, kMeterMaxDB, {})
  {
    SetPeakSize(0.f);
  }

  void Draw(IGraphics& g) override
  {
    const float value = static_cast<float>(std::clamp(GetValue(0), 0., 1.));
    g.FillRect(kPDLightGrey, mRECT, &mBlend);

    if (value <= 0.f)
      return;

    const float fillTop = mRECT.B - (mRECT.H() * value);
    g.FillRect(kPDForeground, IRECT(mRECT.L, fillTop, mRECT.R, mRECT.B), &mBlend);
  }

  void DrawPeak(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override {}
};

class MainAreaControl : public IControl
{
public:
  MainAreaControl(const IRECT& bounds, const IBitmap& ampImage, const IBitmap& cabImage, const IBitmap& noAmpImage,
                  const IBitmap& noCabImage)
  : IControl(bounds)
  , mAmpImage(ampImage)
  , mCabImage(cabImage)
  , mNoAmpImage(noAmpImage)
  , mNoCabImage(noCabImage)
  {
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(kPDBackground, mRECT);

    if (ShouldShowLibrarySetupNotice())
    {
      DrawLibrarySetupNotice(g);
      return;
    }

    IRECT section = mRECT.GetFromLeft(kInputMeterWidth);
    section = IRECT(section.R, mRECT.T, section.R + kAmpImageWidth, mRECT.B);
    DrawWidthFittedClippedBitmap(g, mAmpImage, section, section.Contains(mMouseX, mMouseY) ? 0.8f : 1.f);

    section = IRECT(section.R, mRECT.T, section.R + kMainAreaSpacerWidth, mRECT.B);
    section = IRECT(section.R, mRECT.T, section.R + kCabImageWidth, mRECT.B);
    DrawWidthFittedClippedBitmap(g, mCabImage, section, section.Contains(mMouseX, mMouseY) ? 0.8f : 1.f);
    section = IRECT(section.R, mRECT.T, section.R + kOutputMeterWidth, mRECT.B);
    g.FillRect(kPDBackground, section);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (ShouldShowLibrarySetupNotice())
    {
      if (GetLibraryNoticeButtonBounds().Contains(x, y))
        OpenSettingsScreen();
      return;
    }

    if (auto* ui = GetUI())
    {
      if (GetAmpImageBounds().Contains(x, y))
      {
        if (auto* ampSelector = ui->GetControlWithTag(kCtrlTagAmpSelectorScreen))
          ampSelector->Hide(false);
      }
      else if (GetCabImageBounds().Contains(x, y))
      {
        if (auto* cabSelector = ui->GetControlWithTag(kCtrlTagCabSelectorScreen))
          cabSelector->Hide(false);
      }
      else if (auto* ampSelector = ui->GetControlWithTag(kCtrlTagAmpSelectorScreen))
      {
        ampSelector->Hide(false);
      }

      ui->SetAllControlsDirty();
    }
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    switch (msgTag)
    {
      case kMsgTagClearModel: ClearSelectedImage(SelectionKind::Amp); break;
      case kMsgTagClearIR: ClearSelectedImage(SelectionKind::Cab); break;
      case kMsgTagLoadedModel:
        if (pData != nullptr)
          SetSelectedImage(reinterpret_cast<const char*>(pData), SelectionKind::Amp);
        break;
      case kMsgTagLoadedIR:
        if (pData != nullptr)
          SetSelectedImage(reinterpret_cast<const char*>(pData), SelectionKind::Cab);
        break;
      default: break;
    }
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
  enum class SelectionKind
  {
    Amp,
    Cab
  };

  IRECT GetAmpImageBounds() const
  {
    const auto input = mRECT.GetFromLeft(kInputMeterWidth);
    return IRECT(input.R, mRECT.T, input.R + kAmpImageWidth, mRECT.B);
  }

  IRECT GetCabImageBounds() const
  {
    const auto amp = GetAmpImageBounds();
    const auto spacer = IRECT(amp.R, mRECT.T, amp.R + kMainAreaSpacerWidth, mRECT.B);
    return IRECT(spacer.R, mRECT.T, spacer.R + kCabImageWidth, mRECT.B);
  }

  IRECT GetLibraryContentBounds() const
  {
    const auto amp = GetAmpImageBounds();
    const auto cab = GetCabImageBounds();
    return IRECT(amp.L, mRECT.T, cab.R, mRECT.B);
  }

  IRECT GetLibraryNoticeTextBounds() const
  {
    const auto content = GetLibraryContentBounds();
    const float totalHeight = kMainLibraryNoticeTextHeight + kMainLibraryNoticeButtonGap + kSettingsButtonHeight;
    const float top = content.MH() - (totalHeight / 2.f);
    return IRECT(content.MW() - (kMainLibraryNoticeWidth / 2.f), top,
                 content.MW() + (kMainLibraryNoticeWidth / 2.f), top + kMainLibraryNoticeTextHeight);
  }

  IRECT GetLibraryNoticeButtonBounds() const
  {
    const auto textBounds = GetLibraryNoticeTextBounds();
    const float top = textBounds.B + kMainLibraryNoticeButtonGap;
    return IRECT(textBounds.MW() - (kMainLibraryNoticeButtonWidth / 2.f), top,
                 textBounds.MW() + (kMainLibraryNoticeButtonWidth / 2.f), top + kSettingsButtonHeight);
  }

  bool ShouldShowLibrarySetupNotice()
  {
    return PLUG()->GetNAMRootDirectory().GetLength() == 0 && PLUG()->GetIRRootDirectory().GetLength() == 0;
  }

  void DrawLibrarySetupNotice(IGraphics& g)
  {
    const auto content = GetLibraryContentBounds();
    const auto textBounds = GetLibraryNoticeTextBounds();
    const auto buttonBounds = GetLibraryNoticeButtonBounds();
    const IText text(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);
    const float lineHeight = kSettingsInputLineHeight;
    const float textTop = textBounds.MH() - lineHeight;

    g.FillRect(kPDBackground, content);
    g.DrawText(text, "Set up your amp and/or cab folders",
               IRECT(textBounds.L, textTop, textBounds.R, textTop + lineHeight), &mBlend);
    g.DrawText(text, "in settings to get started.",
               IRECT(textBounds.L, textTop + lineHeight, textBounds.R, textTop + (2.f * lineHeight)), &mBlend);
    DrawNoticeButton(g, buttonBounds, "Settings");
  }

  void DrawNoticeButton(IGraphics& g, const IRECT& bounds, const char* label)
  {
    const IText text(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);

    g.DrawRoundRect(kPDForeground, bounds, kSettingsButtonRadius, &mBlend, kSettingsButtonBorderSize);
    g.DrawText(text, label, bounds, &mBlend);
  }

  void OpenSettingsScreen()
  {
    if (auto* ui = GetUI())
    {
      if (auto* settings = ui->GetControlWithTag(kCtrlTagSettingsBox))
        settings->Hide(false);

      ui->SetAllControlsDirty();
    }
  }

  static void DrawWidthFittedClippedBitmap(IGraphics& g, const IBitmap& bitmap, const IRECT& bounds, float opacity)
  {
    if (!bitmap.IsValid() || bitmap.W() <= 0 || bitmap.H() <= 0)
      return;

    const float scale = bounds.W() / static_cast<float>(bitmap.W());
    const float drawWidth = bitmap.W() * scale;
    const float drawHeight = bitmap.H() * scale;
    const auto imageBounds = bounds.GetCentredInside(drawWidth, drawHeight);
    const IBlend blend(EBlend::Default, opacity);

    g.PathClipRegion(bounds);
    g.DrawFittedBitmap(bitmap, imageBounds, &blend);
    g.PathClipRegion(IRECT());
  }

  void SetSelectedImage(const char* filePath, SelectionKind kind)
  {
    const IBitmap fallback = kind == SelectionKind::Amp ? mNoAmpImage : mNoCabImage;
    IBitmap selected = fallback;

    try
    {
      const auto path = std::filesystem::u8path(filePath);
      const auto directory = GetArtworkSearchDirectory(path, kind);
      std::filesystem::path imagePath;
      if (FindFirstImage(directory, imagePath))
        selected = LoadImage(imagePath, fallback);
    }
    catch (...)
    {
      selected = fallback;
    }

    ReleasePreviousDisplayedImage(kind, selected);

    if (kind == SelectionKind::Amp)
      mAmpImage = selected;
    else
      mCabImage = selected;

    SetDirty(false);
  }

  void ClearSelectedImage(SelectionKind kind)
  {
    const IBitmap fallback = kind == SelectionKind::Amp ? mNoAmpImage : mNoCabImage;
    ReleasePreviousDisplayedImage(kind, fallback);

    if (kind == SelectionKind::Amp)
      mAmpImage = fallback;
    else
      mCabImage = fallback;

    SetDirty(false);
  }

  std::filesystem::path GetArtworkSearchDirectory(const std::filesystem::path& selectedFile, SelectionKind kind)
  {
    if (kind == SelectionKind::Cab)
    {
      const WDL_String& irRoot = PLUG()->GetIRRootDirectory();
      if (irRoot.GetLength())
      {
        const auto root = std::filesystem::u8path(irRoot.Get());
        std::vector<std::filesystem::directory_entry> entries;
        std::error_code ec;

        for (std::filesystem::directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied,
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
          if (entry.is_directory(entryError) && PathContains(entry.path(), selectedFile))
            return entry.path();
        }
      }
    }

    return selectedFile.parent_path();
  }

  static bool PathContains(const std::filesystem::path& parent, const std::filesystem::path& child)
  {
    std::error_code ec;
    const auto relative = std::filesystem::relative(child, parent, ec);
    if (ec || relative.empty())
      return false;

    for (const auto& part : relative)
    {
      if (part == "..")
        return false;
    }

    return true;
  }

  IBitmap LoadImage(const std::filesystem::path& path, const IBitmap& fallback)
  {
    IBitmap bitmap;

    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec))
      return fallback;

#ifdef OS_MAC
    std::vector<uint8_t> data;
    if (EncodeImageAsPNG(path, data))
    {
      const std::string cacheName = PathString(path) + ".main.decoded.png";
      bitmap = GetUI()->LoadBitmap(cacheName.c_str(), data.data(), static_cast<int>(data.size()), 1, false, 1);
    }
#else
    if (CanLoadThumbnailFile(path))
      bitmap = GetUI()->LoadBitmap(PathString(path).c_str());
#endif

    if (bitmap.IsValid() && bitmap.W() > 0 && bitmap.H() > 0)
      return bitmap;

    return fallback;
  }

  void ReleasePreviousDisplayedImage(SelectionKind kind, const IBitmap& replacement)
  {
    const IBitmap& fallback = kind == SelectionKind::Amp ? mNoAmpImage : mNoCabImage;
    const IBitmap& current = kind == SelectionKind::Amp ? mAmpImage : mCabImage;
    if (!current.IsValid() || current.GetAPIBitmap() == fallback.GetAPIBitmap()
        || current.GetAPIBitmap() == replacement.GetAPIBitmap())
      return;

    if (auto* ui = GetUI())
      ui->ReleaseBitmap(current);
  }

  static bool FindFirstImage(const std::filesystem::path& directory, std::filesystem::path& result)
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
      if (entry.is_regular_file(entryError) && IsImageFile(entry.path()))
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
      if (entry.is_directory(entryError) && FindFirstImage(entry.path(), result))
        return true;
    }

    return false;
  }

  static std::string ToLower(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
  }

  static bool IsImageFile(const std::filesystem::path& path)
  {
    const std::string ext = ToLower(path.extension().string());
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp";
  }

  static bool CanLoadThumbnailFile(const std::filesystem::path& path)
  {
    const std::string ext = ToLower(path.extension().string());
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
  }

#ifdef OS_MAC
  static bool EncodeImageAsPNG(const std::filesystem::path& path, std::vector<uint8_t>& data)
  {
    data.clear();

    const std::string pathString = path.string();
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

    CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
    CFRelease(source);

    if (image == nullptr)
      return false;

    if (CGImageGetWidth(image) == 0 || CGImageGetHeight(image) == 0)
    {
      CGImageRelease(image);
      return false;
    }

    CFMutableDataRef pngData = CFDataCreateMutable(kCFAllocatorDefault, 0);
    if (pngData == nullptr)
    {
      CGImageRelease(image);
      return false;
    }

    CGImageDestinationRef destination = CGImageDestinationCreateWithData(pngData, CFSTR("public.png"), 1, nullptr);
    if (destination == nullptr)
    {
      CFRelease(pngData);
      CGImageRelease(image);
      return false;
    }

    CGImageDestinationAddImage(destination, image, nullptr);
    const bool ok = CGImageDestinationFinalize(destination);
    CFRelease(destination);
    CGImageRelease(image);

    if (ok)
    {
      const CFIndex size = CFDataGetLength(pngData);
      if (size > 0 && size <= std::numeric_limits<int>::max())
      {
        const UInt8* bytes = CFDataGetBytePtr(pngData);
        data.assign(bytes, bytes + size);
      }
    }

    CFRelease(pngData);
    return !data.empty();
  }
#endif

  static std::string PathString(const std::filesystem::path& path)
  {
    return path.lexically_normal().string();
  }

  IBitmap mAmpImage;
  IBitmap mCabImage;
  IBitmap mNoAmpImage;
  IBitmap mNoCabImage;
  float mMouseX = -1.f;
  float mMouseY = -1.f;
};
