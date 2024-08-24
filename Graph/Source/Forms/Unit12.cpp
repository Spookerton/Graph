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
#include "Unit12.h"
#include "PyGraph.h"
#include <float.h>
//---------------------------------------------------------------------------
#pragma link "LineSelect"
#pragma link "ExtColorBox"
#pragma link "MyEdit"
#pragma link "ExtComboBox"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TForm12::TForm12(TComponent* Owner, TData &AData)
    : TForm(Owner), Data(AData)
{
  TranslateProperties(this);
  TranslateStrings(ExtColorBox1->Items);
  SetAccelerators(this);
  ScaleForm(this);
  Edit4->Left = Label4->Left + Label4->Width + 5;
  ResizeControl(Edit2, Label10);

  int Left = Label7->Left + TMaxWidth(Label7)(Label8)(Label9) + 5;
  LineSelect1->Left = Left;
  ExtColorBox1->Left = Left - 2;
  Edit3->Left = Left;

  MoveControl(ComboBox1, Label11);
  MoveLabel(ComboBox2, Label12);

  LineSelect1->ItemIndex = Options.DefaultTangent.Style;
  ExtColorBox1->Selected = Options.DefaultTangent.Color;
  UpDown1->Position = Options.DefaultTangent.Size;
  ComboBox1->ItemIndex = 0;
  ComboBox2->ItemIndex = 0;
  FlipForm(this);
}
//---------------------------------------------------------------------------
void __fastcall TForm12::Button1Click(TObject *Sender)
{
  boost::shared_ptr<TTangent> Tan(new TTangent);
  Tan->SetData(&Data);
  int Size = ToIntDef(Edit3->Text, 1);
  //check if size is less than 1
  if(Size <= 0)
  {
    MessageBox(LoadRes(513), LoadRes(RES_ERROR_IN_VALUE));
    Edit3->SetFocus();
    return;
  }

  Tan->SetPos(TTextValue(MakeFloat(Edit1),ToWString(Edit1->Text)));
  Data.AbortUpdate();

  Tan->SetSize(Size);
  Tan->SetStyle(LineSelect1->LineStyle);
  Tan->SetColor(ExtColorBox1->Selected);
  Tan->SetTangentType(RadioGroup1->ItemIndex == 0 ? ttTangent : ttNormal);
  Tan->SetLegendText(ToWString(Edit2->Text));

  Tan->SetFrom(TTextValue(!Edit4->Text.IsEmpty() ? MakeFloat(Edit4) : -INF, ToWString(Edit4->Text)));
  Tan->SetTo(TTextValue(!Edit5->Text.IsEmpty() ? MakeFloat(Edit5) : INF, ToWString(Edit5->Text)));

  Tan->SetEndpointStyle(std::make_pair(ComboBox1->ItemIndex, ComboBox2->ItemIndex));

  if(Tan->GetFrom().Value >= Tan->GetTo().Value)
  {
    MessageBox(LoadRes(511), LoadRes(512));
    return;
  }

  if(!OldTan)
  {
    Parent->InsertChild(Tan);
    UndoList.Push(TUndoAdd(Tan));
  }
  else
  {
    Tan->SetVisible(OldTan->GetVisible());
    Tan->SetShowInLegend(OldTan->GetShowInLegend());
    Tan->Swap(*OldTan);
    UndoList.Push(TUndoChange(OldTan, Tan));
    Tan = OldTan; //Because of the check on Tan below
    Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(OldTan));
  }

  if(!Tan->CalcTan())
  {
    std::wstring Variable = L"t";
    if(boost::shared_ptr<TBaseFuncType> Func = boost::dynamic_pointer_cast<TBaseFuncType>(Parent))
      Variable = Func->GetVariable();
    MessageBox(LoadRes(518, Variable, Tan->GetPos().Text), LoadString(519));
    UndoList.Undo();
    UndoList.ClearRedo();
    return;
  }

  Options.DefaultTangent.Set(LineSelect1->ItemIndex, ExtColorBox1->Selected, Edit3->Text.ToInt());

  ModalResult=mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TForm12::Edit3KeyPress(TObject *Sender, char &Key)
{
  if((!isdigit(Key) && Key!='\b') || (Key=='0' && Edit3->Text.IsEmpty()))
    Key=0;//Allown only digits and backspace. 0 may not be placed first.
}
//---------------------------------------------------------------------------
int TForm12::EditTan(const boost::shared_ptr<TTangent> &Tan)
{
  OldTan = Tan;
  Caption = LoadRes(525);
  Parent = Tan->GetParent();
  Edit1->Text = Tan->GetPos().Text.c_str();
  UpDown1->Position = Tan->GetSize();
  LineSelect1->LineStyle = Tan->GetStyle();
  ExtColorBox1->Selected = Tan->GetColor();
  if(boost::shared_ptr<TBaseFuncType> Func = boost::dynamic_pointer_cast<TBaseFuncType>(Parent))
    Label1->Caption = ToUString(Func->GetVariable() + L"=");
  Edit2->Text = ToUString(Tan->GetLegendText());
  RadioGroup1->ItemIndex = Tan->GetTangentType();
  Edit4->Text = Tan->GetFrom().Text.c_str();
  Edit5->Text = Tan->GetTo().Text.c_str();
  ComboBox1->ItemIndex = Tan->GetEndpointStyle().first;
  ComboBox2->ItemIndex = Tan->GetEndpointStyle().second;

  return ShowModal();
}
//---------------------------------------------------------------------------
int TForm12::InsertTan(const TGraphElemPtr &AParent)
{
  Parent = AParent;
  if(boost::shared_ptr<TBaseFuncType> Func = boost::dynamic_pointer_cast<TBaseFuncType>(Parent))
    Label1->Caption = Func->GetVariable().c_str() + String('=');
  return ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TForm12::Button3Click(TObject *Sender)
{
  Application->HelpContext(HelpContext);
}
//---------------------------------------------------------------------------
void __fastcall TForm12::ComboBoxDrawItem(TWinControl *Control, int Index,
      TRect &Rect, TOwnerDrawState State)
{
  if(TCustomComboBox *ComboBox = dynamic_cast<TCustomComboBox*>(Control))
    DrawComboBoxEndPoint(ComboBox->Canvas, Index, Rect);
}
//---------------------------------------------------------------------------

