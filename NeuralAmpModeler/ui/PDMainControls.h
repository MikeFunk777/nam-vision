#pragma once

// Included from NeuralAmpModeler.cpp inside its anonymous namespace.
// Keep these UI building blocks header-only until the Xcode project is split into compiled sources.

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
