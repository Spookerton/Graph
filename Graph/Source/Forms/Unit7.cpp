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
#include "Unit7.h"
//---------------------------------------------------------------------------
#pragma link "LineSelect"
#pragma link "ExtColorBox"
#pragma link "MyEdit"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TForm7::TForm7(TComponent* Owner, TData &AData)
    : TForm(Owner), Data(AData)
{
  TranslateProperties(this);
  TranslateStrings(ExtColorBox1->Items);
  SetAccelerators(this);
  ScaleForm(this);

  Edit1->Left = Label4->Left + Label4->Width + 5;
  Edit2->Left = Label5->Left + Label5->Width + 5;
  ResizeControl(Edit3, Label10);

  int Left = Label7->Left + Label7->Width + 5;
  LineSelect1->Left = Left;
  ExtColorBox1->Left = Left;

  LineSelect1->ItemIndex = Options.DefaultDif.Style;
  ExtColorBox1->Selected = Options.DefaultDif.Color;
  UpDown1->Position = Options.DefaultDif.Size;
  FlipForm(this);
}
//---------------------------------------------------------------------------
void __fastcall TForm7::Button1Click(TObject *Sender)
{
  Data.AbortUpdate();

  try
  {
    boost::shared_ptr<TBaseFuncType> Dif(Func->MakeDifFunc());

    Dif->SetFrom(TTextValue(Edit1->Text.IsEmpty() ? -INF : MakeFloat(Edit1), ToWString(Edit1->Text)));
    Dif->SetTo(TTextValue(Edit2->Text.IsEmpty() ? INF : MakeFloat(Edit2), ToWString(Edit2->Text)));

    Dif->SetSize(ToIntDef(Edit4->Text, 1));
    Dif->SetStyle(LineSelect1->LineStyle);
    Dif->SetColor(ExtColorBox1->Selected);
    Dif->SetLegendText(ToWString(Edit3->Text));
    Dif->SetSteps(Func->GetSteps());

    Data.Insert(Dif);
    UndoList.Push(TUndoAdd(Dif));
    Options.DefaultDif.Set(LineSelect1->ItemIndex, ExtColorBox1->Selected, ToIntDef(Edit4->Text, 1));
    ModalResult = mrOk;
  }
  catch(Func32::EFuncError &E)
  {
    ShowErrorMsg(E);
  }
}
//---------------------------------------------------------------------------
int TForm7::InsertDif(const boost::shared_ptr<TBaseFuncType> &F)
{
  Func = F;
  Edit1->Text = Func->GetFrom().Text.c_str();
  Edit2->Text = Func->GetTo().Text.c_str();
  return ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TForm7::Button3Click(TObject *Sender)
{
  Application->HelpContext(HelpContext);
}
//---------------------------------------------------------------------------

