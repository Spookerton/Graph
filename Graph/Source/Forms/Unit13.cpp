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
#include "Unit13.h"
#include "Unit8.h"
//---------------------------------------------------------------------------
#pragma link "LineSelect"
#pragma link "MyRadioButton"
#pragma link "MyEdit"
#pragma link "ExtColorBox"
#pragma link "CheckBoxEx"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TForm13::TForm13(TComponent* Owner, TData &AData)
        : TForm(Owner), Data(AData)
{
  int OldLabelWidth = TMaxWidth(Label5)(Label6)(Label7);
  TranslateProperties(this);
  TranslateStrings(ExtColorBox1->Items);
  SetAccelerators(this);
  ScaleForm(this); //Moving this may cause problem for the Arabic translation
  int LabelWidth = TMaxWidth(Label5)(Label6)(Label7);
  if(LabelWidth != OldLabelWidth)
    Width = Width + LabelWidth - OldLabelWidth;
  ResizeControl(Edit5, CheckBox1->Left + CheckBox1->Width);
  PageControl1->ActivePage = TabSheet1;
  FlipForm(this);

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
void __fastcall TForm13::ImageClick(TObject *Sender)
{
  if(Sender == Image1)
  {
    if(RadioButton1->CanFocus())
      RadioButton1->SetFocus();
  }
  else if(Sender == Image2)
  {
    if(RadioButton2->CanFocus())
      RadioButton2->SetFocus();
  }
  else if(Sender == Image3)
  {
    if(RadioButton3->CanFocus())
      RadioButton3->SetFocus();
  }
  else if(Sender == Image4)
  {
    if(RadioButton4->CanFocus())
      RadioButton4->SetFocus();
  }
  else if(Sender == Image5)
	{
    if(RadioButton5->CanFocus())
      RadioButton5->SetFocus();
  }
  else if(Sender == Image6)
  {
    if(RadioButton6->CanFocus())
      RadioButton6->SetFocus();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm13::EditKeyPress(TObject *Sender, char &Key)
{
  if(!isdigit(Key) && Key != '\b')
    Key = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm13::Button1Click(TObject *Sender)
{
  if(ToIntDef(Edit1->Text, -1) < 0)
  {
    Edit1->SetFocus();
    MessageBox(LoadRes(RES_LIMIT_ZERO, StaticText1->Caption), LoadRes(RES_ERROR_IN_VALUE));
    return;
  }

  if(ToIntDef(Edit2->Text, -1) < 1)
  {
    Edit2->SetFocus();
    MessageBox(LoadRes(RES_INT_GREATER_ZERO, Label5->Caption), LoadRes(RES_ERROR_IN_VALUE));
    return;
  }

  try
  {
		boost::shared_ptr<TBaseFuncType> BaseFunc;
    TTrendData Trend;
    if(PageControl1->TabIndex == 1)
    {
      Trend.Type = ttUserModel;
      Trend.Model = ToWString(Edit3->Text);
      if(ListBox1->ItemIndex != -1)
        Trend.Defaults = Data.UserModels[ToWString(ListBox1->Items->Strings[ListBox1->ItemIndex])].Defaults;
    }
    else if(RadioButton6->Checked) //Moving average
    {
      if(Edit4->Text.ToInt() == 0)
      {
        MessageBox(LoadRes(RES_INT_GREATER_ZERO, StaticText2->Caption), LoadRes(RES_ERROR_IN_VALUE));
        return;
      }
      Trend.Type = ttMovingAverage;
      Trend.Period = Edit4->Text.ToInt();
    }
    else
    {
      TTrendType Type;
      if(RadioButton1->Checked)
        Trend.Type = ttLinear;
      else if(RadioButton2->Checked)
        Trend.Type = ttLogarithmic;
      else if(RadioButton3->Checked)
        Trend.Type = ttPolynomial;
      else if(RadioButton4->Checked)
        Trend.Type = ttPower;
      else if(RadioButton5->Checked)
        Trend.Type = ttExponential;
      else
        throw Exception("No radio button selected!");

      Trend.Order = Edit1->Text.ToInt();
      Trend.Intercept = CheckBox1->Checked ? MakeFloat(Edit5) : NAN;
    }

    BaseFunc = CreateTrendline(Series.get(), Trend);
    BaseFunc->SetColor(ExtColorBox1->Selected);
    BaseFunc->SetSize(Edit2->Text.ToInt());
    BaseFunc->SetStyle(LineSelect1->LineStyle);
    Data.Insert(BaseFunc);
    UndoList.Push(TUndoAdd(BaseFunc));
  }
  catch(Func32::EFuncError &Error)
  {
    ShowErrorMsg(Error, PageControl1->TabIndex == 1 ? Edit3 : NULL);
    return;
  }
  catch(Exception &Error)
  {
    ShowErrorMsg(Error);
    return;
  }

  Options.DefaultTrendline.Set(LineSelect1->LineStyle, ExtColorBox1->Selected, Edit2->Text.ToInt());
  ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TForm13::Button3Click(TObject *Sender)
{
  Application->HelpContext(HelpContext);
}
//---------------------------------------------------------------------------
void TForm13::ShowUserModels(const std::wstring &Selected)
{
  ListBox1->Clear();
  for(TUserModels::const_iterator Iter = Data.UserModels.begin(); Iter != Data.UserModels.end(); ++Iter)
    ListBox1->AddItem(Iter->first.c_str(), NULL);
  ListBox1->ItemIndex = ListBox1->Items->IndexOf(Selected.c_str());
}
//---------------------------------------------------------------------------
void __fastcall TForm13::ListBox1Click(TObject *Sender)
{
  if(ListBox1->ItemIndex != -1)
  {
    Edit3->Text = Data.UserModels[ToWString(ListBox1->Items->Strings[ListBox1->ItemIndex])].Model.c_str();
    Button5->Enabled = true;
    Button6->Enabled = true;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm13::Button4Click(TObject *Sender)
{
  try
  {
    std::wstring ModelName;
    if(CreateForm<TForm8>(Data)->AddModel(::ToWString(Edit3->Text), ModelName))
    {
      ShowUserModels(ModelName);
      Button5->Enabled = true;
      Button6->Enabled = true;
    }
  }
  catch(Func32::EFuncError &Error)
  {
    Edit3->SetFocus();
    ShowErrorMsg(Error, Edit3);
    return;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm13::Button6Click(TObject *Sender)
{
  if(ListBox1->ItemIndex == -1)
    return;

  std::wstring ModelName = ToWString(ListBox1->Items->Strings[ListBox1->ItemIndex]);
  if(CreateForm<TForm8>(Data)->EditModel(ModelName))
    ShowUserModels(ModelName);
}
//---------------------------------------------------------------------------
void __fastcall TForm13::Button5Click(TObject *Sender)
{
  if(ListBox1->ItemIndex == -1)
    return;

  if(MessageBox(LoadRes(RES_DELETE_MODEL, ListBox1->Items->Strings[ListBox1->ItemIndex]), LoadRes(RES_CAPTION_DELETE), MB_ICONQUESTION | MB_YESNO) == IDYES)
  {
    Data.UserModels.erase(ToWString(ListBox1->Items->Strings[ListBox1->ItemIndex]));
    ListBox1->Items->Delete(ListBox1->ItemIndex);
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm13::Edit3Change(TObject *Sender)
{
  if(Edit3->Focused())
  {
    ListBox1->ItemIndex = -1;
    Button5->Enabled = false;
    Button6->Enabled = false;
  }
}
//---------------------------------------------------------------------------
int TForm13::InsertTrendline(const boost::shared_ptr<TPointSeries> &ASeries)
{
  Series = ASeries;

  bool NegXFound = false, NegYFound = false, ZeroXFound = false, ZeroYFound = false;

  unsigned Count = Series->PointCount();
  const TPointSeries::TPointList &Points = Series->GetPointList();
  for(unsigned I = 0; I < Count; I++)
  {
    const Func32::TDblPoint &P = Points[I];
    if(P.x < 0)
      NegXFound = true;
    else if(P.x == 0)
      ZeroXFound = true;

    if(P.y < 0)
      NegYFound = true;
    else if(P.y == 0)
      ZeroYFound = true;
  }

  if(NegXFound || ZeroXFound)
    RadioButton2->Enabled = false;

  if(NegYFound || ZeroYFound)
    RadioButton5->Enabled = false;

  if(NegXFound || NegYFound)
    RadioButton4->Enabled = false;

  LineSelect1->LineStyle = static_cast<TPenStyle>(Options.DefaultTrendline.Style);
  ExtColorBox1->Selected = Options.DefaultTrendline.Color;
  UpDown1->Position = Options.DefaultTrendline.Size;

  UpDown2->Max = Count - 1;
  UpDown3->Max = Count - 1;

  ShowUserModels(L"");
  return ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TForm13::RadioButtonClick(TObject *Sender)
{
  Edit1->Enabled = RadioButton3->Checked;
  UpDown2->Enabled = RadioButton3->Checked;

  Edit4->Enabled = RadioButton6->Checked;
  UpDown3->Enabled = RadioButton6->Checked;

  if((RadioButton1->Checked || RadioButton3->Checked || RadioButton5->Checked) && PageControl1->ActivePage == TabSheet1)
  {
    CheckBox1->Enabled = true;
    Edit5->Enabled = true;
    Edit5->Color = clWindow;
  }
  else
  {
    CheckBox1->Enabled = false;
    Edit5->Enabled = false;
    Edit5->Color = clBtnFace;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm13::Edit5Change(TObject *Sender)
{
  CheckBox1->Checked = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm13::CheckBox1Click(TObject *Sender)
{
  //This check is needed because this function may be called as part of the form initialization
  if(Series)
  {
    if(CheckBox1->Checked)
      UpDown2->Max = Series->PointCount();
    else
      UpDown2->Max = Series->PointCount() - 1;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm13::Popup_ImportClick(TObject *Sender)
{
  if(OpenDialog1->Execute())
  {
    std::auto_ptr<TStrings> Strings(new TStringList);
    Strings->LoadFromFile(OpenDialog1->FileName);
    Data.ImportUserModels(Strings->Text.c_str());
    ShowUserModels(L"");
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm13::Popup_ExportClick(TObject *Sender)
{
  if(SaveDialog1->Execute())
  {
    std::auto_ptr<TStrings> Strings(new TStringList);
    Strings->Text = Data.ExportUserModels().c_str();
    Strings->SaveToFile(SaveDialog1->FileName);
  }
}
//---------------------------------------------------------------------------

