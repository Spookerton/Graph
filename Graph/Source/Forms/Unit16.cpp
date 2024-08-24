/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#include "Graph.h"
#pragma hdrstop
#include "Unit16.h"
#include "PyGraph.h"
#include <float.h>
//---------------------------------------------------------------------------
#pragma link "ShadeSelect"
#pragma link "MyRadioButton"
#pragma link "ExtColorBox"
#pragma link "MyEdit"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TForm16::TForm16(TComponent* Owner, TData &AData)
  : TForm(Owner), Data(AData)
{
  TranslateProperties(this);
  TranslateStrings(ExtColorBox1->Items);
  SetAccelerators(this);
  ResizeControl(Edit5, Label12);
  ScaleForm(this);
  FlipForm(this);

  ShadeSelect1->ShadeStyle = static_cast<TBrushStyle>(Options.DefaultShade.Style);
  ExtColorBox1->Selected = Options.DefaultShade.Color;
  EnableGroupBox2(false);

  int I = 1;
  String CmpStr;
  bool Found = true;
  while(Found)
  {
    CmpStr = LoadRes(RES_SHADE) + L" " + I++;
    Found = false;
    for(unsigned N = 0; N < Data.ElemCount(); N++)
      for(unsigned J = 0; J < Data.GetElem(N)->ChildCount(); J++)
        if(boost::shared_ptr<TShading> Shade = boost::dynamic_pointer_cast<TShading>(Data.GetElem(N)->GetChild(J)))
          if(CmpStr == ToUString(Shade->GetLegendText()))
            Found = true;
  }
  Edit5->Text = CmpStr;

  int ImageWidth = MulDiv(64, Options.FontScale * Screen->PixelsPerInch, 9600);
  if(ImageWidth != 120)
  {
    SmoothResize(dynamic_cast<TPngImage*>(Image1->Picture->Graphic), ImageWidth, ImageWidth);
    SmoothResize(dynamic_cast<TPngImage*>(Image2->Picture->Graphic), ImageWidth, ImageWidth);
    SmoothResize(dynamic_cast<TPngImage*>(Image3->Picture->Graphic), ImageWidth, ImageWidth);
    SmoothResize(dynamic_cast<TPngImage*>(Image4->Picture->Graphic), ImageWidth, ImageWidth);
    SmoothResize(dynamic_cast<TPngImage*>(Image5->Picture->Graphic), ImageWidth, ImageWidth);
    SmoothResize(dynamic_cast<TPngImage*>(Image6->Picture->Graphic), ImageWidth, ImageWidth);
  }
}
//---------------------------------------------------------------------------
void TForm16::ShowFuncList()
{
  for(unsigned I = 0; I < Data.ElemCount(); I++)
    if(boost::shared_ptr<TBaseFuncType> F = boost::dynamic_pointer_cast<TBaseFuncType>(Data.GetElem(I)))
      if(F != Func)
      {
        FuncList.push_back(F);
        ListBox1->Items->Add(ToUString(F->MakeLegendText()));
      }

  if(ListBox1->Items->Count)
  {
    if(!!OldShade && OldShade->GetShadeStyle() == ssBetween)
      ListBox1->ItemIndex = IndexOf(FuncList, OldShade->GetFunc2());
    else
      ListBox1->ItemIndex = -1;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm16::ImageClick(TObject *Sender)
{
  if(Sender == Image1)
    RadioButton1->SetFocus();
  else if(Sender == Image2)
    RadioButton2->SetFocus();
  else if(Sender == Image3)
    RadioButton3->SetFocus();
  else if(Sender == Image4)
    RadioButton4->SetFocus();
  else if(Sender == Image5)
  {
    if(RadioButton5->CanFocus())
      RadioButton5->SetFocus();
  }
  else if(Sender == Image6)
    RadioButton6->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TForm16::Button1Click(TObject *Sender)
{
  boost::shared_ptr<TShading> Shade(new TShading);
  TShadeStyle ShadeStyle;
  if(RadioButton1->Checked)
    ShadeStyle = ssXAxis;
  else if(RadioButton2->Checked)
    ShadeStyle = ssBelow;
  else if(RadioButton3->Checked)
    ShadeStyle = ssAbove;
  else if(RadioButton4->Checked)
    ShadeStyle = ssYAxis;
  else if(RadioButton5->Checked)
    ShadeStyle = ssInside;
  else if(RadioButton6->Checked)
    ShadeStyle = ssBetween;
  else
  {
    MessageBox(L"No shading style selected!", L"Selection error");
    return;
  }

  Shade->SetShadeStyle(ShadeStyle);
  Shade->SetBrushStyle(ShadeSelect1->ShadeStyle);
  Shade->SetColor(ExtColorBox1->Selected);

  Shade->SetLegendText(ToWString(Edit5->Text));
  Shade->SetMin(TTextValue(Edit1->Text.IsEmpty() ? -INF : MakeFloat(Edit1), ToWString(Edit1->Text)));
  Shade->SetMax(TTextValue(Edit2->Text.IsEmpty() ? +INF : MakeFloat(Edit2), ToWString(Edit2->Text)));

  Shade->SetExtendMinToIntercept(CheckBox1->Checked);
  Shade->SetExtendMaxToIntercept(CheckBox2->Checked);

  //Initialize values for 2. function to values of 1. function as default
  Shade->SetMin2(Shade->GetMin());
  Shade->SetMax2(Shade->GetMax());
  Shade->SetExtendMin2ToIntercept(Shade->GetExtendMinToIntercept());
  Shade->SetExtendMax2ToIntercept(Shade->GetExtendMaxToIntercept());
  Shade->SetMarkBorder(CheckBox5->Checked);

  if(ShadeStyle == ssBetween)
  {
    if(ListBox1->ItemIndex == -1)
    {
      PageControl1->ActivePageIndex = 2;
      ListBox1->SetFocus();
      MessageBox(LoadRes(539), LoadRes(RES_SELECTION_ERROR));
      return;
    }
    Shade->SetFunc2(FuncList[ListBox1->ItemIndex]);
    if(!Edit3->Text.IsEmpty() || !Edit4->Text.IsEmpty())
    {
      Shade->SetMin2(TTextValue(Edit3->Text.IsEmpty() ? -INF : MakeFloat(Edit3), ToWString(Edit3->Text)));
      Shade->SetMax2(TTextValue(Edit4->Text.IsEmpty() ? +INF : MakeFloat(Edit4), ToWString(Edit4->Text)));

      Shade->SetExtendMin2ToIntercept(CheckBox3->Checked);
      Shade->SetExtendMax2ToIntercept(CheckBox4->Checked);
    }
  }
  else
    Shade->SetFunc2(TBaseFuncPtr());

  if(OldShade)
  {
    Shade->SetVisible(OldShade->GetVisible());
    Shade->SetShowInLegend(OldShade->GetShowInLegend());
    Shade->Swap(*OldShade);
    UndoList.Push(TUndoChange(OldShade, Shade));
    Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(OldShade));
  }
  else
  {
    UndoList.Push(TUndoAdd(Shade));
    Func->InsertChild(Shade);
  }

  Options.DefaultShade.Set(ShadeSelect1->ShadeStyle, ExtColorBox1->Selected, 0);
  ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TForm16::ExtColorBox1Change(TObject *Sender)
{
  ShadeSelect1->Color = ExtColorBox1->Selected;
}
//---------------------------------------------------------------------------
void __fastcall TForm16::RadioButton6CheckedChange(TObject *Sender)
{
  ListBox1->Enabled = RadioButton6->Checked;
  EnableGroupBox2(RadioButton6->Checked);
}
//---------------------------------------------------------------------------
TModalResult TForm16::InsertShade(const boost::shared_ptr<TBaseFuncType> &AFunc)
{
  Func = AFunc;
  return ShowModal();
}
//---------------------------------------------------------------------------
int TForm16::EditShade(const boost::shared_ptr<TShading> &AShade)
{
  OldShade = AShade;
  if(OldShade)
  {
    Caption = LoadRes(538);
    Edit1->Text = OldShade->GetMin().Text.c_str();
    Edit2->Text = OldShade->GetMax().Text.c_str();
    Func = boost::dynamic_pointer_cast<TBaseFuncType>(OldShade->GetParent());
    ShadeSelect1->ShadeStyle = OldShade->GetBrushStyle();
    ExtColorBox1->Selected = OldShade->GetColor();
    CheckBox1->Checked = OldShade->GetExtendMinToIntercept();
    CheckBox2->Checked = OldShade->GetExtendMaxToIntercept();
    CheckBox5->Checked = OldShade->GetMarkBorder();
    Edit5->Text = ToUString(OldShade->GetLegendText());

    GroupBox2->Enabled = false;
    switch(OldShade->GetShadeStyle())
    {
      case ssXAxis:   RadioButton1->Checked = true; break;
      case ssBelow:   RadioButton2->Checked = true; break;
      case ssAbove:   RadioButton3->Checked = true; break;
      case ssYAxis:   RadioButton4->Checked = true; break;
      case ssInside:  RadioButton5->Checked = true; break;
      case ssBetween:
        RadioButton6->Checked = true;
        ListBox1->Enabled = true;
        EnableGroupBox2(true);
        Edit3->Text = OldShade->GetMin2().Text.c_str();
        Edit4->Text = OldShade->GetMax2().Text.c_str();
        CheckBox3->Checked = OldShade->GetExtendMin2ToIntercept();
        CheckBox4->Checked = OldShade->GetExtendMax2ToIntercept();
    }
  }
  return ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TForm16::FormShow(TObject *Sender)
{
  PageControl1->ActivePageIndex = 0;
  ShowFuncList();
}
//---------------------------------------------------------------------------
void __fastcall TForm16::Button3Click(TObject *Sender)
{
  Application->HelpContext(HelpContext);
}
//---------------------------------------------------------------------------
void TForm16::EnableGroupBox2(bool Enable)
{
  Label8->Enabled = Enable;
  Label9->Enabled = Enable;
  CheckBox3->Enabled = Enable;
  CheckBox4->Enabled = Enable;
  Edit3->Enabled = Enable;
  Edit4->Enabled = Enable;
  GroupBox2->Enabled = Enable;
}
//---------------------------------------------------------------------------

