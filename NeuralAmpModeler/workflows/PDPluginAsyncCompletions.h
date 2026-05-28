#pragma once

// Included from NeuralAmpModeler.cpp after the NeuralAmpModeler class is declared.
// These workflow files split the implementation without changing Xcode target membership.

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
      const double completedModelSampleRate = completedModel->GetEncapsulatedSampleRate();
      {
        std::lock_guard<std::mutex> lock(mStagingMutex);
        mStagedModel = std::move(completedModel);
        mNAMPath.Set(completedModelPath.c_str());
      }
      _UpdateSettingsModelSampleRate(completedModelSampleRate);
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
