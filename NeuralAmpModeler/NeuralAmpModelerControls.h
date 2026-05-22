#pragma once

#include <algorithm>
#include <cmath> // std::round
#include <cstdio> // FILE, fclose
#include <cstdint>
#include <cctype>
#include <filesystem>
#include <functional>
#include <limits>
#include <sstream> // std::stringstream
#include <unordered_map> // std::unordered_map
#include <vector>
#include "IControls.h"
#include "IPlugPaths.h"

#ifdef OS_MAC
  #include <CoreFoundation/CoreFoundation.h>
  #include <ImageIO/ImageIO.h>
#endif

#ifdef OS_WIN
  #include <Windows.h>
  #include <Shellapi.h>
#endif

#define PLUG() static_cast<PLUG_CLASS_NAME*>(GetDelegate())
#define NAM_KNOB_HEIGHT 120.0f
#define NAM_SWTICH_HEIGHT 50.0f

using namespace iplug;
using namespace igraphics;

enum class NAMBrowserState
{
  Empty, // when no file loaded, show "Get" button
  Loaded // when file loaded, show "Clear" button
};

// Where the corner button on the plugin (settings, close settings) goes
// :param rect: Rect for the whole plugin's UI
IRECT CornerButtonArea(const IRECT& rect)
{
  const auto mainArea = rect.GetPadded(-20);
  return mainArea.GetFromTRHC(50, 50).GetCentredInside(20, 20);
};

class NAMSquareButtonControl : public ISVGButtonControl
{
public:
  NAMSquareButtonControl(const IRECT& bounds, IActionFunction af, const ISVG& svg)
  : ISVGButtonControl(bounds, af, svg, svg)
  {
  }

  void Draw(IGraphics& g) override
  {
    if (mMouseIsOver)
      g.FillRoundRect(PluginColors::MOUSEOVER, mRECT, 2.f);

    ISVGButtonControl::Draw(g);
  }
};

class NAMCircleButtonControl : public ISVGButtonControl
{
public:
  NAMCircleButtonControl(const IRECT& bounds, IActionFunction af, const ISVG& svg)
  : ISVGButtonControl(bounds, af, svg, svg)
  {
  }

  void Draw(IGraphics& g) override
  {
    if (mMouseIsOver)
      g.FillEllipse(PluginColors::MOUSEOVER, mRECT);

    ISVGButtonControl::Draw(g);
  }
};

/// Full-window dim layer; click dismisses (used for Slim overlay).
class NAMSlimOverlayBackdropControl : public IControl
{
public:
  NAMSlimOverlayBackdropControl(const IRECT& bounds, IActionFunction dismiss)
  : IControl(bounds, dismiss)
  , mDismiss(dismiss)
  {
  }

  void Draw(IGraphics& g) override { g.FillRect(COLOR_BLACK.WithOpacity(0.45f), mRECT); }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mDismiss)
      mDismiss(this);
  }

private:
  IActionFunction mDismiss;
};

class NAMKnobControl : public IVKnobControl, public IBitmapBase
{
public:
  NAMKnobControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, IBitmap bitmap)
  : IVKnobControl(bounds, paramIdx, label, style, true)
  , IBitmapBase(bitmap)
  {
    mInnerPointerFrac = 0.75f;
    mTrackSize = 1.f;
  }

  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }

  void DrawIndicatorTrack(IGraphics& g, float angle, float cx, float cy, float radius) override
  {
    if (mTrackSize > 0.f)
      g.DrawArc(COLOR_BLACK, cx, cy, radius, std::min(angle, mAnchorAngle), std::max(angle, mAnchorAngle), &mBlend,
                mTrackSize);
  }

  void DrawWidget(IGraphics& g) override
  {
    static constexpr float indicatorRadius = 3.f;
    float widgetRadius = GetRadius() * 1.f;
    auto knobRect = mWidgetBounds.GetCentredInside(mWidgetBounds.W(), mWidgetBounds.W());
    const float cx = knobRect.MW(), cy = knobRect.MH();
    const float angle = mAngle1 + (static_cast<float>(GetValue()) * (mAngle2 - mAngle1));
    DrawIndicatorTrack(g, angle, cx + 0.5, cy, widgetRadius);
    g.DrawFittedBitmap(mBitmap, knobRect);
    float data[2][2];
    RadialPoints(angle, cx, cy, mInnerPointerFrac * widgetRadius, mInnerPointerFrac * widgetRadius, 2, data);
    g.FillCircle(COLOR_BLACK, data[1][0], data[1][1], indicatorRadius, &mBlend);
  }
};

class NAMSwitchControl : public IVSlideSwitchControl, public IBitmapBase
{
public:
  NAMSwitchControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, IBitmap bitmap)
  : IVSlideSwitchControl(bounds, paramIdx, label,
                         style.WithRoundness(0.666f)
                           .WithShowValue(false)
                           .WithEmboss(true)
                           .WithShadowOffset(1.5f)
                           .WithDrawShadows(false)
                           .WithColor(kFR, COLOR_BLACK)
                           .WithFrameThickness(0.5f)
                           .WithWidgetFrac(0.5f)
                           .WithLabelOrientation(EOrientation::South))
  , IBitmapBase(bitmap)
  {
  }

  void DrawWidget(IGraphics& g) override
  {
    DrawTrack(g, mWidgetBounds);
    DrawHandle(g, mHandleBounds);
  }

  void DrawTrack(IGraphics& g, const IRECT& bounds) override
  {
    const IColor activeTrackColor(255, 218, 218, 218);
    IRECT handleBounds = GetAdjustedHandleBounds(bounds);
    handleBounds = IRECT(handleBounds.L, handleBounds.T, handleBounds.R, handleBounds.T + mBitmap.H());
    IRECT centreBounds = handleBounds.GetPadded(-mStyle.shadowOffset);
    IRECT shadowBounds = handleBounds.GetTranslated(mStyle.shadowOffset, mStyle.shadowOffset);
    //    const float contrast = mDisabled ? -GRAYED_ALPHA : 0.f;
    float cR = 7.f;
    const float tlr = cR;
    const float trr = cR;
    const float blr = cR;
    const float brr = cR;

    // outer shadow
    if (mStyle.drawShadows)
      g.FillRoundRect(GetColor(kSH), shadowBounds, tlr, trr, blr, brr, &mBlend);

    // Embossed style unpressed
    if (mStyle.emboss)
    {
      // Positive light
      g.FillRoundRect(GetColor(kPR), handleBounds, tlr, trr, blr, brr /*, &blend*/);

      // Negative light
      g.FillRoundRect(GetColor(kSH), shadowBounds, tlr, trr, blr, brr /*, &blend*/);

      // Fill in foreground
      g.FillRoundRect(GetValue() > 0.5 ? activeTrackColor : COLOR_BLACK, centreBounds, tlr, trr, blr, brr, &mBlend);

      // Shade when hovered
      if (mMouseIsOver)
        g.FillRoundRect(GetColor(kHL), centreBounds, tlr, trr, blr, brr, &mBlend);
    }
    else
    {
      g.FillRoundRect(GetValue() > 0.5 ? activeTrackColor : COLOR_BLACK, handleBounds, tlr, trr, blr, brr /*, &blend*/);

      // Shade when hovered
      if (mMouseIsOver)
        g.FillRoundRect(GetColor(kHL), handleBounds, tlr, trr, blr, brr, &mBlend);
    }

    if (mStyle.drawFrame)
      g.DrawRoundRect(GetColor(kFR), handleBounds, tlr, trr, blr, brr, &mBlend, mStyle.frameThickness);
  }

  void DrawHandle(IGraphics& g, const IRECT& filledArea) override
  {
    IRECT r;
    if (GetSelectedIdx() == 0)
    {
      r = filledArea.GetFromLeft(mBitmap.W());
    }
    else
    {
      r = filledArea.GetFromRight(mBitmap.W());
    }

    g.DrawBitmap(mBitmap, r, 0, 0, nullptr);
  }
};

class NAMFileNameControl : public IVButtonControl
{
public:
  NAMFileNameControl(const IRECT& bounds, const char* label, const IVStyle& style)
  : IVButtonControl(bounds, DefaultClickActionFunc, label, style)
  {
  }

  void SetLabelAndTooltip(const char* str)
  {
    SetLabelStr(str);
    SetTooltip(str);
  }

  void SetLabelAndTooltipEllipsizing(const WDL_String& fileName)
  {
    auto EllipsizeFilePath = [](const char* filePath, size_t prefixLength, size_t suffixLength, size_t maxLength) {
      const std::string ellipses = "...";
      assert(maxLength <= (prefixLength + suffixLength + ellipses.size()));
      std::string str{filePath};

      if (str.length() <= maxLength)
      {
        return str;
      }
      else
      {
        return str.substr(0, prefixLength) + ellipses + str.substr(str.length() - suffixLength);
      }
    };

    auto ellipsizedFileName = EllipsizeFilePath(fileName.get_filepart(), 22, 22, 45);
    SetLabelStr(ellipsizedFileName.c_str());
    SetTooltip(fileName.get_filepart());
  }
};

class NAMFileBrowserControl : public IDirBrowseControlBase
{
public:
  NAMFileBrowserControl(const IRECT& bounds, int clearMsgTag, const char* labelStr, const char* fileExtension,
                        IFileDialogCompletionHandlerFunc ch, const IVStyle& style, const ISVG& loadSVG,
                        const ISVG& clearSVG, const ISVG& leftSVG, const ISVG& rightSVG, const IBitmap& bitmap,
                        bool scanRecursively = false)
  : IDirBrowseControlBase(bounds, fileExtension, false, scanRecursively)
  , mClearMsgTag(clearMsgTag)
  , mDefaultLabelStr(labelStr)
  , mCompletionHandlerFunc(ch)
  , mStyle(style.WithColor(kFG, COLOR_TRANSPARENT).WithDrawFrame(false))
  , mBitmap(bitmap)
  , mLoadSVG(loadSVG)
  , mClearSVG(clearSVG)
  , mLeftSVG(leftSVG)
  , mRightSVG(rightSVG)
  , mBrowserState(NAMBrowserState::Empty)
  {
    mIgnoreMouse = true;
  }

  void Draw(IGraphics& g) override
  {
    g.FillRoundRect(COLOR_WHITE, mRECT, 5.f);
    g.DrawRoundRect(PluginColors::NAM_THEMECOLOR.WithOpacity(0.22f), mRECT, 5.f, &mBlend, 1.f);
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu)
    {
      IPopupMenu::Item* pItem = pSelectedMenu->GetChosenItem();

      if (pItem)
      {
        mSelectedItemIndex = mItems.Find(pItem);
        LoadFileAtCurrentIndex();
      }
    }
  }

  void OnAttached() override
  {
    auto prevFileFunc = [&](IControl* pCaller) {
      const auto nItems = NItems();
      if (nItems == 0)
        return;
      mSelectedItemIndex--;

      if (mSelectedItemIndex < 0)
        mSelectedItemIndex = nItems - 1;

      LoadFileAtCurrentIndex();
    };

    auto nextFileFunc = [&](IControl* pCaller) {
      const auto nItems = NItems();
      if (nItems == 0)
        return;
      mSelectedItemIndex++;

      if (mSelectedItemIndex >= nItems)
        mSelectedItemIndex = 0;

      LoadFileAtCurrentIndex();
    };

    auto loadFileFunc = [&](IControl* pCaller) {
      WDL_String fileName;
      WDL_String path;
      GetSelectedFileDirectory(path);
#ifdef NAM_PICK_DIRECTORY
      pCaller->GetUI()->PromptForDirectory(path, [&](const WDL_String& fileName, const WDL_String& path) {
        if (path.GetLength())
        {
          ClearPathList();
          AddPath(path.Get(), "");
          SetupMenu();
          SelectFirstFile();
          LoadFileAtCurrentIndex();
        }
      });
#else
      pCaller->GetUI()->PromptForFile(
        fileName, path, EFileAction::Open, mExtension.Get(), [&](const WDL_String& fileName, const WDL_String& path) {
          if (fileName.GetLength())
          {
            ClearPathList();
            AddPath(path.Get(), "");
            SetupMenu();
            SetSelectedFile(fileName.Get());
            LoadFileAtCurrentIndex();
          }
        });
#endif
    };

    auto clearFileFunc = [&](IControl* pCaller) {
      pCaller->GetDelegate()->SendArbitraryMsgFromUI(mClearMsgTag);
      mFileNameControl->SetLabelAndTooltip(mDefaultLabelStr.Get());
      SetBrowserState(NAMBrowserState::Empty);
      // FIXME disabling output mode...
      //      pCaller->GetUI()->GetControlWithTag(kCtrlTagOutputMode)->SetDisabled(false);
    };

    auto chooseFileFunc = [&, loadFileFunc](IControl* pCaller) {
      if (std::string_view(pCaller->As<IVButtonControl>()->GetLabelStr()) == mDefaultLabelStr.Get())
      {
        loadFileFunc(pCaller);
      }
      else
      {
        CheckSelectedItem();

        if (!mMainMenu.HasSubMenus())
        {
          mMainMenu.SetChosenItemIdx(mSelectedItemIndex);
        }
        pCaller->GetUI()->CreatePopupMenu(*this, mMainMenu, pCaller->GetRECT());
      }
    };

    IRECT padded = mRECT.GetPadded(-6.f).GetHPadded(-2.f);
    const auto buttonWidth = padded.H();
    const auto loadFileButtonBounds = padded.ReduceFromLeft(buttonWidth);
    const auto clearButtonBounds = padded.ReduceFromRight(buttonWidth);
    const auto leftButtonBounds = padded.ReduceFromLeft(buttonWidth);
    const auto rightButtonBounds = padded.ReduceFromLeft(buttonWidth);
    const auto fileNameButtonBounds = padded;

    AddChildControl(new NAMSquareButtonControl(loadFileButtonBounds, DefaultClickActionFunc, mLoadSVG))
      ->SetAnimationEndActionFunction(loadFileFunc);
    AddChildControl(new NAMSquareButtonControl(leftButtonBounds, DefaultClickActionFunc, mLeftSVG))
      ->SetAnimationEndActionFunction(prevFileFunc);
    AddChildControl(new NAMSquareButtonControl(rightButtonBounds, DefaultClickActionFunc, mRightSVG))
      ->SetAnimationEndActionFunction(nextFileFunc);
    AddChildControl(mFileNameControl = new NAMFileNameControl(fileNameButtonBounds, mDefaultLabelStr.Get(), mStyle))
      ->SetAnimationEndActionFunction(chooseFileFunc);

    mClearButton = new NAMSquareButtonControl(clearButtonBounds, DefaultClickActionFunc, mClearSVG);
    mClearButton->SetAnimationEndActionFunction(clearFileFunc);
    AddChildControl(mClearButton);

    // initialize control visibility
    SetBrowserState(NAMBrowserState::Empty);
  }

  void LoadFileAtCurrentIndex()
  {
    if (mSelectedItemIndex > -1 && mSelectedItemIndex < NItems())
    {
      WDL_String fileName, path;
      GetSelectedFile(fileName);
      mFileNameControl->SetLabelAndTooltipEllipsizing(fileName);
      mCompletionHandlerFunc(fileName, path);
    }
  }

  void LoadDirectory(const char* directory)
  {
    if (!CStringHasContents(directory))
      return;

    ClearPathList();
    AddPath(directory, "");
    SetupMenu();
    SelectFirstFile();
    LoadFileAtCurrentIndex();
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    switch (msgTag)
    {
      case kMsgTagLoadFailed:
        // Honestly, not sure why I made a big stink of it before. Why not just say it failed and move on? :)
        {
          std::string label(std::string("(FAILED) ") + std::string(mFileNameControl->GetLabelStr()));
          mFileNameControl->SetLabelAndTooltip(label.c_str());
          SetBrowserState(NAMBrowserState::Empty);
        }
        break;
      case kMsgTagLoadedModel:
      case kMsgTagLoadedIR:
      {
        WDL_String fileName, directory;
        fileName.Set(reinterpret_cast<const char*>(pData));
        directory.Set(reinterpret_cast<const char*>(pData));
        directory.remove_filepart(true);

        ClearPathList();
        AddPath(directory.Get(), "");
        SetupMenu();
        SetSelectedFile(fileName.Get());
        mFileNameControl->SetLabelAndTooltipEllipsizing(fileName);
        SetBrowserState(NAMBrowserState::Loaded);
      }
      break;
      default: break;
    }
  }

private:
  void SelectFirstFile() { mSelectedItemIndex = mFiles.GetSize() ? 0 : -1; }

  void GetSelectedFileDirectory(WDL_String& path)
  {
    GetSelectedFile(path);
    path.remove_filepart();
    return;
  }

  // set the state of the browser and the visibility of the clear button
  void SetBrowserState(NAMBrowserState newState)
  {
    mBrowserState = newState;

    switch (mBrowserState)
    {
      case NAMBrowserState::Empty:
        mClearButton->Hide(true);
        break;
      case NAMBrowserState::Loaded:
        mClearButton->Hide(false);
        break;
    }
  }

  WDL_String mDefaultLabelStr;
  IFileDialogCompletionHandlerFunc mCompletionHandlerFunc;
  NAMFileNameControl* mFileNameControl = nullptr;
  IVStyle mStyle;
  IBitmap mBitmap;
  ISVG mLoadSVG, mClearSVG, mLeftSVG, mRightSVG;
  int mClearMsgTag;

  NAMBrowserState mBrowserState;
  NAMSquareButtonControl* mClearButton = nullptr;
};

class NAMModelThumbnailControl : public IControl
{
public:
  NAMModelThumbnailControl(const IRECT& bounds)
  : IControl(bounds)
  {
  }

  void SetNAMRootDirectory(const char* directory)
  {
    mNAMRootDirectory = NormalizePath(directory);

    if (!mLoadedModelPath.empty())
      LoadThumbnailForModelPath(mLoadedModelPath);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    switch (msgTag)
    {
      case kMsgTagLoadedModel:
        if (pData != nullptr)
        {
          mLoadedModelPath = NormalizePath(reinterpret_cast<const char*>(pData));
          LoadThumbnailForModelPath(mLoadedModelPath);
        }
        break;
      case kMsgTagClearModel:
        mLoadedModelPath.clear();
        SetThumbnailPath("");
        break;
      default: break;
    }
  }

  void Draw(IGraphics& g) override
  {
    const IColor placeholderColor(255, 255, 255, 255);
    const IColor borderColor(255, 214, 214, 214);
    g.FillRoundRect(placeholderColor, mRECT, 5.f);

    const IBitmap thumbnail = GetThumbnail();
    if (thumbnail.IsValid())
      g.DrawFittedBitmap(thumbnail, GetAspectFitRect(thumbnail, mRECT.GetPadded(-2.f)), &mBlend);

    g.DrawRoundRect(borderColor, mRECT, 5.f, &mBlend, 1.f);
  }

private:
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

  static bool IsWebPFile(const std::filesystem::path& path)
  {
    return ToLower(path.extension().string()) == ".webp";
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

  static IRECT GetAspectFitRect(const IBitmap& bitmap, const IRECT& bounds)
  {
    const float bitmapWidth = static_cast<float>(bitmap.W());
    const float bitmapHeight = static_cast<float>(bitmap.H());

    if (bitmapWidth <= 0.f || bitmapHeight <= 0.f || bounds.W() <= 0.f || bounds.H() <= 0.f)
      return bounds;

    const float scale = std::min(bounds.W() / bitmapWidth, bounds.H() / bitmapHeight);
    return bounds.GetCentredInside(bitmapWidth * scale, bitmapHeight * scale);
  }

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

  static bool PathStartsWith(const std::filesystem::path& path, const std::filesystem::path& prefix)
  {
    auto pathIt = path.begin();

    for (auto prefixIt = prefix.begin(); prefixIt != prefix.end(); ++prefixIt, ++pathIt)
    {
      if (pathIt == path.end() || *pathIt != *prefixIt)
        return false;
    }

    return true;
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

  std::filesystem::path GetThumbnailSearchDirectory(const std::string& modelPath) const
  {
    const auto model = std::filesystem::u8path(modelPath).lexically_normal();
    const auto modelDirectory = model.parent_path();

    if (mNAMRootDirectory.empty())
      return modelDirectory;

    const auto root = std::filesystem::u8path(mNAMRootDirectory).lexically_normal();
    if (!PathStartsWith(model, root))
      return modelDirectory;

    const auto relative = model.lexically_relative(root);
    auto firstPart = relative.begin();
    if (firstPart == relative.end())
      return modelDirectory;

    auto secondPart = firstPart;
    ++secondPart;
    if (secondPart == relative.end())
      return root;

    return root / *firstPart;
  }

  void LoadThumbnailForModelPath(const std::string& modelPath)
  {
    if (modelPath.empty())
    {
      SetThumbnailPath("");
      return;
    }

    std::error_code ec;
    const auto searchDirectory = GetThumbnailSearchDirectory(modelPath);
    if (!std::filesystem::is_directory(searchDirectory, ec))
    {
      SetThumbnailPath("");
      return;
    }

    std::filesystem::path thumbnailPath;
    if (FindFirstImage(searchDirectory, thumbnailPath))
      SetThumbnailPath(PathString(thumbnailPath));
    else
      SetThumbnailPath("");
  }

  void SetThumbnailPath(const std::string& path)
  {
    if (path == mThumbnailPath)
      return;

    mThumbnailPath = path;
    mThumbnail = IBitmap();
    mThumbnailData.clear();
    mThumbnailCacheName.clear();
    SetDirty(false);
  }

	  IBitmap GetThumbnail()
	  {
	    if (mThumbnail.IsValid() || mThumbnailPath.empty())
	      return mThumbnail;

	    std::error_code ec;
	    const auto path = std::filesystem::u8path(mThumbnailPath);
	    if (std::filesystem::is_regular_file(path, ec))
	    {
#ifdef OS_MAC
	      if (IsImageFile(path) && EncodeImageAsPNG(path, mThumbnailData))
	      {
	        mThumbnailCacheName = mThumbnailPath + ".decoded.png";
	        mThumbnail = GetUI()->LoadBitmap(mThumbnailCacheName.c_str(), mThumbnailData.data(),
	                                         static_cast<int>(mThumbnailData.size()), 1, false, 1);
	      }
#else
	      if (CanLoadThumbnailFile(path))
	        mThumbnail = GetUI()->LoadBitmap(mThumbnailPath.c_str());
#endif
	    }

	    if (mThumbnail.IsValid() && (mThumbnail.W() <= 0 || mThumbnail.H() <= 0))
	      mThumbnail = IBitmap();

	    return mThumbnail;
	  }

  std::string mNAMRootDirectory;
  std::string mLoadedModelPath;
  std::string mThumbnailPath;
  std::string mThumbnailCacheName;
  std::vector<uint8_t> mThumbnailData;
  IBitmap mThumbnail;
};

class NAMLibraryDrawerButtonControl : public IControl
{
public:
  NAMLibraryDrawerButtonControl(const IRECT& bounds, int sidebarTag)
  : IControl(bounds)
  , mSidebarTag(sidebarTag)
  {
    SetTooltip("Library");
  }

  void Draw(IGraphics& g) override
  {
    const IColor borderColor(255, 214, 214, 214);
    const IColor hoverColor(255, 244, 244, 244);
    const IColor inkColor(255, 20, 20, 20);
    const auto bg = mMouseIsOver ? hoverColor : COLOR_WHITE;

    g.FillRoundRect(bg, mRECT, 5.f);
    g.DrawRoundRect(borderColor, mRECT, 5.f, &mBlend, 1.f);

    const auto icon = mRECT.GetPadded(-8.f);
    const float lineLeft = icon.L;
    const float lineRight = icon.MW() + 1.f;
    const float lineY = icon.T + 3.f;
    g.DrawLine(inkColor, lineLeft, lineY, lineRight, lineY, &mBlend, 1.5f);
    g.DrawLine(inkColor, lineLeft, icon.MH(), lineRight, icon.MH(), &mBlend, 1.5f);
    g.DrawLine(inkColor, lineLeft, icon.B - 3.f, lineRight, icon.B - 3.f, &mBlend, 1.5f);

    if (IsDrawerOpen())
      g.FillTriangle(inkColor, icon.R, icon.T + 2.f, icon.R, icon.B - 2.f, icon.MW() + 4.f, icon.MH(), &mBlend);
    else
      g.FillTriangle(inkColor, icon.MW() + 4.f, icon.T + 2.f, icon.MW() + 4.f, icon.B - 2.f, icon.R, icon.MH(), &mBlend);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (auto* ui = GetUI())
    {
      if (auto* sidebar = ui->GetControlWithTag(mSidebarTag))
      {
        sidebar->Hide(!sidebar->IsHidden());
        ui->SetAllControlsDirty();
      }
    }
  }

private:
  bool IsDrawerOpen() const
  {
    if (auto* ui = GetUI())
    {
      if (auto* sidebar = ui->GetControlWithTag(mSidebarTag))
        return !sidebar->IsHidden();
    }

    return false;
  }

  int mSidebarTag;
};

class NAMLibrarySidebarControl : public IControl
{
public:
  using LoadDirectoryFunc = std::function<void(const char*)>;

  NAMLibrarySidebarControl(const IRECT& bounds, LoadDirectoryFunc loadNAMDirectory, LoadDirectoryFunc loadIRDirectory)
  : IControl(bounds)
  , mLoadNAMDirectory(std::move(loadNAMDirectory))
  , mLoadIRDirectory(std::move(loadIRDirectory))
  {
  }

  void SetRoots(const char* namRootDirectory, const char* irRootDirectory)
  {
    const std::string namRoot = NormalizePath(namRootDirectory);
    const std::string irRoot = NormalizePath(irRootDirectory);

    if (namRoot == mNAMRootDirectory && irRoot == mIRRootDirectory)
      return;

    mNAMRootDirectory = namRoot;
    mIRRootDirectory = irRoot;
    Rescan();
    SetDirty(false);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (pData == nullptr)
      return;

    switch (msgTag)
    {
      case kMsgTagLoadedModel:
        mSelectedNAMDirectory = ParentDirectory(reinterpret_cast<const char*>(pData));
        SetDirty(false);
        break;
      case kMsgTagLoadedIR:
        mSelectedIRDirectory = ParentDirectory(reinterpret_cast<const char*>(pData));
        SetDirty(false);
        break;
      default: break;
    }
  }

  void Draw(IGraphics& g) override
  {
    g.FillRoundRect(COLOR_WHITE, mRECT, 6.f);
    g.DrawRoundRect(PluginColors::NAM_THEMECOLOR.WithOpacity(0.22f), mRECT, 6.f, &mBlend, 1.f);
    const auto splitLine = IRECT(mRECT.L + 1.f, GetLibraryArea(LibraryKind::IR).T, mRECT.R - 1.f,
                                 GetLibraryArea(LibraryKind::IR).T + 1.f);
    g.FillRect(PluginColors::NAM_THEMECOLOR.WithOpacity(0.22f), splitLine);

    mRows.clear();
    const IText sectionText(14.f, COLOR_BLACK, "Roboto-Regular", EAlign::Near, EVAlign::Middle);
    const IText rowText(11.f, PluginColors::NAM_THEMEFONTCOLOR, "Roboto-Regular", EAlign::Near);
    const IText cardTitleText(10.f, PluginColors::NAM_THEMEFONTCOLOR, "Roboto-Regular", EAlign::Center,
                              EVAlign::Middle);
    const IText mutedText(10.f, PluginColors::NAM_THEMEFONTCOLOR.WithOpacity(0.58f), "Roboto-Regular", EAlign::Near);

    DrawLibrary(g, LibraryKind::NAM, mNAMRootDirectory, mNAMFolders, mNAMReadError, mNAMScroll, mNAMContentHeight,
                rowText, mutedText, cardTitleText);
    DrawLibrary(g, LibraryKind::IR, mIRRootDirectory, mIRFolders, mIRReadError, mIRScroll, mIRContentHeight, rowText,
                mutedText, cardTitleText);
    DrawSectionTitle(g, LibraryKind::NAM, "Amps", sectionText);
    DrawSectionTitle(g, LibraryKind::IR, "Cabs", sectionText);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    for (auto& row : mRows)
    {
      if (!row.rect.Contains(x, y) || row.type != RowType::Folder || row.node == nullptr)
        continue;
      if (!GetListArea(row.kind).Contains(x, y))
        continue;

      if (row.expandable)
      {
        row.node->expanded = !row.node->expanded;
        SetDirty(false);
        return;
      }

      if (!row.loadable)
        return;

      if (row.kind == LibraryKind::NAM)
      {
        mSelectedNAMDirectory = row.node->path;
        if (mLoadNAMDirectory)
          mLoadNAMDirectory(row.node->path.c_str());
      }
      else
      {
        mSelectedIRDirectory = row.node->path;
        if (mLoadIRDirectory)
          mLoadIRDirectory(row.node->path.c_str());
      }

      CloseDrawer();
      SetDirty(false);
      return;
    }
  }

  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    LibraryKind kind;
    if (!GetKindAtPoint(x, y, kind))
      return;

    const IRECT listArea = GetListArea(kind);
    float& scroll = ScrollForKind(kind);
    const float contentHeight = ContentHeightForKind(kind);
    if (contentHeight <= listArea.H())
      return;

    scroll -= d * 24.f;
    ClampScroll(scroll, contentHeight, listArea);
    SetDirty(false);
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    IControl::OnMouseOver(x, y, mod);
    mMouseX = x;
    mMouseY = y;
    SetDirty(false);
  }

  void OnMouseOut() override
  {
    IControl::OnMouseOut();
    mMouseX = -1.f;
    mMouseY = -1.f;
    SetDirty(false);
  }

private:
  enum class LibraryKind
  {
    NAM,
    IR
  };

  enum class RowType
  {
    Section,
    Empty,
    Folder
  };

  struct FolderNode
  {
    std::string name;
    std::string path;
    std::string thumbnailPath;
    std::string thumbnailCacheName;
    std::vector<uint8_t> thumbnailData;
    int directFileCount = 0;
    int fileCount = 0;
    bool expanded = false;
    IBitmap thumbnail;
    std::vector<FolderNode> children;
  };

  struct Row
  {
    RowType type = RowType::Empty;
    LibraryKind kind = LibraryKind::NAM;
    FolderNode* node = nullptr;
    std::string label;
    IRECT rect;
    IRECT expanderRect;
    IRECT thumbnailRect;
    IRECT labelRect;
    bool expandable = false;
    bool expanded = false;
    bool loadable = false;
  };

  static constexpr float kSectionHeight = 24.f;
  static constexpr float kEmptyRowHeight = 22.f;
  static constexpr float kFolderRowHeight = 28.f;
  static constexpr float kAmpCardGap = 8.f;
  static constexpr float kAmpCardPad = 8.f;
  static constexpr float kAmpCardTitleHeight = 32.f;
  static constexpr float kAmpCardTitleXPad = 4.f;
  static constexpr float kAmpCardTitleYPad = 5.f;
  static constexpr float kIndent = 12.f;
  static constexpr float kTextPad = 12.f;
  static constexpr float kRightTextPad = 6.f;
  static constexpr float kHeaderTextPad = 48.f;
  static constexpr float kArrowWidth = 12.f;
  static constexpr float kArrowGap = 5.f;

  static std::string ToLower(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
  }

  static bool HasExtension(const std::filesystem::path& path, const char* extension)
  {
    return ToLower(path.extension().string()) == std::string(".") + extension;
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

  static bool IsWebPFile(const std::filesystem::path& path)
  {
    return ToLower(path.extension().string()) == ".webp";
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

  static IRECT GetAspectFitRect(const IBitmap& bitmap, const IRECT& bounds)
  {
    const float bitmapWidth = static_cast<float>(bitmap.W());
    const float bitmapHeight = static_cast<float>(bitmap.H());

    if (bitmapWidth <= 0.f || bitmapHeight <= 0.f || bounds.W() <= 0.f || bounds.H() <= 0.f)
      return bounds;

    const float scale = std::min(bounds.W() / bitmapWidth, bounds.H() / bitmapHeight);
    return bounds.GetCentredInside(bitmapWidth * scale, bitmapHeight * scale);
  }

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

  static std::string ParentDirectory(const char* path)
  {
    if (!CStringHasContents(path))
      return "";

    try
    {
      return PathString(std::filesystem::u8path(path).parent_path());
    }
    catch (...)
    {
      WDL_String directory(path);
      directory.remove_filepart(true);
      return NormalizePath(directory.Get());
    }
  }

  static std::string FileName(const std::filesystem::path& path)
  {
    const std::string name = path.filename().string();
    return name.empty() ? path.string() : name;
  }

  static bool DirectoryExists(const std::string& path)
  {
    if (path.empty())
      return false;

    std::error_code ec;
    return std::filesystem::is_directory(std::filesystem::u8path(path), ec);
  }

  static int CountDirectFilesWithExtension(const std::filesystem::path& directory, const char* extension, bool& readError)
  {
    int count = 0;
    std::error_code ec;
    std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec)
    {
      readError = true;
      return 0;
    }

    for (std::filesystem::directory_iterator end; it != end; it.increment(ec))
    {
      if (ec)
      {
        readError = true;
        break;
      }

      std::error_code entryError;
      if (it->is_regular_file(entryError) && HasExtension(it->path(), extension))
        count++;
    }

    return count;
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

  static bool BuildNAMNode(const std::filesystem::path& directory, FolderNode& node, bool& readError)
  {
    const int fileCount = CountDirectFilesWithExtension(directory, "nam", readError);
    if (fileCount <= 0)
      return false;

    node.name = FileName(directory);
    node.path = PathString(directory);
    node.directFileCount = fileCount;
    node.fileCount = fileCount;

    std::filesystem::path thumbnailPath;
    if (FindFirstImage(directory, thumbnailPath))
      node.thumbnailPath = PathString(thumbnailPath);

    return true;
  }

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

  static bool TextFits(IGraphics& g, const IText& textStyle, const std::string& text, float maxWidth)
  {
    IRECT measured;
    g.MeasureText(textStyle, text.c_str(), measured);
    return measured.W() <= maxWidth;
  }

  static std::vector<std::string> SplitWordToFit(IGraphics& g, const IText& textStyle, const std::string& word,
                                                 float maxWidth)
  {
    std::vector<std::string> chunks;
    size_t start = 0;

    while (start < word.size())
    {
      size_t low = 1;
      size_t high = word.size() - start;
      size_t best = 1;

      while (low <= high)
      {
        const size_t mid = (low + high) / 2;
        const std::string candidate = word.substr(start, mid);

        if (TextFits(g, textStyle, candidate, maxWidth))
        {
          best = mid;
          low = mid + 1;
        }
        else
        {
          if (mid == 0)
            break;
          high = mid - 1;
        }
      }

      chunks.push_back(word.substr(start, best));
      start += best;
    }

    return chunks;
  }

  static std::vector<std::string> WrapTextToFit(IGraphics& g, const IText& textStyle, const std::string& text,
                                                float maxWidth)
  {
    std::vector<std::string> lines;
    if (text.empty() || maxWidth <= 0.f)
      return lines;

    std::string current;
    std::istringstream stream(text);
    std::string word;
    bool foundWord = false;

    const auto placeWord = [&](const std::string& nextWord) {
      if (current.empty())
      {
        if (TextFits(g, textStyle, nextWord, maxWidth))
        {
          current = nextWord;
          return;
        }

        const auto chunks = SplitWordToFit(g, textStyle, nextWord, maxWidth);
        for (size_t i = 0; i < chunks.size(); ++i)
        {
          if (i + 1 == chunks.size())
            current = chunks[i];
          else
            lines.push_back(chunks[i]);
        }
        return;
      }

      const std::string candidate = current + " " + nextWord;
      if (TextFits(g, textStyle, candidate, maxWidth))
      {
        current = candidate;
        return;
      }

      lines.push_back(current);
      current.clear();

      if (TextFits(g, textStyle, nextWord, maxWidth))
      {
        current = nextWord;
        return;
      }

      const auto chunks = SplitWordToFit(g, textStyle, nextWord, maxWidth);
      for (size_t i = 0; i < chunks.size(); ++i)
      {
        if (i + 1 == chunks.size())
          current = chunks[i];
        else
          lines.push_back(chunks[i]);
      }
    };

    while (stream >> word)
    {
      foundWord = true;
      placeWord(word);
    }

    if (!current.empty())
      lines.push_back(current);

    if (!foundWord)
      lines.push_back(text);

    return lines;
  }

  static float GetWrappedTextHeight(IGraphics& g, const IText& textStyle, const std::string& text, float maxWidth)
  {
    const auto lines = WrapTextToFit(g, textStyle, text, maxWidth);
    const float lineHeight = textStyle.mSize + 2.f;
    return std::max(kAmpCardTitleHeight, 2.f * kAmpCardTitleYPad + static_cast<float>(lines.size()) * lineHeight);
  }

  static void DrawWrappedText(IGraphics& g, const IText& textStyle, const std::string& text, const IRECT& bounds)
  {
    const auto lines = WrapTextToFit(g, textStyle, text, bounds.W());
    if (lines.empty())
      return;

    const float lineHeight = textStyle.mSize + 2.f;
    const float totalHeight = static_cast<float>(lines.size()) * lineHeight;
    float y = bounds.T + std::max(0.f, (bounds.H() - totalHeight) * 0.5f);
    const IText lineText = textStyle.WithVAlign(EVAlign::Middle);

    for (const auto& line : lines)
    {
      const IRECT lineRect(bounds.L, y, bounds.R, y + lineHeight);
      g.DrawText(lineText, line.c_str(), lineRect);
      y += lineHeight;
    }
  }

  IRECT GetLibraryArea(LibraryKind kind) const
  {
    const float halfHeight = std::floor(mRECT.H() * 0.5f);
    if (kind == LibraryKind::NAM)
      return IRECT(mRECT.L, mRECT.T, mRECT.R, mRECT.T + halfHeight);

    return IRECT(mRECT.L, mRECT.T + halfHeight, mRECT.R, mRECT.B);
  }

  IRECT GetListArea(LibraryKind kind) const
  {
    return GetLibraryArea(kind).GetReducedFromTop(kSectionHeight).GetPadded(-1.f);
  }

  bool GetKindAtPoint(float x, float y, LibraryKind& kind) const
  {
    if (GetLibraryArea(LibraryKind::NAM).Contains(x, y))
    {
      kind = LibraryKind::NAM;
      return true;
    }

    if (GetLibraryArea(LibraryKind::IR).Contains(x, y))
    {
      kind = LibraryKind::IR;
      return true;
    }

    return false;
  }

  float& ScrollForKind(LibraryKind kind)
  {
    return kind == LibraryKind::NAM ? mNAMScroll : mIRScroll;
  }

  float ContentHeightForKind(LibraryKind kind) const
  {
    return kind == LibraryKind::NAM ? mNAMContentHeight : mIRContentHeight;
  }

  void DrawRow(IGraphics& g, const Row& row, const IRECT& visible, const IText& rowText, const IText& mutedText)
  {
    if (!row.rect.Intersects(visible))
      return;

    switch (row.type)
    {
      case RowType::Empty:
      {
        const std::string label = EllipsizeToFit(g, mutedText, row.label, row.labelRect.W());
        g.DrawText(mutedText, label.c_str(), row.labelRect);
      }
        break;
      case RowType::Folder:
      {
        const bool selected = IsSelected(row);
        const auto bg = selected ? PluginColors::NAM_THEMECOLOR.WithOpacity(0.32f)
                                 : (row.rect.Contains(mMouseX, mMouseY) ? PluginColors::MOUSEOVER
                                                                        : COLOR_TRANSPARENT);

        if (bg.A > 0)
          g.FillRoundRect(bg, row.rect.GetHPadded(3.f), 4.f);

        const IText text = row.loadable ? rowText : rowText.WithFGColor(PluginColors::NAM_THEMEFONTCOLOR.WithOpacity(0.76f));
        const std::string label = EllipsizeToFit(g, text, row.label, row.labelRect.W());
        g.DrawText(text, label.c_str(), row.labelRect);

        if (row.expandable)
          g.DrawText(mutedText, row.expanded ? "v" : ">", row.expanderRect);
      }
      break;
      default: break;
    }
  }

	  IBitmap GetNodeThumbnail(FolderNode& node)
	  {
	    if (node.thumbnail.IsValid() || node.thumbnailPath.empty())
	      return node.thumbnail;

	    std::error_code ec;
	    const auto path = std::filesystem::u8path(node.thumbnailPath);
	    if (std::filesystem::is_regular_file(path, ec))
	    {
#ifdef OS_MAC
	      if (IsImageFile(path) && EncodeImageAsPNG(path, node.thumbnailData))
	      {
	        node.thumbnailCacheName = node.thumbnailPath + ".decoded.png";
	        node.thumbnail = GetUI()->LoadBitmap(node.thumbnailCacheName.c_str(), node.thumbnailData.data(),
	                                             static_cast<int>(node.thumbnailData.size()), 1, false, 1);
	      }
#else
	      if (CanLoadThumbnailFile(path))
	        node.thumbnail = GetUI()->LoadBitmap(node.thumbnailPath.c_str());
#endif
	    }

	    if (node.thumbnail.IsValid() && (node.thumbnail.W() <= 0 || node.thumbnail.H() <= 0))
	      node.thumbnail = IBitmap();

	    return node.thumbnail;
	  }

  void DrawAmpCard(IGraphics& g, Row& row, const IRECT& visible, const IText& titleText)
  {
    if (!row.rect.Intersects(visible) || row.node == nullptr)
      return;

    const IColor borderColor(255, 214, 214, 214);
    const IColor placeholderColor(255, 246, 246, 246);
    const bool selected = IsSelected(row);
    const bool hovered = row.rect.Contains(mMouseX, mMouseY);
    const auto fillColor = selected ? PluginColors::NAM_THEMECOLOR.WithOpacity(0.18f)
                                    : (hovered ? IColor(255, 248, 248, 248) : COLOR_WHITE);

    g.FillRoundRect(fillColor, row.rect, 4.f);
    g.DrawRoundRect(selected ? PluginColors::NAM_THEMECOLOR.WithOpacity(0.75f) : borderColor, row.rect, 4.f, &mBlend,
                    1.f);
    g.FillRect(placeholderColor, row.thumbnailRect);

    const IBitmap thumbnail = GetNodeThumbnail(*row.node);
    if (thumbnail.IsValid())
      g.DrawFittedBitmap(thumbnail, GetAspectFitRect(thumbnail, row.thumbnailRect.GetPadded(-4.f)), &mBlend);

    DrawWrappedText(g, titleText, row.label, row.labelRect);
  }

  void DrawLibrary(IGraphics& g, LibraryKind kind, const std::string& rootDirectory, std::vector<FolderNode>& nodes,
                   bool readError, float& scroll, float& contentHeight, const IText& rowText, const IText& mutedText,
                   const IText& cardTitleText)
  {
    const IRECT listArea = GetListArea(kind);

    ClampScroll(scroll, contentHeight, listArea);
    const size_t startRow = mRows.size();
    float y = listArea.T - scroll;
    AddRowsForLibrary(g, kind, rootDirectory, nodes, readError, listArea, y, cardTitleText);
    contentHeight = std::max(0.f, y - listArea.T + scroll);
    ClampScroll(scroll, contentHeight, listArea);

    g.PathClipRegion(listArea);
    for (size_t i = startRow; i < mRows.size(); ++i)
    {
      if (mRows[i].kind == LibraryKind::NAM && mRows[i].type == RowType::Folder)
        DrawAmpCard(g, mRows[i], listArea, cardTitleText);
      else
        DrawRow(g, mRows[i], listArea, rowText, mutedText);
    }
    g.PathClipRegion(IRECT());
  }

  void DrawSectionTitle(IGraphics& g, LibraryKind kind, const char* title, const IText& sectionText)
  {
    const IRECT area = GetLibraryArea(kind);
    const IRECT headerArea(area.L + 2.f, area.T + 2.f, area.R - 2.f, area.T + kSectionHeight);
    const IRECT labelArea(headerArea.L + kHeaderTextPad, headerArea.T, headerArea.R - kTextPad, headerArea.B - 1.f);

    g.PathClipRegion(IRECT());
    g.FillRect(COLOR_WHITE, headerArea);
    g.DrawText(sectionText, title, labelArea);
    const auto line = IRECT(labelArea.L, headerArea.B - 1.f, headerArea.R - kTextPad, headerArea.B);
    g.FillRect(PluginColors::NAM_THEMECOLOR.WithOpacity(0.35f), line);
  }

  bool BuildNode(const std::filesystem::path& directory, LibraryKind kind, FolderNode& node, int depth, bool& readError)
  {
    const char* extension = kind == LibraryKind::NAM ? "nam" : "wav";
    std::vector<std::filesystem::directory_entry> entries;
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
        break;
      }

      entries.push_back(*it);
    }

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
      return ToLower(a.path().filename().string()) < ToLower(b.path().filename().string());
    });

    node.name = FileName(directory);
    node.path = PathString(directory);
    node.expanded = depth == 0;

    for (const auto& entry : entries)
    {
      std::error_code entryError;
      if (entry.is_symlink(entryError))
        continue;

      entryError.clear();
      if (entry.is_directory(entryError))
      {
        FolderNode child;
        if (BuildNode(entry.path(), kind, child, depth + 1, readError))
        {
          node.fileCount += child.fileCount;
          node.children.push_back(std::move(child));
        }
      }
      else if (entry.is_regular_file(entryError))
      {
        if (HasExtension(entry.path(), extension))
        {
          node.directFileCount++;
          node.fileCount++;
        }
      }
    }

    return node.fileCount > 0;
  }

  std::vector<FolderNode> BuildLibrary(const std::string& rootDirectory, LibraryKind kind, bool& readError)
  {
    std::vector<FolderNode> nodes;

    if (!DirectoryExists(rootDirectory))
    {
      readError = true;
      return nodes;
    }

    if (kind == LibraryKind::NAM)
    {
      FolderNode rootNode;
      if (BuildNAMNode(std::filesystem::u8path(rootDirectory), rootNode, readError))
        nodes.push_back(std::move(rootNode));

      std::vector<std::filesystem::directory_entry> entries;
      std::error_code ec;
      for (std::filesystem::directory_iterator it(std::filesystem::u8path(rootDirectory),
                                                  std::filesystem::directory_options::skip_permission_denied,
                                                  ec),
           end;
           !ec && it != end; it.increment(ec))
      {
        entries.push_back(*it);
      }

      if (ec)
        readError = true;

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

        FolderNode child;
        if (BuildNAMNode(entry.path(), child, readError))
          nodes.push_back(std::move(child));
      }

      return nodes;
    }

    FolderNode root;
    if (!BuildNode(std::filesystem::u8path(rootDirectory), kind, root, 0, readError))
      return nodes;

    nodes = std::move(root.children);

    if (root.fileCount > 0)
    {
      int childFileCount = 0;
      for (const auto& child : nodes)
        childFileCount += child.fileCount;

      if (root.fileCount > childFileCount)
      {
        root.children.clear();
        root.fileCount -= childFileCount;
        root.name = FileName(std::filesystem::u8path(rootDirectory));
        root.expanded = false;
        nodes.insert(nodes.begin(), std::move(root));
      }
    }

    return nodes;
  }

  void Rescan()
  {
    mNAMReadError = false;
    mIRReadError = false;
    mNAMFolders = BuildLibrary(mNAMRootDirectory, LibraryKind::NAM, mNAMReadError);
    mIRFolders = BuildLibrary(mIRRootDirectory, LibraryKind::IR, mIRReadError);
    mNAMScroll = 0.f;
    mIRScroll = 0.f;
  }

  void AddRowsForLibrary(IGraphics& g, LibraryKind kind, const std::string& rootDirectory,
                         std::vector<FolderNode>& nodes, bool readError, const IRECT& listArea, float& y,
                         const IText& cardTitleText)
  {
    if (rootDirectory.empty())
    {
      AddEmptyRow("Choose a folder in Settings", kind, listArea, y);
      return;
    }

    if (nodes.empty())
    {
      AddEmptyRow(readError ? "Re-select folder in Settings" : "No matching folders", kind, listArea, y);
      return;
    }

    if (kind == LibraryKind::NAM)
      AddAmpCardRows(g, kind, nodes, listArea, y, cardTitleText);
    else
    {
      for (auto& node : nodes)
        AddFolderRows(kind, node, 0, listArea, y);
    }
  }

  void AddEmptyRow(const char* text, LibraryKind kind, const IRECT& listArea, float& y)
  {
    Row row;
    row.type = RowType::Empty;
    row.kind = kind;
    row.label = text;
    row.rect = IRECT(listArea.L, y, listArea.R, y + kEmptyRowHeight);
    row.labelRect = IRECT(row.rect.L + kTextPad, row.rect.T, row.rect.R - kRightTextPad, row.rect.B);
    mRows.push_back(row);
    y += kEmptyRowHeight;
  }

  void AddAmpCardRows(IGraphics& g, LibraryKind kind, std::vector<FolderNode>& nodes, const IRECT& listArea, float& y,
                      const IText& titleText)
  {
    if (nodes.empty())
      return;

    const float availableWidth = listArea.W() - (2.f * kAmpCardPad) - kAmpCardGap;
    const float cardWidth = std::floor(availableWidth * 0.5f);
    const float labelWidth = cardWidth - 2.f * kAmpCardTitleXPad;
    const float startX = listArea.L + kAmpCardPad;
    const float startY = y + kAmpCardPad;
    std::vector<float> cardHeights;
    cardHeights.reserve(nodes.size());

    for (const auto& node : nodes)
      cardHeights.push_back(cardWidth + GetWrappedTextHeight(g, titleText, node.name, labelWidth));

    float rowY = startY;
    for (size_t rowStart = 0; rowStart < nodes.size(); rowStart += 2)
    {
      float rowHeight = cardHeights[rowStart];
      if (rowStart + 1 < nodes.size())
        rowHeight = std::max(rowHeight, cardHeights[rowStart + 1]);

      for (size_t column = 0; column < 2 && rowStart + column < nodes.size(); ++column)
      {
        const size_t nodeIndex = rowStart + column;
        const float x = startX + static_cast<float>(column) * (cardWidth + kAmpCardGap);

        Row row;
        row.type = RowType::Folder;
        row.kind = kind;
        row.node = &nodes[nodeIndex];
        row.label = nodes[nodeIndex].name;
        row.loadable = nodes[nodeIndex].directFileCount > 0;
        row.rect = IRECT(x, rowY, x + cardWidth, rowY + rowHeight);
        row.thumbnailRect = IRECT(row.rect.L, row.rect.T, row.rect.R, row.rect.T + cardWidth);
        row.labelRect = IRECT(row.rect.L + kAmpCardTitleXPad, row.thumbnailRect.B + kAmpCardTitleYPad,
                              row.rect.R - kAmpCardTitleXPad, row.rect.B - kAmpCardTitleYPad);
        mRows.push_back(row);
      }

      rowY += rowHeight + kAmpCardGap;
    }

    y = rowY - kAmpCardGap + kAmpCardPad;
  }

  void AddFolderRows(LibraryKind kind, FolderNode& node, int depth, const IRECT& listArea, float& y)
  {
    Row row;
    row.type = RowType::Folder;
    row.kind = kind;
    row.node = &node;
    row.expandable = !node.children.empty();
    row.expanded = node.expanded;
    row.loadable = node.directFileCount > 0 && !row.expandable;
    row.rect = IRECT(listArea.L, y, listArea.R, y + kFolderRowHeight);

    const float labelLeft = listArea.L + kTextPad + depth * kIndent;
    const float maxRight = listArea.R - kRightTextPad;
    if (row.expandable)
    {
      row.expanderRect = IRECT(maxRight - kArrowWidth, row.rect.T, maxRight, row.rect.B);
      row.labelRect = IRECT(labelLeft, row.rect.T, row.expanderRect.L - kArrowGap, row.rect.B);
    }
    else
    {
      row.labelRect = IRECT(labelLeft, row.rect.T, maxRight, row.rect.B);
    }

    row.label = node.name;
    mRows.push_back(row);
    y += kFolderRowHeight;

    if (node.expanded)
    {
      for (auto& child : node.children)
        AddFolderRows(kind, child, depth + 1, listArea, y);
    }
  }

  bool IsSelected(const Row& row) const
  {
    if (row.node == nullptr || !row.loadable)
      return false;

    return row.kind == LibraryKind::NAM ? row.node->path == mSelectedNAMDirectory
                                        : row.node->path == mSelectedIRDirectory;
  }

  void ClampScroll(float& scroll, float contentHeight, const IRECT& listArea)
  {
    const float maxScroll = std::max(0.f, contentHeight - listArea.H());
    scroll = std::clamp(scroll, 0.f, maxScroll);
  }

  void CloseDrawer()
  {
    Hide(true);

    if (auto* ui = GetUI())
      ui->SetAllControlsDirty();
  }

  LoadDirectoryFunc mLoadNAMDirectory;
  LoadDirectoryFunc mLoadIRDirectory;
  std::string mNAMRootDirectory;
  std::string mIRRootDirectory;
  std::string mSelectedNAMDirectory;
  std::string mSelectedIRDirectory;
  std::vector<FolderNode> mNAMFolders;
  std::vector<FolderNode> mIRFolders;
  std::vector<Row> mRows;
  bool mNAMReadError = false;
  bool mIRReadError = false;
  float mNAMScroll = 0.f;
  float mIRScroll = 0.f;
  float mNAMContentHeight = 0.f;
  float mIRContentHeight = 0.f;
  float mMouseX = -1.f;
  float mMouseY = -1.f;
};

class NAMMeterControl : public IVPeakAvgMeterControl<>, public IBitmapBase
{
  static constexpr float KMeterMin = -70.0f;
  static constexpr float KMeterMax = -0.01f;

public:
  NAMMeterControl(const IRECT& bounds, const IBitmap& bitmap, const IVStyle& style)
  : IVPeakAvgMeterControl<>(bounds, "", style.WithShowValue(false).WithDrawFrame(false).WithWidgetFrac(0.8),
                            EDirection::Vertical, {}, 0, KMeterMin, KMeterMax, {})
  , IBitmapBase(bitmap)
  {
    SetPeakSize(1.0f);
  }

  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }

  virtual void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    mWidgetBounds = mWidgetBounds.GetMidHPadded(5).GetVPadded(10);
    MakeTrackRects(mWidgetBounds);
    MakeStepRects(mWidgetBounds, mNSteps);
    SetDirty(false);
  }

  void DrawBackground(IGraphics& g, const IRECT& r) override { g.DrawFittedBitmap(mBitmap, r); }

  void DrawTrackHandle(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  {
    if (r.H() > 2)
      g.FillRect(GetColor(kX1), r, &mBlend);
  }

  void DrawPeak(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  {
    g.DrawGrid(COLOR_BLACK, mTrackBounds.Get()[chIdx], 10, 2);
    g.FillRect(GetColor(kX3), r, &mBlend);
  }
};

// Container where we can refer to children by names instead of indices
class IContainerBaseWithNamedChildren : public IContainerBase
{
public:
  IContainerBaseWithNamedChildren(const IRECT& bounds)
  : IContainerBase(bounds) {};
  ~IContainerBaseWithNamedChildren() = default;

protected:
  IControl* AddNamedChildControl(IControl* control, std::string name, int ctrlTag = kNoTag, const char* group = "")
  {
    // Make sure we haven't already used this name
    assert(mChildNameIndexMap.find(name) == mChildNameIndexMap.end());
    mChildNameIndexMap[name] = NChildren();
    return AddChildControl(control, ctrlTag, group);
  };

  IControl* GetNamedChild(std::string name)
  {
    const int index = mChildNameIndexMap[name];
    return GetChild(index);
  };


private:
  std::unordered_map<std::string, int> mChildNameIndexMap;
}; // class IContainerBaseWithNamedChildren


struct PossiblyKnownParameter
{
  bool known = false;
  double value = 0.0;
};

struct ModelInfo
{
  PossiblyKnownParameter sampleRate;
  PossiblyKnownParameter inputCalibrationLevel;
  PossiblyKnownParameter outputCalibrationLevel;
};

class ModelInfoControl : public IContainerBaseWithNamedChildren
{
public:
  ModelInfoControl(const IRECT& bounds, const IVStyle& style)
  : IContainerBaseWithNamedChildren(bounds)
  , mStyle(style) {};

  void ClearModelInfo()
  {
    static_cast<IVLabelControl*>(GetNamedChild(mControlNames.sampleRate))->SetStr("");
    mHasInfo = false;
  };

  void Hide(bool hide) override
  {
    // Don't show me unless I have info to show!
    IContainerBase::Hide(hide || (!mHasInfo));
  };

  void OnAttached() override
  {
    AddChildControl(new IVLabelControl(GetRECT().SubRectVertical(4, 0), "Model information:", mStyle));
    AddNamedChildControl(new IVLabelControl(GetRECT().SubRectVertical(4, 1), "", mStyle), mControlNames.sampleRate);
    // AddNamedChildControl(
    //   new IVLabelControl(GetRECT().SubRectVertical(4, 2), "", mStyle), mControlNames.inputCalibrationLevel);
    // AddNamedChildControl(
    //   new IVLabelControl(GetRECT().SubRectVertical(4, 3), "", mStyle), mControlNames.outputCalibrationLevel);
  };

  void SetModelInfo(const ModelInfo& modelInfo)
  {
    auto SetControlStr = [&](const std::string& name, const PossiblyKnownParameter& p, const std::string& units,
                             const std::string& childName) {
      std::stringstream ss;
      ss << name << ": ";
      if (p.known)
      {
        ss << p.value << " " << units;
      }
      else
      {
        ss << "(Unknown)";
      }
      static_cast<IVLabelControl*>(GetNamedChild(childName))->SetStr(ss.str().c_str());
    };

    SetControlStr("Sample rate", modelInfo.sampleRate, "Hz", mControlNames.sampleRate);
    // SetControlStr(
    //   "Input calibration level", modelInfo.inputCalibrationLevel, "dBu", mControlNames.inputCalibrationLevel);
    // SetControlStr(
    //   "Output calibration level", modelInfo.outputCalibrationLevel, "dBu", mControlNames.outputCalibrationLevel);

    mHasInfo = true;
  };

private:
  const IVStyle mStyle;
  struct
  {
    const std::string sampleRate = "sampleRate";
    // const std::string inputCalibrationLevel = "inputCalibrationLevel";
    // const std::string outputCalibrationLevel = "outputCalibrationLevel";
  } mControlNames;
  // Do I have info?
  bool mHasInfo = false;
};

class OutputModeControl : public IVRadioButtonControl
{
public:
  OutputModeControl(const IRECT& bounds, int paramIdx, const IVStyle& style, float buttonSize)
  : IVRadioButtonControl(
      bounds, paramIdx, {}, "Output Mode", style, EVShape::Ellipse, EDirection::Vertical, buttonSize) {};

  void SetNormalizedDisable(const bool disable)
  {
    // HACK non-DRY string and hard-coded indices
    std::stringstream ss;
    ss << "Normalized";
    if (disable)
    {
      ss << " [Not supported by model]";
    }
    mTabLabels.Get(1)->Set(ss.str().c_str());
  };
  void SetCalibratedDisable(const bool disable)
  {
    // HACK non-DRY string and hard-coded indices
    std::stringstream ss;
    ss << "Calibrated";
    if (disable)
    {
      ss << " [Not supported by model]";
    }
    mTabLabels.Get(2)->Set(ss.str().c_str());
  };
};

class NAMDirectoryPickerControl : public IControl
{
public:
  using DirectoryChangedFunc = std::function<void(const WDL_String&)>;

  NAMDirectoryPickerControl(const IRECT& bounds, const char* title, const char* emptyText, const WDL_String& directory,
                            DirectoryChangedFunc onDirectoryChanged)
  : IControl(bounds)
  , mTitle(title)
  , mEmptyText(emptyText)
  , mDirectory(directory)
  , mOnDirectoryChanged(std::move(onDirectoryChanged))
  {
  }

  void SetDirectory(const char* directory)
  {
    mDirectory.Set(directory);
    SetDirty(false);
  }

  void Draw(IGraphics& g) override
  {
    const auto bg = mMouseIsOver ? PluginColors::MOUSEOVER : COLOR_WHITE;
    g.FillRoundRect(bg, mRECT, 5.f);
    g.DrawRoundRect(PluginColors::NAM_THEMECOLOR.WithOpacity(0.22f), mRECT, 5.f, &mBlend, 1.f);

    const IText titleText(11.f, PluginColors::NAM_THEMEFONTCOLOR.WithOpacity(0.78f), "Roboto-Regular", EAlign::Near);
    const IText valueText(12.f, PluginColors::NAM_THEMEFONTCOLOR, "Roboto-Regular", EAlign::Near);

    const auto titleRect = mRECT.GetPadded(-8.f).GetFromTop(16.f);
    const auto valueRect = mRECT.GetPadded(-8.f).GetReducedFromTop(16.f);
    g.DrawText(titleText, mTitle.Get(), titleRect);

    std::string label = CStringHasContents(mDirectory.Get()) ? std::string(mDirectory.Get()) : std::string(mEmptyText.Get());
    label = EllipsizePath(label, 58);
    g.DrawText(valueText, label.c_str(), valueRect);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    WDL_String directory(mDirectory);
    GetUI()->PromptForDirectory(directory, [this](const WDL_String& fileName, const WDL_String& path) {
      if (!path.GetLength())
        return;

      mDirectory.Set(path.Get());
      if (mOnDirectoryChanged)
        mOnDirectoryChanged(mDirectory);
      SetDirty(false);
    });
  }

private:
  static std::string EllipsizePath(const std::string& path, size_t maxLength)
  {
    if (path.size() <= maxLength)
      return path;

    if (maxLength < 8)
      return path.substr(0, maxLength);

    const size_t head = (maxLength - 3) / 2;
    const size_t tail = maxLength - 3 - head;
    return path.substr(0, head) + "..." + path.substr(path.size() - tail);
  }

  WDL_String mTitle;
  WDL_String mEmptyText;
  WDL_String mDirectory;
  DirectoryChangedFunc mOnDirectoryChanged;
};

class NAMSettingsPageControl : public IContainerBaseWithNamedChildren
{
public:
  NAMSettingsPageControl(const IRECT& bounds, const IBitmap& inputLevelBackgroundBitmap,
                         const IBitmap& switchBitmap, ISVG closeSVG, const IVStyle& style,
                         const IVStyle& radioButtonStyle)
  : IContainerBaseWithNamedChildren(bounds)
  , mAnimationTime(0)
  , mInputLevelBackgroundBitmap(inputLevelBackgroundBitmap)
  , mSwitchBitmap(switchBitmap)
  , mStyle(style)
  , mRadioButtonStyle(radioButtonStyle)
  , mCloseSVG(closeSVG)
  {
    mIgnoreMouse = false;
  }

  void ClearModelInfo()
  {
    auto* modelInfoControl = static_cast<ModelInfoControl*>(GetNamedChild(mControlNames.modelInfo));
    assert(modelInfoControl != nullptr);
    modelInfoControl->ClearModelInfo();
  }

  bool OnKeyDown(float x, float y, const IKeyPress& key) override
  {
    if (key.VK == kVK_ESCAPE)
    {
      HideAnimated(true);
      return true;
    }

    return false;
  }

  void HideAnimated(bool hide)
  {
    mWillHide = hide;

    if (hide == false)
    {
      mHide = false;
    }
    else // hide subcontrols immediately
    {
      ForAllChildrenFunc([hide](int childIdx, IControl* pChild) { pChild->Hide(hide); });
    }

    SetAnimation(
      [&](IControl* pCaller) {
        auto progress = static_cast<float>(pCaller->GetAnimationProgress());

        if (mWillHide)
          SetBlend(IBlend(EBlend::Default, 1.0f - progress));
        else
          SetBlend(IBlend(EBlend::Default, progress));

        if (progress > 1.0f)
        {
          pCaller->OnEndAnimation();
          IContainerBase::Hide(mWillHide);
          GetUI()->SetAllControlsDirty();
          return;
        }
      },
      mAnimationTime);

    SetDirty(true);
  }

  void OnAttached() override
  {
    const float pad = 20.0f;
    const IVStyle titleStyle = DEFAULT_STYLE.WithValueText(IText(30, COLOR_BLACK, "Michroma-Regular"))
                                 .WithDrawFrame(false)
                                 .WithShadowOffset(2.f);
    const auto text = IText(DEFAULT_TEXT_SIZE, EAlign::Center, PluginColors::HELP_TEXT);
    const auto leftText = text.WithAlign(EAlign::Near);
    const auto style = mStyle.WithDrawFrame(false).WithValueText(text);
    const IVStyle leftStyle = style.WithValueText(leftText);

    AddNamedChildControl(new IPanelControl(GetRECT(), COLOR_WHITE), mControlNames.background)->SetIgnoreMouse(true);
    const auto workingArea = GetRECT().GetPadded(-(pad + 10.0f));
    const auto titleArea = workingArea.GetFromTop(50.0f);
    AddNamedChildControl(new IVLabelControl(titleArea, "SETTINGS", titleStyle), mControlNames.title);

    const auto libraryArea = workingArea.GetReducedFromTop(58.f).GetFromTop(58.f);
    const auto namLibraryArea = libraryArea.GetFromLeft(0.5f * libraryArea.W()).GetReducedFromRight(6.f);
    const auto irLibraryArea = libraryArea.GetFromRight(0.5f * libraryArea.W()).GetReducedFromLeft(6.f);

    AddNamedChildControl(new NAMDirectoryPickerControl(
                           namLibraryArea, "NAM library folder", "Click to choose your amps folder",
                           PLUG()->GetNAMRootDirectory(),
                           [this](const WDL_String& directory) { PLUG()->SetNAMRootDirectory(directory); }),
                         mControlNames.namLibraryDirectory);
    AddNamedChildControl(new NAMDirectoryPickerControl(
                           irLibraryArea, "IR library folder", "Click to choose your cabs folder",
                           PLUG()->GetIRRootDirectory(),
                           [this](const WDL_String& directory) { PLUG()->SetIRRootDirectory(directory); }),
                         mControlNames.irLibraryDirectory);

    // Attach input/output calibration controls
    {
      const float height = NAM_KNOB_HEIGHT + NAM_SWTICH_HEIGHT + 10.0f;
      const float width = workingArea.W();
      const auto inputOutputArea = workingArea.GetReducedFromTop(128.f).GetFromTop(height);
      const auto inputArea = inputOutputArea.GetFromLeft(0.5f * width);
      const auto outputArea = inputOutputArea.GetFromRight(0.5f * width);

      const float knobWidth = 87.0f; // HACK based on looking at the main page knobs.
      const auto inputLevelArea =
        inputArea.GetFromTop(NAM_KNOB_HEIGHT).GetFromBottom(25.0f).GetMidHPadded(0.5f * knobWidth);
      const auto inputSwitchArea = inputArea.GetFromBottom(NAM_SWTICH_HEIGHT).GetMidHPadded(0.5f * knobWidth);

      auto* inputLevelControl = AddNamedChildControl(
        new InputLevelControl(inputLevelArea, kInputCalibrationLevel, mInputLevelBackgroundBitmap, text),
        mControlNames.inputCalibrationLevel, kCtrlTagInputCalibrationLevel);
      inputLevelControl->SetTooltip(
        "The analog level, in dBu RMS, that corresponds to digital level of 0 dBFS peak in the host as its signal "
        "enters this plugin.");
      AddNamedChildControl(
        new NAMSwitchControl(inputSwitchArea, kCalibrateInput, "Calibrate Input", mStyle, mSwitchBitmap),
        mControlNames.calibrateInput, kCtrlTagCalibrateInput);

      // Same-ish height & width as input controls
      const auto outputRadioArea = outputArea.GetFromBottom(
        1.1f * (inputLevelArea.H() + inputSwitchArea.H())); // .GetMidHPadded(0.55f * knobWidth);
      const float buttonSize = 10.0f;
      auto* outputModeControl =
        AddNamedChildControl(new OutputModeControl(outputRadioArea, kOutputMode, mRadioButtonStyle, buttonSize),
                             mControlNames.outputMode, kCtrlTagOutputMode);
      outputModeControl->SetTooltip(
        "How to adjust the level of the output.\nRaw=No adjustment.\nNormalized=Adjust the level so that all models "
        "are about the same loudness.\nCalibrated=Match the input's digital-analog calibration.");
    }

    const float halfWidth = GetRECT().W() / 2.0f - pad;
    const auto bottomArea = GetRECT().GetPadded(-pad).GetFromBottom(78.0f);
    const float lineHeight = 15.0f;
    const auto modelInfoArea = bottomArea.GetFromLeft(halfWidth).GetFromTop(4 * lineHeight);
    const auto aboutArea = bottomArea.GetFromRight(halfWidth).GetFromTop(5 * lineHeight);
    AddNamedChildControl(new ModelInfoControl(modelInfoArea, leftStyle), mControlNames.modelInfo);
    AddNamedChildControl(new AboutControl(aboutArea, leftStyle, leftText), mControlNames.about);

    auto closeAction = [&](IControl* pCaller) {
      static_cast<NAMSettingsPageControl*>(pCaller->GetParent())->HideAnimated(true);
    };
    AddNamedChildControl(
      new NAMSquareButtonControl(CornerButtonArea(GetRECT()), closeAction, mCloseSVG), mControlNames.close);

    OnResize();
  }

  void SetModelInfo(const ModelInfo& modelInfo)
  {
    auto* modelInfoControl = static_cast<ModelInfoControl*>(GetNamedChild(mControlNames.modelInfo));
    assert(modelInfoControl != nullptr);
    modelInfoControl->SetModelInfo(modelInfo);
  };

  void SetLibraryDirectories(const char* namRootDirectory, const char* irRootDirectory)
  {
    if (auto* p = dynamic_cast<NAMDirectoryPickerControl*>(GetNamedChild(mControlNames.namLibraryDirectory)))
      p->SetDirectory(namRootDirectory);
    if (auto* p = dynamic_cast<NAMDirectoryPickerControl*>(GetNamedChild(mControlNames.irLibraryDirectory)))
      p->SetDirectory(irRootDirectory);
  }

private:
  IBitmap mInputLevelBackgroundBitmap;
  IBitmap mSwitchBitmap;
  IVStyle mStyle;
  IVStyle mRadioButtonStyle;
  ISVG mCloseSVG;
  int mAnimationTime = 200;
  bool mWillHide = false;

  // Names for controls
  // Make sure that these are all unique and that you use them with AddNamedChildControl
  struct ControlNames
  {
    const std::string about = "About";
    const std::string background = "Background";
    const std::string calibrateInput = "CalibrateInput";
    const std::string close = "Close";
    const std::string inputCalibrationLevel = "InputCalibrationLevel";
    const std::string irLibraryDirectory = "IRLibraryDirectory";
    const std::string modelInfo = "ModelInfo";
    const std::string namLibraryDirectory = "NAMLibraryDirectory";
    const std::string outputMode = "OutputMode";
    const std::string title = "Title";
  } mControlNames;

  class InputLevelControl : public IEditableTextControl
  {
  public:
    InputLevelControl(const IRECT& bounds, int paramIdx, const IBitmap& bitmap, const IText& text = DEFAULT_TEXT,
                      const IColor& BGColor = DEFAULT_BGCOLOR)
    : IEditableTextControl(bounds, "", text, BGColor)
    , mBitmap(bitmap)
    {
      SetParamIdx(paramIdx);
    };

    void Draw(IGraphics& g) override
    {
      g.DrawFittedBitmap(mBitmap, mRECT);
      ITextControl::Draw(g);
    };

    void SetValueFromUserInput(double normalizedValue, int valIdx) override
    {
      IControl::SetValueFromUserInput(normalizedValue, valIdx);
      const std::string s = ConvertToString(normalizedValue);
      OnTextEntryCompletion(s.c_str(), valIdx);
    };

    void SetValueFromDelegate(double normalizedValue, int valIdx) override
    {
      IControl::SetValueFromDelegate(normalizedValue, valIdx);
      const std::string s = ConvertToString(normalizedValue);
      SetStr(s.c_str());
      SetDirty(false);
    };

  private:
    std::string ConvertToString(const double normalizedValue)
    {
      const double naturalValue = GetParam()->FromNormalized(normalizedValue);
      // And make the value to display
      std::stringstream ss;
      ss << naturalValue << " dBu";
      std::string s = ss.str();
      return s;
    };

    IBitmap mBitmap;
  };

  class AboutControl : public IContainerBase
  {
  public:
    AboutControl(const IRECT& bounds, const IVStyle& style, const IText& text)
    : IContainerBase(bounds)
    , mStyle(style)
    , mText(text) {};

    void OnAttached() override
    {
      WDL_String verStr, buildInfoStr;
      PLUG()->GetPluginVersionStr(verStr);

      buildInfoStr.SetFormatted(100, "Version %s %s %s", verStr.Get(), PLUG()->GetArchStr(), PLUG()->GetAPIStr());

      AddChildControl(new IURLControl(GetRECT().SubRectVertical(5, 0), "Pedal Division NAM",
                                      "https://pedaldivision.com", mText, COLOR_TRANSPARENT,
                                      PluginColors::HELP_TEXT_MO, PluginColors::HELP_TEXT_CLICKED));
      AddChildControl(new IVLabelControl(GetRECT().SubRectVertical(5, 1), "By Pedal Division", mStyle));
      AddChildControl(new IVLabelControl(GetRECT().SubRectVertical(5, 2), buildInfoStr.Get(), mStyle));
      AddChildControl(new IURLControl(GetRECT().SubRectVertical(5, 3),
                                      "Plug-in development: Steve Atkinson, Oli Larkin, ... ",
                                      "https://pedaldivision.com", mText, COLOR_TRANSPARENT, PluginColors::HELP_TEXT_MO,
                                      PluginColors::HELP_TEXT_CLICKED));
      AddChildControl(new ThirdPartyNoticesControl(GetRECT().SubRectVertical(5, 4), mText));
    };

  private:
    class ThirdPartyNoticesControl : public IURLControl
    {
    public:
      ThirdPartyNoticesControl(const IRECT& bounds, const IText& text)
      : IURLControl(bounds, "Third party notices", "", text, COLOR_TRANSPARENT, PluginColors::HELP_TEXT_MO,
                    PluginColors::HELP_TEXT_CLICKED)
      {
      }

      void OnMouseDown(float x, float y, const IMouseMod& mod) override
      {
        WDL_String path;
        bool opened = false;

        if (ResolveNoticesPath(GetUI(), path))
          opened = OpenNoticesPath(GetUI(), path);

        if (!opened)
          ShowOpenError(GetUI());

        GetUI()->ReleaseMouseCapture();
        mClicked = true;
        SetDirty(false);
      }

    private:
      static bool FileExists(const WDL_String& path)
      {
        if (!CStringHasContents(path.Get()))
          return false;

        FILE* file = WDL_fopenA(path.Get(), "rb");
        if (file == nullptr)
          return false;

        fclose(file);
        return true;
      }

      static bool TryNoticePathInDirectory(WDL_String& result, const WDL_String& directory)
      {
        if (!CStringHasContents(directory.Get()))
          return false;

        WDL_String candidate(directory);
        const char lastChar = candidate.Get()[candidate.GetLength() - 1];

        if (!WDL_IS_DIRCHAR(lastChar))
          candidate.Append(WDL_DIRCHAR_STR);

        candidate.Append(kNoticesFileName);

        if (!FileExists(candidate))
          return false;

        result.Set(candidate.Get());
        return true;
      }

      // AAX (and similar) load the binary from Contents\x64 or Contents\Win32 while notices live in
      // Contents\Resources. Same layout as a VST3 bundle; this path is not covered by BundleResourcePath
      // when the plug-in is built as AAX_API only (no VST3_API).
      static bool TryNoticePathSiblingResources(WDL_String& result, const WDL_String& moduleDirectory)
      {
        if (!CStringHasContents(moduleDirectory.Get()))
          return false;

        WDL_String candidate(moduleDirectory);
        const char lastChar = candidate.Get()[candidate.GetLength() - 1];
        if (!WDL_IS_DIRCHAR(lastChar))
          candidate.Append(WDL_DIRCHAR_STR);

        candidate.Append("..");
        candidate.Append(WDL_DIRCHAR_STR);
        candidate.Append("Resources");
        candidate.Append(WDL_DIRCHAR_STR);
        candidate.Append(kNoticesFileName);

        if (!FileExists(candidate))
          return false;

        result.Set(candidate.Get());
        return true;
      }

      static bool ResolveNoticesPath(IGraphics* pGraphics, WDL_String& path)
      {
        path.Set("");

        if (pGraphics == nullptr)
          return false;

#ifdef OS_WIN
        WDL_String directory;
        const auto moduleHandle = static_cast<PluginIDType>(pGraphics->GetWinModuleHandle());

        BundleResourcePath(directory, moduleHandle);
        if (TryNoticePathInDirectory(path, directory))
          return true;

        directory.Set("");
        PluginPath(directory, moduleHandle);
        if (TryNoticePathInDirectory(path, directory))
          return true;

        if (TryNoticePathSiblingResources(path, directory))
          return true;
#endif

        const auto resourceLocation =
          LocateResource(kNoticesFileName, "txt", path, pGraphics->GetBundleID(), pGraphics->GetWinModuleHandle(),
                         pGraphics->GetSharedResourcesSubPath());

        return resourceLocation == EResourceLocation::kAbsolutePath && FileExists(path);
      }

      static bool OpenNoticesPath(IGraphics* pGraphics, const WDL_String& path)
      {
        if (pGraphics == nullptr || !CStringHasContents(path.Get()))
          return false;

#ifdef OS_WIN
        WCHAR pathWide[IPLUG_WIN_MAX_WIDE_PATH];
        UTF8ToUTF16(pathWide, path.Get(), IPLUG_WIN_MAX_WIDE_PATH);

        if (pathWide[0] == 0)
          return false;

        WCHAR canon[IPLUG_WIN_MAX_WIDE_PATH];
        const DWORD nCanon = GetFullPathNameW(pathWide, IPLUG_WIN_MAX_WIDE_PATH, canon, nullptr);
        const WCHAR* const launchPath = (nCanon > 0 && nCanon < IPLUG_WIN_MAX_WIDE_PATH) ? canon : pathWide;

        return ShellExecuteW(nullptr, L"open", launchPath, nullptr, nullptr, SW_SHOWNORMAL) > HINSTANCE(32);
#else
        return pGraphics->OpenURL(path.Get());
#endif
      }

      static void ShowOpenError(IGraphics* pGraphics)
      {
        if (pGraphics == nullptr)
          return;

        const char* const title = "Third party notices";
        const char* const message = "Could not open ThirdPartyNotices.txt.";

#ifdef OS_MAC
        pGraphics->ShowMessageBox(title, message, kMB_OK);
#else
        pGraphics->ShowMessageBox(message, title, kMB_OK);
#endif
      }

      static constexpr const char* kNoticesFileName = "ThirdPartyNotices.txt";
    };

    IVStyle mStyle;
    IText mText;
  };
};
