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
#include <Registry.hpp>
#include <StrUtils.hpp>
#include "Common.h"
#include "Unit1.h"
#include "Unit4.h"
#include "ConfigRegistry.h"
//---------------------------------------------------------------------------
#pragma link "CheckBoxEx"
#pragma link "GridPanelEx"
#pragma link "RadioGroupEx"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TForm4::TForm4(TComponent* Owner, TData &AData)
	: TForm(Owner), Data(AData)
{
  TranslateProperties(this);
  SetAccelerators(this);
  ScaleForm(this);

  CheckBox2->Checked = Application->ShowHint;
  CheckBox3->Checked = Options.SavePos;
  CheckBox4->Checked = Options.CheckForUpdate;
  UpDown1->Position = Options.RoundTo;
  UpDown2->Position = Form1->Recent1->MaxFiles;
  UpDown3->Position = UndoList.GetMaxUndo();
  RadioGroup1->ItemIndex = Options.ComplexFormat;

  //Put the language codes into a ComboBox
  GetLanguageList(ComboBox2->Items);
  OldLanguageIndex = ComboBox2->Items->IndexOf(ToUString(Options.Language));
  ComboBox2->ItemIndex = OldLanguageIndex;
  ComboBox1->Text = String(Options.FontScale) + "%";

  ClientWidth = GridPanelEx1->Width + GridPanelEx1->Left * 2;
  FlipForm(this);
  ComboBox1->SelLength = 0; //Don't know why this is necesarry

  CheckBox5->Checked = Options.CustomDecimalSeparator;
  Edit4->Text = Options.DecimalSeparator;
	Edit4->Enabled = CheckBox5->Checked;
	Edit4->Color = CheckBox5->Checked ? clWindow : clBtnFace;
}
//---------------------------------------------------------------------------
void __fastcall TForm4::Button1Click(TObject *Sender)
{
  if(!CheckLimit(Edit1, LoadRes(RES_LIMIT_ZERO, Label1->Caption),0))
    return;
  if(!CheckLimit(Edit2, LoadRes(510), 0, 9))
    return;
  if(!CheckLimit(Edit3, LoadRes(530, Label3->Caption),0,1000))
    return;

  int FontScale = AnsiReplaceStr(ComboBox1->Text, "%", "").ToInt();
  if(FontScale < 50 || FontScale > 300)
  {
    ComboBox1->SetFocus();
    MessageBox(LoadRes(RES_VALUE_RANGE, Label5->Caption, 50, 300), LoadRes(RES_ERROR_IN_VALUE));
    return;
  }

  Data.AbortUpdate();
  UndoList.Push(MakeUndoObject(Options));

  Options.RoundTo = Edit1->Text.ToInt();
  Form1->Recent1->MaxFiles = Edit2->Text.ToInt();
  UndoList.SetMaxUndo(Edit3->Text.ToInt());
  Options.ComplexFormat = (TComplexFormat)RadioGroup1->ItemIndex;;
	Application->ShowHint = CheckBox2->Checked;
	Form1->TreeView->ToolTips = CheckBox2->Checked;
  Options.SavePos = CheckBox3->Checked;
  Options.CheckForUpdate = CheckBox4->Checked;

  Options.CustomDecimalSeparator = CheckBox5->Checked;
  if(Options.CustomDecimalSeparator && Edit4->Text.Length() > 0)
    Options.DecimalSeparator = Edit4->Text[1];
  else
    Options.DecimalSeparator = GetLocaleChar(GetThreadLocale(), LOCALE_SDECIMAL, '.');
  Options.FontScale = FontScale;

  //Load new language if the setting was changed
  if(OldLanguageIndex != ComboBox2->ItemIndex && ComboBox2->ItemIndex != -1)
  {
    Form1->ChangeLanguage(ComboBox2->Text);
    CreateRegKey(REGISTRY_KEY, "Language", ComboBox2->Text, HKEY_CURRENT_USER);
  }

  ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TForm4::Button3Click(TObject *Sender)
{
  Application->HelpContext(HelpContext);
}
//---------------------------------------------------------------------------
void __fastcall TForm4::ComboBox1KeyPress(TObject *Sender, char &Key)
{
  if(!std::isdigit(Key) && Key != VK_BACK && Key != '%')
    Key = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm4::CheckBox5Click(TObject *Sender)
{
  Edit4->Enabled = CheckBox5->Checked;
	Edit4->Color = CheckBox5->Checked ? clWindow : clBtnFace;
}
//---------------------------------------------------------------------------

