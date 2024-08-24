/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#ifndef DataH
#define DataH
#include "VclObject.h"
#include "GraphElem.h"
#include "Settings.h"
#include "Convert.h"

class TConfigFile;
namespace Graph
{
typedef std::map<std::wstring, double> TDefaultsMap;
struct TUserModel
{
  std::wstring Model;
  TDefaultsMap Defaults;
};

typedef std::map<std::wstring, TUserModel> TUserModels;

typedef bool (__closure *TAbortUpdateEvent)();

class TData
{
  mutable bool Modified;
  mutable bool FNeedDrawing;
  mutable std::wstring GrfName;
  TAbortUpdateEvent OnAbortUpdate;
  TTopGraphElemPtr TopElem;

  void SaveData(TConfigFile &IniFile) const;
  const TData& operator=(const TData&);             //Not implemented
  static void WriteInfoToIni(TConfigFile &IniFile);
  void SaveImage(TConfigFile &IniFile, TCanvas *Canvas, int Width, int Height) const;
  static void WriteElem(TConfigFile &IniFile, const TGraphElemPtr &Elem, struct TElemCount &Count);
  static void WriteCount(TConfigFile &ConfigFile, const TElemCount &Count);

public:
  TAxes Axes;
  TUserModels UserModels;
  TCustomFunctions CustomFunctions;
  TAnimationInfo AnimationInfo;
  TStringMap PluginData;

  TData();
  TData(const TData &OldData);
  void ClearCache();
  void Clear();
  void LoadDefault();
  void LoadFromFile(const std::wstring &FileName);
  void Load(TConfigFile &IniFile);
  void LoadPluginData(const TConfigFileSection &Section);
  bool Save(const std::wstring &FileName, bool Remember) const;
  std::wstring SaveToString(bool ResetModified) const;
  std::wstring SaveToString(const TGraphElemPtr &Elem) const;
  void SaveDefault() const;
  static void CheckIniInfo(const TConfigFile &IniFile);
  void LoadData(const TConfigFile &IniFile);
  void PreprocessGrfFile(TConfigFile &IniFile);
  void Import(const std::wstring &FileName);
  void Import(TConfigFile &ConfigFile);
  void ImportPointSeries(std::istream &Stream, char Separator);
  bool ImportPointSeries(const std::wstring &FileName, char Separator);
  std::wstring CreatePointSeriesDescription() const;
  boost::shared_ptr<TTextLabel> FindLabel(int X, int Y) const; //NULL indicates not label found
  void DeleteLabel(int Index);
  void ImportUserModels(const std::wstring &Str);
  std::wstring ExportUserModels() const;
  void SetModified() const {Modified = true; FNeedDrawing = true;}
  bool IsModified() const {return Modified;}
  void SetNeedDrawing(bool Value=true) {FNeedDrawing = Value;}
  bool NeedDrawing() {return FNeedDrawing;}
  double FindInterception(const TBaseFuncType *Func, int X, int Y) const;

  static void Delete(const TGraphElemPtr &Elem);
  void Insert(const TGraphElemPtr &Elem, int Index=-1);
  static int GetIndex(const TGraphElemPtr &Elem);
  void Replace(unsigned Index, const TGraphElemPtr &Elem);
  static void Replace(const TGraphElemPtr &OldElem, const TGraphElemPtr &NewElem);
  unsigned ElemCount() const {return TopElem->ChildCount();}
  TGraphElemPtr GetElem(unsigned Index) const;
  boost::shared_ptr<TBaseFuncType> GetFuncFromIndex(unsigned Index) const;
  const TTopGraphElemPtr& GetTopElem() const {return TopElem;}
  TGraphElemPtr Back() const {return GetElem(ElemCount()-1);}

  const std::wstring& GetFileName() const {return GrfName;}
  void Update();
  void SetAbortUpdateEvent(TAbortUpdateEvent AOnAbortUpdate) {OnAbortUpdate = AOnAbortUpdate;}
  void AbortUpdate() const {if(OnAbortUpdate) OnAbortUpdate();}

  long double Calc(const std::wstring &Str) const
  {
    return Eval(Str, CustomFunctions.SymbolList, Axes.Trigonometry);
  }

  long double CalcNoThrow(const std::wstring &Str) const
  {
    return Func32::EvalNoThrow(Str, CustomFunctions.SymbolList, Axes.Trigonometry);
  }

  Func32::TComplex CalcComplex(const std::wstring &Str) const
  {
    return EvalComplex(Str, CustomFunctions.SymbolList, Axes.Trigonometry);
  }

  bool IsDependent(const std::wstring &Expression, const std::wstring &SymbolName) const;
};
//---------------------------------------------------------------------------
//extern TData Data;

enum TTraceType {ttTrace, ttIntersection, ttXAxis, ttYAxis, ttExtremeX, ttExtremeY};
double TraceFunction(const TBaseFuncType *Func, TTraceType TraceType, int X, int Y, const TData &Data, const class TDraw &Draw);
bool ExportPointSeries(const TPointSeries *Series, const wchar_t *FileName, char Delimiter, char DecimalSeparator);
double FindNearestPoint(const TBaseFuncType *Func, int X, int Y);

//! This is compatible with Func32::TTrendType
enum TTrendType
{
  ttLinear,       //!< Linear trend (y = a*x + b)
  ttLogarithmic,  //!< Logarithmic trend (y = a*ln(x) + b)
  ttPolynomial,   //!< Polynomial trend (y = an*x^n + ... + a3*x^3 + a2*x^2 + a1*x + a0). The order is given as argument to TrendLine().
  ttPower,        //!< Power trend (y = a*x^b)
  ttExponential,  //!< Exponential trend (y = a*b^x)
  ttMovingAverage,
  ttUserModel
};

struct TTrendData
{
  TTrendType Type;
  unsigned Order; //Only used by Polynomial
  double Intercept; //Not used by UserDefined and MovingAverage
  unsigned Period; //Only use by MovingAverage

  std::wstring Model;    //Only used by UserModel
  TDefaultsMap Defaults; //Only used by UserModel
  TTrendData() : Type(ttLinear), Order(2), Intercept(NAN), Period(1) {}
};

TBaseFuncPtr CreateTrendline(const TPointSeries *Series, const TTrendData &Trend);
} //namespace Graph
using namespace Graph;
//---------------------------------------------------------------------------
#endif
