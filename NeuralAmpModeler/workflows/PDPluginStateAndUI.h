#pragma once

// Included from NeuralAmpModeler.cpp after the NeuralAmpModeler class is declared.
// These workflow files split the implementation without changing Xcode target membership.

void NeuralAmpModeler::_UpdateSettingsModelSampleRate(double sampleRate)
{
  if (auto* pGraphics = GetUI())
  {
    if (auto* settings = pGraphics->GetControlWithTag(kCtrlTagSettingsBox))
    {
      if (sampleRate > 0.0)
        settings->As<PDSettingsScreenControl>()->SetModelSampleRate(sampleRate);
      else
        settings->As<PDSettingsScreenControl>()->ClearModelSampleRate();
    }
  }
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
  else
  {
    double stagedModelSampleRate = 0.0;
    {
      std::lock_guard<std::mutex> lock(mStagingMutex);
      if (mStagedModel != nullptr)
        stagedModelSampleRate = mStagedModel->GetEncapsulatedSampleRate();
    }

    _UpdateSettingsModelSampleRate(stagedModelSampleRate);
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
