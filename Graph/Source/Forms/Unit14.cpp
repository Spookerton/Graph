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
#include <Clipbrd.hpp>
#include "Unit14.h"
#include "ConfigRegistry.h"
#include "ICompCommon.h"
#include "PyGraph.h"
#include "PyVclObject.h"
#include "Unit1.h"
#pragma warn -8072 //Disable warning: Suspicous pointer arithmetic
#include <boost/format.hpp>
//---------------------------------------------------------------------------
#pragma link "Grid"
#pragma link "PointSelect"
#pragma link "LineSelect"
#pragma link "ExtColorBox"
#pragma link "MyEdit"
#pragma link "SaveDialogEx"
#pragma link "ExtColorBox"
#pragma link "Grid"
#pragma link "LineSelect"
#pragma link "MyEdit"
#pragma link "PointSelect"
#pragma link "SaveDialogEx"
#pragma link "ExtComboBox"
#pragma link "GridIntf"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
const int ToGuiAlgorithm[4] = {0, 2, 3, 1};
const TInterpolationAlgorithm FromGuiAlgorithm[4] = {iaLinear, iaCubicSpline2, iaCubicSpline, iaHalfCosine};
//---------------------------------------------------------------------------
__fastcall TForm14::TForm14(TComponent* Owner, TData &AData)
    : TForm(Owner), Data(AData), FontChanged(false)
{
  Translate();
  ScaleForm(this);

  FontDialog1->Font->Assign(Options.DefaultPointLabelFont);
  Edit1->Text = Data.CreatePointSeriesDescription().c_str();

  PointSelect1->ItemIndex = Options.DefaultPoint.Style;
  ExtColorBox1->Selected = Options.DefaultPoint.Color;
  UpDown1->Position = Options.DefaultPoint.Size;

  LineSelect1->LineStyle = static_cast<TPenStyle>(Options.DefaultPointLine.Style);
  ExtColorBox2->Selected = Options.DefaultPointLine.Color;
  UpDown2->Position = Options.DefaultPointLine.Size;

  ComboBox1->ItemIndex = 1;
  UpdateErrorBars();
  PageControl1->ActivePage = TabSheet1;
  ComboBox2->ItemIndex = ToGuiAlgorithm[GetRegValue(REGISTRY_KEY "\\Property", L"Interpolation", HKEY_CURRENT_USER, iaLinear)];
  FlipForm(this);
}
//---------------------------------------------------------------------------
void TForm14::Translate()
{
  TranslateProperties(this);
  TranslateStrings(ExtColorBox1->Items);
  TranslateStrings(ExtColorBox2->Items);
  SetAccelerators(this);

  ResizeControl(Edit1, Label3);
  ResizeControl(ComboBox1, Label8);
  int LabelWidth = TMaxWidth(Label1)(Label2)(Label4)(Label5)(Label6)(Label7)(Label11);
  if(LabelWidth > 61)
    Width = Width + LabelWidth - 61;

  ComboBox2->ItemIndex = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm14::FormShow(TObject *Sender)
{
  //Grid needs do receive focus from something else
  Button1->SetFocus();
  Grid->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Popup_CutClick(TObject *Sender)
{
  Grid->CutToClipboard(Options.DecimalSeparator);
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Popup_CopyClick(TObject *Sender)
{
  Grid->CopyToClipboard(Options.DecimalSeparator);
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Popup_PasteClick(TObject *Sender)
{
  Grid->PasteFromClipboard(Options.DecimalSeparator);
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Popup_DeleteClick(TObject *Sender)
{
  Grid->ClearSelection();
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Popup_SelectClick(TObject *Sender)
{
  Grid->SelectAll();
}
//---------------------------------------------------------------------------
void __fastcall TForm14::PopupMenu1Popup(TObject *Sender)
{
  Popup_Paste->Enabled = Clipboard()->HasFormat(CF_TEXT);
  Popup_Copy->Enabled = Grid->CanCopy();
  Popup_Cut->Enabled = Grid->CanCopy();
}
//---------------------------------------------------------------------------
void __fastcall TForm14::GridEditorKeyPress(TInplaceEdit *InplaceEdit,
      char &Key)
{
  if(Key == '\n')
    Button1->Click();
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Button1Click(TObject *Sender)
{
  if(Apply())
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------
int TForm14::EditPointSeries(const boost::shared_ptr<TPointSeries> &P)
{
  Series = P;
  if(P)
  {
    Caption = LoadRes(537);
    Edit1->Text = ToUString(P->GetLegendText());
    ExtColorBox1->Selected = P->GetFillColor();
    UpDown1->Position = P->GetSize();
    PointSelect1->ItemIndex = P->GetStyle();
    ExtColorBox2->Selected = P->GetLineColor();
    UpDown2->Position = P->GetLineSize();
    LineSelect1->LineStyle = P->GetLineStyle();
		ComboBox2->ItemIndex = ToGuiAlgorithm[P->GetInterpolation()];
    Grid->RowCount = P->PointCount() + 2;
    CheckBox2->Checked = P->GetShowLabels();
    ComboBox1->Enabled = CheckBox2->Checked;
    FontDialog1->Font->Assign(P->GetFont());
    ComboBox1->ItemIndex = P->GetLabelPosition();
    RadioGroup1->ItemIndex = P->GetPointType();

    DataPoints.reserve(P->GetPointData().size() + 10); //Make space for a little extra
    DataPoints = P->GetPointData(); //Create a working copy of all data

    CheckBox3->Checked = P->GetxErrorBarType() != ebtNone;
    switch(P->GetxErrorBarType())
    {
      case ebtNone:
        break;

      case ebtFixed:
        RadioButton1->Checked = true;
        Edit4->Text = P->GetxErrorValue();
        break;

      case ebtRelative:
        RadioButton2->Checked = true;
        Edit5->Text = P->GetxErrorValue();
        break;

      case ebtCustom:
        RadioButton3->Checked = true;
        break;
    }

    CheckBox4->Checked = P->GetyErrorBarType() != ebtNone;
    switch(P->GetyErrorBarType())
    {
      case ebtNone:
        break;

      case ebtFixed:
        RadioButton4->Checked = true;
        Edit6->Text = P->GetyErrorValue();
        break;

      case ebtRelative:
        RadioButton5->Checked = true;
        Edit7->Text = P->GetyErrorValue();
        break;

      case ebtCustom:
        RadioButton6->Checked = true;
        break;
    }
  }
  Button4->Enabled = true;
  return ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TForm14::PaintBox1Paint(TObject *Sender)
{
  try
  {
    int X = PaintBox1->Width / 2;
    int Y = PaintBox1->Height / 2;
    int PointSize = Edit2->Text.ToInt();

    PaintBox1->Canvas->Pen->Width = 1;
    PaintBox1->Canvas->Pen->Color = clBlack;
    if(CheckBox3->Checked && PointSize)
    {
      PaintBox1->Canvas->MoveTo(X - 10, Y);
      PaintBox1->Canvas->LineTo(X + 10, Y);
      PaintBox1->Canvas->MoveTo(X - 10, Y - 4);
      PaintBox1->Canvas->LineTo(X - 10, Y + 5);
      PaintBox1->Canvas->MoveTo(X + 10, Y - 4);
      PaintBox1->Canvas->LineTo(X + 10, Y + 5);
    }

    if(CheckBox4->Checked && PointSize)
    {
      PaintBox1->Canvas->MoveTo(X, Y - 10);
      PaintBox1->Canvas->LineTo(X, Y + 10);
      PaintBox1->Canvas->MoveTo(X - 4, Y - 10);
      PaintBox1->Canvas->LineTo(X + 5, Y - 10);
      PaintBox1->Canvas->MoveTo(X - 4, Y + 10);
      PaintBox1->Canvas->LineTo(X + 5, Y + 10);
    }

    PaintBox1->Canvas->Pen->Handle = SetPen(ExtColorBox2->Selected, LineSelect1->LineStyle, Edit3->Text.ToInt());
    PaintBox1->Canvas->MoveTo(0, Y);
    PaintBox1->Canvas->LineTo(PaintBox1->Width, Y);

    TPointSelect::DrawPoint(PaintBox1->Canvas, TPoint(X, Y), PointSelect1->ItemIndex, clBlack, ExtColorBox1->Selected, PointSize);

    if(CheckBox2->Checked)
    {
      using boost::wformat;
	  std::wstring Str;
      if(RadioGroup1->ItemIndex == 0)
		Str = str(wformat(GuiFormatSettings.CartesianPointFormat) % L"2.37" % L"9.53");
	  else if(Data.Axes.Trigonometry == Func32::Radian)
		Str = str(wformat(GuiFormatSettings.RadianPointFormat) % L"1.18" % L"12.5");
	  else
		Str = str(wformat(GuiFormatSettings.DegreePointFormat) % L"87.3" % L"12.5");
      PaintBox1->Canvas->Font->Assign(FontDialog1->Font);
      TDraw::DrawPointLabel(PaintBox1->Canvas, TPoint(X, Y), PointSize, Str, static_cast<Graph::TLabelPosition>(ComboBox1->ItemIndex));
    }
  }
  catch(...)
  {
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Change(TObject *Sender)
{
  PaintBox1->Invalidate();
  PointSelect1->FillColor = ExtColorBox1->Selected;
  ComboBox1->Enabled = CheckBox2->Checked;
}
//---------------------------------------------------------------------------
HPEN TForm14::SetPen(TColor Color, TPenStyle Style, int Width)
{
  if(Width > 1)
  {
    unsigned long DashStyle[] = {16, 8};
    LOGBRUSH LogBrush;
    LogBrush.lbStyle = BS_SOLID;
    LogBrush.lbColor = Color;
    if(Style == PS_DASH)
      return ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, Width, &LogBrush, sizeof(DashStyle)/sizeof(*DashStyle), DashStyle);
    return ExtCreatePen(PS_GEOMETRIC | Style, Width, &LogBrush, 0, NULL);
  }
  return CreatePen(Width == 0 ? psClear : Style, Width, Color);
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Button3Click(TObject *Sender)
{
  Application->HelpContext(HelpContext);
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Popup_InsertClick(TObject *Sender)
{
  Grid->InsertRows(Grid->Row, 1);
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Popup_RemoveClick(TObject *Sender)
{
  Grid->RemoveRows(Grid->Selection.Top, Grid->Selection.Bottom - Grid->Selection.Top + 1);
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Popup_ImportClick(TObject *Sender)
{
  try
  {
    const char Separators[] = {';',',','\t',' ',0};
    OpenDialog1->Filter = LoadRes(RES_DATA_FILTER);
    if(OpenDialog1->Execute())
    {
      Grid->ImportFromFile(OpenDialog1->FileName, Options.DecimalSeparator, Separators[OpenDialog1->FilterIndex-1]);
      int Row = Grid->Selection.Top;
      if(Grid->Cells[0][Row] == L"X" && Grid->Cells[1][Row] == L"Y")
        Grid->RemoveRows(Row, 1);
    }
  }
  catch(std::bad_alloc &E)
  {
    MessageBox(LoadString(RES_OUT_OF_MEMORY), LoadString(RES_ERROR));
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Button5Click(TObject *Sender)
{
  if(FontDialog1->Execute())
  {
    PaintBox1->Invalidate();
    FontChanged = true;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Popup_ExportClick(TObject *Sender)
{
  SaveDialog1->Filter = LoadRes(RES_EXPORT_DATA_FILTER);
  if(SaveDialog1->Execute())
    if(!Grid->ExportToFile(SaveDialog1->FileName, SaveDialog1->FilterIndex == 1 ? ';' : '\t', Options.DecimalSeparator))
      MessageBox(LoadRes(RES_FILE_ACCESS, SaveDialog1->FileName), LoadRes(RES_WRITE_FAILED), MB_ICONSTOP);
}
//---------------------------------------------------------------------------
void __fastcall TForm14::EditKeyPress(TObject *Sender, char &Key)
{
  if(!isdigit(Key) && Key!='\b')
    Key = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm14::RadioButtonClick(TObject *Sender)
{
  UpdateErrorBars();
  PaintBox1->Invalidate();
}
//---------------------------------------------------------------------------
void TForm14::UpdateErrorBars()
{
  unsigned Cols = 2;
  Cols += CheckBox3->Checked && RadioButton3->Checked;
  Cols += CheckBox4->Checked && RadioButton6->Checked;
  Width = Width + (Cols - Grid->ColCount) * Grid->DefaultColWidth;
  Grid->ColCount = Cols;

  RadioButton1->Enabled = CheckBox3->Checked;
  RadioButton2->Enabled = CheckBox3->Checked;
  RadioButton3->Enabled = CheckBox3->Checked;
  Edit4->Enabled = CheckBox3->Checked;
  Edit5->Enabled = CheckBox3->Checked;

  RadioButton4->Enabled = CheckBox4->Checked;
  RadioButton5->Enabled = CheckBox4->Checked;
  RadioButton6->Enabled = CheckBox4->Checked;
  Edit6->Enabled = CheckBox4->Checked;
  Edit7->Enabled = CheckBox4->Checked;
}
//---------------------------------------------------------------------------
std::wstring& TForm14::GetText(int ACol, int ARow)
{
  unsigned NewSize = std::max(std::max(Grid->RowCount - 2, ARow), static_cast<int>(DataPoints.size()));
  if(NewSize != DataPoints.size())
    DataPoints.resize(NewSize);
  switch(ACol)
  {
    case 0: return DataPoints[ARow-1].First;
    case 1: return DataPoints[ARow-1].Second;
    case 2: return CheckBox3->Enabled && RadioButton3->Checked ? DataPoints[ARow-1].xError.Text : DataPoints[ARow-1].yError.Text;
    case 3: return DataPoints[ARow-1].yError.Text;
  }
  throw Exception("Invalid Coloumn");
}
//---------------------------------------------------------------------------
void __fastcall TForm14::EditChange(TObject *Sender)
{
  if(Sender == Edit4)
    RadioButton1->Checked = true;
  else if(Sender == Edit5)
    RadioButton2->Checked = true;
  else if(Sender == Edit6)
    RadioButton4->Checked = true;
  else if(Sender == Edit7)
    RadioButton5->Checked = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm14::GridGetText(TObject *Sender, int ACol, int ARow,
      String &Value)
{
  if(ARow == 0)
    switch(ACol)
    {
      case 0: Value = RadioGroup1->ItemIndex ? L"\x3B8" : L"X"; break;
      case 1: Value = RadioGroup1->ItemIndex ? L"r" : L"Y"; break;
      case 2: Value = LoadRes(RES_UNCERTAINTY, (CheckBox3->Checked && RadioButton3->Checked) ? "X" : "Y"); break;
      case 3: Value = LoadRes(RES_UNCERTAINTY, "Y"); break;
    }
  else
    Value = GetText(ACol, ARow).c_str();
}
//---------------------------------------------------------------------------
void __fastcall TForm14::GridSetText(TObject *Sender, int ACol, int ARow,
      String Value)
{
  if(ARow > 0)
    GetText(ACol, ARow) = ToWString(Value);
}
//---------------------------------------------------------------------------
void __fastcall TForm14::RadioGroup1Click(TObject *Sender)
{
  Grid->Invalidate();
  PaintBox1->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TForm14::Button4Click(TObject *Sender)
{
  if(!Apply())
    return;
  Data.SetModified();
  Python::ExecutePluginEvent(Python::peChanged, boost::shared_ptr<TGraphElem>(Series));
  Form1->UpdateTreeView();
  Form1->Redraw();
}
//---------------------------------------------------------------------------
bool TForm14::Apply()
{
  TErrorBarType xErrorBarType, yErrorBarType;
  double xErrorValue = 0, yErrorValue = 0;
  if(!CheckBox3->Checked)
    xErrorBarType = ebtNone;
  else if(RadioButton1->Checked)
  {
    xErrorValue = MakeFloat(Edit4, LoadRes(RES_POSITIVE, RadioButton1->Caption), 0);
    xErrorBarType = ebtFixed;
  }
  else if(RadioButton2->Checked)
  {
    xErrorValue = MakeFloat(Edit5, LoadRes(RES_POSITIVE, RadioButton2->Caption), 0);
    xErrorBarType = ebtRelative;
  }
  else
    xErrorBarType = ebtCustom;

  if(!CheckBox4->Checked)
    yErrorBarType = ebtNone;
  else if(RadioButton4->Checked)
  {
    yErrorValue = MakeFloat(Edit6, LoadRes(RES_POSITIVE, RadioButton4->Caption), 0);
    yErrorBarType = ebtFixed;
  }
  else if(RadioButton5->Checked)
  {
    yErrorValue = MakeFloat(Edit7, LoadRes(RES_POSITIVE, RadioButton5->Caption), 0);
    yErrorBarType = ebtRelative;
  }
  else
    yErrorBarType = ebtCustom;

  boost::shared_ptr<TPointSeries> PointSeries(new TPointSeries(
    clBlack,
    ExtColorBox1->Selected,
    ExtColorBox2->Selected,
    Edit2->Text.ToInt(),
    Edit3->Text.ToInt(),
    PointSelect1->ItemIndex,
    LineSelect1->LineStyle,
    FromGuiAlgorithm[ComboBox2->ItemIndex],
    CheckBox2->Checked,
    FontDialog1->Font,
    static_cast<Graph::TLabelPosition>(ComboBox1->ItemIndex),
    RadioGroup1->ItemIndex ? ptPolar : ptCartesian,
    xErrorBarType,
    xErrorValue,
    yErrorBarType,
    yErrorValue
  ));
  PointSeries->SetLegendText(ToWString(Edit1->Text));

  unsigned Count = DataPoints.size();
  for(unsigned I = 0; I < Count; I++)
  {
    DataPoints[I].First = Trim(DataPoints[I].First);
    DataPoints[I].Second = Trim(DataPoints[I].Second);
    if(DataPoints[I].First.empty() && DataPoints[I].Second.empty())
      continue;

    if(DataPoints[I].First.empty() || DataPoints[I].Second.empty())
    {
      Grid->Col = DataPoints[I].Second.empty();
      Grid->Row = I + 1;
      Grid->SetFocus();
      MessageBox(LoadRes(534), LoadRes(533));
      return false;
    }

    //Just for validation
    CellToDouble(Grid, 0, I+1);
    CellToDouble(Grid, 1, I+1);

    PointSeries->InsertPoint(DataPoints[I], -1, false);
  }

  if(PointSeries->PointCount() == 0)
  {
    MessageBox(LoadRes(536), LoadRes(533), MB_ICONWARNING);
    return false;
  }

  if(Series)
  {
    PointSeries->SetData(&Form1->Data);
    PointSeries->SetVisible(Series->GetVisible());
    PointSeries->SetShowInLegend(Series->GetShowInLegend());
    Series->Swap(*PointSeries);
    UndoList.Push(TUndoChange(Series, PointSeries));
    Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(Series));
  }
  else
  {
    UndoList.Push(TUndoAdd(PointSeries));
    Data.Insert(PointSeries);
    Series = PointSeries;
  }

  Series->Update();
  Options.DefaultPoint.Set(PointSelect1->ItemIndex, ExtColorBox1->Selected, Edit2->Text.ToInt());
  Options.DefaultPointLine.Set(LineSelect1->LineStyle, ExtColorBox2->Selected, Edit3->Text.ToInt());
  if(FontChanged)
    Options.DefaultPointLabelFont->Assign(FontDialog1->Font);
  SetRegValue(REGISTRY_KEY "\\Property", L"Interpolation", HKEY_CURRENT_USER, FromGuiAlgorithm[ComboBox2->ItemIndex]);
  return true;
}
//---------------------------------------------------------------------------

