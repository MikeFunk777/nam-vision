#pragma once

// Included from NeuralAmpModeler.cpp after the NeuralAmpModeler class is declared.
// These workflow files split the implementation without changing Xcode target membership.

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
    case kMsgTagClearModel:
    {
      ++mModelLoadRequestId;
      mModelLoadInProgress = false;
      {
        std::lock_guard<std::mutex> lock(mAsyncLoadMutex);
        mPendingModel = nullptr;
        mPendingModelPath.clear();
        mPendingModelError.clear();
        mPendingModelLoadComplete = false;
      }
      {
        std::lock_guard<std::mutex> lock(mStagingMutex);
        mStagedModel = nullptr;
      }
      _UpdateSettingsModelSampleRate(0.0);
      mShouldRemoveModel = true;
      return true;
    }
    case kMsgTagClearIR:
    {
      ++mIRLoadRequestId;
      mIRLoadInProgress = false;
      {
        std::lock_guard<std::mutex> lock(mAsyncLoadMutex);
        mPendingIR = nullptr;
        mPendingIRPath.clear();
        mPendingIRState = dsp::wav::LoadReturnCode::ERROR_OTHER;
        mPendingIRLoadComplete = false;
      }
      {
        std::lock_guard<std::mutex> lock(mStagingMutex);
        mStagedIR = nullptr;
      }
      mShouldRemoveIR = true;
      return true;
    }
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
