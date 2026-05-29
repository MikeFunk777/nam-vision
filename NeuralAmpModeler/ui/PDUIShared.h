#pragma once

// Included from NeuralAmpModeler.cpp inside its anonymous namespace.
// Keep these UI building blocks header-only until the Xcode project is split into compiled sources.

constexpr const char* kPDFontMedium = "PD-Medium";
constexpr const char* kPDFontBold = "PD-Bold";
constexpr float kMainHeaderHeight = 40.f;
constexpr float kMainHeaderBorderSize = 2.f;
constexpr float kMainHeaderPadding = 10.f;
constexpr float kMainHeaderLogoWidth = 80.f;
constexpr float kMainHeaderLogoHeight = 15.f;
constexpr float kMainHeaderSlotSize = 20.f;
constexpr float kHeaderLoadingBarHeight = 6.f;
constexpr float kHeaderLoadingBarFillTimeMs = 1200.f;
constexpr float kHeaderLoadingBarFrameTimeMs = 33.f;
constexpr float kMainAreaTop = 42.f;
constexpr float kMainAreaHeight = 357.f;
constexpr float kInputMeterWidth = 64.f;
constexpr float kMainMeterWidth = 4.f;
constexpr float kMainMeterHeight = 220.f;
constexpr float kAmpImageWidth = 440.f;
constexpr float kMainAreaSpacerWidth = 32.f;
constexpr float kCabImageWidth = 220.f;
constexpr float kOutputMeterWidth = 64.f;
constexpr float kMainLibraryNoticeWidth = 260.f;
constexpr float kMainLibraryNoticeTextHeight = 42.f;
constexpr float kMainLibraryNoticeButtonGap = 12.f;
constexpr float kMainLibraryNoticeButtonWidth = 88.f;
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
constexpr float kSettingsRemoveButtonGap = 10.f;
constexpr float kSettingsRemoveButtonWidth = 58.f;
constexpr float kSettingsPathSetterHeight = (2.f * kSettingsPathSetterPadding) + kSettingsPathTitleHeight +
                                            kSettingsPathTitleValueGap + kSettingsPathValueHeight +
                                            kSettingsPathTitleButtonGap + kSettingsButtonHeight;
constexpr float kSettingsPathContainerTop = kMainHeaderHeight;
constexpr float kSettingsPathContainerHeight = (2.f * kSettingsPathContainerPaddingY) + kSettingsPathSetterHeight +
                                               kMainHeaderBorderSize;
constexpr float kSettingsInputContainerWidth = 480.f;
constexpr float kSettingsInputContainerPaddingY = 18.f;
constexpr float kSettingsInputLabelWidth = 120.f;
constexpr float kSettingsInputFieldWidth = 51.f;
constexpr float kSettingsInputFieldPaddingX = 8.f;
constexpr float kSettingsInputFieldPaddingY = 4.f;
constexpr float kSettingsInputFieldHeight = kSelectorTextSize + (2.f * kSettingsInputFieldPaddingY);
constexpr float kSettingsInputFieldRadius = 2.f;
constexpr float kSettingsInputFieldBorderSize = 2.f;
constexpr float kSettingsInputDescriptionWidth = 245.f;
constexpr float kSettingsInputColumnGap = 32.f;
constexpr float kSettingsInputDescriptionRadioGap = 10.f;
constexpr float kSettingsInputRadioSize = 10.f;
constexpr float kSettingsInputRadioLabelGap = 10.f;
constexpr float kSettingsInputLineHeight = 16.f;
constexpr float kSettingsInputDescriptionHeight = 48.f;
constexpr float kSettingsInputContainerTop = kSettingsPathContainerTop + kSettingsPathContainerHeight;
constexpr float kSettingsInputContainerHeight =
  (2.f * kSettingsInputContainerPaddingY) + kSettingsInputDescriptionHeight +
  kSettingsInputDescriptionRadioGap + kSettingsInputLineHeight + kMainHeaderBorderSize;
constexpr float kSettingsOutputRadioGapY = 10.f;
constexpr float kSettingsOutputRadioStackHeight = (3.f * kSettingsInputLineHeight) + (2.f * kSettingsOutputRadioGapY);
constexpr float kSettingsOutputContainerTop = kSettingsInputContainerTop + kSettingsInputContainerHeight;
constexpr float kSettingsOutputContainerHeight =
  (2.f * kSettingsInputContainerPaddingY) + kSettingsOutputRadioStackHeight + kMainHeaderBorderSize;
constexpr float kSettingsModelInfoContainerTop = kSettingsOutputContainerTop + kSettingsOutputContainerHeight;
constexpr float kSettingsModelInfoContainerHeight =
  (2.f * kSettingsInputContainerPaddingY) + kSettingsInputLineHeight + kMainHeaderBorderSize;
constexpr float kSettingsPluginInfoTextHeight = 48.f;
constexpr float kSettingsPluginInfoContainerTop = kSettingsModelInfoContainerTop + kSettingsModelInfoContainerHeight;
constexpr float kSettingsPluginInfoContainerHeight =
  (2.f * kSettingsInputContainerPaddingY) + kSettingsPluginInfoTextHeight + kMainHeaderBorderSize;
constexpr int kAmpSelectorColumns = 4;
constexpr float kAmpSelectorContentTop = kMainHeaderHeight + kMainHeaderBorderSize;
constexpr float kAmpSelectorCardWidth = 205.f;
constexpr float kAmpSelectorCardPadding = 12.f;
constexpr float kAmpSelectorImageSize = 181.f;
constexpr int kAmpSelectorThumbnailPixelSize = 362;
constexpr float kAmpSelectorTitleMinHeight = 18.f;
constexpr float kAmpSelectorTitlePaddingY = 3.f;
constexpr float kAmpSelectorTitleAreaHeight = 70.f;
constexpr float kAmpSelectorCardHeight =
  (2.f * kAmpSelectorCardPadding) + kAmpSelectorImageSize + kAmpSelectorTitleAreaHeight;
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
