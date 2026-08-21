#include "Widgets/SInputTextField.h"
#include "Widgets/SFGUICanvas.h"
#include "UI/UIConfig.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/GObject.h"
#include "UI/GRoot.h"
#include "Widgets/SShape.h"
#include "Widgets/SSelectionShape.h"
#include "HAL/PlatformTime.h"
#include "UI/UIPackage.h"
#include "Widgets/Font/BaseFont.h"
#include "Widgets/Font/FontManager.h"
#include "Math/UnrealMathUtility.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/KismetStringLibrary.h"

float SInputTextField::CARET_BLINK_INTERVAL = 0.5;

IMPLEMENT_TYPE_ID(SInputTextField)

SInputTextField::SInputTextField(UGObject* InGObject) : SRichTextField(InGObject), m_RestrictPattern(TEXT("*"))
{
	m_ClipType = ETextClipType::ByPixel;
	SetMouseCursor(EMouseCursor::TextEditBeam);
	SetFocusable(true);
	SetTabStop(true);
	BuildCharPositions();
}

void SInputTextField::SetRestrict(const FString& Restrict)
{
	if (m_Restrict != Restrict)
	{
		m_Restrict = UKismetStringLibrary::Trim(Restrict);
		if (!m_Restrict.IsEmpty())
			m_RestrictPattern = FRegexPattern(m_Restrict);
	}
}

void SInputTextField::CollectRenderUnits(
	FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha, float InSaturation) const
{
	// 缓存几何信息供 IME 使用
	SInputTextField* MutableThis = (SInputTextField*)this;
	MutableThis->CaretBlink();
	SRichTextField::CollectRenderUnits(ParentGroup, AllottedGeometry, InAlpha, InSaturation);
}

bool SInputTextField::OnFocusReceived(EFocusCause InCause)
{
	if (!m_bEditing)
		m_bRebuildText = true;
	m_bEditing = true;

	m_bTextChanged = false;

	// 初始化 Undo/Redo 历史
	m_UndoBuffer.Reset();
	m_RedoBuffer.Reset();
	m_CurrentUndoText = m_Text;
	m_bUndoLock = false;

	if (!m_pCaret.IsValid())
		CreateCaret();

	BuildLines();

	float CaretSize = FUIConfig::Config.InputCaretSize;
	// 如果界面缩小过，光标很容易看不见，这里放大一下
	if (GObject.IsValid())
	{
		float UIScale = UWidgetLayoutLibrary::GetViewportScale(GObject->GetWorld());
		if (CaretSize == 1 && UIScale < 1)
			CaretSize /= UIScale;
	}
	m_pCaret->SetSize(CaretSize, m_TextFormat.Size);
	if (m_TextFormat.OutlineSize > 0)
		m_pCaret->SetColor(m_TextFormat.OutlineColor);
	else if (!m_TextFormat.ShadowOffset.IsNearlyZero())
		m_pCaret->SetColor(m_TextFormat.ShadowColor);
	else
		m_pCaret->SetColor(m_TextFormat.Color);
	m_pCaret->SetVisible(m_bEditable);

	m_pSelectionShape->Clear();
	m_pSelectionShape->SetVisible(false);

	// 获得焦点时重置选区头：文本可能已被 SetText 清空/替换，旧选区头残留会导致 UpdateSelection 越界
	auto& cp = GetCharPosition(m_CaretPosition);
	AdjustCaret(cp.CharIndex, true);

	if (InCause == EFocusCause::Navigation)
	{
		SetSelection(0, -1);
		m_HaveNavigationTab = 2;
	}

	m_CachedGeometry = GetPaintSpaceGeometry();

	// 把 Slate 键盘焦点切到 FairyGUI 画布，让按键事件沿焦点路径进入画布；
	// 画布会消费所有按键（见 SFGUICanvas::OnKeyDown），阻止其冒泡到 SViewport 触发引擎输入
	if (GObject.IsValid())
	{
		UGRoot::Get(GObject.Get())->ReturnFocus();
	}

	// 注册并激活 IME 上下文
	if (!m_bDisableIME)
	{
		RegisterIME();
	}

	// 清除之前的组合状态
	m_bIsComposing = false;
	m_Composing = 0;
	m_CompositionBegin = 0;

	if (GObject.IsValid())
		GObject->DispatchEvent(FUIEvents::GetFocus);

	return true;
}

void SInputTextField::OnFocusLost(EFocusCause InCause)
{
	if (!m_bEditing)
		return;

	// 提前标记为非编辑状态，阻止后续 IME 回调（如 SetTextInRange）修改文字
	m_bEditing = false;

	// 清除 Undo/Redo 历史
	m_UndoBuffer.Reset();
	m_RedoBuffer.Reset();
	m_CurrentUndoText.Empty();

	// 如果正在 IME 组合中，放弃组合文字
	if (m_bIsComposing && m_Composing > 0)
	{
		m_Text.RemoveAt(m_CompositionBegin, m_Composing);
		// 恢复组合前选区
		m_SelectionStart = m_CompositionSelectionStart;
		m_CaretPosition = m_CompositionSelectionEnd;

		if (m_IMEChangeNotifier.IsValid())
		{
			m_IMEChangeNotifier->CancelComposition();
		}
	}

	// 注销 IME
	UnregisterIME();

	m_bRebuildText = true;
	m_pCaret->SetVisible(false);
	m_pSelectionShape->SetVisible(false);

	m_HaveNavigationTab = 0;

	// 清除组合状态
	m_bIsComposing = false;
	m_Composing = 0;
	m_CompositionBegin = 0;

	// 失焦时若开启自动提交且有输入/删改，触发 Submit 事件
	if (m_bAutoSubmitOnLostFocus && m_bTextChanged)
	{
		if (GObject.IsValid())
			GObject->DispatchEvent(FUIEvents::Submit, FNVariant(m_Text));
	}

	if (GObject.IsValid())
		GObject->DispatchEvent(FUIEvents::LostFocus);

	// 失焦后把 Slate 键盘焦点归还给游戏视口，恢复引擎按键输入
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

bool SInputTextField::OnKeyChar(const FCharacterEvent& InCharacterEvent)
{
	if (!m_bEditing || !m_bEditable)
		return false;

	// IME 组合中时，字符由 IME 系统通过 SetTextInRange 处理
	if (m_bIsComposing)
		return false;

	TCHAR TypedChar = InCharacterEvent.GetCharacter();
	bool  bAddChar = false;
	if (TChar<TCHAR>::IsPrint(TypedChar) || TypedChar == 0x3000)
	{
		bAddChar = true;
	}
	if (bAddChar)
	{
		ProcessInputChar(TypedChar);
		return true;
	}
	return false;
}
bool SInputTextField::OnKeyDown(const FKeyEvent& InKeyEvent)
{
	if (!m_bEditing)
		return false;
	if (m_HaveNavigationTab > 0)
		m_HaveNavigationTab--;
	FKey Key = InKeyEvent.GetKey();
	bool Handled = true;
	if (Key == EKeys::BackSpace)
	{
		// IME 组合中禁止删除（由 IME 系统处理退格）
		if (m_bIsComposing)
			return true;
		if (InKeyEvent.IsCommandDown())
		{
			if (m_SelectionStart == m_CaretPosition && m_CaretPosition < m_CharPositions.Num() - 1)
				m_SelectionStart = m_CaretPosition + 1;
		}
		else
		{
			if (m_SelectionStart == m_CaretPosition && m_CaretPosition > 0)
				m_SelectionStart = m_CaretPosition - 1;
		}
		if (m_bEditable)
			ReplaceSelection(G_EMPTY_STRING);
	}
	else if (Key == EKeys::Delete)
	{
		// IME 组合中禁止删除
		if (m_bIsComposing)
			return true;
		if (m_SelectionStart == m_CaretPosition && m_CaretPosition < m_CharPositions.Num() - 1)
			m_SelectionStart = m_CaretPosition + 1;
		if (m_bEditable)
			ReplaceSelection(G_EMPTY_STRING);
	}
	else if (Key == EKeys::Left)
	{
		if (!InKeyEvent.IsShiftDown())
			ClearSelection();
		if (m_CaretPosition > 0)
		{
			if (InKeyEvent.IsCommandDown())
			{
				auto& cp = GetCharPosition(m_CaretPosition);
				auto  pLine = m_Lines[cp.LineIndex];
				auto& cp2 = GetCharPosition(INT_MIN, pLine->Y);
				AdjustCaret(cp2.CharIndex, !InKeyEvent.IsShiftDown());
			}
			else
			{
				auto& cp = GetCharPosition(m_CaretPosition - 1);
				AdjustCaret(cp.CharIndex, !InKeyEvent.IsShiftDown());
			}
		}
	}
	else if (Key == EKeys::Right)
	{
		if (!InKeyEvent.IsShiftDown())
			ClearSelection();
		if (m_CaretPosition < m_CharPositions.Num() - 1)
		{
			if (InKeyEvent.IsCommandDown())
			{
				auto& cp = GetCharPosition(m_CaretPosition);
				auto  pLine = m_Lines[cp.LineIndex];
				auto& cp2 = GetCharPosition(float(INT_MAX), pLine->Y);
				AdjustCaret(cp2.CharIndex, !InKeyEvent.IsShiftDown());
			}
			else
			{
				auto& cp = GetCharPosition(m_CaretPosition + 1);
				AdjustCaret(cp.CharIndex, !InKeyEvent.IsShiftDown());
			}
		}
	}
	else if (Key == EKeys::Up)
	{
		if (!InKeyEvent.IsShiftDown())
			ClearSelection();

		auto& cp = GetCharPosition(m_CaretPosition);
		if (cp.LineIndex > 0)
		{
			auto  pLine = m_Lines[cp.LineIndex - 1];
			auto& cp2 = GetCharPosition(cp.OffsetX, pLine->Y);
			AdjustCaret(cp2.CharIndex, !InKeyEvent.IsShiftDown());
		}
	}
	else if (Key == EKeys::Down)
	{
		if (!InKeyEvent.IsShiftDown())
			ClearSelection();

		auto& cp = GetCharPosition(m_CaretPosition);
		if (cp.LineIndex == m_Lines.Num() - 1)
		{
			auto& cp2 = GetCharPosition(m_CharPositions.Num() - 1);
			AdjustCaret(cp2.CharIndex, !InKeyEvent.IsShiftDown());
		}
		else
		{
			auto  pLine = m_Lines[cp.LineIndex + 1];
			auto& cp2 = GetCharPosition(cp.OffsetX, pLine->Y);
			AdjustCaret(cp2.CharIndex, !InKeyEvent.IsShiftDown());
		}
	}
	else if (Key == EKeys::PageUp)
	{
		ClearSelection();
	}
	else if (Key == EKeys::PageDown)
	{
		ClearSelection();
	}
	else if (Key == EKeys::Home)
	{
		if (!InKeyEvent.IsShiftDown())
			ClearSelection();
		auto& cp = GetCharPosition(m_CaretPosition);
		auto  pLine = m_Lines[cp.LineIndex];
		auto& cp2 = GetCharPosition(INT_MIN, pLine->Y);
		AdjustCaret(cp2.CharIndex, !InKeyEvent.IsShiftDown());
	}
	else if (Key == EKeys::End)
	{
		if (!InKeyEvent.IsShiftDown())
			ClearSelection();
		auto& cp = GetCharPosition(m_CaretPosition);
		auto  pLine = m_Lines[cp.LineIndex];
		auto& cp2 = GetCharPosition(float(INT_MAX), pLine->Y);
		AdjustCaret(cp2.CharIndex, !InKeyEvent.IsShiftDown());
	}
	else if (InKeyEvent.IsControlDown() || InKeyEvent.IsCommandDown())
	{
		if (Key == EKeys::A)
		{
			m_SelectionStart = 0;
			AdjustCaret(GetCharPosition(INT_MAX).CharIndex, false);
		}
		else if (Key == EKeys::C)
		{
			DoCopy();
		}
		else if (Key == EKeys::V)
		{
			// IME 组合中禁止粘贴
			if (m_bEditable && !m_bIsComposing)
				DoPaste();
		}
		else if (Key == EKeys::X)
		{
			DoCopy();
			// IME 组合中禁止剪切
			if (m_bEditable && !m_bIsComposing)
				ReplaceSelection(G_EMPTY_STRING);
		}
		else if (Key == EKeys::Z)
		{
			if (m_bEditable && !m_bIsComposing)
			{
				if (InKeyEvent.IsShiftDown())
					PerformRedo();
				else
					PerformUndo();
			}
		}
		else if (Key == EKeys::Y)
		{
			if (m_bEditable && !m_bIsComposing)
				PerformRedo();
		}
		else
		{
			Handled = false;
		}
	}
	else if (Key == EKeys::Enter)
	{
		if (m_bSingleLine || (!m_bWantReturn && !InKeyEvent.IsShiftDown()))
		{
			if (GObject.IsValid() && !m_bIsComposing)
			{
				m_bTextChanged = false;
				GObject->DispatchEvent(FUIEvents::Submit, FNVariant(m_Text));
				// UGRoot::Get(GObject.Get())->ReturnFocus();
			}
			else
			{
				Handled = false;
			}
		}
		else
		{
			if (!m_bIsComposing)
				ProcessInputChar(TEXT('\n'));
			else
				Handled = false;
		}
	}
	else if (Key == EKeys::Tab && ((m_bWantTab && m_HaveNavigationTab <= 0) || InKeyEvent.IsShiftDown()))
	{
		ProcessInputChar(TEXT('\t'));
	}
	else if (Key == EKeys::Escape)
	{
		// 如果正在 IME 组合中，放弃组合文字但不丢失焦点
		if (m_bIsComposing && m_Composing > 0)
		{
			m_Text.RemoveAt(m_CompositionBegin, m_Composing);
			// 恢复组合前选区
			m_SelectionStart = m_CompositionSelectionStart;
			m_CaretPosition = m_CompositionSelectionEnd;

			// 清除组合状态
			m_bIsComposing = false;
			m_Composing = 0;
			m_CompositionSelectionStart = 0;
			m_CompositionSelectionEnd = 0;
			m_bRebuildText = true;

			// 通知 IME 系统取消组合
			if (m_IMEChangeNotifier.IsValid())
			{
				m_IMEChangeNotifier->CancelComposition();
			}

			return true;
		}
		if (GObject.IsValid())
		{
			m_bTextChanged = false;
			GObject->DispatchEvent(FUIEvents::Cancel);
		}

		return true;
	}
	else
	{
		Handled = false;
	}

	return Handled;
}
bool SInputTextField::OnMouseButtonDown(const FPointerEvent& MouseEvent)
{
	if (m_HaveNavigationTab > 0)
		m_HaveNavigationTab--;
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (!m_pCaret.IsValid())
			CreateCaret();

		ClearSelection();

		FVector2D v = MouseEvent.GetScreenSpacePosition();
		v = GetDesktopSpaceGeometry().AbsoluteToLocal(v);
		auto& cp = GetCharPosition(v.X, v.Y);

		AdjustCaret(cp.CharIndex, true);
		m_bCaretDraging = true;
		SFGUICanvas::CaptureMouse(SharedThis(this));

		return SRichTextField::OnMouseButtonDown(MouseEvent);
	}
	return SRichTextField::OnMouseButtonDown(MouseEvent);
}
bool SInputTextField::OnMouseButtonUp(const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		m_bCaretDraging = false;
		if (SFGUICanvas::HasMouseCapture())
		{
			SFGUICanvas::ReleaseMouseCapture();
		}
		return SRichTextField::OnMouseButtonUp(MouseEvent);
	}
	return SRichTextField::OnMouseButtonUp(MouseEvent);
}
bool SInputTextField::OnMouseMove(const FPointerEvent& MouseEvent)
{
	if (m_bEditing && m_bCaretDraging)
	{
		FVector2D v = MouseEvent.GetScreenSpacePosition();
		v = GetDesktopSpaceGeometry().AbsoluteToLocal(v);
		if (!FMath::IsNaN(v.X))
		{
			auto& cp = GetCharPosition(v.X, v.Y);
			if (cp.CharIndex != m_CaretPosition)
				AdjustCaret(cp.CharIndex, false);
		}
	}
	return SRichTextField::OnMouseMove(MouseEvent);
}
bool SInputTextField::OnMouseButtonDoubleClick(const FPointerEvent& MouseEvent)
{
	if (m_bEditing)
	{
		m_SelectionStart = 0;
		AdjustCaret(GetCharPosition(INT_MAX).CharIndex, false);
	}
	return SRichTextField::OnMouseButtonDoubleClick(MouseEvent);
}
bool SInputTextField::OnMouseWheel(const FPointerEvent& MouseEvent)
{
	if (m_bEditing && m_bMouseWheelEnabled)
	{
		auto& cp = GetCharPosition(GUTTER_X, GUTTER_Y);
		int	  vScroll = cp.LineIndex;
		int	  hScroll = cp.CharIndex - m_Lines[cp.LineIndex]->CharIndex;
		if (MouseEvent.GetWheelDelta() > 0)
			vScroll--;
		else
			vScroll++;
		Scroll(hScroll, vScroll);
	}
	return SRichTextField::OnMouseWheel(MouseEvent);
}

void SInputTextField::BuildLines()
{
	if (m_bRebuildText)
	{
		if (m_bEditing)
		{
			// IME 组合中时，使用 HTML <u> 标签为组合文字添加下划线
			if (m_bIsComposing && m_Composing > 0)
			{
				// 提取三段文本：组合前 + 组合中 + 组合后
				FString BeforeComposition = m_Text.Left(m_CompositionBegin);
				FString CompositionText = m_Text.Mid(m_CompositionBegin, m_Composing);
				FString AfterComposition = m_Text.Mid(m_CompositionBegin + m_Composing);

				// XML 转义，防止用户输入的 < > & 等被误解析为 HTML
				auto EscapeXML = [](FString& Str) {
					Str.ReplaceInline(TEXT("&"), TEXT("&amp;"));
					Str.ReplaceInline(TEXT("<"), TEXT("&lt;"));
					Str.ReplaceInline(TEXT(">"), TEXT("&gt;"));
				};
				EscapeXML(BeforeComposition);
				EscapeXML(CompositionText);
				EscapeXML(AfterComposition);

				// 组合文字用 <u> 标签包裹实现下划线
				FString DisplayText =
					BeforeComposition + TEXT("<u>") + CompositionText + TEXT("</u>") + AfterComposition;
				ParseText(DisplayText, false, true);
			}
			else
			{
				ParseText(m_Text, false, false);
			}
		}
		else
			ParseText(m_Text.IsEmpty() ? m_PromptText : m_Text, true, true);
		m_bRebuildText = false;
		m_bRebuildGlyph = true;
	}

	if (m_bRebuildGlyph)
	{
		m_FontSizeScale = 1.0f;
		BuildGlyphs();
		if (m_AutoSize == EAutoSizeType::Shrink)
			DoShrink();
		else if (m_AutoSize == EAutoSizeType::Ellipsis)
			DoEllipsis();
		UpdateSize();
		m_bRebuildGlyph = false;
		m_bRebuildMesh = true;
	}
	if (m_bRebuildMesh)
	{
		BuildMesh();
		m_bRebuildMesh = false;
		BuildCharPositions();
		if (m_bEditing)
			UpdateCaret(m_bMakeCaretVisible);
		m_bMakeCaretVisible = false;
	}
}

void SInputTextField::BuildCharPositions()
{
	m_CharPositions.Reset();
	m_CharPositions.SetNum(m_Text.Len() + 1);
	LineInfo* pLine = nullptr;
	if (m_Text.Len())
	{
		float LetterSpacing = m_TextFormat.LetterSpacing * m_FontSizeScale;

		for (int32 l = 0; l < m_Lines.Num(); l++)
		{
			pLine = m_Lines[l];
			if (pLine->Blocks.Num())
			{
				for (auto pBlock : pLine->Blocks)
				{
					float X = pBlock->X;
					for (int32 c = 0; c < pBlock->Chars.Num(); c++)
					{
						auto pChar = pBlock->Chars[c];
						if (pChar->CharIndex >= 0 && pChar->CharIndex < m_CharPositions.Num())
						{
							auto& Pos = m_CharPositions[pChar->CharIndex];
							Pos.CharIndex = pChar->CharIndex;
							Pos.LineIndex = l;
							Pos.OffsetX = X;
							Pos.OffsetY = pBlock->Y;
							Pos.Width = pChar->Width;
							X += Pos.Width;
							if (c)
								X += LetterSpacing;
						}
					}
				}
			}
			else
			{
				auto& Pos = m_CharPositions[pLine->CharIndex];
				Pos.CharIndex = pLine->CharIndex;
				Pos.LineIndex = l;
				Pos.OffsetX = pLine->X;
				Pos.OffsetY = pLine->Y;
				Pos.Width = 0;
			}
		}
	}
	for (int32 i = 0; i < m_CharPositions.Num(); i++)
	{
		auto& Pos = m_CharPositions[i];
		if (Pos.LineIndex < 0)
		{
			Pos.CharIndex = i;
			if (i)
			{
				auto Prev = m_CharPositions[i - 1];
				Pos.LineIndex = Prev.LineIndex;
				Pos.OffsetX = Prev.OffsetX + Prev.Width;
				Pos.OffsetY = Prev.OffsetY;
				Pos.Width = 0;
			}
			else
			{
				if (m_Lines.Num())
				{
					pLine = m_Lines[0];
					Pos.LineIndex = 0;
					Pos.OffsetX = pLine->X;
					Pos.OffsetY = pLine->Y;
					Pos.Width = 0;
				}
				else
				{
					Pos.LineIndex = 0;
					Pos.OffsetX = 0;
					Pos.OffsetY = 0;
					Pos.Width = 0;
				}
			}
		}
	}
}

const SInputTextField::FCharPosition& SInputTextField::GetCharPosition(int32 CaretIndex)
{
	static FCharPosition s_Empty;
	if (m_CharPositions.Num() <= 0)
		return s_Empty;
	CaretIndex = FMath::Clamp(CaretIndex, 0, m_CharPositions.Num() - 1);
	while (CaretIndex < m_CharPositions.Num() - 1 && m_CharPositions[CaretIndex].LineIndex < 0)
		CaretIndex++;
	return m_CharPositions[CaretIndex];
}
const SInputTextField::FCharPosition& SInputTextField::GetCharPosition(float X, float Y)
{
	static FCharPosition s_Empty;
	if (m_CharPositions.Num() <= 0)
		return s_Empty;
	if (m_CharPositions.Num() <= 1)
		return m_CharPositions[0];

	int32				  len = m_Lines.Num();
	int32				  i;
	STextField::LineInfo* pLine;
	for (i = 0; i < len; i++)
	{
		pLine = m_Lines[i];
		if (pLine->Y + pLine->Height > Y)
			break;
	}
	if (i == len)
		i = len - 1;

	int32 lineIndex = i;

	len = m_CharPositions.Num();
	int32 FirstInLine = -1;
	for (i = 0; i < len; i++)
	{
		auto& v = m_CharPositions[i];
		if (v.LineIndex == lineIndex)
		{
			if (FirstInLine == -1)
				FirstInLine = i;
			if (v.OffsetX + v.Width * 0.5f > X)
				return v;
		}
		else if (FirstInLine != -1)
		{
			if (i - 1 < m_Text.Len() && m_Text[i - 1] == '\n')
				return m_CharPositions[i - 1];
			else
				return v;
		}
	}

	return m_CharPositions[i - 1];
}

void SInputTextField::CreateCaret()
{
	m_pCaret = MakeShared<SShape>(GObject.Get());
	AddChild(m_pCaret.ToSharedRef());
	m_pCaret->SetType(EFGUIShapeType::Rect);
	m_pCaret->SetLineColor(FColor::Transparent);
	m_pCaret->SetTouchable(false);
	m_pCaret->SetVisible(false);

	m_pSelectionShape = MakeShared<SSelectionShape>(GObject.Get());
	AddChild(m_pSelectionShape.ToSharedRef(), EDisplayObjChildLayer::Back);
	m_pSelectionShape->SetPosition(m_TextDrawOffset);
	m_pSelectionShape->SetColor(FUIConfig::Config.InputHighlightColor);
	m_pSelectionShape->SetTouchable(false);
	m_pSelectionShape->SetVisible(false);
}

void SInputTextField::AdjustCaret(int32 CaretIndex, bool MoveSelectionHeader)
{
	// IME 组合中禁止移动光标和改变选区（组合文字位置由 SetTextInRange 控制）
	if (m_bIsComposing)
		return;

	m_CaretPosition = CaretIndex;
	if (MoveSelectionHeader)
		m_SelectionStart = m_CaretPosition;
	UpdateCaret(true);
}

void SInputTextField::UpdateCaret(bool bMakeVisible)
{
	int32 CharIndex = m_CaretPosition;
	if (m_Lines.Num())
	{
		FVector2D			  Pos;
		STextField::LineInfo* pLine = nullptr;
		if (m_bEditing)
		{
			// 当 m_bIsComposing 为 true 时，组合文字已由 SetTextInRange 插入到 m_Text 中，
			// m_CaretPosition 已经在组合文字之后，不需要额外的偏移
			auto& cp = GetCharPosition(m_CaretPosition);
			Pos.X = cp.OffsetX;
			Pos.Y = cp.OffsetY;
			pLine = m_Lines[cp.LineIndex];
		}
		else
		{
			auto& cp = GetCharPosition(m_CaretPosition);
			Pos.X = cp.OffsetX;
			Pos.Y = cp.OffsetY;
			pLine = m_Lines[cp.LineIndex];
		}

		m_pCaret->SetPosition(Pos);
		m_pCaret->SetHeight(pLine->Height > 0 ? pLine->Height : m_TextFormat.Size);
		if (bMakeVisible)
		{
			// Pos、LineInfo、FCharPosition 存储的均为实际渲染坐标，
			// 可见范围为 (GUTTER_X, GUTTER_Y) 到 (GetWidth()-GUTTER_X, GetHeight()-GUTTER_Y)

			const float CaretWidth = m_pCaret.IsValid() ? m_pCaret->GetWidth() : 1.0f;
			const float LineHeight = pLine->Height > 0 ? pLine->Height : m_TextFormat.Size;

			// 考虑前一个字符，计算需要可见区域的左边界
			float LeftEdge = Pos.X;
			if (m_CaretPosition > 0)
				LeftEdge = FMath::Min(LeftEdge, GetCharPosition(m_CaretPosition - 1).OffsetX);

			// 矩形：从左边界到光标右边缘，高度为行高
			ScrollToVisible(LeftEdge, Pos.Y, (Pos.X + CaretWidth) - LeftEdge, LineHeight);
		}
	}
	else
	{
		FName CurFontName = G_DEFAULT_FONT_NAME;
		if (!m_TextFormat.Face.IsNone())
			CurFontName = m_TextFormat.Face;
		UBaseFont* pCurFont = UUIPackage::GetFontManager()->GetFont(CurFontName);
		int32	   LineHeight = pCurFont->GetLineHeight(m_TextFormat.Size);
		// 空文本时光标X位置根据对齐方式计算
		float CaretX = GUTTER_X;
		switch (m_TextFormat.Align)
		{
			case EAlignType::Center:
				CaretX = GetWidth() * 0.5f;
				break;
			case EAlignType::Right:
				CaretX = GetWidth() - GUTTER_X;
				break;
			default:
				break;
		}

		switch (m_VerticalAlign)
		{
			case EVerticalAlignType::Top:
				m_pCaret->SetXY(CaretX, GUTTER_Y);
				break;
			case EVerticalAlignType::Middle:
				m_pCaret->SetXY(CaretX, FMath::Max((GetHeight() - GUTTER_Y * 2 - LineHeight) / 2, GUTTER_Y));
				break;
			case EVerticalAlignType::Bottom:
				m_pCaret->SetXY(CaretX, GetHeight() - GUTTER_Y - LineHeight);
				break;
		}

		m_pCaret->SetHeight(LineHeight);
	}

	if (m_bEditing)
	{
		UpdateSelection(CharIndex);
		m_NextBlink = FPlatformTime::Seconds() + CARET_BLINK_INTERVAL;
		m_pCaret->SetVisible(true);
	}
}

void SInputTextField::ScrollToVisible(float X, float Y, float Width, float Height)
{
	const float ViewLeft = GUTTER_X;
	const float ViewRight = GetWidth() - GUTTER_X;
	const float ViewTop = GUTTER_Y;
	const float ViewBottom = GetHeight() - GUTTER_Y;

	// 计算使矩形可见所需的偏移量
	float dx = 0, dy = 0;

	// 水平方向：先左后右（右边界优先，以 if 而非 else-if 实现）
	if (X < ViewLeft)
		dx = ViewLeft - X;
	if (X + Width + dx > ViewRight)
		dx = ViewRight - (X + Width);

	// 垂直方向
	if (Y < ViewTop)
		dy = ViewTop - Y;
	else if (Y + Height > ViewBottom)
		dy = ViewBottom - (Y + Height);

	ApplyContentOffset(m_TextDrawOffset.X + dx, m_TextDrawOffset.Y + dy);
}

void SInputTextField::Scroll(int32 HScroll, int32 VScroll)
{
	VScroll = FMath::Clamp(VScroll, 0, m_Lines.Num() - 1);
	auto pLine = m_Lines[VScroll];
	HScroll = FMath::Clamp(HScroll, 0, pLine->CharCount - 1);

	auto cp = GetCharPosition(pLine->CharIndex + HScroll);
	// 使用视口尺寸作为矩形尺寸，ScrollToVisible 会将 cp 对齐到视口左上角
	ScrollToVisible(cp.OffsetX, cp.OffsetY, GetWidth() - GUTTER_X * 2, GetHeight() - GUTTER_Y * 2);
}

void SInputTextField::ApplyContentOffset(float NewOX, float NewOY)
{
	// 边界约束：不滚动到文字范围之外
	const float rectWidth = GetWidth() - 1; //-1 to avoid cursor be clipped
	if (rectWidth - NewOX > m_TextWidth)
		NewOX = rectWidth - m_TextWidth;
	if (GetHeight() - NewOY > m_TextHeight)
		NewOY = GetHeight() - m_TextHeight;
	if (NewOX > 0)
		NewOX = 0;
	if (NewOY > 0)
		NewOY = 0;
	NewOX = FMath::Floor(NewOX);
	NewOY = FMath::Floor(NewOY);

	if (NewOX != m_TextDrawOffset.X || NewOY != m_TextDrawOffset.Y)
	{
		m_TextDrawOffset.X = NewOX;
		m_TextDrawOffset.Y = NewOY;
		m_bRebuildMesh = true;

		for (auto pElement : m_HTMLElements)
		{
			auto pObj = pElement->GetHTMLObject();
			if (pObj)
				pObj->SetPosition(pElement->GetPosition().X + NewOX, pElement->GetPosition().Y + NewOY);
		}
	}
}

void SInputTextField::UpdateSelection(int32 cp)
{
	if (m_SelectionStart == cp)
	{
		m_pSelectionShape->Clear();
		m_pSelectionShape->SetVisible(false);
		return;
	}

	int32 start;
	if (m_bEditing && false) // 移除了 m_CompositionString 的遗留引用
	{
		if (m_SelectionStart < m_CaretPosition)
		{
			cp = m_CaretPosition;
			start = m_SelectionStart;
		}
		else
			start = m_SelectionStart; // 移除了 m_CompositionString 遗留引用
	}
	else
		start = m_SelectionStart;
	if (start > cp)
	{
		int32 tmp = start;
		start = cp;
		cp = tmp;
	}
	auto& StartPos = GetCharPosition(start);
	auto& EndPos = GetCharPosition(cp);
	// 空文本时 m_Lines 为空但 LineIndex 会被填 0，必须一并检查上界
	if (StartPos.LineIndex < 0 || StartPos.LineIndex >= m_Lines.Num() || EndPos.LineIndex < 0 || EndPos.LineIndex >= m_Lines.Num())
	{
		UE_LOG(LogFairyGUI, Error, TEXT("invalid selection(%d-%d)"), start, cp);
		return;
	}
	auto& SelectionRects = m_pSelectionShape->GetRects();
	SelectionRects.Reset();
	if (StartPos.LineIndex == EndPos.LineIndex)
	{
		auto pLine = m_Lines[StartPos.LineIndex];
		SelectionRects.Add(FBox2D(
			FVector2D(StartPos.OffsetX, StartPos.OffsetY), FVector2d(EndPos.OffsetX, EndPos.OffsetY + pLine->Height)));
	}
	else
	{
		for (int32 i = StartPos.LineIndex; i <= EndPos.LineIndex; i++)
		{
			auto pLine = m_Lines[i];
			if (pLine->Blocks.Num() == 0)
				continue;
			if (i == StartPos.LineIndex)
			{
				auto pBlock = pLine->Blocks.Last();
				SelectionRects.Add(FBox2D(FVector2D(StartPos.OffsetX, StartPos.OffsetY),
					FVector2d(pBlock->X + pBlock->Width, pBlock->Y + pLine->Height)));
			}
			else if (i == EndPos.LineIndex)
			{
				auto pBlock = pLine->Blocks[0];
				SelectionRects.Add(
					FBox2D(FVector2D(pBlock->X, pBlock->Y), FVector2d(EndPos.OffsetX, EndPos.OffsetY + pLine->Height)));
			}
			else
			{
				auto pBlock = pLine->Blocks[0];
				SelectionRects.Add(FBox2D(
					FVector2D(pBlock->X, pBlock->Y), FVector2d(pBlock->X + pLine->Width, pBlock->Y + pLine->Height)));
			}
		}
	}
	m_pSelectionShape->SetColor(FUIConfig::Config.InputHighlightColor);
	m_pSelectionShape->SetVisible(true);
}
void SInputTextField::CaretBlink()
{
	if (m_bEditing)
	{
		double CurTime = FPlatformTime::Seconds();
		if (m_NextBlink < CurTime)
		{
			m_NextBlink = CurTime + CARET_BLINK_INTERVAL;
			m_pCaret->SetVisible(!m_pCaret->IsVisible());
		}
	}
}

void SInputTextField::ClearSelection()
{
	if (m_SelectionStart != m_CaretPosition)
	{
		if (m_pSelectionShape)
		{
			m_pSelectionShape->Clear();
			m_pSelectionShape->SetVisible(false);
		}
		m_SelectionStart = m_CaretPosition;
	}
}
void SInputTextField::ReplaceSelection(const FString& Value)
{
	int32 t0, t1;
	if (m_SelectionStart != m_CaretPosition)
	{
		if (m_SelectionStart < m_CaretPosition)
		{
			t0 = m_SelectionStart;
			t1 = m_CaretPosition;
			m_CaretPosition = m_SelectionStart;
		}
		else
		{
			t0 = m_CaretPosition;
			t1 = m_SelectionStart;
			m_SelectionStart = m_CaretPosition;
		}
	}
	else
	{
		if (Value.IsEmpty())
			return;
		t0 = t1 = m_CaretPosition;
	}
	if (t0 < 0 || t0 > m_Text.Len())
		t0 = m_Text.Len();
	if (t1 < 0 || t1 > m_Text.Len())
		t1 = m_Text.Len();

	TStringBuilder<512> Buffer;
	Buffer.Append(*m_Text, t0);
	if (!Value.IsEmpty())
	{
		auto NewValue = ValidateInput(Value);
		Buffer.Append(NewValue);

		m_CaretPosition += NewValue.Len();
	}
	Buffer.Append(*m_Text + t1, m_Text.Len() - t1);

	const TCHAR* pNewText = Buffer.ToString();
	int32		 NewTextLen = Buffer.Len();
	if (m_MaxLength > 0 && m_MaxLength < NewTextLen)
		NewTextLen = TruncateText(pNewText, m_MaxLength);

	m_Text = FString(NewTextLen, pNewText);
	m_bRebuildText = true;
	m_bMakeCaretVisible = true;
	OnChanged();
}

FString SInputTextField::GetSelection()
{
	if (m_SelectionStart == m_CaretPosition)
		return G_EMPTY_STRING;

	int32 Start = m_SelectionStart;
	int32 End = m_CaretPosition;
	if (Start < 0)
		Start = m_Text.Len();
	if (End < 0)
		End = m_Text.Len();

	if (Start < End)
		return m_Text.Mid(Start, End - Start);
	else
		return m_Text.Mid(End, Start - End);
}
void SInputTextField::SetSelection(int32 Start, int32 Length)
{
	m_SelectionStart = Start;
	m_CaretPosition = Length < 0 ? INT_MAX : (Start + Length);
	BuildLines();
	int32 cnt = m_CharPositions.Num();
	if (m_CaretPosition >= cnt)
		m_CaretPosition = cnt - 1;
	if (m_SelectionStart >= cnt)
		m_SelectionStart = cnt - 1;
	UpdateCaret(true);
}

void SInputTextField::DoCopy()
{
	FString Content = GetSelection();
	FPlatformApplicationMisc::ClipboardCopy(*Content);
}
void SInputTextField::DoPaste()
{
	FString Content;
	FPlatformApplicationMisc::ClipboardPaste(Content);
	if (!Content.IsEmpty())
		ReplaceSelection(Content);
}

// ========== Undo/Redo ==========

void SInputTextField::RecordUndo()
{
	if (m_bUndoLock)
		return;

	const FString& NewText = m_Text;
	if (m_CurrentUndoText == NewText)
		return;

	m_UndoBuffer.Add(m_CurrentUndoText);
	if (m_UndoBuffer.Num() > MAX_HISTORY_LENGTH)
		m_UndoBuffer.RemoveAt(0);

	// 每次记录新状态时清空 redo 缓冲
	m_RedoBuffer.Reset();

	m_CurrentUndoText = NewText;
}

void SInputTextField::PerformUndo()
{
	if (m_UndoBuffer.Num() == 0)
		return;

	FString Text = m_UndoBuffer.Pop();
	m_RedoBuffer.Add(m_CurrentUndoText);

	m_bUndoLock = true;
	int32 CaretPos = m_CaretPosition;
	m_Text = Text;
	int32 DLen = Text.Len() - m_CurrentUndoText.Len();
	if (DLen < 0)
		m_CaretPosition = FMath::Clamp(CaretPos + DLen, 0, m_Text.Len());
	else
		m_CaretPosition = FMath::Clamp(CaretPos, 0, m_Text.Len());
	m_SelectionStart = m_CaretPosition;
	m_CurrentUndoText = Text;
	m_bUndoLock = false;

	m_bRebuildText = true;
	m_bMakeCaretVisible = true;
	// ponytail: 不触发 OnChanged，undo/redo 非用户编辑
}

void SInputTextField::PerformRedo()
{
	if (m_RedoBuffer.Num() == 0)
		return;

	FString Text = m_RedoBuffer.Pop();
	m_UndoBuffer.Add(m_CurrentUndoText);

	m_bUndoLock = true;
	int32 CaretPos = m_CaretPosition;
	m_Text = Text;
	int32 DLen = Text.Len() - m_CurrentUndoText.Len();
	if (DLen > 0)
		m_CaretPosition = FMath::Clamp(CaretPos + DLen, 0, m_Text.Len());
	else
		m_CaretPosition = FMath::Clamp(CaretPos, 0, m_Text.Len());
	m_SelectionStart = m_CaretPosition;
	m_CurrentUndoText = Text;
	m_bUndoLock = false;

	m_bRebuildText = true;
	m_bMakeCaretVisible = true;
	// ponytail: 不触发 OnChanged，undo/redo 非用户编辑
}

FString SInputTextField::ValidateInput(const FString& Source)
{
	if (!m_Restrict.IsEmpty())
	{
		// 创建正则表达式模式
		FRegexMatcher Matcher(m_RestrictPattern, Source);

		TStringBuilder<512> ResultBuilder;
		int32				LastPos = 0;

		// 遍历所有匹配项
		while (Matcher.FindNext())
		{
			int32 MatchIndex = Matcher.GetMatchBeginning();
			int32 MatchEnd = Matcher.GetMatchEnding();

			if (MatchIndex != LastPos)
			{
				// 保留 tab 和 换行符
				for (int32 i = LastPos; i < MatchIndex; ++i)
				{
					TCHAR Ch = Source[i];
					if (Ch == '\n' || Ch == '\t')
					{
						ResultBuilder.AppendChar(Ch);
					}
				}
			}

			// 添加匹配的内容
			FString MatchedString = Matcher.GetCaptureGroup(0);
			ResultBuilder.Append(MatchedString);
			LastPos = MatchEnd;
		}

		// 处理剩余部分
		for (int32 i = LastPos; i < Source.Len(); ++i)
		{
			TCHAR Ch = Source[i];
			if (Ch == '\n' || Ch == '\t')
			{
				ResultBuilder.AppendChar(Ch);
			}
		}

		return ResultBuilder.ToString();
	}
	else
	{
		return Source;
	}
}
void SInputTextField::OnChanged()
{
	m_bTextChanged = true;
	RecordUndo();
	if (GObject.IsValid())
		GObject->DispatchEvent(FUIEvents::Changed);
}

// ========== IME 管理 ==========

void SInputTextField::RegisterIME()
{
	if (m_bHasRegisteredIME)
		return;

	ITextInputMethodSystem* const TextInputMethodSystem = FSlateApplication::Get().GetTextInputMethodSystem();
	if (!TextInputMethodSystem)
		return;

	m_bHasRegisteredIME = true;
	TSharedRef<ITextInputMethodContext> ContextRef = StaticCastSharedRef<SInputTextField>(SharedThis(this));

	m_IMEChangeNotifier = TextInputMethodSystem->RegisterContext(ContextRef);
	if (m_IMEChangeNotifier.IsValid())
	{
		m_IMEChangeNotifier->NotifyLayoutChanged(ITextInputMethodChangeNotifier::ELayoutChangeType::Created);
	}

	TextInputMethodSystem->ActivateContext(ContextRef);
}

void SInputTextField::UnregisterIME()
{
	if (!m_bHasRegisteredIME)
		return;

	ITextInputMethodSystem* const TextInputMethodSystem = FSlateApplication::Get().GetTextInputMethodSystem();
	if (TextInputMethodSystem)
	{
		TSharedRef<ITextInputMethodContext> ContextRef = StaticCastSharedRef<SInputTextField>(SharedThis(this));

		if (TextInputMethodSystem->IsActiveContext(ContextRef))
		{
			TextInputMethodSystem->DeactivateContext(ContextRef);
		}

		TextInputMethodSystem->UnregisterContext(ContextRef);
	}

	m_bHasRegisteredIME = false;
	m_IMEChangeNotifier.Reset();
}

// ========== ITextInputMethodContext 接口实现 ==========

bool SInputTextField::IsComposing()
{
	return m_bIsComposing;
}

bool SInputTextField::IsReadOnly()
{
	return !m_bEditable;
}

uint32 SInputTextField::GetTextLength()
{
	return static_cast<uint32>(m_Text.Len());
}

void SInputTextField::GetSelectionRange(uint32& OutBeginIndex, uint32& OutLength, ECaretPosition& OutCaretPosition)
{
	// 组合期间向 IME 报告空选区，防止 SetTextInRange 覆盖选区内容
	if (m_bIsComposing)
	{
		OutBeginIndex = static_cast<uint32>(m_CaretPosition);
		OutLength = 0;
		OutCaretPosition = ECaretPosition::Beginning;
	}
	else if (m_SelectionStart != m_CaretPosition)
	{
		if (m_SelectionStart < m_CaretPosition)
		{
			OutBeginIndex = static_cast<uint32>(m_SelectionStart);
			OutLength = static_cast<uint32>(m_CaretPosition - m_SelectionStart);
			OutCaretPosition = ECaretPosition::Ending;
		}
		else
		{
			OutBeginIndex = static_cast<uint32>(m_CaretPosition);
			OutLength = static_cast<uint32>(m_SelectionStart - m_CaretPosition);
			OutCaretPosition = ECaretPosition::Beginning;
		}
	}
	else
	{
		OutBeginIndex = static_cast<uint32>(m_CaretPosition);
		OutLength = 0;
		OutCaretPosition = ECaretPosition::Beginning;
	}
}

void SInputTextField::SetSelectionRange(
	const uint32 InBeginIndex, const uint32 InLength, const ECaretPosition InCaretPosition)
{
	// 组合期间不修改选区和光标，保持原位
	// SetSelectionRange 由 IME 系统调用用于标记组合文字范围，
	// 但组合范围已通过 UpdateCompositionRange 单独跟踪
	if (m_bIsComposing)
		return;

	const uint32 TextLen = static_cast<uint32>(m_Text.Len());
	const uint32 MinIndex = FMath::Min(InBeginIndex, TextLen);
	const uint32 MaxIndex = FMath::Min(MinIndex + InLength, TextLen);

	switch (InCaretPosition)
	{
		case ECaretPosition::Beginning:
			m_CaretPosition = static_cast<int32>(MinIndex);
			m_SelectionStart = static_cast<int32>(MaxIndex);
			break;
		case ECaretPosition::Ending:
			m_SelectionStart = static_cast<int32>(MinIndex);
			m_CaretPosition = static_cast<int32>(MaxIndex);
			break;
	}
}

void SInputTextField::GetTextInRange(const uint32 InBeginIndex, const uint32 InLength, FString& OutString)
{
	OutString = m_Text.Mid(static_cast<int32>(InBeginIndex), static_cast<int32>(InLength));
}

void SInputTextField::SetTextInRange(const uint32 InBeginIndex, const uint32 InLength, const FString& InString)
{
	// 已失去焦点或未在编辑状态，忽略 IME 的回调
	if (!m_bEditing)
		return;

	// 在指定范围内替换文字（由 IME 系统调用，用于插入组合文字或提交最终文字）
	int32 BeginIdx = static_cast<int32>(InBeginIndex);
	int32 RangeLen = static_cast<int32>(InLength);

	// 确保索引在有效范围内
	const int32 ClampedBeginIdx = FMath::Clamp(BeginIdx, 0, m_Text.Len());
	const int32 ClampedEndIdx = FMath::Clamp(BeginIdx + RangeLen, ClampedBeginIdx, m_Text.Len());

	// 构建新文本：替换指定范围
	TStringBuilder<512> Buffer;
	Buffer.Append(*m_Text, ClampedBeginIdx);
	if (!InString.IsEmpty())
	{
		auto NewValue = ValidateInput(InString);
		Buffer.Append(NewValue);
	}
	Buffer.Append(*m_Text + ClampedEndIdx, m_Text.Len() - ClampedEndIdx);

	const TCHAR* pNewText = Buffer.ToString();
	int32		 NewTextLen = Buffer.Len();
	if (m_MaxLength > 0 && m_MaxLength < NewTextLen)
		NewTextLen = TruncateText(pNewText, m_MaxLength);

	m_Text = FString(NewTextLen, pNewText);

	// 组合期间不移动光标和选区，保持原位
	if (!m_bIsComposing)
	{
		m_CaretPosition = ClampedBeginIdx + InString.Len();
		m_SelectionStart = m_CaretPosition;
	}

	m_bRebuildText = true;
	OnChanged();
}

int32 SInputTextField::GetCharacterIndexFromPoint(const FVector2D& InPoint)
{
	FVector2D LocalPos = m_CachedGeometry.AbsoluteToLocal(InPoint);
	auto&	  cp = GetCharPosition(LocalPos.X, LocalPos.Y);
	return cp.CharIndex;
}

bool SInputTextField::GetTextBounds(
	const uint32 InBeginIndex, const uint32 InLength, FVector2D& OutPosition, FVector2D& OutSize)
{
	const int32 BeginIdx = FMath::Min(static_cast<int32>(InBeginIndex), m_CharPositions.Num() - 1);
	const int32 EndIdx = FMath::Min(static_cast<int32>(InBeginIndex + InLength), m_CharPositions.Num() - 1);

	const FCharPosition& BeginPos = GetCharPosition(BeginIdx);
	const FCharPosition& EndPos = GetCharPosition(EndIdx);

	FVector2D LocalPos(BeginPos.OffsetX, BeginPos.OffsetY);
	FVector2D LocalSize(EndPos.OffsetX - BeginPos.OffsetX, 0);

	// 获取行高
	if (BeginPos.LineIndex >= 0 && BeginPos.LineIndex < m_Lines.Num())
	{
		auto pLine = m_Lines[BeginPos.LineIndex];
		LocalSize.Y = pLine->Height;
	}
	else
	{
		LocalSize.Y = m_TextFormat.Size;
	}

	// 转换为屏幕坐标
	OutPosition = LocalPos + m_CachedGeometry.GetAbsolutePosition();
	OutSize = LocalSize;

	return false; // 未裁剪
}

void SInputTextField::GetScreenBounds(FVector2D& OutPosition, FVector2D& OutSize)
{
	OutPosition = m_CachedGeometry.GetAbsolutePosition();
	OutSize = m_CachedGeometry.GetDrawSize();
}

TSharedPtr<FGenericWindow> SInputTextField::GetWindow()
{
	TSharedPtr<SWindow> SlateWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	return SlateWindow.IsValid() ? SlateWindow->GetNativeWindow() : nullptr;
}

void SInputTextField::BeginComposition()
{
	if (!m_bIsComposing)
	{
		m_bIsComposing = true;

		// 保存组合前选区范围，用于组合结束后删除
		m_CompositionSelectionStart = m_SelectionStart;
		m_CompositionSelectionEnd = m_CaretPosition;

		// 组合文字从光标位置开始插入，选区和光标在组合期间保持不变
		m_CompositionBegin = m_CaretPosition;
		m_Composing = 0;
	}
}

void SInputTextField::UpdateCompositionRange(const int32 InBeginIndex, const uint32 InLength)
{
	if (m_bIsComposing)
	{
		m_CompositionBegin = InBeginIndex;
		m_Composing = static_cast<int32>(InLength);
		// 从 m_Text 中提取组合文字
		// m_CompositionString 已移除，组合文字已在 m_Text 中
	}
}

void SInputTextField::EndComposition()
{
	if (m_bIsComposing)
	{
		m_bIsComposing = false;

		// 组合确认：删除原始选区内容，组合文字已在选区之后
		const int32 SelStart = FMath::Min(m_CompositionSelectionStart, m_CompositionSelectionEnd);
		const int32 SelEnd = FMath::Max(m_CompositionSelectionStart, m_CompositionSelectionEnd);
		if (SelStart != SelEnd)
		{
			m_Text.RemoveAt(SelStart, SelEnd - SelStart);
			m_CaretPosition = FMath::Clamp(m_CaretPosition - (SelEnd - SelStart), 0, m_Text.Len());
		}

		m_SelectionStart = m_CaretPosition;
		m_Composing = 0;
		m_CompositionBegin = 0;
		m_CompositionSelectionStart = 0;
		m_CompositionSelectionEnd = 0;

		m_bRebuildText = true;
	}
}

void SInputTextField::OnSizeChanged()
{
	if (m_pBorder.IsValid() && m_pBorder->IsVisible())
	{
		auto Size = GetSize();
		if (Size != m_pBorder->GetSize())
		{
			m_pBorder->SetSize(Size);
		}
	}
}

void SInputTextField::UpdateBorder()
{
	if (m_Border > 0 || m_BackgroundColor.A != 0)
	{
		if (!m_pBorder.IsValid())
		{
			m_pBorder = MakeShared<SShape>(GObject.Get());
			AddChild(m_pBorder.ToSharedRef(), EDisplayObjChildLayer::Back);
			m_pBorder->SetType(EFGUIShapeType::Rect);
			m_pBorder->SetTouchable(false);
		}
		m_pBorder->SetType(m_Corner > 0 ? EFGUIShapeType::RoundRect : EFGUIShapeType::Rect);
		m_pBorder->SetLineColor(m_BorderColor);
		m_pBorder->SetLineWidth(m_Border);
		m_pBorder->SetColor(m_BackgroundColor);
		m_pBorder->SetTopLeftRadius(m_Corner);
		m_pBorder->SetTopRightRadius(m_Corner);
		m_pBorder->SetBottomLeftRadius(m_Corner);
		m_pBorder->SetBottomRightRadius(m_Corner);
		m_pBorder->SetPosition(FVector2D::Zero());
		m_pBorder->SetSize(GetSize());
		m_pBorder->SetVisible(true);
	}
	else
	{
		if (m_pBorder.IsValid())
			m_pBorder->SetVisible(false);
	}
}
void SInputTextField::ProcessInputChar(TCHAR Char)
{
	if (m_SelectionStart == m_CaretPosition)
	{
		if (m_CaretPosition < m_Text.Len())
			m_Text.InsertAt(m_CaretPosition, Char);
		else
			m_Text.AppendChar(Char);
		AdjustCaret(m_CaretPosition + 1, true);
		m_bRebuildText = true;
		m_bMakeCaretVisible = true;
		OnChanged();
	}
	else
	{
		ReplaceSelection(FString(1, &Char));
		ClearSelection();
	}
}