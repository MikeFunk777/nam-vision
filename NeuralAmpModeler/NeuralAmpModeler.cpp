#include <algorithm> // std::clamp, std::min
#include <cmath> // pow
#include <fstream>
#include <filesystem>
#include <iostream>
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

const IColor kPDLightGrey(255, 240, 240, 240);
const IColor kPDForeground(255, 35, 31, 32);
const IColor kPDBackground(255, 255, 255, 255);

class MainAreaControl : public IControl
{
public:
  MainAreaControl(const IRECT& bounds, const IBitmap& ampImage, const IBitmap& cabImage)
  : IControl(bounds)
  , mAmpImage(ampImage)
  , mCabImage(cabImage)
  {
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(kPDBackground, mRECT);

    IRECT section = mRECT.GetFromLeft(kInputMeterWidth);
    section = IRECT(section.R, mRECT.T, section.R + kAmpImageWidth, mRECT.B);
    DrawWidthFittedClippedBitmap(g, mAmpImage, section);

    section = IRECT(section.R, mRECT.T, section.R + kMainAreaSpacerWidth, mRECT.B);
    section = IRECT(section.R, mRECT.T, section.R + kCabImageWidth, mRECT.B);
    DrawWidthFittedClippedBitmap(g, mCabImage, section);
    section = IRECT(section.R, mRECT.T, section.R + kOutputMeterWidth, mRECT.B);
    g.FillRect(kPDBackground, section);
  }

private:
  static void DrawWidthFittedClippedBitmap(IGraphics& g, const IBitmap& bitmap, const IRECT& bounds)
  {
    if (!bitmap.IsValid() || bitmap.W() <= 0 || bitmap.H() <= 0)
      return;

    const float scale = bounds.W() / static_cast<float>(bitmap.W());
    const float drawWidth = bitmap.W() * scale;
    const float drawHeight = bitmap.H() * scale;
    const auto imageBounds = bounds.GetCentredInside(drawWidth, drawHeight);

    g.PathClipRegion(bounds);
    g.DrawFittedBitmap(bitmap, imageBounds);
    g.PathClipRegion(IRECT());
  }

  IBitmap mAmpImage;
  IBitmap mCabImage;
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
    DrawSelector(g, section, "[AMP] Lynx50-CL Ch1-Hi Bogdo - STD");

    section = IRECT(section.R, mRECT.T, section.R + kMainAreaSpacerWidth, mRECT.B);
    section = IRECT(section.R, mRECT.T, section.R + kCabImageWidth, mRECT.B);
    DrawSelector(g, section, "YA MRSH 412 T75 Mix 14");
    section = IRECT(section.R, mRECT.T, section.R + kOutputMeterWidth, mRECT.B);
    g.FillRect(kPDBackground, section);

    const auto borderBounds = IRECT(mRECT.L, mRECT.B - kSelectorAreaBorderSize, mRECT.R, mRECT.B);
    g.FillRect(kPDLightGrey, borderBounds);
  }

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
    const float maxTextWidth = std::max(0.f, bounds.W() - (2.f * kSelectorArrowSize) - (2.f * kSelectorGap));
    const std::string displayLabel = EllipsizeToFit(g, selectorText, label, maxTextWidth);

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

  ISVG mLeftArrow;
  ISVG mRightArrow;
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

void AttachMainAreaComponent(IGraphics& graphics, const IRECT& bounds, const IBitmap& ampImage, const IBitmap& cabImage)
{
  const auto mainAreaBounds = IRECT(bounds.L, bounds.T + kMainAreaTop, bounds.R, bounds.T + kMainAreaTop + kMainAreaHeight);
  graphics.AttachControl(new MainAreaControl(mainAreaBounds, ampImage, cabImage))->SetIgnoreMouse(true);
}

void AttachSelectorAreaComponent(IGraphics& graphics, const IRECT& bounds, const ISVG& leftArrow,
                                 const ISVG& rightArrow)
{
  const auto selectorAreaBounds =
    IRECT(bounds.L, bounds.T + kSelectorAreaTop, bounds.R, bounds.T + kSelectorAreaTop + kSelectorAreaHeight);
  graphics.AttachControl(new SelectorAreaControl(selectorAreaBounds, leftArrow, rightArrow))->SetIgnoreMouse(true);
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
    const auto ampImage = pGraphics->LoadBitmap(PD_EXAMPLE_AMP_FN);
    const auto cabImage = pGraphics->LoadBitmap(PD_EXAMPLE_CAB_FN);
    const auto chevronLeft = pGraphics->LoadSVG(PD_CHEVRON_LEFT_FN);
    const auto chevronRight = pGraphics->LoadSVG(PD_CHEVRON_RIGHT_FN);

    AttachShellComponent(*pGraphics);
    AttachMainHeaderComponent(*pGraphics, pGraphics->GetBounds(), logo, settingsIcon);
    AttachMainAreaComponent(*pGraphics, pGraphics->GetBounds(), ampImage, cabImage);
    AttachSelectorAreaComponent(*pGraphics, pGraphics->GetBounds(), chevronLeft, chevronRight);
    AttachControlAreaComponent(*pGraphics, pGraphics->GetBounds(), powerIcon);
    AttachSettingsScreenComponent(*pGraphics, pGraphics->GetBounds(), settingsBackIcon);
  };
}

NeuralAmpModeler::~NeuralAmpModeler()
{
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
      SendControlMsgFromDelegate(kCtrlTagModelThumbnail, kMsgTagClearModel);
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
    SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    SendControlMsgFromDelegate(kCtrlTagLibrarySidebar, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    SendControlMsgFromDelegate(kCtrlTagModelThumbnail, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    // If it's not loaded yet, then mark as failed.
    // If it's yet to be loaded, then the completion handler will set us straight once it runs.
    if (mModel == nullptr && mStagedModel == nullptr)
      SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadFailed);
  }

  if (mIRPath.GetLength())
  {
    SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
    SendControlMsgFromDelegate(kCtrlTagLibrarySidebar, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
    if (mIR == nullptr && mStagedIR == nullptr)
      SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadFailed);
  }

  if (mModel != nullptr)
  {
    _UpdateControlsFromModel();
  }
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

void NeuralAmpModeler::_RefreshLibrarySidebar()
{
  if (auto* pGraphics = GetUI())
  {
    if (auto* sidebar = pGraphics->GetControlWithTag(kCtrlTagLibrarySidebar))
      sidebar->As<NAMLibrarySidebarControl>()->SetRoots(mNAMRootDirectory.Get(), mIRRootDirectory.Get());
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
  apply(mStagedModel.get());
}

std::string NeuralAmpModeler::_StageModel(const WDL_String& modelPath)
{
  WDL_String previousNAMPath = mNAMPath;
  try
  {
    auto dspPath = std::filesystem::u8path(modelPath.Get());
    std::unique_ptr<nam::DSP> model = nam::get_dsp(dspPath);

    // Check that the model has 1 input and 1 output channel
    if (model->NumInputChannels() != 1)
    {
      throw std::runtime_error("Model must have 1 input channel, but has " + std::to_string(model->NumInputChannels()));
    }
    if (model->NumOutputChannels() != 1)
    {
      throw std::runtime_error("Model must have 1 output channel, but has "
                               + std::to_string(model->NumOutputChannels()));
    }

    std::unique_ptr<ResamplingNAM> temp = std::make_unique<ResamplingNAM>(std::move(model), GetSampleRate());
    temp->Reset(GetSampleRate(), GetBlockSize());
    if (nam::SlimmableModel* slimmable = temp->GetSlimmableModel())
    {
      slimmable->SetSlimmableSize(GetParam(kSlim)->Value());
    }
    mStagedModel = std::move(temp);
    mNAMPath = modelPath;
    SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    SendControlMsgFromDelegate(kCtrlTagLibrarySidebar, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
    SendControlMsgFromDelegate(kCtrlTagModelThumbnail, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  }
  catch (std::runtime_error& e)
  {
    SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadFailed);

    if (mStagedModel != nullptr)
    {
      mStagedModel = nullptr;
    }
    mNAMPath = previousNAMPath;
    std::cerr << "Failed to read DSP module" << std::endl;
    std::cerr << e.what() << std::endl;
    return e.what();
  }
  return "";
}

dsp::wav::LoadReturnCode NeuralAmpModeler::_StageIR(const WDL_String& irPath)
{
  // FIXME it'd be better for the path to be "staged" as well. Just in case the
  // path and the model got caught on opposite sides of the fence...
  WDL_String previousIRPath = mIRPath;
  const double sampleRate = GetSampleRate();
  dsp::wav::LoadReturnCode wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
  try
  {
    auto irPathU8 = std::filesystem::u8path(irPath.Get());
    mStagedIR = std::make_unique<dsp::ImpulseResponse>(irPathU8.string().c_str(), sampleRate);
    wavState = mStagedIR->GetWavState();
  }
  catch (std::runtime_error& e)
  {
    wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
    std::cerr << "Caught unhandled exception while attempting to load IR:" << std::endl;
    std::cerr << e.what() << std::endl;
  }

  if (wavState == dsp::wav::LoadReturnCode::SUCCESS)
  {
    mIRPath = irPath;
    SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
    SendControlMsgFromDelegate(kCtrlTagLibrarySidebar, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
  }
  else
  {
    if (mStagedIR != nullptr)
    {
      mStagedIR = nullptr;
    }
    mIRPath = previousIRPath;
    SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadFailed);
  }

  return wavState;
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
