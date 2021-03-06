#include "stdafx.h"
#include "Direct2DRender.h"

#pragma region Edit
EditElement::EditElement()
{
    fontProperties.fontFamily = _T("Microsoft Yahei");
    fontProperties.size = 12;
}

EditElement::~EditElement()
{
    renderer->Finalize();
}

CString EditElement::GetElementTypeName()
{
    return _T("Edit");
}

cint EditElement::GetTypeId()
{
    return Edit;
}

CColor EditElement::GetColor() const
{
    return color;
}

void EditElement::SetColor(CColor value)
{
    if (color != value)
    {
        color = value;
        if (renderer)
        {
            renderer->OnElementStateChanged();
        }
    }
}

const Font& EditElement::GetFont() const
{
    return fontProperties;
}

void EditElement::SetFont(const Font& value)
{
    if (fontProperties != value)
    {
        fontProperties = value;
        if (renderer)
        {
            renderer->OnElementStateChanged();
        }
    }
}

const CString& EditElement::GetText() const
{
    return text;
}

void EditElement::SetText(const CString& value)
{
    if (text != value)
    {
        text = value;
        if (renderer)
        {
            renderer->OnElementStateChanged();
        }
    }
}

const bool EditElement::IsMultiline() const
{
    return multiline;
}

void EditElement::SetMultiline(const bool& value)
{
    if (multiline != value)
    {
        multiline = value;
        if (renderer)
        {
            renderer->OnElementStateChanged();
        }
    }
}

const bool EditElement::HasCaret() const
{
    return caret;
}

void EditElement::SetCaret(const bool& value)
{
    caret = value;
}

void EditElementRenderer::Render(CRect bounds, CComPtr<ID2D1RenderTarget> r)
{
    auto e = element.lock();
    if (e->flags.self_visible)
    {
        if (!textLayout)
        {
            CreateTextLayout();
        }

        auto x = bounds.left;
        auto y = bounds.top + (bounds.Height() - minSize.cy) / 2;

        auto rt = renderTarget.lock();
        rt->SetTextAntialias(oldFont.antialias, oldFont.verticalAntialias);

        auto dwriteFactory = Direct2D::Singleton().GetDirectWriteFactory();
        DWRITE_TRIMMING trimming;
        CComPtr<IDWriteInlineObject> inlineObject;
        auto hr = textLayout->GetTrimming(&trimming, &inlineObject);
        hr = textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        hr = textLayout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        hr = textLayout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        if (!e->IsMultiline())
        {
            inlineObject = nullptr;
            hr = dwriteFactory->CreateEllipsisTrimmingSign(textLayout, &inlineObject);
            trimming.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
            trimming.delimiter = 1;
            trimming.delimiterCount = 3;
        }
        hr = textLayout->SetTrimming(&trimming, inlineObject);

        CRect textBounds = bounds;
        hr = textLayout->SetMaxWidth((FLOAT)textBounds.Width());
        hr = textLayout->SetMaxHeight((FLOAT)textBounds.Height());

        auto d2dRenderTarget = r ? r : rt->GetDirect2DRenderTarget();
        d2dRenderTarget->DrawTextLayout(
            D2D1::Point2F((FLOAT)textBounds.left, (FLOAT)textBounds.top),
            textLayout,
            brush,
            D2D1_DRAW_TEXT_OPTIONS_NO_SNAP
        );
        if (e->HasCaret()) {
            DWRITE_TEXT_METRICS metrics;
            hr = textLayout->GetMetrics(&metrics);
            if (SUCCEEDED(hr))
            {
                auto right = (FLOAT)(textBounds.left + metrics.widthIncludingTrailingWhitespace + 3.0f);
                d2dRenderTarget->DrawLine(
                    { right, (FLOAT)textBounds.top + metrics.top },
                    { right, (FLOAT)(textBounds.top + metrics.top + metrics.height) }, brush, 1.0f);
            }
        }

        if (oldMaxWidth != textBounds.Width())
        {
            oldMaxWidth = textBounds.Width();
            UpdateMinSize();
        }
    }
    GraphicsRenderer::Render(bounds, r);
}

void EditElementRenderer::OnElementStateChanged()
{
    auto e = element.lock();
    auto rt = renderTarget.lock();
    if (rt)
    {
        CColor color = e->GetColor();
        if (oldColor != color)
        {
            DestroyBrush(rt);
            CreateBrush(rt);
        }

        Font font = e->GetFont();
        if (oldFont != font)
        {
            DestroyTextFormat(rt);
            CreateTextFormat(rt);
        }
    }
    oldText = e->GetText();
    UpdateMinSize();
}

void EditElementRenderer::CreateBrush(std::shared_ptr<Direct2DRenderTarget> _renderTarget)
{
    if (_renderTarget)
    {
        oldColor = element.lock()->GetColor();
        brush = _renderTarget->CreateDirect2DBrush(oldColor);
    }
}

void EditElementRenderer::DestroyBrush(std::shared_ptr<Direct2DRenderTarget> _renderTarget)
{
    if (_renderTarget && brush)
    {
        brush = nullptr;
    }
}

void EditElementRenderer::CreateTextFormat(std::shared_ptr<Direct2DRenderTarget> _renderTarget)
{
    if (_renderTarget)
    {
        oldFont = element.lock()->GetFont();
        textFormat = _renderTarget->CreateDirect2DTextFormat(oldFont);
    }
}

void EditElementRenderer::DestroyTextFormat(std::shared_ptr<Direct2DRenderTarget> _renderTarget)
{
    if (_renderTarget && textFormat)
    {
        textFormat = nullptr;
    }
}

void EditElementRenderer::CreateTextLayout()
{
    if (textFormat)
    {
        BSTR _text = oldText.AllocSysString();
        HRESULT hr = Direct2D::Singleton().GetDirectWriteFactory()->CreateTextLayout(
            _text,
            oldText.GetLength(),
            textFormat->textFormat,
            0,
            0,
            &textLayout);
        SysFreeString(_text);
        if (SUCCEEDED(hr))
        {
            if (oldFont.underline)
            {
                DWRITE_TEXT_RANGE textRange;
                textRange.startPosition = 0;
                textRange.length = oldText.GetLength();
                textLayout->SetUnderline(TRUE, textRange);
            }
            if (oldFont.strikeline)
            {
                DWRITE_TEXT_RANGE textRange;
                textRange.startPosition = 0;
                textRange.length = oldText.GetLength();
                textLayout->SetStrikethrough(TRUE, textRange);
            }
        }
        else
        {
            textLayout = nullptr;
        }
    }
}

void EditElementRenderer::DestroyTextLayout()
{
    if (textLayout)
    {
        textLayout = nullptr;
    }
}

void EditElementRenderer::UpdateMinSize()
{
    float maxWidth = 0;
    DestroyTextLayout();
    bool calculateSizeFromTextLayout = false;
    auto e = element.lock();
    auto rt = renderTarget.lock();
    if (rt)
    {
        CreateTextLayout();
        if (textLayout)
        {
            maxWidth = textLayout->GetMaxWidth();
            if (oldMaxWidth != -1)
            {
                textLayout->SetMaxWidth((float)oldMaxWidth);
            }
            calculateSizeFromTextLayout = true;
        }
    }
    if (calculateSizeFromTextLayout)
    {
        DWRITE_TEXT_METRICS metrics;
        HRESULT hr = textLayout->GetMetrics(&metrics);
        if (SUCCEEDED(hr))
        {
            cint width = (cint)ceil(metrics.widthIncludingTrailingWhitespace);
            minSize = CSize(width, (cint)ceil(metrics.height));
        }
        textLayout->SetMaxWidth(maxWidth);
    }
    else
    {
        minSize = CSize();
    }
}

void EditElementRenderer::InitializeInternal()
{

}

void EditElementRenderer::FinalizeInternal()
{
    DestroyTextLayout();
    auto rt = renderTarget.lock();
    DestroyBrush(rt);
    DestroyTextFormat(rt);
}

void EditElementRenderer::RenderTargetChangedInternal(std::shared_ptr<Direct2DRenderTarget> oldRenderTarget, std::shared_ptr<Direct2DRenderTarget> newRenderTarget)
{
    DestroyBrush(oldRenderTarget);
    DestroyTextFormat(oldRenderTarget);
    CreateBrush(newRenderTarget);
    CreateTextFormat(newRenderTarget);
    UpdateMinSize();
}
#pragma endregion Edit
