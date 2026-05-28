#pragma once

// Included from NeuralAmpModeler.cpp inside its anonymous namespace.
// Keep these UI building blocks header-only until the Xcode project is split into compiled sources.

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
  const auto middleSlot = headerBounds.GetCentredInside(kMainHeaderLogoWidth, kMainHeaderLogoHeight);
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
  graphics.AttachControl(new PDMeterControl(mainAreaBounds.GetFromLeft(kInputMeterWidth)
                                              .GetCentredInside(kMainMeterWidth, kMainMeterHeight)),
                         kCtrlTagInputMeter);
  graphics.AttachControl(new PDMeterControl(mainAreaBounds.GetFromRight(kOutputMeterWidth)
                                              .GetCentredInside(kMainMeterWidth, kMainMeterHeight)),
                         kCtrlTagOutputMeter);
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
  graphics.AttachControl(new IVLabelControl(layout.title, "NAM Division", titleStyle));
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
                                      const IBitmap& fallbackImage, const IBitmap& noSelectionImage)
{
  graphics.AttachControl(new PDAmpSelectorScreenControl(bounds, backIcon, fallbackImage, noSelectionImage, "Amps", "nam",
                                                        PDAmpSelectorScreenControl::Target::Amp),
                         kCtrlTagAmpSelectorScreen)
    ->Hide(true);
}

void AttachCabSelectorScreenComponent(IGraphics& graphics, const IRECT& bounds, const ISVG& backIcon,
                                      const IBitmap& fallbackImage, const IBitmap& noSelectionImage)
{
  graphics.AttachControl(new PDAmpSelectorScreenControl(bounds, backIcon, fallbackImage, noSelectionImage, "Cabs", "wav",
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
