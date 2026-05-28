#include <algorithm> // std::clamp, std::min
#include <array>
#include <chrono>
#include <cmath> // pow
#include <fstream>
#include <filesystem>
#include <iomanip>
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
#include "ui/PDUIShared.h"

#include "ui/PDMainAreaControl.h"

#include "ui/PDSelectorAreaControl.h"

#include "ui/PDMainControls.h"

#include "ui/PDSettingsScreenControl.h"

#include "ui/PDAmpSelectorScreenControl.h"


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

#include "ui/PDUIAttach.h"


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
    const auto noSelectionImage = pGraphics->LoadBitmap(PD_NO_SELECTION_FN);
    const auto chevronLeft = pGraphics->LoadSVG(PD_CHEVRON_LEFT_FN);
    const auto chevronRight = pGraphics->LoadSVG(PD_CHEVRON_RIGHT_FN);

    AttachShellComponent(*pGraphics);
    AttachMainHeaderComponent(*pGraphics, pGraphics->GetBounds(), logo, settingsIcon);
    AttachMainAreaComponent(*pGraphics, pGraphics->GetBounds(), noAmpImage, noCabImage, noAmpImage, noCabImage);
    AttachSelectorAreaComponent(*pGraphics, pGraphics->GetBounds(), chevronLeft, chevronRight);
    AttachControlAreaComponent(*pGraphics, pGraphics->GetBounds(), powerIcon);
    AttachSettingsScreenComponent(*pGraphics, pGraphics->GetBounds(), settingsBackIcon);
    AttachAmpSelectorScreenComponent(*pGraphics, pGraphics->GetBounds(), settingsBackIcon, noAmpImage,
                                     noSelectionImage);
    AttachCabSelectorScreenComponent(*pGraphics, pGraphics->GetBounds(), settingsBackIcon, noCabImage,
                                     noSelectionImage);
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

#include "workflows/PDPluginAudioProcessing.h"

#include "workflows/PDPluginStateAndUI.h"

#include "workflows/PDPluginAsyncCompletions.h"

#include "workflows/PDPluginLibraryDirectories.h"

#include "workflows/PDPluginFileLoading.h"

#include "workflows/PDPluginLibraryPersistence.h"

#include "workflows/PDPluginParametersAndMessages.h"

#include "workflows/PDPluginDSPInternals.h"

// HACK
#include "Unserialization.cpp"
