#pragma once

// Included from NeuralAmpModeler.cpp after the NeuralAmpModeler class is declared.
// These workflow files split the implementation without changing Xcode target membership.

void NeuralAmpModeler::LoadNAMFile(const WDL_String& modelPath)
{
  const uint64_t requestId = ++mModelLoadRequestId;
  const std::string path(modelPath.Get());
  const double sampleRate = GetSampleRate();
  const int blockSize = GetBlockSize();
  const double slimValue = GetParam(kSlim)->Value();
  mModelLoadInProgress = true;

  std::lock_guard<std::mutex> lock(mAsyncLoadMutex);
  mLoadTasks.emplace_back(std::async(std::launch::async, [this, requestId, path, sampleRate, blockSize, slimValue]() {
    std::unique_ptr<ResamplingNAM> loadedModel;
    std::string error;

    try
    {
      auto dspPath = std::filesystem::u8path(path);
      std::unique_ptr<nam::DSP> model = nam::get_dsp(dspPath);

      if (model->NumInputChannels() != 1)
      {
        throw std::runtime_error("Model must have 1 input channel, but has "
                                 + std::to_string(model->NumInputChannels()));
      }
      if (model->NumOutputChannels() != 1)
      {
        throw std::runtime_error("Model must have 1 output channel, but has "
                                 + std::to_string(model->NumOutputChannels()));
      }

      loadedModel = std::make_unique<ResamplingNAM>(std::move(model), sampleRate);
      loadedModel->Reset(sampleRate, blockSize);
      if (nam::SlimmableModel* slimmable = loadedModel->GetSlimmableModel())
        slimmable->SetSlimmableSize(slimValue);
    }
    catch (const std::runtime_error& e)
    {
      error = e.what();
    }
    catch (const std::exception& e)
    {
      error = e.what();
    }
    catch (...)
    {
      error = "Failed to read DSP module";
    }

    if (mShuttingDown || requestId != mModelLoadRequestId.load())
      return;

    std::lock_guard<std::mutex> resultLock(mAsyncLoadMutex);
    mPendingModelRequestId = requestId;
    mPendingModelPath = path;
    mPendingModelError = error;
    mPendingModel = std::move(loadedModel);
    mPendingModelLoadComplete = true;
  }));
}

void NeuralAmpModeler::LoadIRFile(const WDL_String& irPath)
{
  const uint64_t requestId = ++mIRLoadRequestId;
  const std::string path(irPath.Get());
  const double sampleRate = GetSampleRate();
  mIRLoadInProgress = true;

  std::lock_guard<std::mutex> lock(mAsyncLoadMutex);
  mLoadTasks.emplace_back(std::async(std::launch::async, [this, requestId, path, sampleRate]() {
    std::unique_ptr<dsp::ImpulseResponse> loadedIR;
    dsp::wav::LoadReturnCode wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;

    try
    {
      auto irPathU8 = std::filesystem::u8path(path);
      loadedIR = std::make_unique<dsp::ImpulseResponse>(irPathU8.string().c_str(), sampleRate);
      wavState = loadedIR->GetWavState();
    }
    catch (const std::runtime_error& e)
    {
      wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
      std::cerr << "Caught unhandled exception while attempting to load IR:" << std::endl;
      std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
      wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
    }

    if (wavState != dsp::wav::LoadReturnCode::SUCCESS)
      loadedIR = nullptr;

    if (mShuttingDown || requestId != mIRLoadRequestId.load())
      return;

    std::lock_guard<std::mutex> resultLock(mAsyncLoadMutex);
    mPendingIRRequestId = requestId;
    mPendingIRPath = path;
    mPendingIRState = wavState;
    mPendingIR = std::move(loadedIR);
    mPendingIRLoadComplete = true;
  }));
}
