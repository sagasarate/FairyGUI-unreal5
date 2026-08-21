#pragma once

#include "SDisplayObject.h"
#include "Font/NTextFormat.h"
#include "Widgets/SRichTextField.h"
#include "Utils/TypeID.h"
#include "Misc/StringBuilder.h"
#include "GenericPlatform/ITextInputMethodSystem.h"

class SShape;
class SSelectionShape;

class FAIRYGUI_API SInputTextField : public SRichTextField, public ITextInputMethodContext
{
protected:
	struct FCharPosition
	{
		int32 CharIndex;
		int16 LineIndex;
		int16 Width;
		float OffsetX;
		float OffsetY;
		FCharPosition()
		{
			CharIndex = 0;
			LineIndex = -1;
			Width = 0;
			OffsetX = 0;
			OffsetY = 0;
		}
	};

	FString		  m_Restrict;
	FRegexPattern m_RestrictPattern;
	bool		  m_bDisplayAsPassword = false;
	FString		  m_PromptText;
	int32		  m_Border = 0;
	int32		  m_Corner = 0;
	FColor		  m_BorderColor = FColor::Black;
	FColor		  m_BackgroundColor = FColor::Transparent;
	bool		  m_bEditable = true;
	bool		  m_bEditing = false;
	int32		  m_CaretPosition = 0;
	int32		  m_SelectionStart = 0;
	bool		  m_bMakeCaretVisible = false;

	// IME 组合状态
	bool  m_bIsComposing = false; // IME 是否正在组合
	int32 m_Composing = 0;		  // 组合文字的长度
	int32 m_CompositionBegin = 0; // 组合文字在 m_Text 中的起始位置

	// 组合开始时的选区范围，用于组合期间继续显示选区
	int32 m_CompositionSelectionStart = 0;
	int32 m_CompositionSelectionEnd = 0;

	bool m_bCaretDraging = false;

	int32 m_MaxLength = 0;
	bool  m_bKeyboardInput = false;
	bool  m_bHideInput = 0;
	bool  m_bDisableIME = false;
	bool  m_bMouseWheelEnabled = true;
	bool  m_bWantReturn = false;
	bool  m_bWantTab = false;
	bool  m_bAutoSubmitOnLostFocus = false;
	bool  m_bTextChanged = false; // 获得焦点后是否有过输入/删改

	// Undo/Redo 历史
	static constexpr int32 MAX_HISTORY_LENGTH = 5;
	TArray<FString>		   m_UndoBuffer;
	TArray<FString>		   m_RedoBuffer;
	FString				   m_CurrentUndoText;
	bool				   m_bUndoLock = false;

	int32 m_HaveNavigationTab = 0;

	TSharedPtr<SShape>			m_pCaret;
	TSharedPtr<SSelectionShape> m_pSelectionShape;
	double						m_NextBlink = 0;

	TSharedPtr<SShape> m_pBorder;

	TArray<FCharPosition> m_CharPositions;

	// IME 系统相关
	TSharedPtr<ITextInputMethodChangeNotifier> m_IMEChangeNotifier;
	bool									   m_bHasRegisteredIME = false;
	FGeometry								   m_CachedGeometry;

	static float CARET_BLINK_INTERVAL;

public:
	DECLARE_TYPE_ID(SInputTextField, SRichTextField)
	explicit SInputTextField(UGObject* InGObject = nullptr);

	int32 GetMaxLength() const { return m_MaxLength; }
	void  SetMaxLength(int32 Len) { m_MaxLength = Len; }

	bool CanKeyboardInput() const { return m_bKeyboardInput; }
	void EnableKeyboardInput(bool bEnable) { m_bKeyboardInput = bEnable; }

	bool IsHideInput() const { return m_bHideInput; }
	void HideInput(bool bHide) { m_bHideInput = bHide; }

	bool IsEditable() const { return m_bEditable; }
	void SetEditable(bool bEditable) { m_bEditable = bEditable; }

	bool IsIMEEnabled() const { return !m_bDisableIME; }
	void EnableIME(bool bEnable) { m_bDisableIME = !bEnable; }

	bool IsMouseWheelEnabled() const { return m_bMouseWheelEnabled; }
	void EnableMouseWheel(bool bEnable) { m_bMouseWheelEnabled = bEnable; }

	bool IsPassword() const { return m_bDisplayAsPassword; }
	void SetPassword(bool bEnable);

	bool IsWantReturn() const { return m_bWantReturn; }
	void SetWantReturn(bool bWant) { m_bWantReturn = bWant; }

	bool IsWantTab() const { return m_bWantTab; }
	void SetWantTab(bool bWant) { m_bWantTab = bWant; }

	bool IsAutoSubmitOnLostFocus() const { return m_bAutoSubmitOnLostFocus; }
	void SetAutoSubmitOnLostFocus(bool bEnable) { m_bAutoSubmitOnLostFocus = bEnable; }

	const FString& GetRestrict() const { return m_Restrict; }
	void		   SetRestrict(const FString& Restrict);

	const FString& GetPromptText() const { return m_PromptText; }
	void		   SetPromptText(const FString& InText);

	int32 GetBorder() const { return m_Border; }
	void  SetBorder(int32 Border);

	int32 GetCorner() const { return m_Corner; }
	void  SetCorner(int32 Corner);

	const FColor& GetBorderColor() const { return m_BorderColor; }
	void		  SetBorderColor(const FColor& Color);

	const FColor& GetBackgroundColor() const { return m_BackgroundColor; }
	void		  SetBackgroundColor(const FColor& Color);

	void	ClearSelection();
	void	ReplaceSelection(const FString& Value);
	FString GetSelection();
	void	SetSelection(int32 Start, int32 Length);

	// IME 管理
	void RegisterIME();
	void UnregisterIME();

protected:
	virtual void CollectRenderUnits(
		FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha, float InSaturation) const override;

	virtual bool OnFocusReceived(EFocusCause InCause);
	virtual void OnFocusLost(EFocusCause InCause);
	virtual bool OnKeyChar(const FCharacterEvent& InCharacterEvent);
	virtual bool OnKeyDown(const FKeyEvent& InKeyEvent);
	virtual bool OnMouseButtonDown(const FPointerEvent& MouseEvent) override;
	virtual bool OnMouseButtonUp(const FPointerEvent& MouseEvent) override;
	virtual bool OnMouseMove(const FPointerEvent& MouseEvent) override;
	virtual bool OnMouseButtonDoubleClick(const FPointerEvent& MouseEvent) override;
	virtual bool OnMouseWheel(const FPointerEvent& MouseEvent) override;

	virtual void BuildLines() override;

	void				 BuildCharPositions();
	const FCharPosition& GetCharPosition(int32 CaretIndex);
	const FCharPosition& GetCharPosition(float X, float Y);

	void CreateCaret();
	void AdjustCaret(int32 CaretIndex, bool MoveSelectionHeader);
	void UpdateCaret(bool bMakeVisible);
	void ScrollToVisible(float X, float Y, float Width, float Height);
	void ApplyContentOffset(float NewOX, float NewOY);
	void Scroll(int32 HScroll, int32 VScroll);
	void UpdateSelection(int32 cp);
	void CaretBlink();

	void DoCopy();
	void DoPaste();

	// Undo/Redo
	void RecordUndo();
	void PerformUndo();
	void PerformRedo();

	int32 TruncateText(const TCHAR* pStr, int32 Len) // 防止表情符被截断
	{
		if (IsHighSurrogate(pStr[Len - 1]) && IsLowSurrogate(pStr[Len]))
			return Len + 1;
		return Len;
	}
	FString ValidateInput(const FString& source);
	void	OnChanged();

	// -- ITextInputMethodContext 接口实现 --
	virtual bool   IsComposing() override;
	virtual bool   IsReadOnly() override;
	virtual uint32 GetTextLength() override;
	virtual void GetSelectionRange(uint32& OutBeginIndex, uint32& OutLength, ECaretPosition& OutCaretPosition) override;
	virtual void SetSelectionRange(
		const uint32 InBeginIndex, const uint32 InLength, const ECaretPosition InCaretPosition) override;
	virtual void  GetTextInRange(const uint32 InBeginIndex, const uint32 InLength, FString& OutString) override;
	virtual void  SetTextInRange(const uint32 InBeginIndex, const uint32 InLength, const FString& InString) override;
	virtual int32 GetCharacterIndexFromPoint(const FVector2D& InPoint) override;
	virtual bool  GetTextBounds(
		 const uint32 InBeginIndex, const uint32 InLength, FVector2D& OutPosition, FVector2D& OutSize) override;
	virtual void					   GetScreenBounds(FVector2D& OutPosition, FVector2D& OutSize) override;
	virtual TSharedPtr<FGenericWindow> GetWindow() override;
	virtual void					   BeginComposition() override;
	virtual void					   UpdateCompositionRange(const int32 InBeginIndex, const uint32 InLength) override;
	virtual void					   EndComposition() override;

	virtual void OnSizeChanged() override;

	void UpdateBorder();
	void ProcessInputChar(TCHAR Char);
};

inline void SInputTextField::SetPassword(bool bEnable)
{
	if (m_bDisplayAsPassword != bEnable)
	{
		m_bDisplayAsPassword = bEnable;
		m_bRebuildText = true;
	}
}

inline void SInputTextField::SetPromptText(const FString& InText)
{
	m_PromptText = InText;
	m_bRebuildText = true;
}

inline void SInputTextField::SetBorder(int32 Border)
{
	if (m_Border != Border)
	{
		m_Border = Border;
		UpdateBorder();
	}
}

inline void SInputTextField::SetCorner(int32 Corner)
{
	if (m_Corner != Corner)
	{
		m_Corner = Corner;
		UpdateBorder();
	}
}

inline void SInputTextField::SetBorderColor(const FColor& Color)
{
	if (m_BorderColor != Color)
	{
		m_BorderColor = Color;
		UpdateBorder();
	}
}

inline void SInputTextField::SetBackgroundColor(const FColor& Color)
{
	if (m_BackgroundColor != Color)
	{
		m_BackgroundColor = Color;
		UpdateBorder();
	}
}