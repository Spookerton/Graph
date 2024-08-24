/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#ifndef GraphElemH
#define GraphElemH
#include "../Func32/Func32.h"
#include <boost/weak_ptr.hpp>
#include <deque>
#include <boost/enable_shared_from_this.hpp>
#include <boost\math\special_functions\fpclassify.hpp>

class TConfigFileSection;
namespace Graph
{
struct EGraphError : public std::exception
{
  std::wstring Message;
  EGraphError(const std::wstring &Str) : Message(Str) {}
};

typedef boost::shared_ptr<class TGraphElem> TGraphElemPtr;
typedef boost::weak_ptr<class TGraphElem> TWeakGraphElemPtr;
typedef boost::shared_ptr<class TTopGraphElem> TTopGraphElemPtr;
typedef std::map<std::wstring, std::wstring> TStringMap;

struct TGraphElemVisitor
{
  virtual void Visit(class TBaseFuncType &Func) =0;
  virtual void Visit(class TTangent &Tan) = 0;
  virtual void Visit(class TPointSeries &Series) =0;
  virtual void Visit(class TShading &Shade) =0;
  virtual void Visit(class TTextLabel &Label) =0;
  virtual void Visit(class TRelation &Relation) =0;
  virtual void Visit(class TAxesView &AxesView) =0;
};

struct TTextValue
{
  std::wstring Text;
  double Value;
  TTextValue() : Value(0) {}
  explicit TTextValue(double AValue);
  TTextValue(double AValue, const std::wstring &AText) : Value(AValue), Text(AText) {}
  TTextValue(const std::wstring &AText, const class TData &Data, bool IgnoreErrors = false);
  bool operator==(const TTextValue &TextValue) const {return Value == TextValue.Value;}
  bool operator!=(const TTextValue &TextValue) const {return !(*this == TextValue);}
  bool operator==(double a) const {return Value == a;}
  bool operator<(double a) const {return Value < a;}
  bool operator>(double a) const {return Value > a;}
  bool operator<=(double a) const {return Value <= a;}
  bool operator>=(double a) const {return Value >= a;}
  void Update(const class TData &Data);
  void Set(const std::wstring AText, const TData &Data, bool IgnoreErrors = false);
  void Set(double AValue);
  bool IsFinite() const {return boost::math::isfinite(Value);}
  bool IsDependent(const TData &Data, const std::wstring &SymbolName) const;
};

std::wostream& operator<<(std::wostream &Stream, const TTextValue &TextValue);

class TData;
class TBaseFuncType;
typedef boost::shared_ptr<TBaseFuncType> TBaseFuncPtr;

class TGraphElem : public boost::enable_shared_from_this<TGraphElem>
{
  const TGraphElem& operator=(const TGraphElem&); //Not implemented
  bool Visible;
  bool ShowInLegend;
  bool ShowInFunctionList;
  std::wstring LegendText;
  std::vector<TGraphElemPtr> ChildList;
  TWeakGraphElemPtr Parent;
  bool UpdateFinished;
  const TData *Data;
  void *CustomData;
  TStringMap PluginData;

  void SetParent(const TGraphElemPtr &AParent) {Parent = AParent; if(Data) ParentChanged();}

protected:
  virtual void ParentChanged() {}
  void SetModified();

public:
  TGraphElem();
  TGraphElem(const TGraphElem &Elem);
  virtual ~TGraphElem() {}
  virtual void Swap(TGraphElem &O) =0;
  virtual std::wstring MakeLegendText() const {return LegendText.empty() ? MakeText() : LegendText;}
  virtual std::wstring MakeText() const = 0;
  virtual void WriteToIni(TConfigFileSection &Section) const=0;
  virtual void ReadFromIni(const TConfigFileSection &Section) =0;
  virtual void Accept(TGraphElemVisitor&) =0;
  virtual TGraphElemPtr Clone() const = 0;
  virtual void ClearCache();
  virtual void Update();
  const TData& GetData() const {BOOST_ASSERT(Data); return *Data;}
  void SetData(const TData *AData);
  bool DataValid() const {return Data;}
  void SetCustomData(void *ACustomData) {CustomData = ACustomData;}
  void* GetCustomData() const {return CustomData;}
  TStringMap& GetPluginData() {return PluginData;}

  void InsertChild(const TGraphElemPtr &Elem, int Index = -1);
  void ReplaceChild(unsigned Index, const TGraphElemPtr &Elem);
  unsigned GetChildIndex(const TGraphElemPtr &Elem) const;
  void RemoveChild(unsigned Index);
  TGraphElemPtr GetChild(unsigned Index) const {return ChildList.at(Index);} //Don't return a reference as it might be invalid while in use
  unsigned ChildCount() const {return ChildList.size();}
  TGraphElemPtr GetParent() const {return Parent.lock();}
  bool HasParent() const {return !Parent.expired();}
  std::vector<TGraphElemPtr>::const_iterator Begin() const {return ChildList.begin();}
  std::vector<TGraphElemPtr>::const_iterator End() const {return ChildList.end();}
  bool IsUpdateFinished() const {return UpdateFinished;}
  void SetUpdateFinished(bool Value=true) {UpdateFinished = Value;}

  virtual int GetVisible() const {return Visible;}
  virtual void ChangeVisible() {Visible = !Visible; SetModified();}
  virtual void SetVisible(int AVisible) {Visible = AVisible; SetModified();}
  bool GetShowInLegend() const {return ShowInLegend;}
  void SetShowInLegend(bool Value) {ShowInLegend = Value; SetModified();}
  bool GetShowInFunctionList() const {return ShowInFunctionList;}
  void SetShowInFunctionList(bool Value) {ShowInFunctionList = Value; SetModified();}
  const std::wstring& GetLegendText() const {return LegendText;}
  void SetLegendText(const std::wstring &Str) {LegendText = Str; SetModified();}
  virtual bool IsDependent(const std::wstring &SymbolName) const =0;
};

class TTopGraphElem : public TGraphElem
{
public:
  TTopGraphElem() {}
  void Swap(TGraphElem &O) {(void)dynamic_cast<TTopGraphElem&>(O); TGraphElem::Swap(O);}
  std::wstring MakeText() const {return L"";}
  void WriteToIni(TConfigFileSection &Section) const {};
  void ReadFromIni(const TConfigFileSection &Section) {};
  void Accept(TGraphElemVisitor&) {};
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TTopGraphElem(*this));}
  bool IsDependent(const std::wstring &SymbolName) const {return false;}
};
typedef boost::shared_ptr<TTopGraphElem> TTopGraphElemPtr;

enum TLabelPlacement
{
  lpUserTopLeft,
  lpAboveX,
  lpBelowX,
  lpLeftOfY,
  lpRightOfY,
  lpUserTopRight,
  lpUserBottomLeft,
  lpUserBottomRight
};

class TTextLabel : public TGraphElem
{
  std::string Text;
  TLabelPlacement LabelPlacement;
  std::pair<TTextValue,TTextValue> Pos; //Only used when TextLabelPos is tlpCustom
  TRect Rect;
  TColor BackgroundColor;
  TVclObject<TMetafile> Metafile;
  double Rotation; //Rotation in degrees
  std::wstring StatusText; //The text shown in the TreeView
  bool ContainsOleLink; //The Text contains an OLE object link that must be updated when loaded

public:
  TTextLabel();
  TTextLabel(const std::string &Str, TLabelPlacement Placement, const TTextValue &AxPos, const TTextValue &AyPos, TColor Color, double ARotation, bool OleLink);
  void Swap(TGraphElem &O);
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  std::wstring MakeText() const;
  void Accept(TGraphElemVisitor &v) {v.Visit(*this);}
  int UpdateRect(int X, int Y) {int Width = Rect.Width(); int Height = Rect.Height(); Rect = TRect(X, Y, X + Width, Y + Height); return Width;}
  bool IsInsideRect(int X, int Y) const {return InsideRect(Rect, TPoint(X, Y));}
  const std::pair<TTextValue,TTextValue>& GetPos() const {return Pos;}
  void SetPos(const std::pair<TTextValue,TTextValue> &Value) {Pos = Value; SetModified();}
  const std::string& GetText() const {return Text;}
  void SetText(const std::string &Str);
  void Scale(double xSizeMul, double ySizeMul);
  const TRect& GetRect() const {return Rect;}
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TTextLabel(*this));}
  TColor GetBackgroundColor() const {return BackgroundColor;}
  void SetBackgroundColor(TColor Color);
  TLabelPlacement GetPlacement() const {return LabelPlacement;}
  void SetPlacement(TLabelPlacement Value) {LabelPlacement = Value; SetModified();}
  double GetRotation() const {return Rotation;}
  void SetRotation(double Value) {Rotation = Value; SetModified();}
  TMetafile* GetImage() const {return Metafile;}
  void Update();
  bool IsDependent(const std::wstring &SymbolName) const;
  bool GetOleLink() {return ContainsOleLink;}
  void SetOleLink(bool Value) {ContainsOleLink = true;}
};

struct TAxes;
enum TDrawType {dtAuto, dtDots, dtLines};

class TBaseFuncType : public TGraphElem
{
protected:
  TBaseFuncType(const TBaseFuncType &F);
  TTextValue Steps; //Number of steps/evaluations. Rounded to an integer
  TColor Color;
  unsigned Size;
  TPenStyle Style;
  TDrawType DrawType;
  std::pair<unsigned,unsigned> EndpointStyle;
  TTextValue From, To;
  std::vector<TPoint> Points;
  std::vector<unsigned> PointNum;
  std::vector<Func32::TCoordSet<> > CoordList;

public:
  TBaseFuncType();
  void Swap(TGraphElem &O);
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  virtual TBaseFuncPtr MakeDifFunc() =0;
  virtual void GetCurrentRange(double &Min, double &Max, double &ds) const;
  virtual const TTextValue& GetSteps() const {return Steps;}
  void SetSteps(const TTextValue &Value) {Steps = Value; SetModified(); ClearCache();}
  virtual std::wstring GetVariable() const {return L"";}
  virtual const Func32::TBaseFunc& GetFunc() const =0;
  Func32::TBaseFunc& GetFunc() {return const_cast<Func32::TBaseFunc&>(const_cast<const TBaseFuncType*>(this)->GetFunc());}
  void ClearCache();
  void Update();
  Func32::TCoord<long double> Eval(long double t) const;
  virtual long double CalcArea(long double From, long double To) const;
  long double CalcArcLength(long double From, long double To) const;
  bool IsDependent(const std::wstring &SymbolName) const;

  const std::vector<TPoint>& GetPoints() const {return Points;}
  const std::vector<unsigned>& GetPointNum() const {return PointNum;}
  const std::vector<Func32::TCoordSet<> >& GetCoordList() const {return CoordList;}
  void SetPoints(std::vector<TPoint> &P, std::vector<unsigned> &Num, std::vector<Func32::TCoordSet<> > &Coord) {Points.swap(P); PointNum.swap(Num); CoordList.swap(Coord);}

  void SetColor(TColor Value) {Color = Value; SetModified();}
  TColor GetColor() const {return Color;}
  void SetSize(unsigned Value) {Size = Value; SetModified();}
  unsigned GetSize() const {return Size;}
  void SetStyle(TPenStyle Value) {Style = Value; SetModified();}
  TPenStyle GetStyle() const {return Style;}
  void SetDrawType(TDrawType Value) {DrawType = Value; SetModified();}
  TDrawType GetDrawType() const {return DrawType;};
  void SetEndpointStyle(const std::pair<unsigned,unsigned> &Value) {EndpointStyle = Value; SetModified();}
  const std::pair<unsigned,unsigned>& GetEndpointStyle() const {return EndpointStyle;}
  void SetFrom(const TTextValue &Value) {From = Value; SetModified(); ClearCache();}
  const TTextValue& GetFrom() const {return From;}
  void SetTo(const TTextValue &Value) {To = Value; SetModified(); ClearCache();}
  const TTextValue& GetTo() const {return To;}
};

enum TTangentType {ttTangent, ttNormal};
class TTangent : public TBaseFuncType
{
  mutable double a, q;  //Calculated at last redraw; a!=INF: y=ax+q, a==INF: x=q
  mutable Func32::TParamFunc TanFunc;
  TTextValue t;
  TTangentType TangentType;
  void UpdateTan(double a1, double q1);

protected:
  void ParentChanged();
public:
  TTangent();
  void Swap(TGraphElem &O);
  std::wstring MakeText() const;
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  std::wstring MakeLegendText() const;
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TTangent(*this));}
  TBaseFuncPtr MakeDifFunc() {throw Exception("Tangent cannot be differentiated");}
  bool IsValid() const; //Indicates the parent function is valid at t
  void GetCurrentRange(double &Min, double &Max, double &ds) const;
  void Accept(TGraphElemVisitor &v) {v.Visit(*this);}
  const Func32::TParamFunc& GetFunc() const {return TanFunc;}
  void Update();
  long double CalcArea(long double From, long double To) const;
  bool CalcTan();
  bool IsDependent(const std::wstring &SymbolName) const;
  void SetPos(const TTextValue &Value) {t = Value; SetModified(); Update();}
  const TTextValue& GetPos() const {return t;}
  void SetTangentType(TTangentType Value) {TangentType = Value; SetModified();}
  TTangentType GetTangentType() const {return TangentType;}
};

class TStdFunc : public TBaseFuncType
{
  std::wstring Equation;
  Func32::TFunc Func;
  mutable Func32::TFunc Dif;

public:
  TStdFunc() {} //Used with ReadFromIni();
  TStdFunc(const std::wstring &AText, const Func32::TSymbolList &SymbolList, Func32::TTrigonometry Trig);
  explicit TStdFunc(const Func32::TFunc &AFunc);

  void Swap(TGraphElem &O);
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TStdFunc(*this));}
  std::wstring MakeText() const;
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  TBaseFuncPtr MakeDifFunc();
  void GetCurrentRange(double &Min, double &Max, double &ds) const;
  void Accept(TGraphElemVisitor &v) {v.Visit(*this);}
  std::wstring GetVariable() const {return L"x";}
  const Func32::TFunc& GetFunc() const {return Func;}
  const Func32::TFunc& GetDif() const;
  const std::wstring& GetEquation() const {return Equation;}
  void SetEquation(const std::wstring &AText);
};
typedef boost::shared_ptr<TStdFunc> TStdFuncPtr;

class TParFunc : public TBaseFuncType
{
  std::pair<std::wstring,std::wstring> Equation;
  Func32::TParamFunc Func;

public:
  TParFunc() {}
  TParFunc(const std::wstring &AxText, const std::wstring &AyText, const Func32::TSymbolList &SymbolList, Func32::TTrigonometry Trig);
  TParFunc(const Func32::TParamFunc &AFunc);

  void Swap(TGraphElem &O);
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TParFunc(*this));}
  std::wstring MakeText() const;
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  TBaseFuncPtr MakeDifFunc();
  void Accept(TGraphElemVisitor &v) {v.Visit(*this);}
  std::wstring GetVariable() const {return L"t";}
  const Func32::TParamFunc& GetFunc() const {return Func;}
  const std::pair<std::wstring,std::wstring>& GetEquation() const {return Equation;}
  void SetEquation(const std::pair<std::wstring,std::wstring> &Value);
};
typedef boost::shared_ptr<TParFunc> TParFuncPtr;

class TPolFunc : public TBaseFuncType
{
  std::wstring Equation;
  Func32::TPolarFunc Func;

public:
  TPolFunc() {}
  TPolFunc(const std::wstring &AText, const Func32::TSymbolList &SymbolList, Func32::TTrigonometry Trig);

  void Swap(TGraphElem &O);
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TPolFunc(*this));}
  std::wstring MakeText() const;
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  TBaseFuncPtr MakeDifFunc();
  void Accept(TGraphElemVisitor &v) {v.Visit(*this);}
  std::wstring GetVariable() const {return L"t";}
  const Func32::TPolarFunc& GetFunc() const {return Func;}
  const std::wstring& GetEquation() const {return Equation;}
  void SetEquation(const std::wstring &Value);
};

enum TErrorBarType {ebtNone, ebtFixed, ebtRelative, ebtCustom};
struct TPointSeriesPoint
{
	std::wstring First; //X or angle
  std::wstring Second; //Y or r
  TTextValue xError, yError; //Data for error bars; only used if Uncertainty=utCustom

  TPointSeriesPoint() {}
  TPointSeriesPoint(const std::wstring &AFirst, const std::wstring &ASecond, const TTextValue &XError, const TTextValue &YError)
    : First(AFirst), Second(ASecond), xError(XError), yError(YError) {}
  TPointSeriesPoint(const std::wstring &AFirst, const std::wstring &ASecond)
    : First(AFirst), Second(ASecond) {}
};

enum TInterpolationAlgorithm {iaLinear, iaCubicSpline, iaHalfCosine, iaCubicSpline2};
enum TLabelPosition {lpAbove, lpBelow, lpLeft, lpRight, lpAboveLeft, lpAboveRight, lpBelowLeft, lpBelowRight};
enum TPointType {ptCartesian, ptPolar};

class TPointSeries : public TGraphElem
{
public:
  typedef std::vector<TPointSeriesPoint> TPointData;
  typedef std::vector<Func32::TDblPoint> TPointList;
private:
  TColor FrameColor;
  TColor FillColor;
  TColor LineColor;
  unsigned Size;
  unsigned Style; //Marker type
  unsigned LineSize;
  TPenStyle LineStyle;
  TInterpolationAlgorithm Interpolation;
  bool ShowLabels;
  TLabelPosition LabelPosition;
  TVclObject<TFont> Font;
  TPointData PointData;
  TPointList PointList;
  TErrorBarType xErrorBarType, yErrorBarType;
  double xErrorValue, yErrorValue; //Data for error bars; only used if Uncertainty!=utCustom
  TPointType PointType;

  Func32::TDblPoint ConvertPoint(const TPointSeriesPoint &P) const;
public:
  TPointSeries(TColor AFrameColor=clBlack, TColor AFillColor=clRed, TColor ALineColor=clRed,
    unsigned ASize=1, unsigned ALineSize=1, unsigned AStyle=0, TPenStyle ALineStyle=psSolid,
    TInterpolationAlgorithm AInterpolation=iaLinear, bool AShowLabels=false, TFont *AFont=NULL,
		TLabelPosition ALabelPosition=lpBelow, TPointType APointType=ptCartesian,
    TErrorBarType XErrorBarType=ebtNone, double XErrorValue=0, TErrorBarType YErrorBarType=ebtNone, double YErrorValue=0);

  void Swap(TGraphElem &Elem);
  std::wstring MakeText() const {return GetLegendText();}
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  void Accept(TGraphElemVisitor &v) {v.Visit(*this);}
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TPointSeries(*this));}
  TPointList::const_iterator FindPoint(double x) const;

	void InsertDblPoint(const Func32::TDblPoint &Point, int Index=-1);
	void InsertPoint(const TPointSeriesPoint &Point, int Index=-1, bool AutoUpdate=true);
  void ReplaceDblPoint(const Func32::TDblPoint &Point, unsigned Index);
  void ReplacePoint(const TPointSeriesPoint &Point, unsigned Index);
  void DeletePoint(unsigned Index, unsigned Count=1);
  const Func32::TDblPoint& GetDblPoint(int Index) const {return PointList.at(Index);}
  const TPointSeriesPoint& GetPoint(unsigned Index) const {return PointData.at(Index);}
  unsigned PointCount() const {return PointData.size();}
  const std::vector<TPointSeriesPoint>& GetPointData() const {return PointData;}
  const TPointList& GetPointList() const {return PointList;}

  double GetXError(unsigned Index) const;
  double GetYError(unsigned Index) const;
  void Assign(const TPointData &APointData) {PointData = APointData;}
  void Swap(TPointData &APointData) {PointData.swap(APointData);}
  void Update();

  TErrorBarType GetxErrorBarType() const {return xErrorBarType;}
  void SetxErrorBarType(TErrorBarType Value) {xErrorBarType = Value; SetModified();}
  TErrorBarType GetyErrorBarType() const {return yErrorBarType;}
  void SetyErrorBarType(TErrorBarType Value) {yErrorBarType = Value; SetModified();}
	double GetxErrorValue() const {return xErrorValue;}
  void SetxErrorValue(double Value) {xErrorValue = Value; SetModified();}
  double GetyErrorValue() const {return yErrorValue;}
  void SetyErrorValue(double Value) {yErrorValue = Value; SetModified();}
  TColor GetFillColor() const {return FillColor;}
  void SetFillColor(TColor Value) {FillColor = Value; SetModified();}
  unsigned GetSize() const {return Size;}
  void SetSize(unsigned Value) {Size = Value; SetModified();}
  unsigned GetStyle() const {return Style;}
  void SetStyle(unsigned Value) {Style = Value; SetModified();}
  TColor GetLineColor() const {return LineColor;}
  void SetLineColor(TColor Value) {LineColor = Value; SetModified();}
  TColor GetFrameColor() const {return FrameColor;}
  void SetFrameColor(TColor Value) {FrameColor = Value; SetModified();}
  unsigned GetLineSize() const {return LineSize;}
  void SetLineSize(unsigned Value) {LineSize = Value; SetModified();}
  TPenStyle GetLineStyle() const {return LineStyle;}
  void SetLineStyle(TPenStyle Value) {LineStyle = Value; SetModified();}
  TInterpolationAlgorithm GetInterpolation() const {return Interpolation;}
  void SetInterpolation(TInterpolationAlgorithm Value) {Interpolation = Value; SetModified();}
  bool GetShowLabels() const {return ShowLabels;}
  void SetShowLabels(bool Value) {ShowLabels = Value; SetModified();}
  TFont* GetFont() const {return Font;}
  TLabelPosition GetLabelPosition() const {return LabelPosition;}
  void SetLabelPosition(TLabelPosition Value) {LabelPosition = Value; SetModified();}
  TPointType GetPointType() const {return PointType;}
  void SetPointType(TPointType Value) {PointType = Value; SetModified();}
  bool IsDependent(const std::wstring &SymbolName) const;
};

Func32::TDblPoint FindCoord(TPointSeries::TPointList::const_iterator Iter, double x);
typedef boost::shared_ptr<TPointSeries> TPointSeriesPtr;

enum TShadeStyle {ssAbove, ssBelow, ssXAxis, ssYAxis, ssBetween, ssInside};

class TShading : public TGraphElem
{
  boost::shared_ptr<class TRegion> Region;
  TShadeStyle ShadeStyle;
  TBrushStyle BrushStyle;
  TColor Color;
  bool MarkBorder;
  TBaseFuncPtr Func2;
  TTextValue sMin;
  TTextValue sMax;
  TTextValue sMin2;
  TTextValue sMax2;
  bool ExtendMinToIntercept;
  bool ExtendMaxToIntercept;
  bool ExtendMin2ToIntercept;
  bool ExtendMax2ToIntercept;
public:
  TShading();
  TShading(const TShading &O);

  void Swap(TGraphElem &O);
  std::wstring MakeText() const;
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  void Accept(TGraphElemVisitor &v) {v.Visit(*this);}
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TShading(*this));}
  void Update();
  void ClearCache();
  bool IsDependent(const std::wstring &SymbolName) const;
  const TRegion* GetRegion() const {return Region.get();}
  void SetRegion(boost::shared_ptr<TRegion> Value) {Region = Value;}

  TColor GetColor() const {return Color;}
  void SetColor(TColor Value) {Color=Value; SetModified();}
  TShadeStyle GetShadeStyle() const {return ShadeStyle;}
  void SetShadeStyle(TShadeStyle Value) {ShadeStyle=Value; SetModified();}
  TBrushStyle GetBrushStyle() const {return BrushStyle;}
  void SetBrushStyle(TBrushStyle Value) {BrushStyle=Value; SetModified();}
  bool GetMarkBorder() const {return MarkBorder;}
  void SetMarkBorder(bool Value) {MarkBorder=Value; SetModified();}
  const TBaseFuncPtr &GetFunc2() const {return Func2;}
  void SetFunc2(const TBaseFuncPtr &Value) {Func2=Value; SetModified();}

  const TTextValue GetMin() const {return sMin;}
  void SetMin(const TTextValue &Value) {sMin = Value; SetModified(); ClearCache();}
  const TTextValue GetMax() const {return sMax;}
  void SetMax(const TTextValue &Value) {sMax = Value; SetModified(); ClearCache();}
  const TTextValue GetMin2() const {return sMin2;}
  void SetMin2(const TTextValue &Value) {sMin2 = Value; SetModified(); ClearCache();}
  const TTextValue GetMax2() const {return sMax2;}
  void SetMax2(const TTextValue &Value) {sMax2 = Value; SetModified(); ClearCache();}

  bool GetExtendMinToIntercept() const {return ExtendMinToIntercept;}
  void SetExtendMinToIntercept(bool Value) {ExtendMinToIntercept = Value; SetModified(); ClearCache();}
  bool GetExtendMaxToIntercept() const {return ExtendMaxToIntercept;}
  void SetExtendMaxToIntercept(bool Value) {ExtendMaxToIntercept = Value; SetModified(); ClearCache();}
  bool GetExtendMin2ToIntercept() const {return ExtendMin2ToIntercept;}
  void SetExtendMin2ToIntercept(bool Value) {ExtendMin2ToIntercept = Value; SetModified(); ClearCache();}
  bool GetExtendMax2ToIntercept() const {return ExtendMax2ToIntercept;}
  void SetExtendMax2ToIntercept(bool Value) {ExtendMax2ToIntercept = Value; SetModified(); ClearCache();}
};
typedef boost::shared_ptr<TShading> TShadingPtr;

enum TRelationType {rtEquation, rtInequality};
enum TRelationTrace {rtXValue, rtYValue};
class TRelation : public TGraphElem
{
  std::wstring Text;
  std::wstring ConstraintsText;
  Func32::TCustomFunc Func;
  Func32::TCustomFunc Constraints;
  TColor Color;
  unsigned Size;
  TBrushStyle BrushStyle;
  TRelationType RelationType;
  TPenStyle LineStyle;
  unsigned Alpha;
  std::vector<TPoint> PolylinePoints;
  std::vector<int> PolylineCounts;

  static long double Trace(const Func32::TCustomFunc &Func, long double Min, long double Max, long double FixedValue, TRelationTrace RelationTrace);

public:
  TRelation();
  TRelation(const std::wstring &AText, const std::wstring &AConstraints, const Func32::TSymbolList &SymbolList, Func32::TTrigonometry Trig);
  TRelation(const TRelation &Relation);

  void Swap(TGraphElem &O);
  std::wstring MakeText() const;
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  void Accept(TGraphElemVisitor &v) {v.Visit(*this);}
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TRelation(*this));}
  void SetConstraints(const std::wstring &AConstraintsText, const Func32::TSymbolList &SymbolList);
  void Update();
  void SetPoints(std::vector<TPoint> &APolylinePoints, std::vector<int> &APolylineCounts); //This will steal the content of the vectors

  TColor GetColor() const {return Color;}
  void SetColor(TColor Value) {Color = Value; SetModified();}
  TBrushStyle GetBrushStyle() const {return BrushStyle;}
  void SetBrushStyle(TBrushStyle Value) {BrushStyle = Value; SetModified();}
  const std::wstring& GetText() const {return Text;}
  const std::wstring& GetConstraints() const {return ConstraintsText;}
  long double Eval(const std::vector<long double> &Args, Func32::ECalcError &E) const;
  long double Eval(long double x, long double y) const;
  bool EvalConstraints(const std::vector<long double> &Args, Func32::ECalcError &E) const;
  long double EvalRelation(const std::vector<long double> &Args, Func32::ECalcError &E) const;
  void ClearCache();
  TRelationType GetRelationType() const {return RelationType;}
  unsigned GetSize() const {return Size;}
  void SetSize(unsigned Value) {Size = Value; SetModified();}
  bool IsDependent(const std::wstring &SymbolName) const;
  TPenStyle GetLineStyle() const {return LineStyle;}
  void SetLineStyle(TPenStyle PenStyle) {LineStyle = PenStyle; SetModified();}
  unsigned GetAlpha() const {return Alpha;}
  void SetAlpha(unsigned AAlpha) {Alpha = AAlpha; SetModified();}
  const std::vector<TPoint>& GetPolylinePoints() const {return PolylinePoints;}
  const std::vector<int>& GetPolylineCounts() const {return PolylineCounts;}
  long double Trace(long double Min, long double Max, long double FixedValue, TRelationTrace RelationTrace) const;
  std::vector<long double> Analyse(long double Min, long double Max, unsigned Steps, long double FixedValue, TRelationTrace RelationTrace) const;
};

class TAxesView : public TGraphElem
{
public:
  TAxesView() {SetShowInLegend(false);}
  std::wstring MakeText() const;
  void WriteToIni(TConfigFileSection &Section) const;
  void ReadFromIni(const TConfigFileSection &Section);
  void Accept(TGraphElemVisitor &v) {v.Visit(*this);}
  TGraphElemPtr Clone() const {return TGraphElemPtr(new TAxesView(*this));}
  int GetVisible() const;
  void SetVisible(int Value);
  void ChangeVisible();
  const TAxes& GetAxes() const;
  bool IsDependent(const std::wstring &SymbolName) const;
  void Swap(TGraphElem &O) {(void)dynamic_cast<TAxesView&>(O); TGraphElem::Swap(O);}
};
} //namespace Graph
using namespace Graph;
//---------------------------------------------------------------------------
#endif
