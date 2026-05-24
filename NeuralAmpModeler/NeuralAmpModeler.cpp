#include <algorithm> // std::clamp, std::min
#include <chrono>
#include <cmath> // pow
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <utility>

#include "Colors.h"
#include "../NeuralAmpModelerCore/NAM/activations.h"
#include "../NeuralAmpModelerCore/NAM/get_dsp.h"
// clang-format off
// These includes need to happen in this order or else the latter won't know
// a bunch of stuff.
#include "NeuralAmpModeler.h"
#include "IPlug_include_in_plug_src.h"
// clang-format on
#include "architecture.hpp"

#include "NeuralAmpModelerControls.h"

using namespace iplug;
using namespace igraphics;

const double kDCBlockerFrequency = 5.0;

// Styles
const IVColorSpec colorSpec{
  DEFAULT_BGCOLOR, // Background
  PluginColors::NAM_THEMECOLOR, // Foreground
  PluginColors::NAM_THEMECOLOR.WithOpacity(0.3f), // Pressed
  PluginColors::NAM_THEMECOLOR.WithOpacity(0.4f), // Frame
  PluginColors::MOUSEOVER, // Highlight
  DEFAULT_SHCOLOR, // Shadow
  PluginColors::NAM_THEMECOLOR, // Extra 1
  COLOR_RED, // Extra 2 --> color for clipping in meters
  PluginColors::NAM_THEMECOLOR.WithContrast(0.1f), // Extra 3
};

const IVStyle style =
  IVStyle{true, // Show label
          true, // Show value
          colorSpec,
          {DEFAULT_TEXT_SIZE + 3.f, EVAlign::Middle, PluginColors::NAM_THEMEFONTCOLOR}, // Knob label text5
          {DEFAULT_TEXT_SIZE + 3.f, EVAlign::Bottom, PluginColors::NAM_THEMEFONTCOLOR}, // Knob value text
          DEFAULT_HIDE_CURSOR,
          DEFAULT_DRAW_FRAME,
          false,
          DEFAULT_EMBOSS,
          0.2f,
          2.f,
          DEFAULT_SHADOW_OFFSET,
          DEFAULT_WIDGET_FRAC,
          DEFAULT_WIDGET_ANGLE};
const IVStyle titleStyle =
  DEFAULT_STYLE.WithValueText(IText(30, COLOR_BLACK, "Michroma-Regular")).WithDrawFrame(false).WithShadowOffset(2.f);
const IVStyle radioButtonStyle =
  style
    .WithColor(EVColor::kON, PluginColors::NAM_THEMECOLOR) // Pressed buttons and their labels
    .WithColor(EVColor::kOFF, PluginColors::NAM_THEMECOLOR.WithOpacity(0.1f)) // Unpressed buttons
    .WithColor(EVColor::kX1, PluginColors::NAM_THEMECOLOR.WithOpacity(0.6f)); // Unpressed buttons' labels

EMsgBoxResult _ShowMessageBox(iplug::igraphics::IGraphics* pGraphics, const char* str, const char* caption,
                              EMsgBoxType type)
{
#ifdef OS_MAC
  // macOS is backwards?
  return pGraphics->ShowMessageBox(caption, str, type);
#else
  return pGraphics->ShowMessageBox(str, caption, type);
#endif
}

const std::string kCalibrateInputParamName = "CalibrateInput";
const bool kDefaultCalibrateInput = false;
const std::string kInputCalibrationLevelParamName = "InputCalibrationLevel";
const double kDefaultInputCalibrationLevel = 12.0;

namespace
{
constexpr const char* kLibrarySettingsFileName = "library-settings.json";
constexpr const char* kNAMRootDirectoryKey = "NAMRootDirectory";
constexpr const char* kIRRootDirectoryKey = "IRRootDirectory";
constexpr const char* kNAMRootDirectoryBookmarkKey = "NAMRootDirectoryBookmark";
constexpr const char* kIRRootDirectoryBookmarkKey = "IRRootDirectoryBookmark";
constexpr const char* kPDFontMedium = "PD-Medium";
constexpr const char* kPDFontBold = "PD-Bold";
constexpr float kMainHeaderHeight = 40.f;
constexpr float kMainHeaderBorderSize = 2.f;
constexpr float kMainHeaderPadding = 10.f;
constexpr float kMainHeaderLogoWidth = 80.f;
constexpr float kMainHeaderLogoHeight = 20.f;
constexpr float kMainHeaderSlotSize = 20.f;
constexpr float kHeaderLoadingBarHeight = 6.f;
constexpr float kHeaderLoadingBarFillTimeMs = 1200.f;
constexpr float kHeaderLoadingBarFrameTimeMs = 33.f;
constexpr float kMainAreaTop = 42.f;
constexpr float kMainAreaHeight = 357.f;
constexpr float kInputMeterWidth = 64.f;
constexpr float kAmpImageWidth = 440.f;
constexpr float kMainAreaSpacerWidth = 32.f;
constexpr float kCabImageWidth = 220.f;
constexpr float kOutputMeterWidth = 64.f;
constexpr float kSelectorAreaTop = kMainAreaTop + kMainAreaHeight;
constexpr float kSelectorAreaHeight = 48.f;
constexpr float kSelectorAreaBorderSize = 2.f;
constexpr float kSelectorArrowSize = 16.f;
constexpr float kSelectorGap = 6.f;
constexpr float kSelectorTextSize = 14.f;
constexpr float kControlAreaContainerTop = kSelectorAreaTop + kSelectorAreaHeight;
constexpr float kControlAreaContainerHeight = 115.f;
constexpr float kControlAreaWidth = 338.f;
constexpr float kControlAreaHeight = 87.f;
constexpr float kMainKnobContainerWidth = 36.f;
constexpr float kMainKnobContainerHeight = 71.f;
constexpr float kMainKnobGap = 18.f;
constexpr float kMainKnobDiameter = 28.f;
constexpr float kMainKnobStrokeWidth = 2.f;
constexpr float kMainKnobDotDiameter = 4.f;
constexpr float kMainKnobLabelHeight = 16.f;
constexpr float kMainKnobValueHeight = 16.f;
constexpr float kEQContainerPaddingX = 16.f;
constexpr float kEQContainerPaddingY = 8.f;
constexpr float kEQContainerBorderRadius = 9.f;
constexpr float kEQContainerBorderSize = 2.f;
constexpr float kEQPowerButtonSize = 12.f;
constexpr float kEQPowerButtonInset = 3.f;
constexpr float kEQContainerWidth = (3.f * kMainKnobContainerWidth) + (2.f * kMainKnobGap) + (2.f * kEQContainerPaddingX);
constexpr float kEQContainerHeight = kMainKnobContainerHeight + (2.f * kEQContainerPaddingY);
constexpr float kSettingsPathContainerWidth = 480.f;
constexpr float kSettingsPathContainerPaddingY = 32.f;
constexpr float kSettingsPathSetterWidth = 224.f;
constexpr float kSettingsPathSetterPadding = 12.f;
constexpr float kSettingsPathSetterRadius = 9.f;
constexpr float kSettingsPathSetterBorderSize = 2.f;
constexpr float kSettingsPathSetterGap = 32.f;
constexpr float kSettingsPathTitleHeight = 16.f;
constexpr float kSettingsPathValueHeight = 16.f;
constexpr float kSettingsPathTitleValueGap = 4.f;
constexpr float kSettingsPathTitleButtonGap = 10.f;
constexpr float kSettingsButtonPaddingX = 12.f;
constexpr float kSettingsButtonPaddingY = 8.f;
constexpr float kSettingsButtonRadius = 4.f;
constexpr float kSettingsButtonBorderSize = 2.f;
constexpr float kSettingsButtonHeight = kSelectorTextSize + (2.f * kSettingsButtonPaddingY);
constexpr float kSettingsButtonLabelWidth = 104.f;
constexpr float kSettingsButtonWidth = kSettingsButtonLabelWidth + (2.f * kSettingsButtonPaddingX);
constexpr float kSettingsPathSetterHeight = (2.f * kSettingsPathSetterPadding) + kSettingsPathTitleHeight +
                                            kSettingsPathTitleValueGap + kSettingsPathValueHeight +
                                            kSettingsPathTitleButtonGap + kSettingsButtonHeight;
constexpr float kSettingsPathContainerTop = kMainHeaderHeight;
constexpr float kSettingsPathContainerHeight = (2.f * kSettingsPathContainerPaddingY) + kSettingsPathSetterHeight +
                                               kMainHeaderBorderSize;
constexpr int kAmpSelectorColumns = 4;
constexpr float kAmpSelectorContentTop = kMainHeaderHeight + kMainHeaderBorderSize;
constexpr float kAmpSelectorCardWidth = 205.f;
constexpr float kAmpSelectorCardPadding = 12.f;
constexpr float kAmpSelectorImageSize = 181.f;
constexpr float kAmpSelectorTitleMinHeight = 18.f;
constexpr float kAmpSelectorTitlePaddingY = 3.f;
constexpr float kAmpSelectorCardHeight = (2.f * kAmpSelectorCardPadding) + kAmpSelectorImageSize;
constexpr float kAmpSelectorWheelStep = 24.f;
constexpr float kSubfolderPromptCardWidth = 320.f;
constexpr float kSubfolderPromptCardPadding = 32.f;
constexpr float kSubfolderPromptCardRadius = 5.f;
constexpr float kSubfolderPromptTextHeight = 92.f;
constexpr float kSubfolderPromptTextButtonGap = 16.f;
constexpr float kSubfolderPromptButtonGap = 10.f;
constexpr float kSubfolderPromptSelectButtonWidth = 136.f;
constexpr float kSubfolderPromptLoadAllButtonWidth = 86.f;
constexpr float kSubfolderPromptCardHeight = (2.f * kSubfolderPromptCardPadding) + kSubfolderPromptTextHeight +
                                             kSubfolderPromptTextButtonGap + kSettingsButtonHeight;

const IColor kPDLightGrey(255, 240, 240, 240);
const IColor kPDForeground(255, 35, 31, 32);
const IColor kPDBackground(255, 255, 255, 255);

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
    if (pData == nullptr)
      return;

    switch (msgTag)
    {
      case kMsgTagLoadedModel: SetSelectedImage(reinterpret_cast<const char*>(pData), SelectionKind::Amp); break;
      case kMsgTagLoadedIR: SetSelectedImage(reinterpret_cast<const char*>(pData), SelectionKind::Cab); break;
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
    if (pData == nullptr)
      return;

    switch (msgTag)
    {
      case kMsgTagLoadedModel: SetSelectedFile(SelectorKind::Amp, reinterpret_cast<const char*>(pData)); break;
      case kMsgTagLoadedIR: SetSelectedFile(SelectorKind::Cab, reinterpret_cast<const char*>(pData)); break;
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

class PDMainKnobControl : public IKnobControlBase
{
public:
  PDMainKnobControl(const IRECT& bounds, int paramIdx, const char* label, const char* valueSuffix = "",
                    int opacityParamIdx = kNoParameter)
  : IKnobControlBase(bounds, paramIdx)
  , mLabel(label)
  , mValueSuffix(valueSuffix)
  , mOpacityParamIdx(opacityParamIdx)
  {
  }

  void Draw(IGraphics& g) override
  {
    const IBlend blend(EBlend::Default, GetOpacity());
    const IText text(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);
    const auto labelBounds = IRECT(mRECT.L, mRECT.T, mRECT.R, mRECT.T + kMainKnobLabelHeight);
    const auto valueBounds = IRECT(mRECT.L, mRECT.B - kMainKnobValueHeight, mRECT.R, mRECT.B);
    const float knobAvailableTop = labelBounds.B;
    const float knobAvailableBottom = valueBounds.T;
    const float cx = mRECT.MW();
    const float cy = knobAvailableTop + ((knobAvailableBottom - knobAvailableTop) / 2.f);
    const float knobRadius = kMainKnobDiameter / 2.f;
    const float arcRadius = std::min(mRECT.W(), knobAvailableBottom - knobAvailableTop) / 2.f;

    g.DrawText(text, mLabel.c_str(), labelBounds, &blend);
    g.DrawCircle(kPDForeground, cx, cy, knobRadius, &blend, kMainKnobStrokeWidth);

    const float angleStart = -135.f;
    const float angleEnd = 135.f;
    const float angle = angleStart + (static_cast<float>(GetValue()) * (angleEnd - angleStart));
    g.DrawArc(kPDForeground, cx, cy, arcRadius, angleStart, angle, &blend, kMainKnobStrokeWidth);

    float dotPoints[2][2];
    const float dotRadius = kMainKnobDotDiameter / 2.f;
    RadialPoints(angle, cx, cy, knobRadius * 0.68f, knobRadius * 0.68f, 2, dotPoints);
    g.FillCircle(kPDForeground, dotPoints[1][0], dotPoints[1][1], dotRadius, &blend);

    WDL_String value;
    if (const auto* param = GetParam())
      param->GetDisplay(value, true);
    const std::string valueText = FormatValueText(value.Get());

    g.DrawText(text, valueText.c_str(), valueBounds, &blend);
  }

private:
  float GetOpacity()
  {
    if (mOpacityParamIdx == kNoParameter || GetDelegate() == nullptr)
      return 1.f;

    const auto* param = GetDelegate()->GetParam(mOpacityParamIdx);
    return param != nullptr && param->Bool() ? 1.f : 0.5f;
  }

  std::string FormatValueText(const char* value) const
  {
    std::string valueText = value ? value : "";
    if (!mValueSuffix.empty() && valueText.find(mValueSuffix) == std::string::npos)
      valueText += mValueSuffix;

    return valueText;
  }

  std::string mLabel;
  std::string mValueSuffix;
  int mOpacityParamIdx;
};

class ControlAreaControl : public IControl
{
public:
  ControlAreaControl(const IRECT& bounds)
  : IControl(bounds)
  {
  }

  void Draw(IGraphics& g) override { g.FillRect(kPDBackground, mRECT); }
};

class PDEQContainerControl : public IControl
{
public:
  PDEQContainerControl(const IRECT& bounds, int opacityParamIdx)
  : IControl(bounds)
  , mOpacityParamIdx(opacityParamIdx)
  {
  }

  void Draw(IGraphics& g) override
  {
    const IBlend blend(EBlend::Default, GetOpacity());
    g.DrawRoundRect(kPDForeground, mRECT, kEQContainerBorderRadius, &blend, kEQContainerBorderSize);
  }

private:
  float GetOpacity()
  {
    if (mOpacityParamIdx == kNoParameter || GetDelegate() == nullptr)
      return 1.f;

    const auto* param = GetDelegate()->GetParam(mOpacityParamIdx);
    return param != nullptr && param->Bool() ? 1.f : 0.5f;
  }

  int mOpacityParamIdx;
};

class PDPowerButtonControl : public IControl
{
public:
  PDPowerButtonControl(const IRECT& bounds, const ISVG& icon, int paramIdx)
  : IControl(bounds, paramIdx)
  , mIcon(icon)
  {
  }

  void Draw(IGraphics& g) override
  {
    const IBlend blend(EBlend::Default, GetValue() > 0.5 ? 1.f : 0.5f);
    g.DrawSVG(mIcon, mRECT, &blend);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    SetValueFromUserInput(GetValue() > 0.5 ? 0.0 : 1.0);

    if (auto* ui = GetUI())
      ui->SetAllControlsDirty();
  }

private:
  ISVG mIcon;
};

class PDSettingsScreenControl : public IControl
{
public:
  PDSettingsScreenControl(const IRECT& bounds, const ISVG& backIcon)
  : IControl(bounds)
  , mBackIcon(backIcon)
  {
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
    g.DrawText(titleText, "Settings", headerBounds, &mBlend);
    DrawPathSettingContainer(g);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (GetBackButtonBounds().Contains(x, y))
    {
      Hide(true);
      if (auto* ui = GetUI())
        ui->SetAllControlsDirty();
    }

    if (GetNAMSelectButtonBounds().Contains(x, y))
    {
      PromptForLibraryDirectory(LibraryTarget::NAM);
      return;
    }

    if (GetIRSelectButtonBounds().Contains(x, y))
    {
      PromptForLibraryDirectory(LibraryTarget::IR);
      return;
    }
  }

private:
  enum class LibraryTarget
  {
    NAM,
    IR
  };

  IRECT GetBackButtonBounds() const
  {
    return IRECT(mRECT.L + kMainHeaderPadding, mRECT.T + kMainHeaderPadding,
                 mRECT.L + kMainHeaderPadding + kMainHeaderSlotSize,
                 mRECT.T + kMainHeaderPadding + kMainHeaderSlotSize);
  }

  IRECT GetPathSettingContainerBounds() const
  {
    const float left = mRECT.MW() - (kSettingsPathContainerWidth / 2.f);
    return IRECT(left, mRECT.T + kSettingsPathContainerTop, left + kSettingsPathContainerWidth,
                 mRECT.T + kSettingsPathContainerTop + kSettingsPathContainerHeight);
  }

  IRECT GetNAMPathSetterBounds() const
  {
    const auto container = GetPathSettingContainerBounds();
    return IRECT(container.L, container.T + kSettingsPathContainerPaddingY, container.L + kSettingsPathSetterWidth,
                 container.T + kSettingsPathContainerPaddingY + kSettingsPathSetterHeight);
  }

  IRECT GetIRPathSetterBounds() const
  {
    const auto nam = GetNAMPathSetterBounds();
    return IRECT(nam.R + kSettingsPathSetterGap, nam.T, nam.R + kSettingsPathSetterGap + kSettingsPathSetterWidth,
                 nam.B);
  }

  IRECT GetSelectButtonBounds(const IRECT& pathSetterBounds, bool hasSelectedPath) const
  {
    const float left = pathSetterBounds.L + kSettingsPathSetterPadding;
    float top = pathSetterBounds.T + kSettingsPathSetterPadding + kSettingsPathTitleHeight;
    if (hasSelectedPath)
      top += kSettingsPathTitleValueGap + kSettingsPathValueHeight;
    top += kSettingsPathTitleButtonGap;
    return IRECT(left, top, left + kSettingsButtonWidth, top + kSettingsButtonHeight);
  }

  IRECT GetNAMSelectButtonBounds()
  {
    return GetSelectButtonBounds(GetNAMPathSetterBounds(), GetNAMRootDirectory().GetLength() > 0);
  }
  IRECT GetIRSelectButtonBounds()
  {
    return GetSelectButtonBounds(GetIRPathSetterBounds(), GetIRRootDirectory().GetLength() > 0);
  }

  void DrawPathSettingContainer(IGraphics& g)
  {
    const auto container = GetPathSettingContainerBounds();
    const auto borderBounds =
      IRECT(container.L, container.B - kMainHeaderBorderSize, container.R, container.B);

    DrawPathSetter(g, GetNAMPathSetterBounds(), "NAM Library folder", GetNAMRootDirectory());
    DrawPathSetter(g, GetIRPathSetterBounds(), "IR Library folder", GetIRRootDirectory());
    g.FillRect(kPDLightGrey, borderBounds);
  }

  void DrawPathSetter(IGraphics& g, const IRECT& bounds, const char* title, const WDL_String& path)
  {
    const IText titleText(kSelectorTextSize, kPDForeground, kPDFontBold, EAlign::Near, EVAlign::Middle);
    const IText pathText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Near, EVAlign::Middle);
    const bool hasSelectedPath = path.GetLength() > 0;
    const auto titleBounds =
      IRECT(bounds.L + kSettingsPathSetterPadding, bounds.T + kSettingsPathSetterPadding,
            bounds.R - kSettingsPathSetterPadding, bounds.T + kSettingsPathSetterPadding + kSettingsPathTitleHeight);

    g.DrawRoundRect(kPDForeground, bounds, kSettingsPathSetterRadius, &mBlend, kSettingsPathSetterBorderSize);
    g.DrawText(titleText, title, titleBounds, &mBlend);

    if (hasSelectedPath)
    {
      const auto pathBounds = IRECT(titleBounds.L, titleBounds.B + kSettingsPathTitleValueGap, titleBounds.R,
                                    titleBounds.B + kSettingsPathTitleValueGap + kSettingsPathValueHeight);
      g.DrawText(pathText, EllipsizeToFit(g, pathText, path.Get(), pathBounds.W()).c_str(), pathBounds, &mBlend);
    }

    DrawGeneralButton(g, GetSelectButtonBounds(bounds, hasSelectedPath),
                      hasSelectedPath ? "Change folder" : "Select folder");
  }

  void DrawGeneralButton(IGraphics& g, const IRECT& bounds, const char* label)
  {
    const IText text(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);

    g.DrawRoundRect(kPDForeground, bounds, kSettingsButtonRadius, &mBlend, kSettingsButtonBorderSize);
    g.DrawText(text, label, bounds, &mBlend);
  }

  void PromptForLibraryDirectory(LibraryTarget target)
  {
    WDL_String directory(target == LibraryTarget::NAM ? PLUG()->GetNAMRootDirectory().Get()
                                                      : PLUG()->GetIRRootDirectory().Get());

    GetUI()->PromptForDirectory(directory, [this, target](const WDL_String& fileName, const WDL_String& path) {
      if (!path.GetLength())
        return;

      if (target == LibraryTarget::NAM)
        PLUG()->SetNAMRootDirectory(path);
      else
        PLUG()->SetIRRootDirectory(path);

      SetDirty(false);
    });
  }

  WDL_String GetNAMRootDirectory() { return PLUG()->GetNAMRootDirectory(); }
  WDL_String GetIRRootDirectory() { return PLUG()->GetIRRootDirectory(); }

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

  ISVG mBackIcon;
};

class PDAmpSelectorScreenControl : public IControl
{
public:
  enum class Target
  {
    Amp,
    Cab
  };

  PDAmpSelectorScreenControl(const IRECT& bounds, const ISVG& backIcon, const IBitmap& fallbackImage,
                             const char* title, const char* extension, Target target)
  : IControl(bounds)
  , mBackIcon(backIcon)
  , mFallbackImage(fallbackImage)
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
    const int rows = (static_cast<int>(mFolders.size()) + kAmpSelectorColumns - 1) / kAmpSelectorColumns;
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

    if (mRootDirectory.empty())
    {
      g.PathClipRegion(IRECT());
      return;
    }

    for (int i = 0; i < static_cast<int>(mFolders.size()); ++i)
    {
      const int column = i % kAmpSelectorColumns;
      const int row = i / kAmpSelectorColumns;
      auto& folder = mFolders[static_cast<size_t>(i)];
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
      DrawContainedBitmap(g, folder.thumbnail.IsValid() ? folder.thumbnail : mFallbackImage, imageBounds, 1.f);
      DrawBottomTitleOverlay(g, titleText, folder, cardBounds);
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

  void DrawBottomTitleOverlay(IGraphics& g, const IText& textStyle, Folder& folder, const IRECT& cardBounds)
  {
    const auto textBounds = IRECT(cardBounds.L + kAmpSelectorCardPadding, cardBounds.T + kAmpSelectorCardPadding,
                                  cardBounds.R - kAmpSelectorCardPadding, cardBounds.B);
    const auto& lines = GetTitleLines(g, textStyle, folder, textBounds.W());
    if (lines.empty())
      return;

    const float lineHeight = textStyle.mSize + 2.f;
    const float totalTextHeight = static_cast<float>(lines.size()) * lineHeight;
    const float overlayHeight = std::max(kAmpSelectorTitleMinHeight, totalTextHeight + (2.f * kAmpSelectorTitlePaddingY));
    const auto overlayBounds = IRECT(textBounds.L, cardBounds.B - overlayHeight, textBounds.R, cardBounds.B);
    const IText lineText = textStyle.WithVAlign(EVAlign::Middle).WithAlign(EAlign::Center);

    g.FillRect(kPDBackground, overlayBounds);

    float y = overlayBounds.B - kAmpSelectorTitlePaddingY - totalTextHeight;
    for (const auto& line : lines)
    {
      const IRECT lineBounds(overlayBounds.L, y, overlayBounds.R, y + lineHeight);
      g.DrawText(lineText, line.c_str(), lineBounds, &mBlend);
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
      "or load all files found in all sub-folders (this might have performance implications).";

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

      if (folder.hasDirectSubfolders)
      {
        ShowSubfolderPrompt(folder);
        return;
      }

      LoadFolderFiles(folder.directFiles.empty() ? folder.recursiveFiles : folder.directFiles);
    });
  }

  void SelectFolderAtPoint(float x, float y)
  {
    const auto scrollArea = GetScrollArea();
    if (!scrollArea.Contains(x, y))
      return;

    const int index = GetFolderIndexAtPoint(x, y);
    if (index < 0 || index >= static_cast<int>(mFolders.size()))
      return;

    const auto& folder = mFolders[static_cast<size_t>(index)];
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

  void CloseScreen()
  {
    ReleaseThumbnails();
    Hide(true);
    if (auto* ui = GetUI())
      ui->SetAllControlsDirty();
  }

  int GetFolderIndexAtPoint(float x, float y) const
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
    if (index < 0 || index >= static_cast<int>(mFolders.size()))
      return -1;

    return index;
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
      folder.thumbnail = GetUI()->LoadBitmap(folder.thumbnailPath.c_str());

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
    LoadFolderThumbnail(folder);

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
    LoadFolderThumbnail(folder);

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
      if (entry.is_regular_file(entryError) && IsGeneratedThumbnailJPG(entry.path()))
      {
        result = entry.path();
        return true;
      }
    }

    for (const auto& entry : entries)
    {
      std::error_code entryError;
      if (entry.is_regular_file(entryError) && IsJPG(entry.path()))
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
      if (entry.is_directory(entryError) && FindFirstJPG(entry.path(), result))
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
    return source.parent_path() / (source.stem().string() + "@181px.jpg");
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
    const std::string stem = ToLower(path.stem().string());
    return IsJPG(path) && stem.size() >= 6 && stem.ends_with("@181px");
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

    int maxPixelSize = static_cast<int>(kAmpSelectorImageSize);
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
  std::string mTitle;
  std::string mExtension;
  Target mTarget;
  std::string mRootDirectory;
  std::vector<Folder> mFolders;
  Folder mSubfolderPromptFolder;
  bool mShowingSubfolderPrompt = false;
  float mScroll = 0.f;
};

bool GetLibrarySettingsPath(std::filesystem::path& path)
{
#if defined OS_MAC || defined OS_WIN
  WDL_String directory;
  INIPath(directory, BUNDLE_NAME);

  if (!CStringHasContents(directory.Get()))
    return false;

  path = std::filesystem::u8path(directory.Get()) / kLibrarySettingsFileName;
  return true;
#else
  return false;
#endif
}

struct UIAssets
{
  ISVG gear;
  ISVG file;
  ISVG cross;
  ISVG rightArrow;
  ISVG leftArrow;
  ISVG modelIcon;
  ISVG irIconOn;
  ISVG irIconOff;
  ISVG slimIcon;
  IBitmap fileBackground;
  IBitmap inputLevelBackground;
  IBitmap knobBackground;
  IBitmap switchHandle;
  IBitmap meterBackground;
};

struct UILayout
{
  IRECT bounds;
  IRECT title;
  IRECT modelIcon;
  IRECT modelThumbnail;
  IRECT modelBrowser;
  IRECT irBrowser;
  IRECT irSwitch;
  IRECT slimButton;
  IRECT inputKnob;
  IRECT noiseGateKnob;
  IRECT bassKnob;
  IRECT midKnob;
  IRECT trebleKnob;
  IRECT outputKnob;
  IRECT noiseGateToggle;
  IRECT eqToggle;
  IRECT inputMeter;
  IRECT outputMeter;
  IRECT sidebar;
  IRECT drawerButton;
  IRECT settingsButton;
  IRECT slimKnob;
};

void ConfigureUIHost(IGraphics& graphics)
{
  graphics.AttachCornerResizer(EUIResizerMode::Scale, false);
  graphics.AttachTextEntryControl();
  graphics.EnableMouseOver(true);
  graphics.EnableTooltips(true);
  graphics.EnableMultiTouch(true);
  graphics.LoadFont("Roboto-Regular", ROBOTO_FN);
  graphics.LoadFont("Michroma-Regular", MICHROMA_FN);
  graphics.LoadFont(kPDFontMedium, PD_FONT_MEDIUM_FN);
  graphics.LoadFont(kPDFontBold, PD_FONT_BOLD_FN);
}

UIAssets LoadUIAssets(IGraphics& graphics)
{
  return {
    graphics.LoadSVG(GEAR_FN),
    graphics.LoadSVG(FILE_FN),
    graphics.LoadSVG(CLOSE_BUTTON_FN),
    graphics.LoadSVG(RIGHT_ARROW_FN),
    graphics.LoadSVG(LEFT_ARROW_FN),
    graphics.LoadSVG(MODEL_ICON_FN),
    graphics.LoadSVG(IR_ICON_ON_FN),
    graphics.LoadSVG(IR_ICON_OFF_FN),
    graphics.LoadSVG(SLIMMABLE_ICON_FN),
    graphics.LoadBitmap(FILEBACKGROUND_FN),
    graphics.LoadBitmap(INPUTLEVELBACKGROUND_FN),
    graphics.LoadBitmap(KNOBBACKGROUND_FN),
    graphics.LoadBitmap(SLIDESWITCHHANDLE_FN),
    graphics.LoadBitmap(METERBACKGROUND_FN),
  };
}

UILayout BuildUILayout(const IRECT& bounds)
{
  UILayout layout;
  layout.bounds = bounds;

  const auto mainArea = bounds.GetPadded(-20);
  const auto contentArea = mainArea.GetPadded(-10);
  const auto drawerWidth = 321.0f;
  const auto drawerButtonSize = 32.0f;
  const auto mainContentArea = contentArea;
  const auto titleHeight = 50.0f;

  layout.sidebar = contentArea.GetFromLeft(drawerWidth).GetReducedFromLeft(6.0f);
  layout.drawerButton = IRECT(layout.sidebar.L - drawerButtonSize - 2.f, layout.sidebar.T + 8.f, layout.sidebar.L - 2.f,
                              layout.sidebar.T + 8.f + drawerButtonSize);
  layout.title = mainContentArea.GetFromTop(titleHeight);

  const auto knobsPad = 20.0f;
  const auto knobsExtraSpaceBelowTitle = 25.0f;
  const auto singleKnobPad = -2.0f;
  const auto knobsArea = mainContentArea.GetFromTop(NAM_KNOB_HEIGHT)
                           .GetReducedFromLeft(knobsPad)
                           .GetReducedFromRight(knobsPad)
                           .GetVShifted(titleHeight + knobsExtraSpaceBelowTitle);
  layout.inputKnob = knobsArea.GetGridCell(0, kInputLevel, 1, numKnobs).GetPadded(-singleKnobPad);
  layout.noiseGateKnob = knobsArea.GetGridCell(0, kNoiseGateThreshold, 1, numKnobs).GetPadded(-singleKnobPad);
  layout.bassKnob = knobsArea.GetGridCell(0, kToneBass, 1, numKnobs).GetPadded(-singleKnobPad);
  layout.midKnob = knobsArea.GetGridCell(0, kToneMid, 1, numKnobs).GetPadded(-singleKnobPad);
  layout.trebleKnob = knobsArea.GetGridCell(0, kToneTreble, 1, numKnobs).GetPadded(-singleKnobPad);
  layout.outputKnob = knobsArea.GetGridCell(0, kOutputLevel, 1, numKnobs).GetPadded(-singleKnobPad);
  layout.noiseGateToggle =
    layout.noiseGateKnob.GetVShifted(layout.noiseGateKnob.H()).SubRectVertical(2, 0).GetReducedFromTop(10.0f);
  layout.eqToggle = layout.midKnob.GetVShifted(layout.midKnob.H()).SubRectVertical(2, 0).GetReducedFromTop(10.0f);

  const auto fileWidth = 200.0f;
  const auto fileHeight = 30.0f;
  const auto irYOffset = 38.0f;
  layout.modelBrowser =
    mainContentArea.GetFromBottom((2.0f * fileHeight)).GetFromTop(fileHeight).GetMidHPadded(fileWidth).GetVShifted(-1);
  layout.slimButton = IRECT(layout.modelBrowser.R + 6.f, layout.modelBrowser.MH() - 14.f,
                            layout.modelBrowser.R + 6.f + 2.f * 28.f, layout.modelBrowser.MH() + 14.f);
  layout.modelIcon = layout.modelBrowser.GetFromLeft(30).GetTranslated(-40, 10);
  layout.irBrowser = layout.modelBrowser.GetVShifted(irYOffset);
  layout.irSwitch = layout.irBrowser.GetFromLeft(30.0f).GetHShifted(-40.0f).GetScaledAboutCentre(0.6f);
  layout.modelThumbnail =
    IRECT(mainContentArea.L, mainContentArea.T + 252.f, mainContentArea.R, layout.modelBrowser.T - 18.f)
      .GetCentredInside(330.f, 150.f);

  layout.inputMeter = mainContentArea.GetFromLeft(30).GetHShifted(-20).GetMidVPadded(100).GetVShifted(-25);
  layout.outputMeter = mainContentArea.GetFromRight(30).GetHShifted(20).GetMidVPadded(100).GetVShifted(-25);
  layout.settingsButton = CornerButtonArea(bounds);
  layout.slimKnob = bounds.GetCentredInside(100.f, NAM_KNOB_HEIGHT + 24.f);

  return layout;
}

void AttachShellComponent(IGraphics& graphics)
{
  graphics.AttachPanelBackground(kPDBackground);
}

void AttachMainHeaderComponent(IGraphics& graphics, const IRECT& bounds, const ISVG& logo, const ISVG& settingsIcon)
{
  const auto headerBounds = IRECT(bounds.L, bounds.T, bounds.R, bounds.T + kMainHeaderHeight);
  const auto borderBounds =
    IRECT(headerBounds.L, headerBounds.B - kMainHeaderBorderSize, headerBounds.R, headerBounds.B);
  const auto middleSlot = IRECT(headerBounds.MW() - (kMainHeaderLogoWidth / 2.f), headerBounds.T + kMainHeaderPadding,
                                headerBounds.MW() + (kMainHeaderLogoWidth / 2.f),
                                headerBounds.T + kMainHeaderPadding + kMainHeaderLogoHeight);
  const auto rightSlot =
    IRECT(headerBounds.R - kMainHeaderPadding - kMainHeaderSlotSize, headerBounds.T + kMainHeaderPadding,
          headerBounds.R - kMainHeaderPadding, headerBounds.T + kMainHeaderPadding + kMainHeaderSlotSize);
  IGraphics* ui = &graphics;

  graphics.AttachControl(new IPanelControl(headerBounds, kPDBackground))->SetIgnoreMouse(true);
  graphics.AttachControl(new IPanelControl(borderBounds, kPDLightGrey))->SetIgnoreMouse(true);
  graphics.AttachControl(new ISVGControl(middleSlot, logo))->SetIgnoreMouse(true);
  graphics.AttachControl(new NAMHeaderIconButtonControl(
    rightSlot,
    [ui](IControl* pCaller) {
      if (auto* settings = ui->GetControlWithTag(kCtrlTagSettingsBox))
        settings->Hide(false);

      ui->SetAllControlsDirty();
    },
    settingsIcon));
}

void AttachHeaderLoadingBarComponent(IGraphics& graphics, const IRECT& bounds)
{
  const float top = bounds.T + kMainHeaderHeight - (kHeaderLoadingBarHeight / 2.f);
  const auto loadingBarBounds = IRECT(bounds.L, top, bounds.R, top + kHeaderLoadingBarHeight);
  graphics.AttachControl(new PDHeaderLoadingBarControl(loadingBarBounds), kCtrlTagHeaderLoadingBar);
}

void AttachMainAreaComponent(IGraphics& graphics, const IRECT& bounds, const IBitmap& ampImage, const IBitmap& cabImage,
                             const IBitmap& noAmpImage, const IBitmap& noCabImage)
{
  const auto mainAreaBounds = IRECT(bounds.L, bounds.T + kMainAreaTop, bounds.R, bounds.T + kMainAreaTop + kMainAreaHeight);
  graphics.AttachControl(new MainAreaControl(mainAreaBounds, ampImage, cabImage, noAmpImage, noCabImage), kCtrlTagMainArea);
}

void AttachSelectorAreaComponent(IGraphics& graphics, const IRECT& bounds, const ISVG& leftArrow,
                                 const ISVG& rightArrow)
{
  const auto selectorAreaBounds =
    IRECT(bounds.L, bounds.T + kSelectorAreaTop, bounds.R, bounds.T + kSelectorAreaTop + kSelectorAreaHeight);
  graphics.AttachControl(new SelectorAreaControl(selectorAreaBounds, leftArrow, rightArrow), kCtrlTagSelectorArea);
}

void AttachControlAreaComponent(IGraphics& graphics, const IRECT& bounds, const ISVG& powerIcon)
{
  const auto containerBounds = IRECT(bounds.L, bounds.T + kControlAreaContainerTop, bounds.R,
                                    bounds.T + kControlAreaContainerTop + kControlAreaContainerHeight);
  const auto controlAreaBounds = containerBounds.GetCentredInside(kControlAreaWidth, kControlAreaHeight);
  const float standaloneKnobTop =
    controlAreaBounds.T + ((controlAreaBounds.H() - kMainKnobContainerHeight) / 2.f);
  auto makeKnobBounds = [](float left, float top) {
    return IRECT(left, top, left + kMainKnobContainerWidth, top + kMainKnobContainerHeight);
  };

  const auto inputKnobBounds = makeKnobBounds(controlAreaBounds.L, standaloneKnobTop);
  const auto gateKnobBounds = makeKnobBounds(inputKnobBounds.R + kMainKnobGap, standaloneKnobTop);
  const auto eqBounds = IRECT(gateKnobBounds.R + kMainKnobGap, controlAreaBounds.T,
                              gateKnobBounds.R + kMainKnobGap + kEQContainerWidth,
                              controlAreaBounds.T + kEQContainerHeight);
  const float eqKnobTop = eqBounds.T + kEQContainerPaddingY;
  const auto bassKnobBounds = makeKnobBounds(eqBounds.L + kEQContainerPaddingX, eqKnobTop);
  const auto middleKnobBounds = makeKnobBounds(bassKnobBounds.R + kMainKnobGap, eqKnobTop);
  const auto trebleKnobBounds = makeKnobBounds(middleKnobBounds.R + kMainKnobGap, eqKnobTop);
  const auto outputKnobBounds = makeKnobBounds(eqBounds.R + kMainKnobGap, standaloneKnobTop);
  const auto eqPowerButtonBounds =
    IRECT(eqBounds.R - kEQPowerButtonInset - kEQPowerButtonSize, eqBounds.T + kEQPowerButtonInset,
          eqBounds.R - kEQPowerButtonInset, eqBounds.T + kEQPowerButtonInset + kEQPowerButtonSize);
  const auto gatePowerButtonBounds =
    IRECT(gateKnobBounds.R, eqBounds.T + kEQPowerButtonInset, gateKnobBounds.R + kEQPowerButtonSize,
          eqBounds.T + kEQPowerButtonInset + kEQPowerButtonSize);

  graphics.AttachControl(new ControlAreaControl(containerBounds))->SetIgnoreMouse(true);
  graphics.AttachControl(new PDEQContainerControl(eqBounds, kEQActive))->SetIgnoreMouse(true);
  graphics.AttachControl(new PDMainKnobControl(inputKnobBounds, kInputLevel, "Input", "dB"));
  graphics.AttachControl(new PDMainKnobControl(gateKnobBounds, kNoiseGateThreshold, "Gate", "dB", kNoiseGateActive));
  graphics.AttachControl(new PDPowerButtonControl(gatePowerButtonBounds, powerIcon, kNoiseGateActive));
  graphics.AttachControl(new PDMainKnobControl(bassKnobBounds, kToneBass, "Bass", "", kEQActive), -1, "EQ_KNOBS");
  graphics.AttachControl(new PDMainKnobControl(middleKnobBounds, kToneMid, "Middle", "", kEQActive), -1, "EQ_KNOBS");
  graphics.AttachControl(new PDMainKnobControl(trebleKnobBounds, kToneTreble, "Treble", "", kEQActive), -1,
                         "EQ_KNOBS");
  graphics.AttachControl(new PDPowerButtonControl(eqPowerButtonBounds, powerIcon, kEQActive));
  graphics.AttachControl(new PDMainKnobControl(outputKnobBounds, kOutputLevel, "Output", "dB"));
}

void AttachHeaderComponent(IGraphics& graphics, const UILayout& layout, const UIAssets& assets)
{
  graphics.AttachControl(new IVLabelControl(layout.title, "Pedal Division NAM", titleStyle));
  graphics.AttachControl(new ISVGControl(layout.modelIcon, assets.modelIcon));
}

void AttachModelThumbnailComponent(IGraphics& graphics, const UILayout& layout, const WDL_String& namRootDirectory)
{
  auto* modelThumbnail = new NAMModelThumbnailControl(layout.modelThumbnail);
  graphics.AttachControl(modelThumbnail, kCtrlTagModelThumbnail);
  modelThumbnail->SetNAMRootDirectory(namRootDirectory.Get());
}

void AttachFileBrowsersComponent(IGraphics& graphics, const UILayout& layout, const UIAssets& assets,
                                 IFileDialogCompletionHandlerFunc loadModelCompletionHandler,
                                 IFileDialogCompletionHandlerFunc loadIRCompletionHandler)
{
#ifdef NAM_PICK_DIRECTORY
  const std::string defaultNamFileString = "Select model directory...";
  const std::string defaultIRString = "Select IR directory...";
#else
  const std::string defaultNamFileString = "Select model...";
  const std::string defaultIRString = "Select IR...";
#endif

  graphics.AttachControl(new NAMFileBrowserControl(layout.modelBrowser, kMsgTagClearModel, defaultNamFileString.c_str(),
                                                   "nam", loadModelCompletionHandler, style, assets.file, assets.cross,
                                                   assets.leftArrow, assets.rightArrow, assets.fileBackground, true),
                         kCtrlTagModelFileBrowser);
  graphics.AttachControl(new ISVGSwitchControl(layout.irSwitch, {assets.irIconOff, assets.irIconOn}, kIRToggle));
  graphics.AttachControl(new NAMFileBrowserControl(layout.irBrowser, kMsgTagClearIR, defaultIRString.c_str(), "wav",
                                                   loadIRCompletionHandler, style, assets.file, assets.cross,
                                                   assets.leftArrow, assets.rightArrow, assets.fileBackground, true),
                         kCtrlTagIRFileBrowser);
}

void AttachToneControlsComponent(IGraphics& graphics, const UILayout& layout, const UIAssets& assets)
{
  graphics.AttachControl(new NAMSwitchControl(layout.noiseGateToggle, kNoiseGateActive, "Noise Gate", style,
                                              assets.switchHandle));
  graphics.AttachControl(new NAMSwitchControl(layout.eqToggle, kEQActive, "EQ", style, assets.switchHandle));

  graphics.AttachControl(new NAMKnobControl(layout.inputKnob, kInputLevel, "", style, assets.knobBackground));
  graphics.AttachControl(new NAMKnobControl(layout.noiseGateKnob, kNoiseGateThreshold, "", style, assets.knobBackground));
  graphics.AttachControl(new NAMKnobControl(layout.bassKnob, kToneBass, "", style, assets.knobBackground), -1,
                         "EQ_KNOBS");
  graphics.AttachControl(new NAMKnobControl(layout.midKnob, kToneMid, "", style, assets.knobBackground), -1,
                         "EQ_KNOBS");
  graphics.AttachControl(new NAMKnobControl(layout.trebleKnob, kToneTreble, "", style, assets.knobBackground), -1,
                         "EQ_KNOBS");
  graphics.AttachControl(new NAMKnobControl(layout.outputKnob, kOutputLevel, "", style, assets.knobBackground));
}

void AttachMetersComponent(IGraphics& graphics, const UILayout& layout, const UIAssets& assets)
{
  graphics.AttachControl(new NAMMeterControl(layout.inputMeter, assets.meterBackground, style), kCtrlTagInputMeter);
  graphics.AttachControl(new NAMMeterControl(layout.outputMeter, assets.meterBackground, style), kCtrlTagOutputMeter);
}

void AttachLibraryDrawerComponent(IGraphics& graphics, const UILayout& layout, const WDL_String& namRootDirectory,
                                  const WDL_String& irRootDirectory)
{
  IGraphics* ui = &graphics;
  auto loadDirectoryIntoBrowser = [ui](int browserTag, const char* directory) {
    if (auto* browser = ui->GetControlWithTag(browserTag))
      browser->As<NAMFileBrowserControl>()->LoadDirectory(directory);
  };

  auto* librarySidebar = new NAMLibrarySidebarControl(
    layout.sidebar, [loadDirectoryIntoBrowser](const char* directory) {
      loadDirectoryIntoBrowser(kCtrlTagModelFileBrowser, directory);
    },
    [loadDirectoryIntoBrowser](const char* directory) { loadDirectoryIntoBrowser(kCtrlTagIRFileBrowser, directory); });
  graphics.AttachControl(librarySidebar, kCtrlTagLibrarySidebar)->Hide(true);
  librarySidebar->SetRoots(namRootDirectory.Get(), irRootDirectory.Get());
  graphics.AttachControl(new NAMLibraryDrawerButtonControl(layout.drawerButton, kCtrlTagLibrarySidebar),
                         kCtrlTagLibraryDrawerButton);
}

void AttachSettingsScreenComponent(IGraphics& graphics, const IRECT& bounds, const ISVG& backIcon)
{
  graphics.AttachControl(new PDSettingsScreenControl(bounds, backIcon), kCtrlTagSettingsBox)->Hide(true);
}

void AttachAmpSelectorScreenComponent(IGraphics& graphics, const IRECT& bounds, const ISVG& backIcon,
                                      const IBitmap& fallbackImage)
{
  graphics.AttachControl(new PDAmpSelectorScreenControl(bounds, backIcon, fallbackImage, "Amps", "nam",
                                                        PDAmpSelectorScreenControl::Target::Amp),
                         kCtrlTagAmpSelectorScreen)
    ->Hide(true);
}

void AttachCabSelectorScreenComponent(IGraphics& graphics, const IRECT& bounds, const ISVG& backIcon,
                                      const IBitmap& fallbackImage)
{
  graphics.AttachControl(new PDAmpSelectorScreenControl(bounds, backIcon, fallbackImage, "Cabs", "wav",
                                                        PDAmpSelectorScreenControl::Target::Cab),
                         kCtrlTagCabSelectorScreen)
    ->Hide(true);
}

void AttachSlimComponent(IGraphics& graphics, const UILayout& layout, const UIAssets& assets)
{
  auto hideSlimOverlay = [](IControl* pCaller) {
    IGraphics* ui = pCaller->GetUI();
    if (auto* backdrop = ui->GetControlWithTag(kCtrlTagSlimOverlayBackdrop))
      backdrop->Hide(true);
    if (auto* knob = ui->GetControlWithTag(kCtrlTagSlimKnob))
      knob->Hide(true);
    ui->SetAllControlsDirty();
  };

  auto showSlimOverlay = [](IControl* pCaller) {
    IGraphics* ui = pCaller->GetUI();
    if (auto* backdrop = ui->GetControlWithTag(kCtrlTagSlimOverlayBackdrop))
      backdrop->Hide(false);
    if (auto* knob = ui->GetControlWithTag(kCtrlTagSlimKnob))
      knob->Hide(false);
    ui->SetAllControlsDirty();
  };

  graphics.AttachControl(new NAMSquareButtonControl(layout.slimButton, DefaultClickActionFunc, assets.slimIcon),
                         kCtrlTagSlimmableIcon)
    ->SetAnimationEndActionFunction(showSlimOverlay)
    ->Hide(true);
  graphics.AttachControl(new NAMSlimOverlayBackdropControl(layout.bounds, hideSlimOverlay), kCtrlTagSlimOverlayBackdrop)
    ->Hide(true);
  graphics.AttachControl(new NAMKnobControl(layout.slimKnob, kSlim, "Slim", style, assets.knobBackground),
                         kCtrlTagSlimKnob)
    ->Hide(true);
}

void AttachInteractionDefaults(IGraphics& graphics)
{
  graphics.ForAllControlsFunc([](IControl* pControl) {
    pControl->SetMouseEventsWhenDisabled(true);
    pControl->SetMouseOverWhenDisabled(true);
  });
}
} // namespace


NeuralAmpModeler::NeuralAmpModeler(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  _InitToneStack();
  nam::activations::Activation::enable_fast_tanh();
  GetParam(kInputLevel)->InitGain("Input", 0.0, -20.0, 20.0, 0.1);
  GetParam(kToneBass)->InitDouble("Bass", 5.0, 0.0, 10.0, 0.1);
  GetParam(kToneMid)->InitDouble("Middle", 5.0, 0.0, 10.0, 0.1);
  GetParam(kToneTreble)->InitDouble("Treble", 5.0, 0.0, 10.0, 0.1);
  GetParam(kOutputLevel)->InitGain("Output", 0.0, -40.0, 40.0, 0.1);
  GetParam(kNoiseGateThreshold)->InitGain("Threshold", -80.0, -100.0, 0.0, 0.1);
  GetParam(kNoiseGateActive)->InitBool("NoiseGateActive", true);
  GetParam(kEQActive)->InitBool("ToneStack", true);
  GetParam(kOutputMode)->InitEnum("OutputMode", 1, {"Raw", "Normalized", "Calibrated"}); // TODO DRY w/ control
  GetParam(kIRToggle)->InitBool("IRToggle", true);
  GetParam(kCalibrateInput)->InitBool(kCalibrateInputParamName.c_str(), kDefaultCalibrateInput);
  GetParam(kInputCalibrationLevel)
    ->InitDouble(kInputCalibrationLevelParamName.c_str(), kDefaultInputCalibrationLevel, -60.0, 60.0, 0.1, "dBu");
  GetParam(kSlim)->InitDouble("Slim", 0.0, 0.0, 1.0, 0.01);

  mNoiseGateTrigger.AddListener(&mNoiseGateGain);
  _LoadLibrarySettings();

  mMakeGraphicsFunc = [&]() {

#ifdef OS_IOS
    auto scaleFactor = GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT) * 0.85f;
#else
    auto scaleFactor = 1.0f;
#endif

    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, scaleFactor);
  };

  mLayoutFunc = [](IGraphics* pGraphics) {
    ConfigureUIHost(*pGraphics);
    const auto assets = LoadUIAssets(*pGraphics);
    const auto logo = pGraphics->LoadSVG(PD_LOGO_FN);
    const auto settingsIcon = pGraphics->LoadSVG(PD_ICON_SETTINGS_FN);
    const auto settingsBackIcon = pGraphics->LoadSVG(PD_ICON_ARROW_LEFT_FN);
    const auto powerIcon = pGraphics->LoadSVG(PD_ICON_POWER_FN);
    const auto noAmpImage = pGraphics->LoadBitmap(PD_NO_AMP_FN);
    const auto noCabImage = pGraphics->LoadBitmap(PD_NO_CAB_FN);
    const auto chevronLeft = pGraphics->LoadSVG(PD_CHEVRON_LEFT_FN);
    const auto chevronRight = pGraphics->LoadSVG(PD_CHEVRON_RIGHT_FN);

    AttachShellComponent(*pGraphics);
    AttachMainHeaderComponent(*pGraphics, pGraphics->GetBounds(), logo, settingsIcon);
    AttachMainAreaComponent(*pGraphics, pGraphics->GetBounds(), noAmpImage, noCabImage, noAmpImage, noCabImage);
    AttachSelectorAreaComponent(*pGraphics, pGraphics->GetBounds(), chevronLeft, chevronRight);
    AttachControlAreaComponent(*pGraphics, pGraphics->GetBounds(), powerIcon);
    AttachSettingsScreenComponent(*pGraphics, pGraphics->GetBounds(), settingsBackIcon);
    AttachAmpSelectorScreenComponent(*pGraphics, pGraphics->GetBounds(), settingsBackIcon, noAmpImage);
    AttachCabSelectorScreenComponent(*pGraphics, pGraphics->GetBounds(), settingsBackIcon, noCabImage);
    AttachHeaderLoadingBarComponent(*pGraphics, pGraphics->GetBounds());
  };
}

NeuralAmpModeler::~NeuralAmpModeler()
{
  mShuttingDown = true;
  for (auto& task : mLoadTasks)
  {
    if (task.valid())
      task.get();
  }
  _DeallocateIOPointers();
}

void NeuralAmpModeler::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames)
{
  const size_t numChannelsExternalIn = (size_t)NInChansConnected();
  const size_t numChannelsExternalOut = (size_t)NOutChansConnected();
  const size_t numChannelsInternal = kNumChannelsInternal;
  const size_t numFrames = (size_t)nFrames;
  const double sampleRate = GetSampleRate();

  // Disable floating point denormals
  std::fenv_t fe_state;
  std::feholdexcept(&fe_state);
  disable_denormals();

  _PrepareBuffers(numChannelsInternal, numFrames);
  // Input is collapsed to mono in preparation for the NAM.
  _ProcessInput(inputs, numFrames, numChannelsExternalIn, numChannelsInternal);
  _ApplyDSPStaging();
  const bool noiseGateActive = GetParam(kNoiseGateActive)->Value();
  const bool toneStackActive = GetParam(kEQActive)->Value();

  // Noise gate trigger
  sample** triggerOutput = mInputPointers;
  if (noiseGateActive)
  {
    const double time = 0.01;
    const double threshold = GetParam(kNoiseGateThreshold)->Value(); // GetParam...
    const double ratio = 0.1; // Quadratic...
    const double openTime = 0.005;
    const double holdTime = 0.01;
    const double closeTime = 0.05;
    const dsp::noise_gate::TriggerParams triggerParams(time, threshold, ratio, openTime, holdTime, closeTime);
    mNoiseGateTrigger.SetParams(triggerParams);
    mNoiseGateTrigger.SetSampleRate(sampleRate);
    triggerOutput = mNoiseGateTrigger.Process(mInputPointers, numChannelsInternal, numFrames);
  }

  if (mModel != nullptr)
  {
    mModel->process(triggerOutput, mOutputPointers, nFrames);
  }
  else
  {
    _FallbackDSP(triggerOutput, mOutputPointers, numChannelsInternal, numFrames);
  }
  // Apply the noise gate after the NAM
  sample** gateGainOutput =
    noiseGateActive ? mNoiseGateGain.Process(mOutputPointers, numChannelsInternal, numFrames) : mOutputPointers;

  sample** toneStackOutPointers = (toneStackActive && mToneStack != nullptr)
                                    ? mToneStack->Process(gateGainOutput, numChannelsInternal, nFrames)
                                    : gateGainOutput;

  sample** irPointers = toneStackOutPointers;
  if (mIR != nullptr && GetParam(kIRToggle)->Value())
    irPointers = mIR->Process(toneStackOutPointers, numChannelsInternal, numFrames);

  // And the HPF for DC offset (Issue 271)
  const double highPassCutoffFreq = kDCBlockerFrequency;
  // const double lowPassCutoffFreq = 20000.0;
  const recursive_linear_filter::HighPassParams highPassParams(sampleRate, highPassCutoffFreq);
  // const recursive_linear_filter::LowPassParams lowPassParams(sampleRate, lowPassCutoffFreq);
  mHighPass.SetParams(highPassParams);
  // mLowPass.SetParams(lowPassParams);
  sample** hpfPointers = mHighPass.Process(irPointers, numChannelsInternal, numFrames);
  // sample** lpfPointers = mLowPass.Process(hpfPointers, numChannelsInternal, numFrames);

  // restore previous floating point state
  std::feupdateenv(&fe_state);

  // Let's get outta here
  // This is where we exit mono for whatever the output requires.
  _ProcessOutput(hpfPointers, outputs, numFrames, numChannelsInternal, numChannelsExternalOut);
  // _ProcessOutput(lpfPointers, outputs, numFrames, numChannelsInternal, numChannelsExternalOut);
  // * Output of input leveling (inputs -> mInputPointers),
  // * Output of output leveling (mOutputPointers -> outputs)
  _UpdateMeters(mInputPointers, outputs, numFrames, numChannelsInternal, numChannelsExternalOut);
}

void NeuralAmpModeler::OnReset()
{
  const auto sampleRate = GetSampleRate();
  const int maxBlockSize = GetBlockSize();

  // Tail is because the HPF DC blocker has a decay.
  // 10 cycles should be enough to pass the VST3 tests checking tail behavior.
  // I'm ignoring the model & IR, but it's not the end of the world.
  const int tailCycles = 10;
  SetTailSize(tailCycles * (int)(sampleRate / kDCBlockerFrequency));
  mInputSender.Reset(sampleRate);
  mOutputSender.Reset(sampleRate);
  // If there is a model or IR loaded, they need to be checked for resampling.
  _ResetModelAndIR(sampleRate, GetBlockSize());
  mToneStack->Reset(sampleRate, maxBlockSize);
  _UpdateLatency();
}

void NeuralAmpModeler::OnIdle()
{
  _ProcessAsyncLoadCompletions();
  _CollectFinishedLoadTasks();

  if (auto* pGraphics = GetUI())
  {
    if (auto* loadingBar = pGraphics->GetControlWithTag(kCtrlTagHeaderLoadingBar))
    {
      auto* headerLoadingBar = loadingBar->As<PDHeaderLoadingBarControl>();
      headerLoadingBar->SetLoading(mModelLoadInProgress || mIRLoadInProgress);
      headerLoadingBar->Tick();
    }
  }

  mInputSender.TransmitData(*this);
  mOutputSender.TransmitData(*this);

  if (mNewModelLoadedInDSP)
  {
    if (auto* pGraphics = GetUI())
    {
      _UpdateControlsFromModel();
      mNewModelLoadedInDSP = false;
    }
  }
  if (mModelCleared)
  {
    if (auto* pGraphics = GetUI())
    {
      // FIXME -- need to disable only the "normalized" model
      // pGraphics->GetControlWithTag(kCtrlTagOutputMode)->SetDisabled(false);
      if (auto* p = pGraphics->GetControlWithTag(kCtrlTagSlimmableIcon))
        p->Hide(true);
      if (auto* p = pGraphics->GetControlWithTag(kCtrlTagSlimOverlayBackdrop))
        p->Hide(true);
      if (auto* p = pGraphics->GetControlWithTag(kCtrlTagSlimKnob))
        p->Hide(true);
      _SendControlMsgIfAttached(kCtrlTagModelThumbnail, kMsgTagClearModel);
      pGraphics->SetAllControlsDirty();
      mModelCleared = false;
    }
  }
}

bool NeuralAmpModeler::SerializeState(IByteChunk& chunk) const
{
  // If this isn't here when unserializing, then we know we're dealing with something before v0.8.0.
  WDL_String header("###NeuralAmpModeler###"); // Don't change this!
  chunk.PutStr(header.Get());
  // Plugin version, so we can load legacy serialized states in the future!
  WDL_String version(PLUG_VERSION_STR);
  chunk.PutStr(version.Get());
  // Model directory (don't serialize the model itself; we'll just load it again
  // when we unserialize)
  chunk.PutStr(mNAMPath.Get());
  chunk.PutStr(mIRPath.Get());
  chunk.PutStr(mNAMRootDirectory.Get());
  chunk.PutStr(mIRRootDirectory.Get());
  return SerializeParams(chunk);
}

int NeuralAmpModeler::UnserializeState(const IByteChunk& chunk, int startPos)
{
  // Look for the expected header. If it's there, then we'll know what to do.
  WDL_String header;
  int pos = startPos;
  pos = chunk.GetStr(header, pos);

  const char* kExpectedHeader = "###NeuralAmpModeler###";
  if (strcmp(header.Get(), kExpectedHeader) == 0)
  {
    return _UnserializeStateWithKnownVersion(chunk, pos);
  }
  else
  {
    return _UnserializeStateWithUnknownVersion(chunk, startPos);
  }
}

void NeuralAmpModeler::OnUIOpen()
{
  Plugin::OnUIOpen();

  _RefreshLibrarySidebar();

  if (mNAMPath.GetLength())
  {
    _SendControlMsgIfAttached(kCtrlTagModelFileBrowser, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    _SendControlMsgIfAttached(kCtrlTagLibrarySidebar, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    _SendControlMsgIfAttached(kCtrlTagModelThumbnail, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    _SendControlMsgIfAttached(kCtrlTagMainArea, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    _SendControlMsgIfAttached(kCtrlTagSelectorArea, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    // If it's not loaded yet, then mark as failed.
    // If it's yet to be loaded, then the completion handler will set us straight once it runs.
    if (mModel == nullptr && mStagedModel == nullptr && !mModelLoadInProgress)
      _SendControlMsgIfAttached(kCtrlTagModelFileBrowser, kMsgTagLoadFailed);
  }

  if (mIRPath.GetLength())
  {
    _SendControlMsgIfAttached(kCtrlTagIRFileBrowser, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
    _SendControlMsgIfAttached(kCtrlTagLibrarySidebar, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
    _SendControlMsgIfAttached(kCtrlTagMainArea, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
    _SendControlMsgIfAttached(kCtrlTagSelectorArea, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
    if (mIR == nullptr && mStagedIR == nullptr && !mIRLoadInProgress)
      _SendControlMsgIfAttached(kCtrlTagIRFileBrowser, kMsgTagLoadFailed);
  }

  if (mModel != nullptr)
  {
    _UpdateControlsFromModel();
  }
}

void NeuralAmpModeler::_SendControlMsgIfAttached(int ctrlTag, int msgTag, int dataSize, const void* pData)
{
  if (auto* pGraphics = GetUI())
  {
    if (pGraphics->GetControlWithTag(ctrlTag) != nullptr)
      SendControlMsgFromDelegate(ctrlTag, msgTag, dataSize, pData);
  }
}

void NeuralAmpModeler::_ProcessAsyncLoadCompletions()
{
  std::unique_ptr<ResamplingNAM> completedModel;
  std::string completedModelPath;
  std::string completedModelError;
  uint64_t completedModelRequestId = 0;
  bool hasCompletedModel = false;

  std::unique_ptr<dsp::ImpulseResponse> completedIR;
  std::string completedIRPath;
  dsp::wav::LoadReturnCode completedIRState = dsp::wav::LoadReturnCode::ERROR_OTHER;
  uint64_t completedIRRequestId = 0;
  bool hasCompletedIR = false;

  {
    std::lock_guard<std::mutex> lock(mAsyncLoadMutex);
    if (mPendingModelLoadComplete)
    {
      completedModel = std::move(mPendingModel);
      completedModelPath = std::move(mPendingModelPath);
      completedModelError = std::move(mPendingModelError);
      completedModelRequestId = mPendingModelRequestId;
      mPendingModelLoadComplete = false;
      hasCompletedModel = true;
    }

    if (mPendingIRLoadComplete)
    {
      completedIR = std::move(mPendingIR);
      completedIRPath = std::move(mPendingIRPath);
      completedIRState = mPendingIRState;
      completedIRRequestId = mPendingIRRequestId;
      mPendingIRLoadComplete = false;
      hasCompletedIR = true;
    }
  }

  if (hasCompletedModel && completedModelRequestId == mModelLoadRequestId.load())
  {
    mModelLoadInProgress = false;
    if (completedModel != nullptr)
    {
      {
        std::lock_guard<std::mutex> lock(mStagingMutex);
        mStagedModel = std::move(completedModel);
        mNAMPath.Set(completedModelPath.c_str());
      }
      _SendLoadedModelMessages();
    }
    else
    {
      _SendControlMsgIfAttached(kCtrlTagModelFileBrowser, kMsgTagLoadFailed);
      std::cerr << "Failed to read DSP module" << std::endl;
      if (!completedModelError.empty())
        std::cerr << completedModelError << std::endl;
    }
  }

  if (hasCompletedIR && completedIRRequestId == mIRLoadRequestId.load())
  {
    mIRLoadInProgress = false;
    if (completedIRState == dsp::wav::LoadReturnCode::SUCCESS && completedIR != nullptr)
    {
      {
        std::lock_guard<std::mutex> lock(mStagingMutex);
        mStagedIR = std::move(completedIR);
        mIRPath.Set(completedIRPath.c_str());
      }
      _SendLoadedIRMessages();
    }
    else
    {
      _SendControlMsgIfAttached(kCtrlTagIRFileBrowser, kMsgTagLoadFailed);
    }
  }
}

void NeuralAmpModeler::_CollectFinishedLoadTasks()
{
  std::lock_guard<std::mutex> lock(mAsyncLoadMutex);
  auto task = mLoadTasks.begin();
  while (task != mLoadTasks.end())
  {
    if (task->valid() && task->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
      task->get();
      task = mLoadTasks.erase(task);
    }
    else
    {
      ++task;
    }
  }
}

void NeuralAmpModeler::_SendLoadedModelMessages()
{
  _SendControlMsgIfAttached(kCtrlTagModelFileBrowser, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  _SendControlMsgIfAttached(kCtrlTagLibrarySidebar, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  _SendControlMsgIfAttached(kCtrlTagModelThumbnail, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  _SendControlMsgIfAttached(kCtrlTagMainArea, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  _SendControlMsgIfAttached(kCtrlTagSelectorArea, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
}

void NeuralAmpModeler::_SendLoadedIRMessages()
{
  _SendControlMsgIfAttached(kCtrlTagIRFileBrowser, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
  _SendControlMsgIfAttached(kCtrlTagLibrarySidebar, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
  _SendControlMsgIfAttached(kCtrlTagMainArea, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
  _SendControlMsgIfAttached(kCtrlTagSelectorArea, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
}

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

void NeuralAmpModeler::LoadNAMFile(const WDL_String& modelPath)
{
  const uint64_t requestId = ++mModelLoadRequestId;
  const std::string path(modelPath.Get());
  const double sampleRate = GetSampleRate();
  const int blockSize = GetBlockSize();
  const double slimValue = GetParam(kSlim)->Value();
  mModelLoadInProgress = true;

  std::lock_guard<std::mutex> lock(mAsyncLoadMutex);
  mLoadTasks.emplace_back(std::async(std::launch::async, [this, requestId, path, sampleRate, blockSize, slimValue]() {
    std::unique_ptr<ResamplingNAM> loadedModel;
    std::string error;

    try
    {
      auto dspPath = std::filesystem::u8path(path);
      std::unique_ptr<nam::DSP> model = nam::get_dsp(dspPath);

      if (model->NumInputChannels() != 1)
      {
        throw std::runtime_error("Model must have 1 input channel, but has "
                                 + std::to_string(model->NumInputChannels()));
      }
      if (model->NumOutputChannels() != 1)
      {
        throw std::runtime_error("Model must have 1 output channel, but has "
                                 + std::to_string(model->NumOutputChannels()));
      }

      loadedModel = std::make_unique<ResamplingNAM>(std::move(model), sampleRate);
      loadedModel->Reset(sampleRate, blockSize);
      if (nam::SlimmableModel* slimmable = loadedModel->GetSlimmableModel())
        slimmable->SetSlimmableSize(slimValue);
    }
    catch (const std::runtime_error& e)
    {
      error = e.what();
    }
    catch (const std::exception& e)
    {
      error = e.what();
    }
    catch (...)
    {
      error = "Failed to read DSP module";
    }

    if (mShuttingDown || requestId != mModelLoadRequestId.load())
      return;

    std::lock_guard<std::mutex> resultLock(mAsyncLoadMutex);
    mPendingModelRequestId = requestId;
    mPendingModelPath = path;
    mPendingModelError = error;
    mPendingModel = std::move(loadedModel);
    mPendingModelLoadComplete = true;
  }));
}

void NeuralAmpModeler::LoadIRFile(const WDL_String& irPath)
{
  const uint64_t requestId = ++mIRLoadRequestId;
  const std::string path(irPath.Get());
  const double sampleRate = GetSampleRate();
  mIRLoadInProgress = true;

  std::lock_guard<std::mutex> lock(mAsyncLoadMutex);
  mLoadTasks.emplace_back(std::async(std::launch::async, [this, requestId, path, sampleRate]() {
    std::unique_ptr<dsp::ImpulseResponse> loadedIR;
    dsp::wav::LoadReturnCode wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;

    try
    {
      auto irPathU8 = std::filesystem::u8path(path);
      loadedIR = std::make_unique<dsp::ImpulseResponse>(irPathU8.string().c_str(), sampleRate);
      wavState = loadedIR->GetWavState();
    }
    catch (const std::runtime_error& e)
    {
      wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
      std::cerr << "Caught unhandled exception while attempting to load IR:" << std::endl;
      std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
      wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
    }

    if (wavState != dsp::wav::LoadReturnCode::SUCCESS)
      loadedIR = nullptr;

    if (mShuttingDown || requestId != mIRLoadRequestId.load())
      return;

    std::lock_guard<std::mutex> resultLock(mAsyncLoadMutex);
    mPendingIRRequestId = requestId;
    mPendingIRPath = path;
    mPendingIRState = wavState;
    mPendingIR = std::move(loadedIR);
    mPendingIRLoadComplete = true;
  }));
}

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

void NeuralAmpModeler::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    // Changes to the input gain
    case kCalibrateInput:
    case kInputCalibrationLevel:
    case kInputLevel: _SetInputGain(); break;
    // Changes to the output gain
    case kOutputLevel:
    case kOutputMode: _SetOutputGain(); break;
    // Tone stack:
    case kToneBass: mToneStack->SetParam("bass", GetParam(paramIdx)->Value()); break;
    case kToneMid: mToneStack->SetParam("middle", GetParam(paramIdx)->Value()); break;
    case kToneTreble: mToneStack->SetParam("treble", GetParam(paramIdx)->Value()); break;
    case kSlim: _ApplySlimParamToLoadedNAMs(); break;
    default: break;
  }
}

void NeuralAmpModeler::OnParamChangeUI(int paramIdx, EParamSource source)
{
  if (auto pGraphics = GetUI())
  {
    bool active = GetParam(paramIdx)->Bool();

    switch (paramIdx)
    {
      case kNoiseGateActive:
        if (auto* control = pGraphics->GetControlWithParamIdx(kNoiseGateThreshold))
          control->SetDisabled(!active);
        pGraphics->SetAllControlsDirty();
        break;
      case kEQActive:
        pGraphics->ForControlInGroup("EQ_KNOBS", [active](IControl* pControl) { pControl->SetDisabled(!active); });
        pGraphics->SetAllControlsDirty();
        break;
      case kIRToggle:
        if (auto* control = pGraphics->GetControlWithTag(kCtrlTagIRFileBrowser))
          control->SetDisabled(!active);
        break;
      default: break;
    }
  }
}

bool NeuralAmpModeler::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  switch (msgTag)
  {
    case kMsgTagClearModel: mShouldRemoveModel = true; return true;
    case kMsgTagClearIR: mShouldRemoveIR = true; return true;
    case kMsgTagHighlightColor:
    {
      mHighLightColor.Set((const char*)pData);

      if (GetUI())
      {
        GetUI()->ForStandardControlsFunc([&](IControl* pControl) {
          if (auto* pVectorBase = pControl->As<IVectorBase>())
          {
            IColor color = IColor::FromColorCodeStr(mHighLightColor.Get());

            pVectorBase->SetColor(kX1, color);
            pVectorBase->SetColor(kPR, color.WithOpacity(0.3f));
            pVectorBase->SetColor(kFR, color.WithOpacity(0.4f));
            pVectorBase->SetColor(kX3, color.WithContrast(0.1f));
          }
          pControl->GetUI()->SetAllControlsDirty();
        });
      }

      return true;
    }
    default: return false;
  }
}

// Private methods ============================================================

void NeuralAmpModeler::_AllocateIOPointers(const size_t nChans)
{
  if (mInputPointers != nullptr)
    throw std::runtime_error("Tried to re-allocate mInputPointers without freeing");
  mInputPointers = new sample*[nChans];
  if (mInputPointers == nullptr)
    throw std::runtime_error("Failed to allocate pointer to input buffer!\n");
  if (mOutputPointers != nullptr)
    throw std::runtime_error("Tried to re-allocate mOutputPointers without freeing");
  mOutputPointers = new sample*[nChans];
  if (mOutputPointers == nullptr)
    throw std::runtime_error("Failed to allocate pointer to output buffer!\n");
}

void NeuralAmpModeler::_ApplyDSPStaging()
{
  std::unique_lock<std::mutex> stagingLock(mStagingMutex, std::try_to_lock);
  if (!stagingLock.owns_lock())
    return;

  // Remove marked modules
  if (mShouldRemoveModel)
  {
    mModel = nullptr;
    mNAMPath.Set("");
    mShouldRemoveModel = false;
    mModelCleared = true;
    _UpdateLatency();
    _SetInputGain();
    _SetOutputGain();
  }
  if (mShouldRemoveIR)
  {
    mIR = nullptr;
    mIRPath.Set("");
    mShouldRemoveIR = false;
  }
  // Move things from staged to live
  if (mStagedModel != nullptr)
  {
    mModel = std::move(mStagedModel);
    mStagedModel = nullptr;
    mNewModelLoadedInDSP = true;
    _UpdateLatency();
    _SetInputGain();
    _SetOutputGain();
  }
  if (mStagedIR != nullptr)
  {
    mIR = std::move(mStagedIR);
    mStagedIR = nullptr;
  }
}

void NeuralAmpModeler::_DeallocateIOPointers()
{
  if (mInputPointers != nullptr)
  {
    delete[] mInputPointers;
    mInputPointers = nullptr;
  }
  if (mInputPointers != nullptr)
    throw std::runtime_error("Failed to deallocate pointer to input buffer!\n");
  if (mOutputPointers != nullptr)
  {
    delete[] mOutputPointers;
    mOutputPointers = nullptr;
  }
  if (mOutputPointers != nullptr)
    throw std::runtime_error("Failed to deallocate pointer to output buffer!\n");
}

void NeuralAmpModeler::_FallbackDSP(iplug::sample** inputs, iplug::sample** outputs, const size_t numChannels,
                                    const size_t numFrames)
{
  for (auto c = 0; c < numChannels; c++)
    for (auto s = 0; s < numFrames; s++)
      mOutputArray[c][s] = mInputArray[c][s];
}

void NeuralAmpModeler::_ResetModelAndIR(const double sampleRate, const int maxBlockSize)
{
  std::lock_guard<std::mutex> stagingLock(mStagingMutex);

  // Model
  if (mStagedModel != nullptr)
  {
    mStagedModel->Reset(sampleRate, maxBlockSize);
  }
  else if (mModel != nullptr)
  {
    mModel->Reset(sampleRate, maxBlockSize);
  }

  // IR
  if (mStagedIR != nullptr)
  {
    const double irSampleRate = mStagedIR->GetSampleRate();
    if (irSampleRate != sampleRate)
    {
      const auto irData = mStagedIR->GetData();
      mStagedIR = std::make_unique<dsp::ImpulseResponse>(irData, sampleRate);
    }
  }
  else if (mIR != nullptr)
  {
    const double irSampleRate = mIR->GetSampleRate();
    if (irSampleRate != sampleRate)
    {
      const auto irData = mIR->GetData();
      mStagedIR = std::make_unique<dsp::ImpulseResponse>(irData, sampleRate);
    }
  }
}

void NeuralAmpModeler::_SetInputGain()
{
  iplug::sample inputGainDB = GetParam(kInputLevel)->Value();
  // Input calibration
  if ((mModel != nullptr) && (mModel->HasInputLevel()) && GetParam(kCalibrateInput)->Bool())
  {
    inputGainDB += GetParam(kInputCalibrationLevel)->Value() - mModel->GetInputLevel();
  }
  mInputGain = DBToAmp(inputGainDB);
}

void NeuralAmpModeler::_SetOutputGain()
{
  double gainDB = GetParam(kOutputLevel)->Value();
  if (mModel != nullptr)
  {
    const int outputMode = GetParam(kOutputMode)->Int();
    switch (outputMode)
    {
      case 1: // Normalized
        if (mModel->HasLoudness())
        {
          const double loudness = mModel->GetLoudness();
          const double targetLoudness = -18.0;
          gainDB += (targetLoudness - loudness);
        }
        break;
      case 2: // Calibrated
        if (mModel->HasOutputLevel())
        {
          const double inputLevel = GetParam(kInputCalibrationLevel)->Value();
          const double outputLevel = mModel->GetOutputLevel();
          gainDB += (outputLevel - inputLevel);
        }
        break;
      case 0: // Raw
      default: break;
    }
  }
  mOutputGain = DBToAmp(gainDB);
}

void NeuralAmpModeler::_ApplySlimParamToLoadedNAMs()
{
  const double v = GetParam(kSlim)->Value();
  auto apply = [v](ResamplingNAM* p) {
    if (p == nullptr)
      return;
    if (nam::SlimmableModel* s = p->GetSlimmableModel())
      s->SetSlimmableSize(v);
  };
  apply(mModel.get());
  std::lock_guard<std::mutex> stagingLock(mStagingMutex);
  apply(mStagedModel.get());
}

size_t NeuralAmpModeler::_GetBufferNumChannels() const
{
  // Assumes input=output (no mono->stereo effects)
  return mInputArray.size();
}

size_t NeuralAmpModeler::_GetBufferNumFrames() const
{
  if (_GetBufferNumChannels() == 0)
    return 0;
  return mInputArray[0].size();
}

void NeuralAmpModeler::_InitToneStack()
{
  // If you want to customize the tone stack, then put it here!
  mToneStack = std::make_unique<dsp::tone_stack::BasicNamToneStack>();
}
void NeuralAmpModeler::_PrepareBuffers(const size_t numChannels, const size_t numFrames)
{
  const bool updateChannels = numChannels != _GetBufferNumChannels();
  const bool updateFrames = updateChannels || (_GetBufferNumFrames() != numFrames);
  //  if (!updateChannels && !updateFrames)  // Could we do this?
  //    return;

  if (updateChannels)
  {
    _PrepareIOPointers(numChannels);
    mInputArray.resize(numChannels);
    mOutputArray.resize(numChannels);
  }
  if (updateFrames)
  {
    for (auto c = 0; c < mInputArray.size(); c++)
    {
      mInputArray[c].resize(numFrames);
      std::fill(mInputArray[c].begin(), mInputArray[c].end(), 0.0);
    }
    for (auto c = 0; c < mOutputArray.size(); c++)
    {
      mOutputArray[c].resize(numFrames);
      std::fill(mOutputArray[c].begin(), mOutputArray[c].end(), 0.0);
    }
  }
  // Would these ever get changed by something?
  for (auto c = 0; c < mInputArray.size(); c++)
    mInputPointers[c] = mInputArray[c].data();
  for (auto c = 0; c < mOutputArray.size(); c++)
    mOutputPointers[c] = mOutputArray[c].data();
}

void NeuralAmpModeler::_PrepareIOPointers(const size_t numChannels)
{
  _DeallocateIOPointers();
  _AllocateIOPointers(numChannels);
}

void NeuralAmpModeler::_ProcessInput(iplug::sample** inputs, const size_t nFrames, const size_t nChansIn,
                                     const size_t nChansOut)
{
  // We'll assume that the main processing is mono for now. We'll handle dual amps later.
  if (nChansOut != 1)
  {
    std::stringstream ss;
    ss << "Expected mono output, but " << nChansOut << " output channels are requested!";
    throw std::runtime_error(ss.str());
  }

  // On the standalone, we can probably assume that the user has plugged into only one input and they expect it to be
  // carried straight through. Don't apply any division over nChansIn because we're just "catching anything out there."
  // However, in a DAW, it's probably something providing stereo, and we want to take the average in order to avoid
  // doubling the loudness. (This would change w/ double mono processing)
  double gain = mInputGain;
#ifndef APP_API
  gain /= (float)nChansIn;
#endif
  // Assume _PrepareBuffers() was already called
  for (size_t c = 0; c < nChansIn; c++)
    for (size_t s = 0; s < nFrames; s++)
      if (c == 0)
        mInputArray[0][s] = gain * inputs[c][s];
      else
        mInputArray[0][s] += gain * inputs[c][s];
}

void NeuralAmpModeler::_ProcessOutput(iplug::sample** inputs, iplug::sample** outputs, const size_t nFrames,
                                      const size_t nChansIn, const size_t nChansOut)
{
  const double gain = mOutputGain;
  // Assume _PrepareBuffers() was already called
  if (nChansIn != 1)
    throw std::runtime_error("Plugin is supposed to process in mono.");
  // Broadcast the internal mono stream to all output channels.
  const size_t cin = 0;
  for (auto cout = 0; cout < nChansOut; cout++)
    for (auto s = 0; s < nFrames; s++)
#ifdef APP_API // Ensure valid output to interface
      outputs[cout][s] = std::clamp(gain * inputs[cin][s], -1.0, 1.0);
#else // In a DAW, other things may come next and should be able to handle large
      // values.
      outputs[cout][s] = gain * inputs[cin][s];
#endif
}

void NeuralAmpModeler::_UpdateControlsFromModel()
{
  if (mModel == nullptr)
  {
    return;
  }
  if (auto* pGraphics = GetUI())
  {
    const bool disableInputCalibrationControls = !mModel->HasInputLevel();
    if (auto* calibrateInput = pGraphics->GetControlWithTag(kCtrlTagCalibrateInput))
      calibrateInput->SetDisabled(disableInputCalibrationControls);
    if (auto* inputCalibrationLevel = pGraphics->GetControlWithTag(kCtrlTagInputCalibrationLevel))
      inputCalibrationLevel->SetDisabled(disableInputCalibrationControls);
    if (auto* outputMode = pGraphics->GetControlWithTag(kCtrlTagOutputMode))
    {
      auto* c = static_cast<OutputModeControl*>(outputMode);
      c->SetNormalizedDisable(!mModel->HasLoudness());
      c->SetCalibratedDisable(!mModel->HasOutputLevel());
    }

    if (auto* pSlimIcon = pGraphics->GetControlWithTag(kCtrlTagSlimmableIcon))
    {
      const bool show = mModel->GetSlimmableModel() != nullptr;
      pSlimIcon->Hide(!show);
    }
  }
}

void NeuralAmpModeler::_UpdateLatency()
{
  int latency = 0;
  if (mModel)
  {
    latency += mModel->GetLatency();
  }
  // Other things that add latency here...

  // Feels weird to have to do this.
  if (GetLatency() != latency)
  {
    SetLatency(latency);
  }
}

void NeuralAmpModeler::_UpdateMeters(sample** inputPointer, sample** outputPointer, const size_t nFrames,
                                     const size_t nChansIn, const size_t nChansOut)
{
  // Right now, we didn't specify MAXNC when we initialized these, so it's 1.
  const int nChansHack = 1;
  mInputSender.ProcessBlock(inputPointer, (int)nFrames, kCtrlTagInputMeter, nChansHack);
  mOutputSender.ProcessBlock(outputPointer, (int)nFrames, kCtrlTagOutputMeter, nChansHack);
}

// HACK
#include "Unserialization.cpp"
