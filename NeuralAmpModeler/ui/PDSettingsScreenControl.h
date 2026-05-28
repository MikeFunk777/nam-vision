#pragma once

// Included from NeuralAmpModeler.cpp inside its anonymous namespace.
// Keep these UI building blocks header-only until the Xcode project is split into compiled sources.

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
    DrawInputSection(g);
    DrawOutputSection(g);
    DrawModelInformationSection(g);
    DrawPluginInformationSection(g);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (GetBackButtonBounds().Contains(x, y))
    {
      Hide(true);
      if (auto* ui = GetUI())
        ui->SetAllControlsDirty();
      return;
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

    if (!mInputCalibrationDisabled && GetInputCalibrationFieldBounds().Contains(x, y))
    {
      const IText text(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);
      mEditingInputCalibrationLevel = true;
      GetUI()->CreateTextEntry(*this, text, GetInputCalibrationFieldBounds(), FormatInputCalibrationLevel().c_str());
      return;
    }

    if (!mInputCalibrationDisabled && GetCalibrateInputRadioHitBounds().Contains(x, y))
    {
      const double value = PLUG()->GetParam(kCalibrateInput)->Bool() ? 0.0 : 1.0;
      SetParameterFromUI(kCalibrateInput, value);
      SetDirty(false);
      return;
    }

    const int outputModeIndex = GetOutputModeIndexForPoint(x, y);
    if (outputModeIndex >= 0)
    {
      SetParameterFromUI(kOutputMode, PLUG()->GetParam(kOutputMode)->ToNormalized(outputModeIndex));
      SetDirty(false);
      return;
    }
  }

  void OnTextEntryCompletion(const char* str, int valIdx) override
  {
    if (!mEditingInputCalibrationLevel)
      return;

    mEditingInputCalibrationLevel = false;

    char* end = nullptr;
    const double value = std::strtod(str, &end);
    if (end == str)
    {
      SetDirty(false);
      return;
    }

    const auto* param = PLUG()->GetParam(kInputCalibrationLevel);
    const double clippedValue = std::clamp(value, param->GetMin(), param->GetMax());
    SetParameterFromUI(kInputCalibrationLevel, param->ToNormalized(clippedValue));
    SetDirty(false);
  }

  void SetInputCalibrationDisabled(bool disabled)
  {
    mInputCalibrationDisabled = disabled;
    SetDirty(false);
  }

  void SetOutputModeSupport(bool normalizedSupported, bool calibratedSupported)
  {
    mOutputNormalizedSupported = normalizedSupported;
    mOutputCalibratedSupported = calibratedSupported;
    SetDirty(false);
  }

  void SetModelSampleRate(double sampleRate)
  {
    mModelSampleRate = sampleRate;
    mHasModelSampleRate = sampleRate > 0.0;
    SetDirty(false);
  }

  void ClearModelSampleRate()
  {
    mModelSampleRate = 0.0;
    mHasModelSampleRate = false;
    SetDirty(false);
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

  IRECT GetInputSectionBounds() const
  {
    const float left = mRECT.MW() - (kSettingsInputContainerWidth / 2.f);
    return IRECT(left, mRECT.T + kSettingsInputContainerTop, left + kSettingsInputContainerWidth,
                 mRECT.T + kSettingsInputContainerTop + kSettingsInputContainerHeight);
  }

  IRECT GetOutputSectionBounds() const
  {
    const float left = mRECT.MW() - (kSettingsInputContainerWidth / 2.f);
    return IRECT(left, mRECT.T + kSettingsOutputContainerTop, left + kSettingsInputContainerWidth,
                 mRECT.T + kSettingsOutputContainerTop + kSettingsOutputContainerHeight);
  }

  IRECT GetModelInformationSectionBounds() const
  {
    const float left = mRECT.MW() - (kSettingsInputContainerWidth / 2.f);
    return IRECT(left, mRECT.T + kSettingsModelInfoContainerTop, left + kSettingsInputContainerWidth,
                 mRECT.T + kSettingsModelInfoContainerTop + kSettingsModelInfoContainerHeight);
  }

  IRECT GetPluginInformationSectionBounds() const
  {
    const float left = mRECT.MW() - (kSettingsInputContainerWidth / 2.f);
    return IRECT(left, mRECT.T + kSettingsPluginInfoContainerTop, left + kSettingsInputContainerWidth,
                 mRECT.T + kSettingsPluginInfoContainerTop + kSettingsPluginInfoContainerHeight);
  }

  IRECT GetInputSectionContentBounds() const
  {
    const auto container = GetInputSectionBounds();
    return IRECT(container.L, container.T + kSettingsInputContainerPaddingY, container.R,
                 container.B - kSettingsInputContainerPaddingY - kMainHeaderBorderSize);
  }

  IRECT GetOutputSectionContentBounds() const
  {
    const auto container = GetOutputSectionBounds();
    return IRECT(container.L, container.T + kSettingsInputContainerPaddingY, container.R,
                 container.B - kSettingsInputContainerPaddingY - kMainHeaderBorderSize);
  }

  IRECT GetModelInformationSectionContentBounds() const
  {
    const auto container = GetModelInformationSectionBounds();
    return IRECT(container.L, container.T + kSettingsInputContainerPaddingY, container.R,
                 container.B - kSettingsInputContainerPaddingY - kMainHeaderBorderSize);
  }

  IRECT GetPluginInformationSectionContentBounds() const
  {
    const auto container = GetPluginInformationSectionBounds();
    return IRECT(container.L, container.T + kSettingsInputContainerPaddingY, container.R,
                 container.B - kSettingsInputContainerPaddingY - kMainHeaderBorderSize);
  }

  IRECT GetInputTitleBounds() const
  {
    const auto content = GetInputSectionContentBounds();
    return IRECT(content.L, content.T, content.L + kSettingsInputLabelWidth,
                 content.T + kSettingsInputLineHeight);
  }

  IRECT GetOutputTitleBounds() const
  {
    const auto content = GetOutputSectionContentBounds();
    return IRECT(content.L, content.T, content.L + kSettingsInputLabelWidth,
                 content.T + kSettingsInputLineHeight);
  }

  IRECT GetModelInformationTitleBounds() const
  {
    const auto content = GetModelInformationSectionContentBounds();
    return IRECT(content.L, content.T, content.L + kSettingsInputLabelWidth,
                 content.T + kSettingsInputLineHeight);
  }

  IRECT GetPluginInformationTitleBounds() const
  {
    const auto content = GetPluginInformationSectionContentBounds();
    return IRECT(content.L, content.T, content.L + kSettingsInputLabelWidth,
                 content.T + kSettingsInputLineHeight);
  }

  IRECT GetInputCalibrationFieldBounds() const
  {
    const auto title = GetInputTitleBounds();
    const float left = title.R + kSettingsInputColumnGap;
    return IRECT(left, title.T, left + kSettingsInputFieldWidth, title.T + kSettingsInputFieldHeight);
  }

  IRECT GetOutputEmptyFieldBounds() const
  {
    const auto title = GetOutputTitleBounds();
    const float left = title.R + kSettingsInputColumnGap;
    return IRECT(left, title.T, left + kSettingsInputFieldWidth, title.T + kSettingsInputFieldHeight);
  }

  IRECT GetModelInformationEmptyFieldBounds() const
  {
    const auto title = GetModelInformationTitleBounds();
    const float left = title.R + kSettingsInputColumnGap;
    return IRECT(left, title.T, left + kSettingsInputFieldWidth, title.T + kSettingsInputFieldHeight);
  }

  IRECT GetPluginInformationEmptyFieldBounds() const
  {
    const auto title = GetPluginInformationTitleBounds();
    const float left = title.R + kSettingsInputColumnGap;
    return IRECT(left, title.T, left + kSettingsInputFieldWidth, title.T + kSettingsInputFieldHeight);
  }

  IRECT GetInputDescriptionBounds() const
  {
    const auto field = GetInputCalibrationFieldBounds();
    const float left = field.R + kSettingsInputColumnGap;
    return IRECT(left, field.T, left + kSettingsInputDescriptionWidth,
                 field.T + kSettingsInputDescriptionHeight);
  }

  IRECT GetOutputModeAreaBounds() const
  {
    const auto field = GetOutputEmptyFieldBounds();
    const float left = field.R + kSettingsInputColumnGap;
    return IRECT(left, field.T, left + kSettingsInputDescriptionWidth,
                 field.T + kSettingsOutputRadioStackHeight);
  }

  IRECT GetModelInformationValueBounds() const
  {
    const auto field = GetModelInformationEmptyFieldBounds();
    const float left = field.R + kSettingsInputColumnGap;
    return IRECT(left, field.T, left + kSettingsInputDescriptionWidth, field.T + kSettingsInputLineHeight);
  }

  IRECT GetPluginInformationValueBounds() const
  {
    const auto field = GetPluginInformationEmptyFieldBounds();
    const float left = field.R + kSettingsInputColumnGap;
    return IRECT(left, field.T, left + kSettingsInputDescriptionWidth, field.T + kSettingsPluginInfoTextHeight);
  }

  IRECT GetDefaultCalibrateInputRadioBounds() const
  {
    const auto description = GetInputDescriptionBounds();
    const float top = description.B + kSettingsInputDescriptionRadioGap;
    return IRECT(description.L, top, description.R, top + kSettingsInputLineHeight);
  }

  IRECT GetCalibrateInputRadioHitBounds() const
  {
    return mCalibrateInputRadioBounds.W() > 0.f ? mCalibrateInputRadioBounds : GetDefaultCalibrateInputRadioBounds();
  }

  int GetOutputModeIndexForPoint(float x, float y) const
  {
    for (int i = 0; i < 3; ++i)
    {
      if (mOutputModeRadioBounds[i].W() > 0.f && mOutputModeRadioBounds[i].Contains(x, y))
        return i;
    }

    const auto area = GetOutputModeAreaBounds();
    for (int i = 0; i < 3; ++i)
    {
      const float top = area.T + (i * (kSettingsInputLineHeight + kSettingsOutputRadioGapY));
      if (IRECT(area.L, top, area.R, top + kSettingsInputLineHeight).Contains(x, y))
        return i;
    }

    return -1;
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

  void DrawInputSection(IGraphics& g)
  {
    const auto container = GetInputSectionBounds();
    const auto borderBounds =
      IRECT(container.L, container.B - kMainHeaderBorderSize, container.R, container.B);
    const IBlend disabledBlend(EBlend::Default, mInputCalibrationDisabled ? 0.5f : 1.f);
    const IText titleText(kSelectorTextSize, kPDForeground, kPDFontBold, EAlign::Far, EVAlign::Top);
    const IText fieldText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Center, EVAlign::Middle);
    const IText descriptionText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Near, EVAlign::Top);
    const IText radioText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Near, EVAlign::Middle);
    const auto fieldBounds = GetInputCalibrationFieldBounds();
    const auto descriptionBounds = GetInputDescriptionBounds();
    const float descriptionBottom = DrawWrappedText(
      g, descriptionText,
      "The analog level, in dBu RMS, that corresponds to digital level of 0 dBFS peak in the host as "
      "its signal enters this plugin.",
      descriptionBounds, &mBlend);
    const auto radioBounds = IRECT(descriptionBounds.L, descriptionBottom + kSettingsInputDescriptionRadioGap,
                                   descriptionBounds.R,
                                   descriptionBottom + kSettingsInputDescriptionRadioGap + kSettingsInputLineHeight);
    const float radioTop = radioBounds.MH() - (kSettingsInputRadioSize / 2.f);
    const auto radioCircleBounds =
      IRECT(radioBounds.L, radioTop, radioBounds.L + kSettingsInputRadioSize, radioTop + kSettingsInputRadioSize);
    const auto radioLabelBounds =
      IRECT(radioCircleBounds.R + kSettingsInputRadioLabelGap, radioBounds.T, radioBounds.R, radioBounds.B);

    g.DrawText(titleText, "Input", GetInputTitleBounds(), &mBlend);
    g.DrawRoundRect(kPDForeground, fieldBounds, kSettingsInputFieldRadius, &disabledBlend,
                    kSettingsInputFieldBorderSize);
    g.DrawText(fieldText, FormatInputCalibrationLevel().c_str(), fieldBounds, &disabledBlend);
    mCalibrateInputRadioBounds = radioBounds;

    if (PLUG()->GetParam(kCalibrateInput)->Bool())
      g.FillEllipse(kPDForeground, radioCircleBounds, &disabledBlend);
    else
      g.DrawEllipse(kPDForeground, radioCircleBounds, &disabledBlend, 1.f);

    g.DrawText(radioText, "Calibrate Input", radioLabelBounds, &disabledBlend);
    g.FillRect(kPDLightGrey, borderBounds);
  }

  void DrawOutputSection(IGraphics& g)
  {
    const auto container = GetOutputSectionBounds();
    const auto borderBounds =
      IRECT(container.L, container.B - kMainHeaderBorderSize, container.R, container.B);
    const IText titleText(kSelectorTextSize, kPDForeground, kPDFontBold, EAlign::Far, EVAlign::Top);
    const IText radioText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Near, EVAlign::Middle);
    const auto radioArea = GetOutputModeAreaBounds();
    const int selectedIndex = PLUG()->GetParam(kOutputMode)->Int();

    g.DrawText(titleText, "Output", GetOutputTitleBounds(), &mBlend);

    DrawRadioRow(g, radioText, GetOutputModeRadioBounds(radioArea, 0), "Raw", selectedIndex == 0, &mBlend);
    DrawRadioRow(g, radioText, GetOutputModeRadioBounds(radioArea, 1),
                 GetOutputModeLabel(1, "Normalized", mOutputNormalizedSupported).c_str(), selectedIndex == 1,
                 &mBlend);
    DrawRadioRow(g, radioText, GetOutputModeRadioBounds(radioArea, 2),
                 GetOutputModeLabel(2, "Calibrated", mOutputCalibratedSupported).c_str(), selectedIndex == 2,
                 &mBlend);

    g.FillRect(kPDLightGrey, borderBounds);
  }

  void DrawModelInformationSection(IGraphics& g)
  {
    const auto container = GetModelInformationSectionBounds();
    const auto borderBounds =
      IRECT(container.L, container.B - kMainHeaderBorderSize, container.R, container.B);
    const IText titleText(kSelectorTextSize, kPDForeground, kPDFontBold, EAlign::Far, EVAlign::Top);
    const IText valueText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Near, EVAlign::Top);

    g.DrawText(titleText, "Model Information", GetModelInformationTitleBounds(), &mBlend);
    g.DrawText(valueText, FormatModelSampleRate().c_str(), GetModelInformationValueBounds(), &mBlend);
    g.FillRect(kPDLightGrey, borderBounds);
  }

  void DrawPluginInformationSection(IGraphics& g)
  {
    const auto container = GetPluginInformationSectionBounds();
    const auto borderBounds =
      IRECT(container.L, container.B - kMainHeaderBorderSize, container.R, container.B);
    const IText titleText(kSelectorTextSize, kPDForeground, kPDFontBold, EAlign::Far, EVAlign::Top);
    const IText valueText(kSelectorTextSize, kPDForeground, kPDFontMedium, EAlign::Near, EVAlign::Top);

    g.DrawText(titleText, "Plugin", GetPluginInformationTitleBounds(), &mBlend);
    DrawWrappedText(g, valueText,
                    "Based on Neural Amp Modeler by Steven Atkinson. Modified by Kyle Wetton.",
                    GetPluginInformationValueBounds(), &mBlend);
    g.FillRect(kPDLightGrey, borderBounds);
  }

  IRECT GetOutputModeRadioBounds(const IRECT& radioArea, int index)
  {
    const float top = radioArea.T + (index * (kSettingsInputLineHeight + kSettingsOutputRadioGapY));
    mOutputModeRadioBounds[index] = IRECT(radioArea.L, top, radioArea.R, top + kSettingsInputLineHeight);
    return mOutputModeRadioBounds[index];
  }

  void DrawRadioRow(IGraphics& g, const IText& textStyle, const IRECT& bounds, const char* label, bool selected,
                    const IBlend* blend)
  {
    const float radioTop = bounds.MH() - (kSettingsInputRadioSize / 2.f);
    const auto radioCircleBounds =
      IRECT(bounds.L, radioTop, bounds.L + kSettingsInputRadioSize, radioTop + kSettingsInputRadioSize);
    const auto labelBounds =
      IRECT(radioCircleBounds.R + kSettingsInputRadioLabelGap, bounds.T, bounds.R, bounds.B);

    if (selected)
      g.FillEllipse(kPDForeground, radioCircleBounds, blend);
    else
      g.DrawEllipse(kPDForeground, radioCircleBounds, blend, 1.f);

    g.DrawText(textStyle, label, labelBounds, blend);
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

  void SetParameterFromUI(int paramIdx, double normalizedValue)
  {
    GetDelegate()->SendParameterValueFromUI(paramIdx, normalizedValue);
    if (auto* ui = GetUI())
      ui->SetAllControlsDirty();
  }

  std::string FormatInputCalibrationLevel()
  {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << PLUG()->GetParam(kInputCalibrationLevel)->Value() << " dBu";
    return ss.str();
  }

  static std::string GetOutputModeLabel(int index, const char* label, bool supported)
  {
    if (index == 0 || supported)
      return label;

    return std::string(label) + " [Not supported by model]";
  }

  std::string FormatModelSampleRate() const
  {
    if (!mHasModelSampleRate)
      return "";

    if (mModelSampleRate >= 1000.0)
    {
      const double sampleRateKhz = mModelSampleRate / 1000.0;
      const double roundedSampleRateKhz = std::round(sampleRateKhz);
      std::stringstream ss;

      if (std::fabs(sampleRateKhz - roundedSampleRateKhz) < 0.05)
        ss << static_cast<int>(roundedSampleRateKhz);
      else
        ss << std::fixed << std::setprecision(1) << sampleRateKhz;

      ss << "kHz";
      return ss.str();
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << mModelSampleRate << "Hz";
    return ss.str();
  }

  static float DrawWrappedText(IGraphics& g, const IText& textStyle, const std::string& text, const IRECT& bounds,
                               const IBlend* blend)
  {
    std::istringstream words(text);
    std::string word;
    std::string line;
    float top = bounds.T;
    float bottom = bounds.T;
    IRECT measured;

    while (words >> word)
    {
      const std::string candidate = line.empty() ? word : line + " " + word;
      g.MeasureText(textStyle, candidate.c_str(), measured);

      if (!line.empty() && measured.W() > bounds.W())
      {
        g.DrawText(textStyle, line.c_str(), IRECT(bounds.L, top, bounds.R, top + kSettingsInputLineHeight), blend);
        top += kSettingsInputLineHeight;
        bottom = top;
        line = word;
        if (top + kSettingsInputLineHeight > bounds.B)
          break;
      }
      else
      {
        line = candidate;
      }
    }

    if (!line.empty() && top + kSettingsInputLineHeight <= bounds.B)
    {
      g.DrawText(textStyle, line.c_str(), IRECT(bounds.L, top, bounds.R, top + kSettingsInputLineHeight), blend);
      bottom = top + kSettingsInputLineHeight;
    }

    return bottom;
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

  ISVG mBackIcon;
  IRECT mCalibrateInputRadioBounds;
  std::array<IRECT, 3> mOutputModeRadioBounds;
  bool mEditingInputCalibrationLevel = false;
  bool mInputCalibrationDisabled = false;
  bool mOutputNormalizedSupported = true;
  bool mOutputCalibratedSupported = true;
  double mModelSampleRate = 0.0;
  bool mHasModelSampleRate = false;
};
