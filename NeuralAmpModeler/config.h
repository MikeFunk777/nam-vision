#define PLUG_NAME "NAM Division"
#define PLUG_MFR "Pedal Division"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'NMDv'
#define PLUG_MFR_ID 'NDiv'
#define PLUG_URL_STR "https://pedaldivision.com"
#define PLUG_EMAIL_STR ""
#define PLUG_COPYRIGHT_STR "Copyright 2026 Pedal Division"
#define PLUG_CLASS_NAME NeuralAmpModeler
#define BUNDLE_NAME "namdivision"
#define BUNDLE_MFR "PedalDivision"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "namdivision"

#ifdef APP_API
  #define PLUG_CHANNEL_IO "1-2"
#else
  #define PLUG_CHANNEL_IO "1-1 1-2 2-2"
#endif

#define PLUG_LATENCY 0
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 820
#define PLUG_HEIGHT 562
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0
#define PLUG_MAX_WIDTH PLUG_WIDTH * 4
#define PLUG_MAX_HEIGHT PLUG_HEIGHT * 4

#define AUV2_ENTRY NeuralAmpModeler_Entry
#define AUV2_ENTRY_STR "NeuralAmpModeler_Entry"
#define AUV2_FACTORY NeuralAmpModeler_Factory
#define AUV2_VIEW_CLASS NeuralAmpModeler_View
#define AUV2_VIEW_CLASS_STR "NeuralAmpModeler_View"

#define AAX_TYPE_IDS 'NDv1'
#define AAX_TYPE_IDS_AUDIOSUITE 'NDvA'
#define AAX_PLUG_MFR_STR "Pedal Division"
#define AAX_PLUG_NAME_STR "NAM Division\nIPEF"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 1

#define VST3_SUBCATEGORY "Fx"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTO_FN "Roboto-Regular.ttf"
#define MICHROMA_FN "Michroma-Regular.ttf"

#define GEAR_FN "Gear.svg"
#define FILE_FN "File.svg"
#define CLOSE_BUTTON_FN "Cross.svg"
#define LEFT_ARROW_FN "ArrowLeft.svg"
#define RIGHT_ARROW_FN "ArrowRight.svg"
#define MODEL_ICON_FN "ModelIcon.svg"
#define IR_ICON_ON_FN "IRIconOn.svg"
#define IR_ICON_OFF_FN "IRIconOff.svg"
#define GLOBE_ICON_FN "Globe.svg"
#define SLIMMABLE_ICON_FN "SlimmableIcon.svg"

#define PD_LOGO_FN "pd/logo.svg"
#define PD_ICON_SETTINGS_FN "pd/icon-settings.svg"
#define PD_ICON_ARROW_LEFT_FN "pd/icon-arrow-left.svg"
#define PD_ICON_POWER_FN "pd/icon-power.svg"
#define PD_EXAMPLE_AMP_FN "pd/example-amp.jpg"
#define PD_EXAMPLE_CAB_FN "pd/example-cab.jpg"
#define PD_NO_AMP_FN "pd/no-amp.jpg"
#define PD_NO_CAB_FN "pd/no-cab.jpg"
#define PD_NO_SELECTION_FN "pd/no-selection.jpg"
#define PD_CHEVRON_LEFT_FN "pd/chevron-left.svg"
#define PD_CHEVRON_RIGHT_FN "pd/chevron-right.svg"
#define PD_FONT_MEDIUM_FN "pd/medium.ttf"
#define PD_FONT_BOLD_FN "pd/bold.ttf"

#define BACKGROUND_FN "Background.jpg"
#define BACKGROUND2X_FN "Background@2x.jpg"
#define BACKGROUND3X_FN "Background@3x.jpg"
#define KNOBBACKGROUND_FN "KnobBackground.png"
#define KNOBBACKGROUND2X_FN "KnobBackground@2x.png"
#define KNOBBACKGROUND3X_FN "KnobBackground@3x.png"
#define FILEBACKGROUND_FN "FileBackground.png"
#define FILEBACKGROUND2X_FN "FileBackground@2x.png"
#define FILEBACKGROUND3X_FN "FileBackground@3x.png"
#define INPUTLEVELBACKGROUND_FN "InputLevelBackground.png"
#define INPUTLEVELBACKGROUND2X_FN "InputLevelBackground@2x.png"
#define INPUTLEVELBACKGROUND3X_FN "InputLevelBackground@3x.png"
#define LINES_FN "Lines.png"
#define LINES2X_FN "Lines@2x.png"
#define LINES3X_FN "Lines@3x.png"
#define SLIDESWITCHHANDLE_FN "SlideSwitchHandle.png"
#define SLIDESWITCHHANDLE2X_FN "SlideSwitchHandle@2x.png"
#define SLIDESWITCHHANDLE3X_FN "SlideSwitchHandle@3x.png"

#define METERBACKGROUND_FN "MeterBackground.png"
#define METERBACKGROUND2X_FN "MeterBackground@2x.png"
#define METERBACKGROUND3X_FN "MeterBackground@3x.png"

// Issue 291
// On the macOS standalone, we might not have permissions to traverse the file directory, so we have the app ask the
// user to pick a directory instead of the file in the directory.
// Everyone else is fine though.
#if defined(APP_API) && defined(__APPLE__)
  #define NAM_PICK_DIRECTORY
#endif
