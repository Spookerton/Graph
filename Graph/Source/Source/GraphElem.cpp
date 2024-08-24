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
#include "GraphElem.h"
#include <float.h>
#include "ConfigFile.h"
#include "IGraphic.h"
#include "Tokenizer.h"
#include "VersionInfo.h"
#include "GuiUtil.h"
#include <boost/lexical_cast.hpp>
#include <boost/mem_fn.hpp>
#include <boost/bind.hpp>
//---------------------------------------------------------------------------
namespace Graph
{
long double FastCalc(const std::wstring &Text, const TData &Data, bool IgnoreErrors)
{
  if(Text.empty())
  {
    if(!IgnoreErrors)
      throw Func32::EFuncError(Func32::ecEmptyString);
    return NAN;
  }

  //Optimize for numbers
  //Make sure there is no 'e' (Euler's constant) as it will be interpretted as 'E'
  if(Text.find(L"e") == std::wstring::npos)
  {
    //use wcstod as boost::lexical_cast is slow when it fails
    wchar_t *endptr;
    double Value = wcstod(Text.c_str(), &endptr);
    if(endptr == Text.c_str() + Text.size())
      return Value;
    if(Text == L"INF")
      //wcstod() handles "+INF" and "-INF"
      return INF;
  }

  return IgnoreErrors ? Data.CalcNoThrow(Text) : Data.Calc(Text);
}
//---------------------------------------------------------------------------
////////////////
// TTextValue //
////////////////
TTextValue::TTextValue(double AValue) : Value(AValue)
{
  if(_finite(Value))
    Text = ToWString(AValue);
}
//---------------------------------------------------------------------------
TTextValue::TTextValue(const std::wstring &AText, const TData &Data, bool IgnoreErrors)
{
  Value = FastCalc(AText, Data, IgnoreErrors);
  //If AText fails to evaluate, it returns NAN; -INF/+INF in the .grf file are shown as empty fields in the GUI
  Text = _finite(Value) || _isnan(Value) ? AText : std::wstring();
}
//---------------------------------------------------------------------------
void TTextValue::Update(const TData &Data)
{
  //Nothing to update. Values is already +/-INF
  if(!Text.empty())
    Value = FastCalc(Text, Data, true);
}
//---------------------------------------------------------------------------
void TTextValue::Set(const std::wstring AText, const TData &Data, bool IgnoreErrors)
{
  Value = FastCalc(AText, Data, IgnoreErrors);
  //If AText fails to evaluate, it returns NAN; -INF/+INF in the .grf file are shown as empty fields in the GUI
  Text = _finite(Value) || _isnan(Value) ? AText : std::wstring();
}
//---------------------------------------------------------------------------
void TTextValue::Set(double AValue)
{
  Value = AValue;
  Text = _finite(Value) ? ToWString(AValue) : std::wstring();
}
//---------------------------------------------------------------------------
bool TTextValue::IsDependent(const TData &Data, const std::wstring &SymbolName) const
{
  try
  {
    return Func32::TFunc(Text, L"", Data.CustomFunctions.SymbolList).IsDependent(SymbolName);
  }
  catch(Func32::EFuncError &E)
  {
    return false;
  }
}
//---------------------------------------------------------------------------
std::wostream& operator<<(std::wostream &Stream, const TTextValue &TextValue)
{
  if(TextValue.Value == INF)
    return Stream << L"+INF";
  else if(TextValue.Value == -INF)
    return Stream << L"-INF";
  return Stream << TextValue.Text;
}
//---------------------------------------------------------------------------
////////////////
// TGraphElem //
////////////////
TGraphElem::TGraphElem()
  : Visible(true), ShowInLegend(true), UpdateFinished(false), Data(NULL),
    ShowInFunctionList(true), CustomData(NULL)
{
}
//---------------------------------------------------------------------------
TGraphElem::TGraphElem(const TGraphElem &Elem)
  : Visible(Elem.Visible), ShowInLegend(Elem.ShowInLegend), LegendText(Elem.LegendText),
    UpdateFinished(false), Data(NULL), ShowInFunctionList(Elem.ShowInFunctionList),
    PluginData(Elem.PluginData), CustomData(NULL) //CustomData is not copied
{
}
//---------------------------------------------------------------------------
//Swaps everything except children and parent
void TGraphElem::Swap(TGraphElem &O)
{
  std::swap(Visible, O.Visible);
  std::swap(ShowInLegend, O.ShowInLegend);
  std::swap(ShowInFunctionList, O.ShowInFunctionList);
  std::swap(LegendText, O.LegendText);
  std::swap(UpdateFinished, O.UpdateFinished);
  std::swap(CustomData, O.CustomData);
  PluginData.swap(O.PluginData);
  ClearCache();
  O.ClearCache();
}
//---------------------------------------------------------------------------
void TGraphElem::SetData(const TData *AData)
{
  Data = AData;
  std::for_each(ChildList.begin(), ChildList.end(), boost::bind(&TGraphElem::SetData, _1, Data));
}
//---------------------------------------------------------------------------
void TGraphElem::WriteToIni(TConfigFileSection &Section) const
{
  Section.Write(L"Visible", Visible, true);
  Section.Write(L"ShowInLegend", ShowInLegend, true);
  Section.Write(L"ShowInFunctionList", ShowInFunctionList, true);
  Section.Write(L"LegendText", LegendText, std::wstring());
  std::map<std::wstring,std::wstring>::const_iterator End = PluginData.end();
  for(std::map<std::wstring,std::wstring>::const_iterator Iter = PluginData.begin(); Iter != End; ++Iter)
    Section.Write(L"$" + Iter->first, Iter->second);
}
//---------------------------------------------------------------------------
void TGraphElem::ReadFromIni(const TConfigFileSection &Section)
{
	Visible = Section.Read(L"Visible", true);
	ShowInLegend = Section.Read(L"ShowInLegend", true);
	ShowInFunctionList = Section.Read(L"ShowInFunctionList", true);
	LegendText = Section.Read(L"LegendText", L"");
	TConfigFileSection::TIterator End = Section.End();
	for(TConfigFileSection::TIterator Iter = Section.Begin(); Iter != End; ++Iter)
		if(Iter->first[0] == L'$')
			PluginData[Iter->first.substr(1)] = Iter->second;
}
//---------------------------------------------------------------------------
void TGraphElem::InsertChild(const TGraphElemPtr &Elem, int Index)
{
  TGraphElemPtr Parent = Elem->GetParent();
  if(Parent)
  {
    int ParentIndex = Parent->GetChildIndex(Elem);
    Parent->RemoveChild(ParentIndex);
  }

  if(Index == -1)
    ChildList.push_back(Elem);
  else
    ChildList.insert(ChildList.begin() + Index, Elem);
  Elem->SetData(Data);
  Elem->SetParent(shared_from_this());
  SetModified();
}
//---------------------------------------------------------------------------
void TGraphElem::ReplaceChild(unsigned Index, const TGraphElemPtr &Elem)
{
  BOOST_ASSERT(Index < ChildList.size());
  if(Elem->GetParent())
    Elem->GetParent()->RemoveChild(Elem->GetParent()->GetChildIndex(Elem));
  ChildList[Index]->SetParent(boost::shared_ptr<TBaseFuncType>());
  ChildList[Index] = Elem;
  Elem->SetData(&GetData());
  Elem->SetParent(shared_from_this());
  SetModified();
}
//---------------------------------------------------------------------------
unsigned TGraphElem::GetChildIndex(const TGraphElemPtr &Elem) const
{
  return IndexOf(ChildList, Elem);
}
//---------------------------------------------------------------------------
void TGraphElem::RemoveChild(unsigned Index)
{
  BOOST_ASSERT(Index < ChildList.size());
  ChildList[Index]->SetParent(TGraphElemPtr());
  ChildList.erase(ChildList.begin() + Index);
  SetModified();
}
//---------------------------------------------------------------------------
void TGraphElem::ClearCache()
{
  for(unsigned I = 0; I < ChildList.size(); I++)
    ChildList[I]->ClearCache();
}
//---------------------------------------------------------------------------
void TGraphElem::Update()
{
  std::for_each(ChildList.begin(), ChildList.end(), boost::mem_fn(&TGraphElem::Update));
}
//---------------------------------------------------------------------------
void TGraphElem::SetModified()
{
  if(Data)
    Data->SetModified();
}
//---------------------------------------------------------------------------
///////////////////
// TBaseFuncType //
///////////////////
TBaseFuncType::TBaseFuncType() :
  Color(clRed), Size(1), Style(psSolid), DrawType(dtAuto),
  EndpointStyle(0, 0), From(-INF), To(INF)
{
}
//---------------------------------------------------------------------------
//NOTE: This does not copy tangents; Use CopyTangents()
TBaseFuncType::TBaseFuncType(const TBaseFuncType &F) : TGraphElem(F), DrawType(F.DrawType),
  Color(F.Color), Size(F.Size), Style(F.Style),
  From(F.From), To(F.To), Steps(F.Steps), EndpointStyle(F.EndpointStyle)
{
}
//---------------------------------------------------------------------------
void TBaseFuncType::Swap(TGraphElem &O)
{
  TBaseFuncType &Other = dynamic_cast<TBaseFuncType&>(O);
  TGraphElem::Swap(O);
  std::swap(Steps, Other.Steps);
  std::swap(Color, Other.Color);
  std::swap(Size, Other.Size);
  std::swap(Style, Other.Style);
  std::swap(DrawType, Other.DrawType);
  std::swap(EndpointStyle, Other.EndpointStyle);
  std::swap(From, Other.From);
  std::swap(To, Other.To);
  Points.swap(Other.Points);
  PointNum.swap(Other.PointNum);
  CoordList.swap(Other.CoordList);
}
//---------------------------------------------------------------------------
void TBaseFuncType::WriteToIni(TConfigFileSection &Section) const
{
  TGraphElem::WriteToIni(Section);

  Section.Write(L"From", From.Text, std::wstring());
  Section.Write(L"To", To.Text, std::wstring());
  Section.Write(L"Steps", Steps.Text, std::wstring());
  Section.Write(L"Style", Style, psSolid);
  Section.Write(L"Color", Color);
	Section.Write(L"Size", Size, 1U);
  Section.Write(L"StartPoint", EndpointStyle.first, 0U);
  Section.Write(L"EndPoint", EndpointStyle.second, 0U);
  Section.Write(L"DrawType", DrawType, dtAuto);
}
//---------------------------------------------------------------------------
void TBaseFuncType::ReadFromIni(const TConfigFileSection &Section)
{
  const TData &Data = GetData();
  GetFunc().SetTrigonometry(GetData().Axes.Trigonometry);
  Style = Section.Read(L"Style", psSolid);
  Color = Section.Read(L"Color", clRed);
  Size = Section.Read(L"Size", 1);
  if(Section.KeyExists(L">From"))
    From.Set(Section.Read(L">From", L"-INF"), GetData(), true); //Caused by bug in MS Outlook Express
  else
    From.Set(Section.Read(L"From", L"-INF"), Data, true);
  To.Set(Section.Read(L"To", L"+INF"), Data, true);
  Steps.Set(Section.Read(L"Steps", L""), Data, true);
  EndpointStyle.first = Section.Read(L"StartPoint", 0);
  EndpointStyle.second = Section.Read(L"EndPoint", 0);
  DrawType = Section.Read(L"DrawType", dtAuto);

  TGraphElem::ReadFromIni(Section);
}
//---------------------------------------------------------------------------
void TBaseFuncType::ClearCache()
{
  Points.clear();
  PointNum.clear();
	CoordList.clear();
  TGraphElem::ClearCache();
}
//---------------------------------------------------------------------------
void TBaseFuncType::Update()
{
  GetFunc().SetTrigonometry(GetData().Axes.Trigonometry);
  GetFunc().Update(GetData().CustomFunctions.SymbolList);
  From.Update(GetData());
  To.Update(GetData());
  Steps.Update(GetData());
  Steps.Value = floor(Steps.Value); //Make sure Steps is an integer
  if(Steps.Value < 0)
    Steps.Value = 0;
  TGraphElem::Update();
}
//---------------------------------------------------------------------------
void TBaseFuncType::GetCurrentRange(double &Min, double &Max, double &ds) const
{
  Min = From.Value;
  Max = To.Value;
  ds = Steps.Value == 1 ? 1 : (To.Value - From.Value) / (Steps.Value - 1); //If Steps, From and To are defined, ds must be exact
}
//---------------------------------------------------------------------------
Func32::TCoord<long double> TBaseFuncType::Eval(long double t) const
{
  if(!GetData().Axes.CalcComplex)
    return GetFunc().Calc(t);

	Func32::TCoord<Func32::TComplex> Result = GetFunc().Calc(Func32::TComplex(t));
  if(imag(Result.x) || imag(Result.y))
    throw Func32::ECalcError(Func32::ecComplexError);
  return Func32::TCoord<long double>(real(Result.x), real(Result.y));
}
//---------------------------------------------------------------------------
long double TBaseFuncType::CalcArea(long double From, long double To) const
{
  return GetFunc().CalcArea(From, To, 1E-3);
}
//---------------------------------------------------------------------------
long double TBaseFuncType::CalcArcLength(long double From, long double To) const
{
  if(To < From)
    std::swap(From, To);
  return GetFunc().CalcArc(From, To, 1E-3);
}
//---------------------------------------------------------------------------
bool TBaseFuncType::IsDependent(const std::wstring &SymbolName) const
{
  return GetFunc().IsDependent(SymbolName) || From.IsDependent(GetData(), SymbolName) ||
    To.IsDependent(GetData(), SymbolName) || Steps.IsDependent(GetData(), SymbolName);
}
//---------------------------------------------------------------------------
//////////////
// TStdFunc //
//////////////
TStdFunc::TStdFunc(const std::wstring &AText, const Func32::TSymbolList &SymbolList, Func32::TTrigonometry Trig)
  : Equation(AText), Func(AText, L"x", SymbolList, Trig)
{
}
//---------------------------------------------------------------------------
TStdFunc::TStdFunc(const Func32::TFunc &AFunc)
  : Equation(AFunc.MakeText(L"x", Options.RoundTo)), Func(AFunc)
{
}
//---------------------------------------------------------------------------
void TStdFunc::Swap(TGraphElem &O)
{
  TStdFunc &Other = dynamic_cast<TStdFunc&>(O);
  TBaseFuncType::Swap(O);
  std::swap(Equation, Other.Equation);
  Func.Swap(Other.Func);
  Dif.Swap(Other.Dif);
}
//---------------------------------------------------------------------------
void TStdFunc::SetEquation(const std::wstring &AText)
{
  Func.SetFunc(AText, L"x", GetData().CustomFunctions.SymbolList);
  Dif.Clear();
  Equation = AText;
  ClearCache();
  SetModified();
}
//---------------------------------------------------------------------------
void TStdFunc::WriteToIni(TConfigFileSection &Section) const
{
  Section.Write(L"FuncType", ftStdFunc);
  Section.Write(L"y", Equation);

	TBaseFuncType::WriteToIni(Section);
}
//---------------------------------------------------------------------------
void TStdFunc::ReadFromIni(const TConfigFileSection &Section)
{
  TBaseFuncType::ReadFromIni(Section);
  Equation = Section.Read(L"y", L"");
  Dif.Clear();
  try
  {
    Func.SetFunc(Equation, L"x", GetData().CustomFunctions.SymbolList);
  }
  catch(Func32::EParseError &E)
  {
    ShowStatusError(GetErrorMsg(E));
  }
}
//---------------------------------------------------------------------------
std::wstring TStdFunc::MakeText() const
{
  if(Equation.size() >= 100)
    return L"f(x)=" + Equation.substr(0, 100) + L"...";
  return L"f(x)=" + Equation;
}
//---------------------------------------------------------------------------
boost::shared_ptr<TBaseFuncType> TStdFunc::MakeDifFunc()
{
  boost::shared_ptr<TStdFunc> DifFunc(new TStdFunc);
  DifFunc->Func = GetDif();
  DifFunc->Equation = DifFunc->Func.MakeText();
  return DifFunc;
}
//---------------------------------------------------------------------------
void TStdFunc::GetCurrentRange(double &Min, double &Max, double &ds) const
{
  const TAxis &xAxis = GetData().Axes.xAxis;
  Min = std::max(xAxis.Min, From.Value);
  Max = std::min(xAxis.Max, To.Value);
  if(Steps.Value == 1)
    ds = 1;
  else if(From.IsFinite() && To.IsFinite() && Steps.Value > 0)
  {
    ds = (To.Value - From.Value) / (Steps.Value - 1); //If Steps, From and To are defined, ds must be exact
    if(DrawType != dtAuto && Min > From.Value)
      Min = From.Value + floor((Min - From.Value) / ds) * ds;
    if(DrawType != dtAuto && Max < To.Value)
      Max = To.Value - floor((To.Value - Max) / ds) * ds;
  }
  else if(Steps.Value > 0)
    ds = (Max - Min) / (Steps.Value - 1); //If Steps, From and To are defined, ds must be exact
  else
    ds = 0;
}
//---------------------------------------------------------------------------
const Func32::TFunc& TStdFunc::GetDif() const
{
  if(Dif.IsEmpty())
    Dif = Func.MakeDif();
  return Dif;
}
//---------------------------------------------------------------------------
//////////////
// TParFunc //
//////////////
TParFunc::TParFunc(const std::wstring &AxText, const std::wstring &AyText, const Func32::TSymbolList &SymbolList, Func32::TTrigonometry Trig)
  : Equation(AxText, AyText), Func(AxText, AyText, L"t", SymbolList, Trig)
{
  SetSteps(TTextValue(1000));
  From.Set(-10);
  To.Set(10);
}
//---------------------------------------------------------------------------
TParFunc::TParFunc(const Func32::TParamFunc &AFunc)
  : Equation(AFunc.MakeXText(), AFunc.MakeYText()), Func(AFunc)
{
}
//---------------------------------------------------------------------------
void TParFunc::SetEquation(const std::pair<std::wstring,std::wstring> &Value)
{
  Func.SetFunc(Value.first, Value.second, L"t", GetData().CustomFunctions.SymbolList);
  Equation = Value;
  ClearCache();
  SetModified();
}
//---------------------------------------------------------------------------
void TParFunc::Swap(TGraphElem &O)
{
  TParFunc &Other = dynamic_cast<TParFunc&>(O);
  TBaseFuncType::Swap(O);
  std::swap(Equation, Other.Equation);
  Func.Swap(Other.Func);
}
//---------------------------------------------------------------------------
void TParFunc::WriteToIni(TConfigFileSection &Section) const
{
  Section.Write(L"FuncType", ftParFunc);
  Section.Write(L"x", Equation.first);
  Section.Write(L"y", Equation.second);

  TBaseFuncType::WriteToIni(Section);
}
//---------------------------------------------------------------------------
void TParFunc::ReadFromIni(const TConfigFileSection &Section)
{
  try
  {
    Equation.first = Section.Read(L"x", L"");
    Equation.second = Section.Read(L"y", L"");
    Func.SetFunc(Equation.first, Equation.second, L"t", GetData().CustomFunctions.SymbolList);
  }
  catch(Func32::EParseError &E)
  {
    ShowStatusError(GetErrorMsg(E));
  }
  TBaseFuncType::ReadFromIni(Section);
}
//---------------------------------------------------------------------------
std::wstring TParFunc::MakeText() const
{
  //Ensure that the text is not too long to show in a dialog box.
  std::wstring Str = L"x(t)=";
  if(Equation.first.size() >= 100)
    Str += Equation.first.substr(0, 100) + L"... , y(t)=";
  else
    Str += Equation.first + L", y(t)=";
  if(Equation.second.size() >= 100)
    Str += Equation.second.substr(0, 100) + L"...";
  else
    Str += Equation.second;
  return Str;
}
//---------------------------------------------------------------------------
boost::shared_ptr<TBaseFuncType> TParFunc::MakeDifFunc()
{
  boost::shared_ptr<TParFunc> DifFunc(new TParFunc);
  DifFunc->Func = Func.MakeDif();
  DifFunc->Equation.first = DifFunc->Func.MakeXText();
  DifFunc->Equation.second = DifFunc->Func.MakeYText();
  return DifFunc;
}
//---------------------------------------------------------------------------
//////////////
// TPolFunc //
//////////////
TPolFunc::TPolFunc(const std::wstring &AText, const Func32::TSymbolList &SymbolList, Func32::TTrigonometry Trig)
  : Equation(AText), Func(AText, L"t", SymbolList, Trig)
{
  SetSteps(TTextValue(1000));
  From.Set(0);
  To = TTextValue(2*M_PI, L"2pi");
}
//---------------------------------------------------------------------------
void TPolFunc::SetEquation(const std::wstring &Value)
{
  Func.SetFunc(Value, L"t", GetData().CustomFunctions.SymbolList);
  Equation = Value;
  ClearCache();
  SetModified();
}
//---------------------------------------------------------------------------
void TPolFunc::Swap(TGraphElem &O)
{
  TPolFunc &Other = dynamic_cast<TPolFunc&>(O);
  TBaseFuncType::Swap(O);
  std::swap(Equation, Other.Equation);
  Func.Swap(Other.Func);
}
//---------------------------------------------------------------------------
void TPolFunc::WriteToIni(TConfigFileSection &Section) const
{
  Section.Write(L"FuncType", ftPolFunc);
  Section.Write(L"r", Equation);

  TBaseFuncType::WriteToIni(Section);
}
//---------------------------------------------------------------------------
void TPolFunc::ReadFromIni(const TConfigFileSection &Section)
{
  try
  {
    Equation = Section.Read(L"r", L"");
    Func.SetFunc(Equation, L"t", GetData().CustomFunctions.SymbolList);
  }
  catch(Func32::EParseError &E)
  {
    ShowStatusError(GetErrorMsg(E));
  }

  TBaseFuncType::ReadFromIni(Section);
}
//---------------------------------------------------------------------------
std::wstring TPolFunc::MakeText() const
{
  if(Equation.size() <= 100)
    return L"r(t)=" + Equation;
  return L"r(t)=" + Equation.substr(0, 100) + L"...";
}
//---------------------------------------------------------------------------
boost::shared_ptr<TBaseFuncType> TPolFunc::MakeDifFunc()
{
  boost::shared_ptr<TPolFunc> DifFunc(new TPolFunc);
  DifFunc->Func = Func.MakeDif();
  DifFunc->Equation = DifFunc->Func.MakeText();
  return DifFunc;
}
//---------------------------------------------------------------------------
//////////
// TTangent //
//////////
TTangent::TTangent()
  : TangentType(ttTangent), a(NAN), q(NAN)
{
  DrawType = dtLines;
}
//---------------------------------------------------------------------------
void TTangent::Swap(TGraphElem &O)
{
  TTangent &Other = dynamic_cast<TTangent&>(O);
  TBaseFuncType::Swap(O);
  std::swap(t, Other.t);
  std::swap(TangentType, Other.TangentType);
}
//---------------------------------------------------------------------------
void TTangent::WriteToIni(TConfigFileSection &Section) const
{
  Section.Write(L"t", t.Text);
  Section.Write(L"From", From, TTextValue(-INF));
  Section.Write(L"To", To, TTextValue(+INF));
  Section.Write(L"Style", Style, psSolid);
  Section.Write(L"Color", Color);
  Section.Write(L"Size", Size, 1U);
  Section.Write(L"TangentType", TangentType, ttTangent);

  TBaseFuncType::WriteToIni(Section);
}
//---------------------------------------------------------------------------
void TTangent::ReadFromIni(const TConfigFileSection &Section)
{
  t.Text = Section.Read(L"t", L"0");
  t.Value = GetData().Calc(t.Text);
  TangentType = Section.Read(L"TangentType", ttTangent);

  TBaseFuncType::ReadFromIni(Section);
  DrawType = dtLines; //Overwrite the default from TBaseFuncType::ReadFromIni()
}
//---------------------------------------------------------------------------
std::wstring TTangent::MakeText() const
{
  if(boost::shared_ptr<TBaseFuncType> Func = boost::dynamic_pointer_cast<TBaseFuncType>(GetParent()))
    return Func->GetVariable() + L"=" + t.Text;
  return L"";
}
//---------------------------------------------------------------------------
std::wstring TTangent::MakeLegendText() const
{
  if(!GetLegendText().empty())
    return GetLegendText();

  if(_isnan(a))
    return L"";
  if(!_finite(a))
    return L"x=" + RoundToString(q, GetData());
  return L"y=" + RoundToString(a, GetData()) + L"x" + (q < 0 ? L'-' : L'+') + RoundToString(std::abs(q), GetData());
}
//---------------------------------------------------------------------------
void TTangent::UpdateTan(double a1, double q1)
{
  a = a1;
  q = q1;
  if(_finite(a))
  {
    TanFunc.SetFunc(L"t", ToWString(a) + L"t+" + ToWString(q));
    Steps.Value = 0;
  }
  else
  {
    Steps.Value = 2;
    if(_finite(q))
      TanFunc.SetFunc(ToWString(q), L"t");
    else
      TanFunc.Clear();
  }
  Points.clear();
}
//---------------------------------------------------------------------------
bool TTangent::IsValid() const
{
  return !_isnan(a);
}
//---------------------------------------------------------------------------
void TTangent::GetCurrentRange(double &Min, double &Max, double &ds) const
{
  Min = _finite(a) ? std::max(GetData().Axes.xAxis.Min, From.Value) : GetData().Axes.yAxis.Min;
  Max = _finite(a) ? std::min(GetData().Axes.xAxis.Max, To.Value) : GetData().Axes.yAxis.Max;
  ds = boost::math::isfinite(a) ? 0 : 2;
}
//---------------------------------------------------------------------------
void TTangent::Update()
{
  TBaseFuncType::Update();
  t.Update(GetData());
  CalcTan(); //Update a and q
}
//---------------------------------------------------------------------------
void TTangent::ParentChanged()
{
  Update();
}
//---------------------------------------------------------------------------
long double TTangent::CalcArea(long double From, long double To) const
{
  if(_isnanl(a))
		return NAN; //The tangent is not valid, i.e. it is touching a undefined function
	if(_finitel(a))
		return a*To*To/2 + q*To - a*From*From/2 - q*From;
  return 0;    //The tangent is vertical
}
//---------------------------------------------------------------------------
//Calculate data used to draw a tangent
//(x,y) is the point there the tangent crosses the function and a is the slope
bool TTangent::CalcTan()
{
  boost::shared_ptr<TBaseFuncType> F = boost::dynamic_pointer_cast<TBaseFuncType>(GetParent());
  if(!F)
    return false;

  try
  {
    const Func32::TBaseFunc &Func = F->GetFunc();

    double x = Func.CalcX(t.Value); //Find x(t)
    double y = Func.CalcY(t.Value); //Find y(t)

    double a = Func.CalcSlope(t.Value);

    if(TangentType == ttNormal)
    {
      if(_finite(a))
        a = std::abs(a) < MIN_ZERO ? INF : -1/a;
      else
        a = 0;
    }
    double q = _finite(a) ? y - a*x : x;
    UpdateTan(a, q);
    return true;
  }
  catch(Func32::EFuncError&)
  {
    UpdateTan(NAN, NAN);
    return false;
  }
}
//---------------------------------------------------------------------------
bool TTangent::IsDependent(const std::wstring &SymbolName) const
{
  return t.IsDependent(GetData(), SymbolName) || TBaseFuncType::IsDependent(SymbolName);
}
//---------------------------------------------------------------------------
//////////////
// TShading //
//////////////
TShading::TShading()
  : ShadeStyle(ssXAxis), BrushStyle(bsFDiagonal), Color(clGreen),
  ExtendMinToIntercept(false), ExtendMaxToIntercept(false), ExtendMin2ToIntercept(false),
  ExtendMax2ToIntercept(false), MarkBorder(true),
  sMin(-INF), sMax(INF), sMin2(-INF), sMax2(INF)
{
  SetLegendText(L"Shading");
}
//---------------------------------------------------------------------------
TShading::TShading(const TShading &O)
  : TGraphElem(O), ShadeStyle(O.ShadeStyle), BrushStyle(O.BrushStyle), Color(O.Color),
  ExtendMinToIntercept(O.ExtendMinToIntercept), ExtendMaxToIntercept(O.ExtendMaxToIntercept),
  ExtendMin2ToIntercept(O.ExtendMin2ToIntercept), ExtendMax2ToIntercept(O.ExtendMax2ToIntercept),
  MarkBorder(O.MarkBorder), sMin(O.sMin), sMax(O.sMax), sMin2(O.sMin2), sMax2(O.sMax2)
{
}
//---------------------------------------------------------------------------
void TShading::Swap(TGraphElem &O)
{
  TShading &Other = dynamic_cast<TShading&>(O);
  TGraphElem::Swap(O);
  std::swap(ShadeStyle, Other.ShadeStyle);
  std::swap(BrushStyle, Other.BrushStyle);
  std::swap(Color, Other.Color);
  std::swap(MarkBorder, Other.MarkBorder);
  Func2.swap(Other.Func2);
  std::swap(sMin, Other.sMin);
  std::swap(sMax, Other.sMax);
  std::swap(sMin2, Other.sMin2);
  std::swap(sMax2, Other.sMax2);
  std::swap(ExtendMinToIntercept, Other.ExtendMinToIntercept);
  std::swap(ExtendMaxToIntercept, Other.ExtendMaxToIntercept);
  std::swap(ExtendMin2ToIntercept, Other.ExtendMin2ToIntercept);
  std::swap(ExtendMax2ToIntercept, Other.ExtendMax2ToIntercept);
}
//---------------------------------------------------------------------------
void TShading::WriteToIni(TConfigFileSection &Section) const
{
  TGraphElem::WriteToIni(Section);

  Section.Write(L"ShadeStyle", ShadeStyle);
  Section.Write(L"BrushStyle", BrushStyle);
  Section.Write(L"Color", Color);

  unsigned FuncCount = 0;
  unsigned FuncNo = 0;
	unsigned Func2No = 0;
	unsigned ElemCount = GetData().ElemCount();
	for(unsigned I = 0; I < ElemCount; I++)
	{
		TGraphElemPtr Elem = GetData().GetElem(I);
		if(dynamic_cast<TBaseFuncType*>(Elem.get()))
			FuncCount++;
		if(Elem == GetParent())
			FuncNo = FuncCount;
    if(Elem == Func2)
      Func2No = FuncCount;
  }

  //Warning: After copy-paste we may not have a Func2
  BOOST_ASSERT(!Func2 || Func2No); //If Func2 is defined we must have a number for it

  Section.Write(L"FuncNo", FuncNo);
  Section.Write(L"Func2No", Func2No, 0U);
  Section.Write(L"sMin", sMin, TTextValue(-INF));
  Section.Write(L"sMax", sMax, TTextValue(+INF));
  Section.Write(L"sMin2", sMin2, TTextValue(-INF));
  Section.Write(L"sMax2", sMax2, TTextValue(+INF));
  Section.Write(L"ExtendMinToIntercept", ExtendMinToIntercept, false);
  Section.Write(L"ExtendMaxToIntercept", ExtendMaxToIntercept, false);
  Section.Write(L"ExtendMin2ToIntercept", ExtendMin2ToIntercept, false);
  Section.Write(L"ExtendMax2ToIntercept", ExtendMax2ToIntercept, false);
  Section.Write(L"MarkBorder", MarkBorder, true);
}
//---------------------------------------------------------------------------
void TShading::ReadFromIni(const TConfigFileSection &Section)
{
  TGraphElem::ReadFromIni(Section);

  //For backward compatibility
  if(GetLegendText().empty())
    SetLegendText(LoadString(RES_SHADE));

  ShadeStyle = Section.Read(L"ShadeStyle", ssXAxis);
  BrushStyle = Section.Read(L"BrushStyle", bsBDiagonal);
  Color = Section.Read(L"Color", clGreen);

  int Func2No = Section.Read(L"Func2No", 0) - 1;
  Func2.reset();
  if(Func2No != -1)
    Func2 = GetData().GetFuncFromIndex(Func2No);

  sMin.Set(Section.Read(L"sMin", L"-INF"), GetData(), true);
  sMax.Set(Section.Read(L"sMax", L"+INF"), GetData(), true);
  sMin2.Set(Section.Read(L"sMin2", L"-INF"), GetData(), true);
  sMax2.Set(Section.Read(L"sMax2", L"+INF"), GetData(), true);
  ExtendMinToIntercept = Section.Read(L"ExtendMinToIntercept", false);
  ExtendMaxToIntercept = Section.Read(L"ExtendMaxToIntercept", false);
  ExtendMin2ToIntercept = Section.Read(L"ExtendMin2ToIntercept", false);
  ExtendMax2ToIntercept = Section.Read(L"ExtendMax2ToIntercept", false);
  MarkBorder = Section.Read(L"MarkBorder", true);

  //For backweards compatibility
  if(_isnan(sMin.Value)) sMin.Set(L"-INF", GetData());
  if(_isnan(sMax.Value)) sMax.Set(L"+INF", GetData());
  if(_isnan(sMin2.Value)) sMin2.Set(L"-INF", GetData());
  if(_isnan(sMax2.Value)) sMax2.Set(L"+INF", GetData());
}
//---------------------------------------------------------------------------
std::wstring TShading::MakeText() const
{
  return GetLegendText();
}
//---------------------------------------------------------------------------
void TShading::Update()
{
  sMin.Update(GetData());
  sMax.Update(GetData());
  sMin2.Update(GetData());
  sMax2.Update(GetData());
}
//---------------------------------------------------------------------------
void TShading::ClearCache()
{
  Region.reset();
}
//---------------------------------------------------------------------------
bool TShading::IsDependent(const std::wstring &SymbolName) const
{
  return sMin.IsDependent(GetData(), SymbolName) || sMax.IsDependent(GetData(), SymbolName) ||
    sMin2.IsDependent(GetData(), SymbolName) || sMax2.IsDependent(GetData(), SymbolName);
}
//---------------------------------------------------------------------------
//////////////////
// TPointSeries //
//////////////////
TPointSeries::TPointSeries(TColor AFrameColor, TColor AFillColor, TColor ALineColor, unsigned ASize, unsigned ALineSize,
  unsigned AStyle, TPenStyle ALineStyle, TInterpolationAlgorithm AInterpolation, bool AShowLabels,
  TFont *AFont, TLabelPosition ALabelPosition, TPointType APointType,
  TErrorBarType XErrorBarType, double XErrorValue, TErrorBarType YErrorBarType, double YErrorValue)
  : FrameColor(AFrameColor), FillColor(AFillColor), LineColor(ALineColor), Size(ASize), LineSize(ALineSize),
    Style(AStyle), LineStyle(ALineStyle), Interpolation(AInterpolation), ShowLabels(AShowLabels),
    Font(AFont), LabelPosition(ALabelPosition), PointType(APointType),
    xErrorBarType(XErrorBarType), xErrorValue(XErrorValue), yErrorBarType(YErrorBarType), yErrorValue(YErrorValue)
{
  SetLegendText(L"Point series");
}
//---------------------------------------------------------------------------
void TPointSeries::Swap(TGraphElem &O)
{
  TPointSeries &Other = dynamic_cast<TPointSeries&>(O);
  TGraphElem::Swap(O);
  std::swap(FrameColor, Other.FrameColor);
  std::swap(FillColor, Other.FillColor);
  std::swap(LineColor, Other.LineColor);
  std::swap(Size, Other.Size);
  std::swap(Style, Other.Style);
  std::swap(LineSize, Other.LineSize);
  std::swap(LineStyle, Other.LineStyle);
  std::swap(Interpolation, Other.Interpolation);
  std::swap(ShowLabels, Other.ShowLabels);
  std::swap(LabelPosition, Other.LabelPosition);
  Font.Swap(Other.Font);
  PointData.swap(Other.PointData);
  PointList.swap(Other.PointList);
  std::swap(xErrorBarType, Other.xErrorBarType);
  std::swap(yErrorBarType, Other.yErrorBarType);
  std::swap(xErrorValue, Other.xErrorValue);
  std::swap(yErrorValue, Other.yErrorValue);
  std::swap(PointType, Other.PointType);
}
//---------------------------------------------------------------------------
void TPointSeries::InsertDblPoint(const Func32::TDblPoint &Point, int Index)
{
  TPointSeriesPoint P;
  if(PointType == ptPolar)
  {
    std::pair<long double,long double> Polar = GetPolarCoord(Point, GetData().Axes.Trigonometry);
    P.First = RoundToString(Polar.second, 6);
    P.Second = RoundToString(Polar.first, 6);
  }
  else
  {
    P.First = RoundToString(Point.x, 6);
    P.Second = RoundToString(Point.y, 6);
  }

  if(Index == -1)
  {
    PointData.push_back(P);
    PointList.push_back(Point);
  }
  else
  {
    PointData.insert(PointData.begin() + Index, P);
    PointList.insert(PointList.begin() + Index, Point);
  }
  SetModified();
}
//---------------------------------------------------------------------------
void TPointSeries::InsertPoint(const TPointSeriesPoint &Point, int Index, bool AutoUpdate)
{
  if(Index >= static_cast<int>(PointData.size()) || Index < -1)
    throw std::out_of_range("Index out of range.");
  PointData.insert(Index == -1 ? PointData.end() : PointData.begin() + Index, Point);
  if(AutoUpdate)
    PointList.insert(Index == -1 ? PointList.end() : PointList.begin() + Index, ConvertPoint(Point));
  SetModified();
}
//---------------------------------------------------------------------------
void TPointSeries::ReplaceDblPoint(const Func32::TDblPoint &Point, unsigned Index)
{
  TPointSeriesPoint P;
  if(PointType == ptPolar)
  {
    std::pair<long double,long double> Polar = GetPolarCoord(Point, GetData().Axes.Trigonometry);
    P.First = RoundToString(Polar.second, 2);
    P.Second = RoundToString(Polar.first, 2);
  }
  else
  {
    P.First = RoundToString(Point.x, 3);
    P.Second = RoundToString(Point.y, 3);
  }
  PointData.at(Index) = P;
  PointList.at(Index) = Point;
  SetModified();
}
//---------------------------------------------------------------------------
void TPointSeries::ReplacePoint(const TPointSeriesPoint &Point, unsigned Index)
{
  PointData.at(Index) = Point;
  PointList.at(Index) = ConvertPoint(Point);
  SetModified();
}
//---------------------------------------------------------------------------
void TPointSeries::DeletePoint(unsigned Index, unsigned Count)
{
  if(Index >= PointData.size())
    throw std::out_of_range("Index out of range.");
  PointData.erase(PointData.begin() + Index, PointData.begin() + Index + Count);
  PointList.erase(PointList.begin() + Index, PointList.begin() + Index + Count);
  SetModified();
}
//---------------------------------------------------------------------------
void TPointSeries::WriteToIni(TConfigFileSection &Section) const
{
  Section.Write(L"FrameColor", FrameColor, clBlack);
  Section.Write(L"FillColor", FillColor);
  Section.Write(L"LineColor", LineColor);
  Section.Write(L"Size", Size);
  Section.Write(L"Style", Style);
  Section.Write(L"LineSize", LineSize, 1U);
  Section.Write(L"LineStyle", LineStyle, psClear);
  Section.Write(L"Interpolation", Interpolation, iaLinear);
  Section.Write(L"ShowLabels", ShowLabels, false);
  Section.Write(L"Font", FontToStr(Font), DEFAULT_POINT_FONT);
  Section.Write(L"LabelPosition", LabelPosition);
  Section.Write(L"PointType", PointType, ptCartesian);
  Section.Write(L"PointCount", PointList.size());

  std::wostringstream Str;
  for(unsigned N = 0; N < PointList.size(); N++)
  {
    if(PointData[N].First.find(',') == std::string::npos)
      Str << PointData[N].First << ',';
    else
      Str << '"' << PointData[N].First << "\",";

    if(PointData[N].Second.find(',') == std::string::npos)
      Str << PointData[N].Second << ';';
    else
      Str << '"' << PointData[N].Second << "\";";
  }
  Section.Write(L"Points", Str.str());

  Section.Write(L"xErrorBarType", xErrorBarType, ebtNone);
  Section.Write(L"xErrorBarValue", xErrorValue, 0.0);
  if(xErrorBarType == ebtCustom)
  {
    Str.str(L"");
    for(unsigned N = 0; N < PointList.size(); N++)
      Str << PointData[N].xError << ';';
    Section.Write(L"xErrorBarData", Str.str(), std::wstring());
  }

  Section.Write(L"yErrorBarType", yErrorBarType, ebtNone);
  Section.Write(L"yErrorBarValue", yErrorValue, 0.0);
  if(yErrorBarType == ebtCustom)
  {
    Str.str(L"");
    for(unsigned N = 0; N < PointList.size(); N++)
        Str << PointData[N].yError << ';';
    Section.Write(L"yErrorBarData", Str.str(), std::wstring());
  }

  TGraphElem::WriteToIni(Section);
}
//---------------------------------------------------------------------------
void TPointSeries::ReadFromIni(const TConfigFileSection &Section)
{
  TGraphElem::ReadFromIni(Section);
  //For backward compatibility
  if(GetLegendText().empty())
    SetLegendText(Section.Read(L"Text", Section.GetName()));

  FrameColor = Section.Read(L"FrameColor", clBlack);
  FillColor = Section.Read(L"FillColor", clRed);
  LineColor = Section.Read(L"LineColor", clBlue);
  Size = Section.Read(L"Size", 5);
  Style = Section.Read(L"Style", 0);
  LineSize = Section.Read(L"LineSize", 1);
  LineStyle = Section.Read(L"LineStyle", psClear);
  if(Section.KeyExists(L"SmoothLine"))
    Interpolation = Section.Read(L"SmoothLine", iaLinear);
  else
    Interpolation = Section.Read(L"Interpolation", iaLinear);
  ShowLabels = Section.Read(L"ShowLabels", false);
  StrToFont(Section.Read(L"Font", DEFAULT_POINT_FONT), Font);
  LabelPosition = Section.Read(L"LabelPosition", lpBelow);
  PointType = Section.Read(L"PointType", ptCartesian);

  TTokenizer Tokens(Section.Read(L"Points", L""), L';');
  std::wistringstream xStream(Section.Read(L"xErrorBarData", L""));
  std::wistringstream yStream(Section.Read(L"yErrorBarData", L""));
  TTokenizer Token(L"", L',', L'"');
  unsigned PointCount = Section.Read(L"PointCount", 0U);
  PointData.reserve(PointCount);

  while(Token = Tokens.Next(), Tokens)
  {
    std::wstring x, y, xError, yError;
    Token >> x >> y;

    getline(xStream, xError, L';');
    getline(yStream, yError, L';');
    TTextValue XError, YError;

    try
    {
      if(!xError.empty())
        XError.Set(xError, GetData());
      if(!yError.empty())
        YError.Set(yError, GetData());
    }
    catch(Func32::EParseError &E)
    {
      ShowStatusError(GetErrorMsg(E));
      XError.Set(xError, GetData(), true);
      YError.Set(yError, GetData(), true);
    }
    PointData.push_back(TPointSeriesPoint(x, y, XError, YError));
  }

  xErrorBarType = Section.Read(L"xErrorBarType", ebtNone);
  xErrorValue = Section.Read(L"xErrorBarValue", 0.0);

  yErrorBarType = Section.Read(L"yErrorBarType", ebtNone);
  yErrorValue = Section.Read(L"yErrorBarValue", 0.0);
}
//---------------------------------------------------------------------------
double TPointSeries::GetXError(unsigned Index) const
{
  switch(xErrorBarType)
  {
    case ebtNone:
      return 0;

    case ebtFixed:
      return xErrorValue;

    case ebtRelative:
      return xErrorValue * PointList[Index].x / 100;

    case ebtCustom:
      return PointData[Index].xError.Value;
  }
  return 0;
}
//---------------------------------------------------------------------------
double TPointSeries::GetYError(unsigned Index) const
{
  switch(yErrorBarType)
  {
    case ebtNone:
      return 0;

    case ebtFixed:
      return yErrorValue;

    case ebtRelative:
      return yErrorValue * PointList[Index].y / 100;

    case ebtCustom:
      return PointData[Index].yError.Value;
  }
  return 0;
}
//---------------------------------------------------------------------------
Func32::TDblPoint FindCoord(TPointSeries::TPointList::const_iterator Iter, double x)
{
//  if(Interpolation == iaLinear)
  {
    double a = ((Iter+1)->y - Iter->y) / ((Iter+1)->x - Iter->x);
    double b = Iter->y - a * Iter->x;
    return Func32::TDblPoint(x, a*x+b);
  }

  throw EAbort("");
}
//---------------------------------------------------------------------------
TPointSeries::TPointList::const_iterator TPointSeries::FindPoint(double x) const
{
  for(unsigned I = 0; I < PointList.size() - 1; I++)
    if((PointList[I].x <= x && PointList[I+1].x >= x) ||
       (PointList[I].x >= x && PointList[I+1].x <= x))
      return PointList.begin() + I;

  throw EAbort("");
}
//---------------------------------------------------------------------------
Func32::TDblPoint TPointSeries::ConvertPoint(const TPointSeriesPoint &P) const
{
  Func32::TDblPoint Point;
  long double a = FastCalc(P.First, GetData(), true); //x or theta
  long double b = FastCalc(P.Second, GetData(), true); //y or r
  if(PointType == ptPolar)
  {
    Func32::TTrigonometry Trig = GetData().Axes.Trigonometry;
    Point.x = b * std::cos(Trig == Func32::Degree ? a*M_PI/180 : a);
    Point.y = b * std::sin(Trig == Func32::Degree ? a*M_PI/180 : a);
  }
  else
    Point.x = a, Point.y = b;
  return Point;
}
//---------------------------------------------------------------------------
void TPointSeries::Update()
{
  PointList.resize(PointData.size());
  for(unsigned I = 0; I < PointList.size(); I++)
  {
    PointList[I] = ConvertPoint(PointData[I]);
    PointData[I].xError.Update(GetData());
    PointData[I].yError.Update(GetData());
  }
}
//---------------------------------------------------------------------------
bool TPointSeries::IsDependent(const std::wstring &SymbolName) const
{
  for(unsigned I = 0; I < PointList.size(); I++)
  {
    if(GetData().IsDependent(PointData[I].First, SymbolName) ||
      GetData().IsDependent(PointData[I].Second, SymbolName) ||
      PointData[I].xError.IsDependent(GetData(), SymbolName) ||
      PointData[I].yError.IsDependent(GetData(), SymbolName))
        return true;
  }
  return false;
}
//---------------------------------------------------------------------------
////////////////
// TTextLabel //
////////////////
TTextLabel::TTextLabel()
  : LabelPlacement(lpUserTopLeft), Rect(0,0,0,0), Rotation(0), Pos(TTextValue(0), TTextValue(0)),
    BackgroundColor(clNone), ContainsOleLink(false)
{
  SetShowInLegend(false);
}
//---------------------------------------------------------------------------
TTextLabel::TTextLabel(const std::string &Str, TLabelPlacement Placement, const TTextValue &AxPos, const TTextValue &AyPos, TColor Color, double ARotation, bool OleLink)
  : Text(Str), LabelPlacement(Placement), Pos(AxPos, AyPos),
    BackgroundColor(Color), Rotation(ARotation), ContainsOleLink(OleLink)
{
  SetShowInLegend(false);
  StatusText = RtfToPlainText(Str);
  //Update() must be called after Label is added to Data
}
//---------------------------------------------------------------------------
void TTextLabel::Swap(TGraphElem &O)
{
  TTextLabel &Other = dynamic_cast<TTextLabel&>(O);
  TGraphElem::Swap(O);
  std::swap(Text, Other.Text);
  std::swap(LabelPlacement, Other.LabelPlacement);
  std::swap(Pos, Other.Pos);
  std::swap(Rect, Other.Rect);
  std::swap(BackgroundColor, Other.BackgroundColor);
  std::swap(Rotation, Other.Rotation);
  std::swap(StatusText, Other.StatusText);
  std::swap(ContainsOleLink, Other.ContainsOleLink);
  Metafile.Swap(Other.Metafile);
}
//---------------------------------------------------------------------------
void TTextLabel::WriteToIni(TConfigFileSection &Section) const
{
  Section.Write(L"Placement", LabelPlacement);
  if(LabelPlacement == lpUserTopLeft || LabelPlacement >= lpUserTopRight)
    Section.Write(L"Pos", Pos.first.Text + L";" + Pos.second.Text);
  Section.Write(L"Rotation", Rotation, 0.0);
  Section.Write(L"Text", ToWString(EncodeEscapeSequence(Text)));
  Section.Write(L"BackgroundColor", BackgroundColor);
  Section.Write(L"OleLink", ContainsOleLink, false);

  TGraphElem::WriteToIni(Section);
}
//---------------------------------------------------------------------------
void TTextLabel::ReadFromIni(const TConfigFileSection &Section)
{
  LabelPlacement = Section.Read(L"Placement", lpUserTopLeft);
  std::wstring Temp = Section.Read(L"Pos", L"0;0");
  size_t n = Temp.find(L";");
  if(n == std::string::npos)
  {
    //For backwards compatibility
    Func32::TDblPoint P = Section.Read(L"Pos", Func32::TDblPoint(0,0));
    Pos.first = TTextValue(P.x);
    Pos.second = TTextValue(P.y);
  }
  else
  {
    Pos.first.Set(Temp.substr(0, n), GetData(), true);
    Pos.second.Set(Temp.substr(n+1), GetData(), true);
  }

  Rotation = Section.Read(L"Rotation", 0.0);
  Text = DecodeEscapeSequence(ToString(Section.Read(L"Text", L"ERROR")));
  BackgroundColor = Section.Read(L"BackgroundColor", clNone);
  ContainsOleLink = Section.Read(L"OleLink", false);
  if(ContainsOleLink)
    Text = ToString(UpdateRichTextOleObjects(ToUString(Text)));

  StatusText = RtfToPlainText(Text);

  TGraphElem::ReadFromIni(Section);
  SetShowInLegend(false); //Overwrite data; Label is never shown in legend
}
//---------------------------------------------------------------------------
std::wstring TTextLabel::MakeText() const
{
  return StatusText.empty() ? LoadString(RES_LABEL) : StatusText;
}
//---------------------------------------------------------------------------
void TTextLabel::Scale(double xSizeMul, double ySizeMul)
{
  Rect.Right = Rect.Left + Rect.Width() * xSizeMul;
  Rect.Bottom = Rect.Top + Rect.Height() * ySizeMul;
  Metafile->MMWidth = Metafile->MMWidth * xSizeMul;
  Metafile->MMHeight = Metafile->MMHeight * ySizeMul;
}
//---------------------------------------------------------------------------
void TTextLabel::Update()
{
  String Str = ReplaceExpression(ToUString(Text), GetData());
  TPoint Size = RichTextSize(Str);
  Metafile->Width = Size.x;
  Metafile->Height = Size.y;
  std::auto_ptr<TMetafileCanvas> Canvas(new TMetafileCanvas(Metafile, 0));

  RenderRichText(Str, Canvas.get(), TPoint(0, 0), Size.x, BackgroundColor);
  Canvas.reset();
  Rect = Rotate(Metafile, Rotation);

  Pos.first.Update(GetData());
  Pos.second.Update(GetData());
}
//---------------------------------------------------------------------------
void TTextLabel::SetText(const std::string &Str)
{
  Text = Str;
  StatusText = RtfToPlainText(Str);
  if(DataValid())
    Update();
  SetModified();
}
//---------------------------------------------------------------------------
void TTextLabel::SetBackgroundColor(TColor Color)
{
  BackgroundColor = Color;
  if(DataValid())
    Update();
  SetModified();
}
//---------------------------------------------------------------------------
bool TTextLabel::IsDependent(const std::wstring &SymbolName) const
{
  size_t Index = std::wstring::npos;
  while((Index = Text.rfind("%(", Index-1)) != std::wstring::npos)
    try
		{
      size_t Index2 = FindEndPar(Text, Index);
      if(Index2 == std::string::npos)
        break;
      unsigned Length = Index2 - Index + 1;
      if(Func32::TFunc(ToWString(Text.substr(Index+2, Index2-Index-2)), L"",
        GetData().CustomFunctions.SymbolList).IsDependent(SymbolName))
        return true;
    }
    catch(Func32::EFuncError &Error)
    {
    }
  return Pos.first.IsDependent(GetData(), SymbolName) || Pos.second.IsDependent(GetData(), SymbolName);
}
//---------------------------------------------------------------------------
///////////////
// TRelation //
///////////////
TRelation::TRelation()
  : Color(clGreen), BrushStyle(bsBDiagonal), Size(1), LineStyle(psSolid), Alpha(100)
{
}
//---------------------------------------------------------------------------
TRelation::TRelation(const std::wstring &AText, const std::wstring &AConstraints, const Func32::TSymbolList &SymbolList, Func32::TTrigonometry Trig)
  : Text(AText), Color(clGreen), BrushStyle(bsFDiagonal), Size(1), LineStyle(psSolid),
    Alpha(100)
{
  std::vector<std::wstring> Args;
  Args.push_back(L"x");
  Args.push_back(L"y");
  Func.SetTrigonometry(Trig);
  Constraints.SetTrigonometry(Trig);
  Func.SetFunc(Text, Args, SymbolList);
  if(Func.GetFunctionType() != Func32::ftInequality && Func.GetFunctionType() != Func32::ftEquation)
    throw EGraphError(LoadString(RES_INVALID_RELATION));

  RelationType = Func.GetFunctionType() == Func32::ftInequality ? rtInequality : rtEquation;
  if(RelationType == rtEquation && Size == 0)
    Size = 1;

  if(Func.GetFunctionType() == Func32::ftEquation)
    Func.RemoveRelation();

  if(!AConstraints.empty())
  {
    Constraints.SetFunc(AConstraints, Args, SymbolList);
    ConstraintsText = AConstraints;
  }
}
//---------------------------------------------------------------------------
TRelation::TRelation(const TRelation &Relation)
  : TGraphElem(Relation), Text(Relation.Text), ConstraintsText(Relation.ConstraintsText),
    Func(Relation.Func), Constraints(Relation.Constraints), RelationType(Relation.RelationType),
    Color(Relation.Color), BrushStyle(Relation.BrushStyle), Size(Relation.Size),
    LineStyle(Relation.LineStyle), Alpha(Relation.Alpha)
{
}
//---------------------------------------------------------------------------
void TRelation::Swap(TGraphElem &O)
{
  TRelation &Other = dynamic_cast<TRelation&>(O);
  TGraphElem::Swap(O);
  std::swap(Text, Other.Text);
  std::swap(ConstraintsText, Other.ConstraintsText);
  Func.Swap(Other.Func);
  Constraints.Swap(Other.Constraints);
  std::swap(Color, Other.Color);
  std::swap(Size, Other.Size);
  std::swap(BrushStyle, Other.BrushStyle);
  std::swap(RelationType, Other.RelationType);
  std::swap(LineStyle, Other.LineStyle);
  std::swap(Alpha, Other.Alpha);
}
//---------------------------------------------------------------------------
void TRelation::WriteToIni(TConfigFileSection &Section) const
{
  Section.Write(L"Relation", Text);
  Section.Write(L"Constraints", ConstraintsText, std::wstring());
  Section.Write(L"Style", BrushStyle);
  Section.Write(L"LineStyle", LineStyle);
  Section.Write(L"Color", Color);
  Section.Write(L"Alpha", Alpha);
  Section.Write(L"Size", Size, 1U);

  TGraphElem::WriteToIni(Section);
}
//---------------------------------------------------------------------------
void TRelation::ReadFromIni(const TConfigFileSection &Section)
{
  Func.SetTrigonometry(GetData().Axes.Trigonometry);
  Constraints.SetTrigonometry(GetData().Axes.Trigonometry);
  Text = Section.Read(L"Relation", L"");
  ConstraintsText = Section.Read(L"Constraints", L"");
  BrushStyle = Section.Read(L"Style", bsBDiagonal);
  LineStyle = Section.Read(L"LineStyle", psSolid);
  Color = Section.Read(L"Color", clGreen);
  Alpha = Section.Read(L"Alpha", 100);
  Size = Section.Read(L"Size", 1);
  std::vector<std::wstring> Args;
  Args.push_back(L"x");
  Args.push_back(L"y");
  try
  {
    Func.SetFunc(Text, Args, GetData().CustomFunctions.SymbolList );
    if(!ConstraintsText.empty())
      Constraints.SetFunc(ConstraintsText, Args, GetData().CustomFunctions.SymbolList);
  }
  catch(Func32::EParseError &E)
  {
    ShowStatusError(GetErrorMsg(E));
  }
  RelationType = Func.GetFunctionType() == Func32::ftInequality ? rtInequality : rtEquation;
  if(Func.GetFunctionType() == Func32::ftEquation)
    Func.RemoveRelation();

  TGraphElem::ReadFromIni(Section);
}
//---------------------------------------------------------------------------
std::wstring TRelation::MakeText() const
{
  return ConstraintsText.empty() ? Text : Text + L"; " + ConstraintsText;
}
//---------------------------------------------------------------------------
void TRelation::SetConstraints(const std::wstring &AConstraintsText, const Func32::TSymbolList &SymbolList)
{
  std::vector<std::wstring> Args;
  Args.push_back(L"x");
  Args.push_back(L"y");
  Constraints.SetFunc(AConstraintsText, Args, SymbolList);
  ConstraintsText = AConstraintsText;
}
//---------------------------------------------------------------------------
bool TRelation::EvalConstraints(const std::vector<long double> &Args, Func32::ECalcError &E) const
{
  if(Constraints.IsEmpty())
    return true;

  long double Valid = Constraints.Calc(Args, E);
  return E.ErrorCode == Func32::ecNoError && Valid;
}
//---------------------------------------------------------------------------
long double TRelation::EvalRelation(const std::vector<long double> &Args, Func32::ECalcError &E) const
{
  long double Result = Func.Calc(Args, E);
  if(E.ErrorCode != Func32::ecNoError)
    return NAN;
  return Result;
}
//---------------------------------------------------------------------------
long double TRelation::Eval(const std::vector<long double> &Args, Func32::ECalcError &E) const
{
  if(!Constraints.IsEmpty())
  {
    long double Valid = Constraints.Calc(Args, E);
    if(E.ErrorCode != Func32::ecNoError || !Valid)
      return NAN;
  }

  long double Result = Func.Calc(Args, E);
  if(E.ErrorCode != Func32::ecNoError)
    return NAN;
  return Result;
}
//---------------------------------------------------------------------------
long double TRelation::Eval(long double x, long double y) const
{
  std::vector<long double> Args;
  Args.push_back(x);
  Args.push_back(y);
  if(!Constraints.IsEmpty())
  {
    long double Valid = Constraints.Calc(Args);
    if(!Valid)
      return NAN;
  }

  return Func.Calc(Args);
}
//---------------------------------------------------------------------------
void TRelation::ClearCache()
{
  PolylinePoints.clear();
  PolylineCounts.clear();
}
//---------------------------------------------------------------------------
void TRelation::Update()
{
  Func.SetTrigonometry(GetData().Axes.Trigonometry);
  Constraints.SetTrigonometry(GetData().Axes.Trigonometry);
  Func.Update(GetData().CustomFunctions.SymbolList);
  Constraints.Update(GetData().CustomFunctions.SymbolList);
}
//---------------------------------------------------------------------------
bool TRelation::IsDependent(const std::wstring &SymbolName) const
{
  return Func.IsDependent(SymbolName) || Constraints.IsDependent(SymbolName);
}
//---------------------------------------------------------------------------
void TRelation::SetPoints(std::vector<TPoint> &APolylinePoints, std::vector<int> &APolylineCounts)
{
  PolylinePoints.clear();
  PolylineCounts.clear();
  PolylinePoints.swap(APolylinePoints);
  PolylineCounts.swap(APolylineCounts);
}
//---------------------------------------------------------------------------
/** Analyse an equation to find all X/Y-coordinates given the other coordinate.
 *  \param Min: Start X/Y-coordinate to search.
 *  \param Max: End X/Y-coordinate to search.
 *  \param Steps: Number of steps between Min and Max to analyse for roots.
 *  \param FixedValue: The fixed part of the X/Y-coordinate set.
 *  \param RelationTrace: Tells weither we are searching for the X- or Y-coordinate. If rtXValue, Min and Max are X-coordinates and FixedValue is the Y-coordinate.
 *  \return Vector with all X/Y-coordinates corresponding to the FixedValue.
 */
std::vector<long double> TRelation::Analyse(long double Min, long double Max, unsigned Steps, long double FixedValue, TRelationTrace RelationTrace) const
{
  Func32::ECalcError E;
  std::vector<long double> Result;
  std::vector<long double> Args(2);
  Args[RelationTrace == rtXValue] = FixedValue;
  Args[RelationTrace != rtXValue] = Min;
  long double Values[3] = {NAN};
  Values[1] = EvalRelation(Args, E);
  long double ds = (Max - Min) / Steps;
  for(long double s = Min + ds; s < Max; s += ds)
  {
    Args[RelationTrace != rtXValue] = s;
    Values[2] = EvalRelation(Args, E);
    if(_finitel(Values[1]) && _finitel(Values[2]))
    {
      if((Values[1] < 0) != (Values[2] < 0))
      {
        Args[RelationTrace != rtXValue] = Trace(s-ds, s, FixedValue, RelationTrace);
        if(EvalConstraints(Args, E))
          Result.push_back(Args[RelationTrace != rtXValue]);
      }
      else if(_finitel(Values[0]) && (Values[0] < Values[1]) != (Values[1] < Values[2]))
        if((Values[1] + Values[1] - Values[0] < 0) != (Values[1] < 0) ||
           (Values[1] - (Values[2] - Values[1]) < 0) != (Values[1] < 0))
        {
          Func32::TCustomFunc Dif = Func.MakeDif(RelationTrace != rtXValue);
          Args[RelationTrace != rtXValue] = Trace(Dif, s-ds-ds, s, FixedValue, RelationTrace);
          if(EvalConstraints(Args, E))
            Result.push_back(Args[RelationTrace != rtXValue]);
        }
    }
    Values[0] = Values[1];
    Values[1] = Values[2];
  }
  return Result;
}
//---------------------------------------------------------------------------
long double TRelation::Trace(long double Min, long double Max, long double FixedValue, TRelationTrace RelationTrace) const
{
  return Trace(Func, Min, Max, FixedValue, RelationTrace);
}
//---------------------------------------------------------------------------
/** Find a root of a function given a Min and Max on each side of the root.
 *  This will use binary search to find the root.
 *  \param Func: The function to find the root for.
 *  \param Min: A X/Y-coordinate before the root.
 *  \param Max: A X/Y-coordinate after the root.
 *  \param FixedValue: The fixed X/Y-coordinate we are not searching for.
 *  \param RelationTrace: Tells weither we are searching for the X- or Y-coordinate. If rtXValue, Min and Max are X-coordinates and FixedValue is the Y-coordinate.
 *  \return The found X/Y-coordinate or NAN on errors.
 */
long double TRelation::Trace(const Func32::TCustomFunc &Func, long double Min, long double Max, long double FixedValue, TRelationTrace RelationTrace)
{
  const long double Tol = 1E-16;
  const unsigned MaxCount = 100; //Max number of iterations to avoid infinite loop;
  try
  {
    std::vector<long double> Args(2);
    Args[RelationTrace == rtXValue] = FixedValue;
    Args[RelationTrace != rtXValue] = Min;
    long double MinValue = Func.Calc(Args);
    Args[RelationTrace != rtXValue] = Max;
    long double MaxValue = Func.Calc(Args);

    if((MinValue < 0) == (MaxValue < 0))
      return NAN;
    unsigned Count;
    for(Count = 0; Count < MaxCount; Count++)
    {
      long double Middle = (Min + Max) / 2;
      Args[RelationTrace != rtXValue] = Middle;
      long double MiddleValue = Func.Calc(Args);
      if((MinValue < 0) == (MiddleValue < 0))
        Min = Middle, MinValue = MiddleValue;
      else
        Max = Middle, MaxValue = MiddleValue;

      if(std::abs(Max - Min) < Tol)
        break;
    }
    return (Max + Min) / 2;
  }
  catch(Func32::EFuncError &E)
  {
    return NAN;
  }
}
//---------------------------------------------------------------------------
///////////////
// TAxesView //
///////////////
void TAxesView::ReadFromIni(const TConfigFileSection &Section)
{
  SetShowInLegend(false);
}
//---------------------------------------------------------------------------
void TAxesView::WriteToIni(TConfigFileSection &Section) const
{
  GetData().Axes.WriteToIni(Section);
}
//---------------------------------------------------------------------------
std::wstring TAxesView::MakeText() const
{
  return LoadString(RES_AXES);
}
//---------------------------------------------------------------------------
int TAxesView::GetVisible() const
{
  switch(GetData().Axes.AxesStyle)
  {
    case asCrossed: return 1;
    case asBoxed:   return -1;
    default:        return 0;
  }
}
//---------------------------------------------------------------------------
void TAxesView::ChangeVisible()
{
  TData &Data = const_cast<TData&>(GetData());
  switch(Data.Axes.AxesStyle)
  {
    case asCrossed:
      Data.Axes.AxesStyle = asBoxed;
      Data.ClearCache();
      break;

    case asBoxed:
      Data.Axes.AxesStyle = ::asNone;
      Data.ClearCache();
      break;

    default:
      Data.Axes.AxesStyle = asCrossed;
      break;
  }
}
//---------------------------------------------------------------------------
void TAxesView::SetVisible(int Value)
{
  TData &Data = const_cast<TData&>(GetData());
  switch(Value)
  {
    case 1: Data.Axes.AxesStyle = asCrossed; break;
    case -1: Data.Axes.AxesStyle = asBoxed; break;
    default: Data.Axes.AxesStyle = asNone; break;
  }
  Data.ClearCache();
  SetModified();
}
//---------------------------------------------------------------------------
const TAxes& TAxesView::GetAxes() const
{
  return GetData().Axes;
}
//---------------------------------------------------------------------------
bool TAxesView::IsDependent(const std::wstring &SymbolName) const
{
  return false;
}
//---------------------------------------------------------------------------
} //namespace Graph
