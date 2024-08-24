/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "CompilerWorkaround.h"
#include <fstream>
#include <stdio.h>
#include <ActnMan.hpp>
#include <Themes.hpp>
#include <Rtti.hpp>
#include <Vcl.ActnCtrls.hpp>
#include <Vcl.ThemedActnCtrls.hpp>
#include <Vcl.XPActnCtrls.hpp>
#include <memory>
#ifdef _WIN64
#include <boost\math\special_functions\fpclassify.hpp>
#endif
//---------------------------------------------------------------------------
/* Workaround for problem with support for Unicode filenames in std::fstream in CB2009.
 * It looks like the file fiopen.cpp is not compiled for Unicode support.
 * The following code is copied from Source\cpprtl\Source\Dinkumware\source\fiopen.cpp for wchar_t* instead of char*
 */
#undef _SYSCH
#define _SYSCH(x) L##x
#undef _Sysch_t
#define _Sysch_t wchar_t
#undef _CSTD
#define _CSTD
#define _Xfopen	_wfopen
namespace std
{
_CRTIMP2 FILE *_Fiopen(const _CSTD _Sysch_t *filename,
	ios_base::openmode mode, int)	// protection currently unused
	{	// open a file with native name
	static const _CSTD _Sysch_t *mods[] =
		{	// fopen mode strings corresponding to valid[i]
		_SYSCH("r"), _SYSCH("w"), _SYSCH("w"), _SYSCH("a"),
		_SYSCH("rb"), _SYSCH("wb"), _SYSCH("wb"), _SYSCH("ab"),
		_SYSCH("r+"), _SYSCH("w+"), _SYSCH("a+"),
		_SYSCH("r+b"), _SYSCH("w+b"), _SYSCH("a+b"),
		0};

	static const int valid[] =
		{	// valid combinations of open flags
		ios_base::in,
		ios_base::out,
		ios_base::out | ios_base::trunc,
		ios_base::out | ios_base::app,
		ios_base::in | ios_base::binary,
		ios_base::out | ios_base::binary,
		ios_base::out | ios_base::trunc | ios_base::binary,
		ios_base::out | ios_base::app | ios_base::binary,
		ios_base::in | ios_base::out,
		ios_base::in | ios_base::out | ios_base::trunc,
		ios_base::in | ios_base::out | ios_base::app,
		ios_base::in | ios_base::out | ios_base::binary,
		ios_base::in | ios_base::out | ios_base::trunc
			| ios_base::binary,
		ios_base::in | ios_base::out | ios_base::app
			| ios_base::binary,
		0};

	FILE *fp = 0;
	int n;
	ios_base::openmode atendflag = mode & ios_base::ate;
	ios_base::openmode norepflag = mode & ios_base::_Noreplace;

	if (mode & ios_base::_Nocreate)
		mode |= ios_base::in;	// file must exist
	if (mode & ios_base::app)
		mode |= ios_base::out;	// extension -- app implies out

	mode &= ~(ios_base::ate | ios_base::_Nocreate | ios_base::_Noreplace);
	for (n = 0; valid[n] != 0 && valid[n] != mode; ++n)
		;	// look for a valid mode

	if (valid[n] == 0)
		return (0);	// no valid mode
	else if (norepflag && mode & (ios_base::out || ios_base::app)
		&& (fp = _Xfopen(filename, _SYSCH("r"))) != 0)
		{	// file must not exist, close and fail
		fclose(fp);
		return (0);
		}
	else if (fp != 0 && fclose(fp) != 0)
		return (0);	// can't close after test open
// should open with protection here, if other than default
	else if ((fp = _Xfopen(filename, mods[n])) == 0)
		return (0);	// open failed

	if (!atendflag || fseek(fp, 0, SEEK_END) == 0)
		return (fp);	// no need to seek to end, or seek succeeded

	fclose(fp);	// can't position at end
	return (0);
	}
}
//---------------------------------------------------------------------------
//Workaround for bug in TCustomActionPopupMenuEx.LoadMenu() in ActnPopup.pas
//Hint is not assigned in the loop.
//This function is a copy of TCustomActionControl.SetSelected() in ActnMan.pas but
//compensates for the bug in TCustomActionPopupMenuEx.LoadMenu().
void __fastcall Actnman::TCustomActionControl::SetSelected(bool Value)
{
  if(Value != FSelected)
  {
    FSelected = Value;
    if(Value)
      UpdateSelection();
    Invalidate();
    if(Value && ActionClient->Action != NULL)
    {
      TCustomAction *Action = static_cast<TCustomAction*>(ActionClient->Action);
      if(Action->Hint == "" && ActionClient->Tag != 0)
        Action->Hint = reinterpret_cast<TMenuItem*>(ActionClient->Tag)->Hint;
      Application->Hint = GetLongHint(Action->Hint);
		}
		else
			Application->Hint = ""; //CancelHint();
		if(Value)
			NotifyWinEvent(EVENT_OBJECT_FOCUS, Parent->Handle, OBJID_CLIENT, ActionClient->Index + 1);
	}
}
//---------------------------------------------------------------------------
//Fix for minor bug in DrawCloseButton() inside TDockTree::PaintDockFrame().
//twCloseButtonNormal will draw a button where the cross is not scaled under Windows 7,
//which makes it look ugly. Instead we use twSmallCloseButtonNormal when the button is small.
//TDockTree::FGrabberSize can only be changed through RTTI.
//TDockTree::AdjustDockRect() is using the GrabberSize constant instead of the FGrabberSize member.
class TFixedDockTree : public TDockTree
{
	void DrawThemedCloseButton(TCanvas *Canvas, int Left, int Top, int Size);
  void DrawThemedGrabber(TCanvas *Canvas, TThemedRebar GripperType, int Left, int Top, int Right, int Bottom);
protected:
	void __fastcall PaintDockFrame(TCanvas *TCanvas, TControl *Control, const TRect &Rect);
  void __fastcall AdjustDockRect(TControl *Control, TRect &ARect);
public:
  __fastcall TFixedDockTree(TWinControl *DockSite);
};

__fastcall TFixedDockTree::TFixedDockTree(TWinControl *DockSite)
  : TDockTree(DockSite)
{
  TRttiContext Context;
  TRttiType *DockTreeType = Context.GetType(__classid(TDockTree));
  TRttiField *GrabberSize = DockTreeType->GetField("FGrabberSize");
  GrabberSize->SetValue(this, TValue::From((Screen->PixelsPerInch * 12) / 96));
}

void TFixedDockTree::DrawThemedCloseButton(TCanvas *Canvas, int Left, int Top, int Size)
{
	TRect DrawRect(Left, Top, Left+Size, Top+Size);
	TThemedWindow ButtonType = Size < 15 ? twSmallCloseButtonNormal : twCloseButtonNormal;
	TThemedElementDetails Details = ThemeServices()->GetElementDetails(ButtonType);
	ThemeServices()->DrawElement(Canvas->Handle, Details, DrawRect, NULL);
}

void TFixedDockTree::DrawThemedGrabber(TCanvas *Canvas, TThemedRebar GripperType, int Left, int Top, int Right, int Bottom)
{
  TRect DrawRect(Left, Top, Right, Bottom);
  TThemedElementDetails Details = ThemeServices()->GetElementDetails(GripperType);
  ThemeServices()->DrawElement(Canvas->Handle, Details, DrawRect);
}

void __fastcall TFixedDockTree::PaintDockFrame(TCanvas *Canvas, TControl *Control, const TRect &Rect)
{
	if(ThemeServices()->ThemesEnabled())
  {
    int GrabberSize = (Screen->PixelsPerInch * 12) / 96;
    int GrabberWidth = (Screen->PixelsPerInch * 10) / 96;
		if(DockSite->Align == alTop || DockSite->Align == alBottom)
    {
			DrawThemedCloseButton(Canvas, Rect.Left+1, Rect.Top+1, GrabberWidth);
      DrawThemedGrabber(Canvas, trGripperVert, Rect.Left+1, Rect.Top+GrabberSize+1, Rect.Left+GrabberWidth, Rect.Bottom-2);
    }
		else
    {
			DrawThemedCloseButton(Canvas, Rect.Right-GrabberWidth-1, Rect.Top+1, GrabberWidth);
      DrawThemedGrabber(Canvas, trGripper, Rect.Left+2, Rect.Top+1,  Rect.Right-GrabberSize-2, Rect.Top+GrabberWidth);
    }
  }
  else
  	TDockTree::PaintDockFrame(Canvas, Control, Rect);
}

void __fastcall TFixedDockTree::AdjustDockRect(TControl *Control, TRect &ARect)
{
  int GrabberSize = (Screen->PixelsPerInch * 12) / 96;
//Allocate room for the caption on the left if docksite is horizontally
//oriented, otherwise allocate room for the caption on the top. }
  if(DockSite->Align == alTop || DockSite->Align == alBottom)
    ARect.Left += GrabberSize;
  else
    ARect.Top += GrabberSize;
}
//---------------------------------------------------------------------------
class TInitWorkaround
{
public:
	TInitWorkaround()
	{
		DefaultDockTreeClass = __classid(TFixedDockTree);
	}
} InitWorkaround;
//---------------------------------------------------------------------------
extern "C" int _RTLENTRY  _EXPFUNC __iswctype(int c, int type);
static int IsType(wchar_t c, int Type)
{
	if(c >= 256)
	{
		WORD d;
		GetStringTypeW(CT_CTYPE1, (LPCWSTR)&c, 1, &d);
		return d & Type;
	}
	return __iswctype(c, Type);
}

int _RTLENTRY _EXPFUNC std::iswspace(wchar_t c)
{
	return IsType(c, _IS_SP);
}

int _RTLENTRY _EXPFUNC std::iswdigit(wchar_t c)
{
	return IsType(c, _IS_DIG);
}
//---------------------------------------------------------------------------
//Workaround for access violation when right Alt (Alt Gr) is pressed on a
//MacBook Pro with Windows 8.1 running under Parallels. See QC #120388
//Seems to be fixed in an update to Parallels
/*
namespace Menus
{
bool __fastcall IsAltGRPressed()
{
  return false;
}
}*/
//---------------------------------------------------------------------------
namespace Winapi { namespace Windows
{
  //Missing declaration in Windows.hpp
  const unsigned HH_DISPLAY_TOPIC      = 0;
  extern PACKAGE HWND __fastcall HtmlHelp(HWND hWndCaller, System::WideChar * pszFile, unsigned uCommand, NativeUInt dwData);
}}
void ShowHelp(const String &File, const String &HelpFile)
{
  //Workaround for bug in THtmlHelpViewer, which only support the .htm extension
  String Str = (HelpFile.IsEmpty() ? Application->HelpFile : HelpFile) + "::/" + File;
  Winapi::Windows::HtmlHelp(NULL, Str.c_str(), Winapi::Windows::HH_DISPLAY_TOPIC, 0);
}
//---------------------------------------------------------------------------
//Workaround for bug in C++ Builder XE6
//When an action is deleted, e.g. in a plugin, TBasicActionLink.FAction is set to NULL
//but TBasicActionLink::Update() is still called.
bool __fastcall System::Classes::TBasicActionLink::Update()
{
  return FAction != NULL && !FAction->Suspended() && FAction->Update();
}
//---------------------------------------------------------------------------
//Vcl selects image list based on the SmallIcons property.
//This replacement selectes LargeImages if for toolbars and Images otherwise
//This is done because setting SmallIcons to false causes problems as it is simetimes assumed that large icons are 32x32 pixels.
Vcl::Imglist::TCustomImageList* __fastcall Vcl::Actnman::TCustomActionControl::FindImageList(bool UseLargeImages, bool &Disabled, System::Uitypes::TImageIndex ImageIndex)
{
  Disabled = false;
  if(ActionClient == NULL)
    return NULL;
  if(ActionClient->OwningCollection->ActionManager->LargeImages && dynamic_cast<Vcl::Actnctrls::TCustomActionToolBar*>(ActionBar))
    return ActionClient->OwningCollection->ActionManager->LargeImages;
  return ActionClient->OwningCollection->ActionManager->Images;
}
//---------------------------------------------------------------------------
//Replacement for the VCL function TCustomActionControl::GetImageSize()
//This is needed because of the replacement of TCustomActionControl::FindImageList() above.
//GetImageSize() must use the same image list as FindImageList().
//This is done because setting SmallIcons to false causes the VCL to think that the icons are at least 32x32 pixels.
TPoint __fastcall Vcl::Actnman::TCustomActionControl::GetImageSize()
{
  if(!HasGlyph() || !ActionClient->ShowGlyph)
    return TPoint(0 ,0);
  TCustomImageList *ImageList;
  if(ActionClient->OwningCollection->ActionManager->LargeImages && dynamic_cast<Vcl::Actnctrls::TCustomActionToolBar*>(ActionBar))
    ImageList = ActionClient->OwningCollection->ActionManager->LargeImages;
  else
    ImageList = ActionClient->OwningCollection->ActionManager->Images;
  if(ImageList == NULL)
    return TPoint(0, 0);
  return TPoint(ImageList->Width, ImageList->Height);
}
//---------------------------------------------------------------------------
//Workaround for bug where separators sometimes changes size to the same as a normal button.
void __fastcall Vcl::Actnctrls::TCustomButtonControl::CalcBounds()
{
  if(!Separator)
  {
    TCustomActionControl::CalcBounds();
    Width = Width + 1;
  }
}
//---------------------------------------------------------------------------
//Workaround for separators in the toolbar being highlighed (hot) when the mouse is over the separator.
//It seems intentional as the check for !ActionClient->Separator is commented out in the original VCL code.
//It might be possible to fix it by creating a new style instead. The problem only exist when StyleServices()->IsSystemStyle is true.
void __fastcall Vcl::Themedactnctrls::TThemedButtonControl::DrawBackground(TRect &PaintRect)
{
  const TThemedToolBar DisabledState[2] = {ttbButtonDisabled, ttbButtonPressed};
  const TThemedToolBar CheckedState[2] = {ttbButtonHot, ttbButtonCheckedHot};

  if(!StyleServices()->IsSystemStyle && ActionClient->Separator)
    return;

  int SaveIndex = SaveDC(Canvas->Handle);
  try
  {
    if(Enabled && !ActionBar->DesignMode)
    {
      if((MouseInControl || IsChecked()) && ActionClient && !ActionClient->Separator) //IJO: Added check for Separator
      {
        StyleServices()->DrawElement(Canvas->Handle, StyleServices()->GetElementDetails(CheckedState[IsChecked() || (FState = bsDown)]), PaintRect);

        if(!MouseInControl)
          StyleServices()->DrawElement(Canvas->Handle, StyleServices()->GetElementDetails(ttbButtonPressed), PaintRect);
      }
      else
        StyleServices()->DrawElement(Canvas->Handle, StyleServices()->GetElementDetails(ttbButtonNormal), PaintRect);
    }
    else
      StyleServices()->DrawElement(Canvas->Handle, StyleServices()->GetElementDetails(DisabledState[IsChecked()]), PaintRect);
  }
  __finally
  {
    RestoreDC(Canvas->Handle, SaveIndex);
  }
}
//---------------------------------------------------------------------------
void __fastcall Vcl::Xpactnctrls::TXPStyleMenuItem::CalcBounds()
{
  TCustomMenuItem::CalcBounds();
  if(Separator)
    Height = 3;
  else
    Height = Height + 2;

  bool Dummy = false;
  TCustomImageList *ImageList = FindImageList(false, Dummy, 0);
  if(ImageList != NULL && ! Separator)
  {
    int DeltaWidth = ImageList->Width - 16;
    int NewHeight = std::max(Height, ImageList->Height + 9);
    TextBounds.Offset(DeltaWidth, (NewHeight - Height) / 2);
    ShortCutBounds.Offset(DeltaWidth, (NewHeight - Height) / 2);
    Width = Width + DeltaWidth;
    Height = NewHeight;
  }
}
//---------------------------------------------------------------------------
void __fastcall Vcl::Xpactnctrls::TXPStyleMenuItem::DrawBackground(TRect &PaintRect)
{
  if (ActionClient == NULL)
    return;

  if(ActionClient->Color != clDefault)
    Canvas->Brush->Color = ActionClient->Color;

  TRect BGRect = PaintRect;
  if(TextBounds.Left > GlyphPos.x)
    BGRect.Left = 0;
  TRect BannerRect = PaintRect;
  bool Dummy = false;
  TCustomImageList *ImageList = FindImageList(false, Dummy, 0);
  BannerRect.Right = ImageList != NULL ? ImageList->Width + 9 : 9;     //IJO: Changed from 25
  BGRect.Left = BannerRect.Right - BannerRect.Left;
  if(ActionClient->Unused())
    Canvas->Brush->Color = ActionBar->ColorMap->UnusedColor;
  else
    Canvas->Brush->Color = ActionBar->ColorMap->Color;
  Canvas->FillRect(BannerRect);
  Canvas->Brush->Color = Menu->ColorMap->MenuColor;

  if((Selected && Enabled) || (Selected && !MouseSelected))
  {
    if(Enabled && !ActionBar->DesignMode)
      if(!Separator || (Separator && ActionBar->DesignMode))
        Canvas->Brush->Color = Menu->ColorMap->SelectedColor;
    PaintRect.Right--;
  }
  inherited::DrawBackground(BGRect);
  if(!ActionBar->DesignMode && Separator)
    return;
  if(!Mouse->IsDragging && ((Selected && Enabled) || (Selected && !MouseSelected)))
  {
    Canvas->FillRect(PaintRect);
    Canvas->Brush->Color = ActionBar->ColorMap->BtnFrameColor;
    PaintRect.Right++;
    Canvas->FrameRect(PaintRect);
  }
}
//---------------------------------------------------------------------------
//Workaround for problem creating drag images. The original uses TreeView_CreateDragImage(), which seems to use masks to create a new icon.
//This doesn't work for icons with alpha blending. Instead we create a new image by drawing
//the normal icon and adding the node text.
void __fastcall Vcl::Comctrls::TCustomTreeView::DoStartDrag(TDragObject *&DragObject)
{
  TWinControl::DoStartDrag(DragObject);
  TTreeNode *DragNode = FDragNode;
  FLastDropTarget = NULL;
  FDragNode = NULL;
  if(DragNode == NULL)
  {
    POINT P;
    GetCursorPos(&P);
    TPoint P1 = ScreenToClient(TPoint(P.x, P.y));
    DragNode = GetNodeAt(P1.X, P1.Y);
  }
  if(DragNode != NULL)
  {
    HIMAGELIST ImageHandle = TreeView_CreateDragImage(Handle, DragNode->ItemId);
    if(ImageHandle != 0)
    {
      std::auto_ptr<TBitmap> Bitmap(new TBitmap);
      Bitmap->Canvas->Font->Assign(Font);
      Bitmap->Canvas->Font->Quality = TFontQuality::fqNonAntialiased;
      TSize Size = Bitmap->Canvas->TextExtent(DragNode->Text);
      Bitmap->Width = Size.cx + Images->Width + 5;
      Bitmap->Height = Images->Height;
      Bitmap->Canvas->Brush->Color = clWhite;
      Bitmap->Canvas->FillRect(TRect(0, 0, Bitmap->Width, Bitmap->Height));
      Images->Draw(Bitmap->Canvas, 0, 0, DragNode->ImageIndex);
      Bitmap->Canvas->TextOut(Images->Width + 5, (Images->Height - Size.cy) / 2, DragNode->Text);
      FDragImage->Clear();
      FDragImage->SetSize(Bitmap->Width, Bitmap->Height);
      FDragImage->AddMasked(Bitmap.get(), clWhite);
      FDragImage->SetDragImage(0, 2, 2);
    }
  }
}
//---------------------------------------------------------------------------
#ifdef _WIN64
int _RTLENTRY _EXPFUNC std::_finite(double __d)
{
  return boost::math::isfinite(__d);
}

int _RTLENTRY _EXPFUNC std::_finitel(long double __ld)
{
  return boost::math::isfinite(__ld);
}

int _RTLENTRY _EXPFUNC std::_isnanl(long double __ld)
{
  return boost::math::isnan(__ld);
}

double _RTLENTRY _EXPFUNC std::pow10(int __p)
{
  return std::pow10l(__p);
}
#endif
//---------------------------------------------------------------------------
namespace boost
{
void tss_cleanup_implemented()
{
}
}

extern "C" double exp2(double x)
{
  assert(0);
  return 0;
}

