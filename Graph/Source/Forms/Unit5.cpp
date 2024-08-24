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
#include "Unit5.h"
#include "Unit1.h"
#include <float.h>
#include "PyGraph.h"
//---------------------------------------------------------------------------
#pragma link "LineSelect"
#pragma link "MyEdit"
#pragma link "ExtColorBox"
#pragma link "GridPanelEx"
#pragma link "ExtComboBox"
#pragma link "FocusPanel"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TForm5::TForm5(TComponent* Owner, TData &AData)
	: TForm(Owner), OldItemIndex(0), Data(AData)
{
  Translate();
  ScaleForm(this);

  TempData[1].From = "-10";
  TempData[1].To = "10";
  TempData[1].Steps = "1000";

  TempData[2].From = "0";
  TempData[2].To = Data.Axes.Trigonometry == Func32::Degree ? "360" : "2pi";
  TempData[2].Steps = "1000";

  LineSelect1->LineStyle = static_cast<TPenStyle>(Options.DefaultFunction.Style);
  ExtColorBox1->Selected = Options.DefaultFunction.Color;
  UpDown1->Position = Options.DefaultFunction.Size;
  ComboBox2->ItemIndex = 0;
  ComboBox3->ItemIndex = 0;

  //Now this is silly: We don't want to flip the group box, so we flip it twice
  if(SysLocale.MiddleEast)
    GroupBox1->FlipChildren(false);
  FlipForm(this);
	ComboBox1Change(NULL);
}
//---------------------------------------------------------------------------
void TForm5::Translate()
{
  TranslateProperties(ExtColorBox1);
  TranslateProperties(this);
  TranslateStrings(ExtColorBox1->Items);
  SetAccelerators(this);

  ResizeControl(ComboBox1, Label1);
  ResizeControl(Edit3, Label4);
  ResizeControl(Edit4, Label5);
  ResizeControl(Edit5, Label6);
  ResizeControl(Edit7, Label10);

  int Left = Label7->Left + Label7->Width + 5;
  LineSelect1->Left = Left;
  ExtColorBox1->Left = Left;
          
  Left = ComboBox4->Left - TMaxWidth(Label13)(Label8) - 5;
  Label13->Left = Left;
  Label8->Left = Left;

  if(LineSelect1->Left + LineSelect1->Width > Label13->Left)
    Width = Width + LineSelect1->Left + LineSelect1->Width - Label13->Left + 10;
 
  MoveControl(ComboBox2, Label11);
  MoveLabel(ComboBox3, Label12);

  ComboBox1->ItemIndex = 0;
  ComboBox2->ItemIndex = 0;
  ComboBox3->ItemIndex = 0;
  ComboBox4->ItemIndex = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm5::Button1Click(TObject *Sender)
{
  boost::shared_ptr<TBaseFuncType> Func;
  TTextValue Steps;
  Steps.Text = ToWString(Edit5->Text);
  Steps.Value = 0;
  try
  {
    switch(ComboBox1->ItemIndex)
    {
      case 0:
        Func.reset(new TStdFunc(ToWString(Edit1->Text), Data.CustomFunctions.SymbolList, Data.Axes.Trigonometry));
        Func->SetSteps(TTextValue(0));
        if(!Steps.Text.empty())
          Steps.Value = MakeInt(Edit5, Label6->Caption);
        break;

      case 1:
        Func.reset(new TParFunc(ToWString(Edit1->Text), ToWString(Edit2->Text), Data.CustomFunctions.SymbolList, Data.Axes.Trigonometry));
        Steps.Value = MakeInt(Edit5, Label6->Caption);
        break;

      case 2:
        Func.reset(new TPolFunc(ToWString(Edit1->Text), Data.CustomFunctions.SymbolList, Data.Axes.Trigonometry));
        Steps.Value = MakeInt(Edit5, Label6->Caption);
        break;
    }
  }
  catch(Func32::EParseError &ParseError)
  {
    ShowErrorMsg(ParseError, ParseError.ErrorFunc == Func32::efParamY ? Edit2 : Edit1);
    return;
  }

  Func->SetFrom(TTextValue(!Edit3->Text.IsEmpty() || ComboBox1->ItemIndex ? MakeFloat(Edit3) : -INF, ToWString(Edit3->Text)));
  Func->SetTo(TTextValue(!Edit4->Text.IsEmpty() || ComboBox1->ItemIndex ? MakeFloat(Edit4) : INF, ToWString(Edit4->Text)));
  Func->SetSteps(Steps);

  if(Func->GetFrom().Value >= Func->GetTo().Value)
    Form1->ShowStatusError(LoadRes(511));

  int WidthValue = ToIntDef(Edit6->Text, -1);
  if(WidthValue < 1)
  {
    MessageBox(LoadRes(513), LoadRes(514));
    return;
  }

  Func->SetColor(ExtColorBox1->Selected);
  Func->SetSize(WidthValue);
  Func->SetStyle(LineSelect1->LineStyle);
  Func->SetLegendText(ToWString(Edit7->Text));
  Func->SetEndpointStyle(std::make_pair(ComboBox2->ItemIndex, ComboBox3->ItemIndex));
  Func->SetDrawType(static_cast<TDrawType>(ComboBox4->ItemIndex));

  Data.AbortUpdate();
  if(F)
  {
    Func->SetVisible(F->GetVisible());
    Func->SetShowInLegend(F->GetShowInLegend());
    if(typeid(*F) == typeid(*Func))
    {
      F->Swap(*Func);
      UndoList.Push(TUndoChange(F, Func));
      Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(F));
      F->Update(); //Make sure tangents are updated
    }
    else
    {
      Data.Replace(F, Func);
      UndoList.Push(TUndoReplace(F, Func));
      Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(Func));
      Func->Update(); //Make sure tangents are updated
    }
  }
  else
  {
    UndoList.Push(TUndoAdd(Func));
    Data.Insert(Func);
    Func->Update(); //Make sure tangents are updated
  }

  Options.DefaultFunction.Set(LineSelect1->LineStyle, ExtColorBox1->Selected, WidthValue);
  ModalResult = mrOk;
}
//---------------------------------------------------------------------------
int TForm5::EditFunc(boost::shared_ptr<TBaseFuncType> Func)
{
  F = Func;
  if(F)
  {
    if(TStdFunc *StdFunc = dynamic_cast<TStdFunc*>(F.get()))
    {
      ComboBox1->ItemIndex = 0;
      Edit1->Text = StdFunc->GetEquation().c_str();
    }
    else if(TParFunc *ParFunc = dynamic_cast<TParFunc*>(F.get()))
    {
      ComboBox1->ItemIndex = 1;
      Edit1->Text = ToUString(ParFunc->GetEquation().first);
      Edit2->Text = ToUString(ParFunc->GetEquation().second);
    }
    else if(TPolFunc *PolFunc = dynamic_cast<TPolFunc*>(F.get()))
    {
      ComboBox1->ItemIndex = 2;
      Edit1->Text = PolFunc->GetEquation().c_str();
    }

    Caption = LoadRes(524);
    Edit3->Text = F->GetFrom().Text.c_str();
    Edit4->Text = F->GetTo().Text.c_str();
//    if(F->GetSteps())
      Edit5->Text = ToUString(F->GetSteps().Text);
    LineSelect1->LineStyle = F->GetStyle();
    UpDown1->Position = F->GetSize();
    ExtColorBox1->Selected = F->GetColor();
    Edit7->Text = ToUString(F->GetLegendText());
    ComboBox2->ItemIndex = F->GetEndpointStyle().first;
    ComboBox3->ItemIndex = F->GetEndpointStyle().second;
    ComboBox4->ItemIndex = F->GetDrawType();
    LineSelect1->Enabled = F->GetDrawType() != dtDots;
  }

  OldItemIndex = ComboBox1->ItemIndex;
  ComboBox1Change(NULL);
  return ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TForm5::ComboBox1Change(TObject *Sender)
{
  //Save current state
  TempData[OldItemIndex].Text1 = Edit1->Text;
  TempData[OldItemIndex].Text2 = Edit2->Text;
  TempData[OldItemIndex].From = Edit3->Text;
  TempData[OldItemIndex].To = Edit4->Text;
  TempData[OldItemIndex].Steps = Edit5->Text;

  OldItemIndex = ComboBox1->ItemIndex;

  Edit1->Text = TempData[ComboBox1->ItemIndex].Text1;
  Edit2->Text = TempData[ComboBox1->ItemIndex].Text2;
  Edit3->Text = TempData[ComboBox1->ItemIndex].From;
  Edit4->Text = TempData[ComboBox1->ItemIndex].To;
  Edit5->Text = TempData[ComboBox1->ItemIndex].Steps;

  if(ComboBox1->ItemIndex == 1)
  {
    Edit2->Visible = true;
    Label3->Visible = true;
  }
  else
  {
    Edit2->Visible = false;
    Label3->Visible = false;
  }

  switch(ComboBox1->ItemIndex)
  {
    case 0:
      Label2->Caption = "f(x)=";
      break;
    case 1:
      Label2->Caption = "x(t)=";
      Label3->Caption = "y(t)=";
      break;
    case 2:
      Label2->Caption = "r(t)=";
      break;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm5::Button3Click(TObject *Sender)
{
  Application->HelpContext(HelpContext);
}
//---------------------------------------------------------------------------
void __fastcall TForm5::ComboBox1KeyPress(TObject *Sender, char &Key)
{
  if(std::isgraph(Key))
  {
    Edit1->SetFocus();
    Edit1->SelText = Key;
    Key = 0;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm5::ComboBoxDrawItem(TWinControl *Control, int Index,
      TRect &Rect, TOwnerDrawState State)
{
  if(TCustomComboBox *ComboBox = dynamic_cast<TCustomComboBox*>(Control))
    DrawComboBoxEndPoint(ComboBox->Canvas, Index, Rect);
}
//---------------------------------------------------------------------------
void __fastcall TForm5::ComboBox4Change(TObject *Sender)
{
  LineSelect1->Enabled = ComboBox4->ItemIndex != 1;
}
//---------------------------------------------------------------------------

