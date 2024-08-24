%module Data
#define SWIG_NO_EXPORT_ITERATOR_METHODS

%include "stl.i"
%include "std_string.i"
%include "std_wstring.i"
%include "boost_shared_ptr.i"
%include "attribute.i"

%pythonnondynamic;

%shared_ptr(TGraphElem)
%shared_ptr(TTopGraphElem)
%shared_ptr(TBaseFuncType)
%shared_ptr(TStdFunc)
%shared_ptr(TParFunc)
%shared_ptr(TPolFunc)
%shared_ptr(TTangent)
%shared_ptr(TPointSeries)
%shared_ptr(TTextLabel)
%shared_ptr(TShading)
%shared_ptr(TRelation)
%shared_ptr(TAxesView)

%include "Types.i"

%template(TStringMap) std::map<std::wstring,std::wstring>;
typedef std::map<std::wstring, std::wstring> TStringMap;

%begin %{
#define WRAP_PYOBJECTS
#include "Graph.h"
#include "Unit1.h"
#include "Python.hpp"
#include "PythonBind.h"
#include "PyGraph.h"
#include "PyVclConvert.h"
#include "PyVclObject.h"
#pragma warn -8060
#pragma warn -8070

//Bug in BCB XE6 64 bit: several extern "C" functions with static variables with the same name seems to be merged.
#define SWIG_module SWIG_module_Data
%}

//Replacement for %attribute that calls PushUndoElem() before the set function and optionally UpdateTreeView() after the set function.
%define %GraphAttribute(Class, AttributeType, AttributeName, GetMethod, SetMethod, Update)
  #if Update
    %attribute_custom(%arg(Class), %arg(AttributeType), AttributeName, GetMethod, SetMethod, self_->GetMethod(), PushUndoElem(self_); self_->SetMethod(val_); if(self_->HasParent()) UpdateTreeView();)
  #else
    %attribute_custom(%arg(Class), %arg(AttributeType), AttributeName, GetMethod, SetMethod, self_->GetMethod(), PushUndoElem(self_); self_->SetMethod(val_))
  #endif
%enddef

//Replacement for %attribute2 that calls PushUndoElem() before the set function and optionally UpdateTreeView() after the set function.
%define %GraphAttribute2(Class, AttributeType, AttributeName, GetMethod, SetMethod, Update)
  #if Update
    %attribute_custom(%arg(Class), %arg(AttributeType), AttributeName, GetMethod, SetMethod, &self_->GetMethod(), PushUndoElem(self_); self_->SetMethod(*val_); if(self_->HasParent()) UpdateTreeView();)
  #else
    %attribute_custom(%arg(Class), %arg(AttributeType), AttributeName, GetMethod, SetMethod, &self_->GetMethod(), PushUndoElem(self_); self_->SetMethod(*val_))
  #endif
%enddef

%define CHECK_GRAPH_ELEM(TType)
  if(dynamic_cast<TType*>(Ptr))
    return SWIG_NewPointerObj(new boost::shared_ptr<TType>(boost::static_pointer_cast<TType>(Elem)), SWIGTYPE_p_boost__shared_ptrT_##TType##_t, SWIG_POINTER_OWN |  0 );
%enddef

%define CREATE_CONVERT_TO_CPP_FUNC(TType)
boost::shared_ptr<TType> ConvertTo##TType(PyObject *O)
{
  boost::shared_ptr<##TType##> arg1;
  void *argp1;
  int newmem = 0;
  int res1 = SWIG_ConvertPtrAndOwn(O, &argp1, SWIGTYPE_p_boost__shared_ptrT_##TType##_t,  0 , &newmem);
  if (!SWIG_IsOK(res1)) {
    SWIG_exception_fail(SWIG_ArgError(res1), "argument of type 'boost::shared_ptr<##TType##>'"); 
  }
  if (argp1) arg1 = *(reinterpret_cast< boost::shared_ptr<##TType##> * >(argp1));
  if (newmem & SWIG_CAST_NEW_MEMORY) delete reinterpret_cast< boost::shared_ptr<##TType##> * >(argp1);
fail:
  return arg1;
}
%enddef

%header {
PyObject* DownCastSharedPtr(const boost::shared_ptr<TGraphElem> &Elem)
{
  TGraphElem *Ptr = Elem.get();
  if(Ptr == NULL)
    return SWIG_Py_Void();
  CHECK_GRAPH_ELEM(TStdFunc)
  CHECK_GRAPH_ELEM(TParFunc)
  CHECK_GRAPH_ELEM(TPolFunc)
  CHECK_GRAPH_ELEM(TTangent)
  CHECK_GRAPH_ELEM(TPointSeries)
  CHECK_GRAPH_ELEM(TTextLabel)
  CHECK_GRAPH_ELEM(TShading)
  CHECK_GRAPH_ELEM(TRelation)
  CHECK_GRAPH_ELEM(TAxesView)
  CHECK_GRAPH_ELEM(TTopGraphElem)
  return SWIG_NewPointerObj(new boost::shared_ptr< TGraphElem >(Elem), SWIGTYPE_p_boost__shared_ptrT_TGraphElem_t, SWIG_POINTER_OWN |  0 );
}

CREATE_CONVERT_TO_CPP_FUNC(TPointSeries)
}

%inline
{
void PushUndoElem(TGraphElem *Elem)
{
  if(Elem->HasParent())
  {
    Form1->Data.AbortUpdate();
    const TUndoChange *UndoChange = UndoList.Peek<TUndoChange>();
    if(Form1->ForceUndo() || UndoChange == NULL || UndoChange->GetElem().get() != Elem)
      UndoList.Push(TUndoChange(Elem->shared_from_this()));
    
  }
}
}

HANDLE_FPU(Redraw)
HANDLE_PUSH_UNDO_ELEM(InsertDblPoint)
HANDLE_PUSH_UNDO_ELEM(InsertPoint)
HANDLE_PUSH_UNDO_ELEM(ReplaceDblPoint)
HANDLE_PUSH_UNDO_ELEM(ReplacePoint)
HANDLE_PUSH_UNDO_ELEM(DeletePoint)
HANDLE_FPU(TStdFunc::TStdFunc)
HANDLE_FPU(TParFunc::TParFunc)
HANDLE_FPU(TPolFunc::TPolFunc)

typedef boost::shared_ptr<class TGraphElem> TGraphElemPtr;

%inline %{
static void AbortUpdate() {Form1->Draw.AbortUpdate();}
static bool IsUpdating() {return Form1->Draw.Updating();}
static void Redraw() {Form1->Redraw();}
static void UpdateTreeView() {Form1->UpdateTreeView();}
static TGraphElemPtr GetSelected() {return Form1->GetSelected();}
static void SetSelected(const TGraphElemPtr &Elem) {Form1->SetSelected(Elem);}

static unsigned ChildCount(const TGraphElemPtr &Elem) {return Elem->ChildCount();}
static TGraphElemPtr GetChild(const TGraphElemPtr &Elem, unsigned Index) {return Elem->GetChild(Index);}
static void RemoveChild(const TGraphElemPtr &Elem, unsigned Index)
{
  Form1->Data.AbortUpdate();
  UndoList.Push(TUndoDel(Elem->GetChild(Index), Elem, Index));
  Elem->RemoveChild(Index);
  UpdateTreeView();
}

static void InsertChild(const TGraphElemPtr &Elem, const TGraphElemPtr &Child, int Index)
{
  Form1->Data.AbortUpdate();
  UndoList.BeginMultiUndo();
  TGraphElemPtr Parent = Child->GetParent();
  if(Parent)
    UndoList.Push(TUndoDel(Child, Parent, Parent->GetChildIndex(Child)));
  Elem->InsertChild(Child, Index);
  UndoList.Push(TUndoAdd(Child));
  UndoList.EndMultiUndo();
  UpdateTreeView();
}

static void ReplaceChild(const TGraphElemPtr &Elem, unsigned Index, const TGraphElemPtr &Child)
{
  Form1->Data.AbortUpdate();
  UndoList.Push(TUndoReplace(Elem->GetChild(Index), Child));
  Elem->ReplaceChild(Index, Child);
  UpdateTreeView();
}

static bool CompareElem(const TGraphElemPtr &E1, const TGraphElemPtr &E2) {return E1.get() == E2.get();}
static TStringMap& GetPluginData() {return Form1->Data.PluginData;}
static const boost::shared_ptr<TTopGraphElem>& GetTopElem() {return Form1->Data.GetTopElem();}
%}

//This must be placed after the %shared_ptr macro
%typemap(out) boost::shared_ptr<TBaseFuncType>
{
  $result = DownCastSharedPtr($1);
}

//This will add a check to all functions in the affected classes and call DataValid() to check if it is okay to interact with the object.
//Otherwise the object has been deleted and an exception is thrown.
%typemap(check) TGraphElem*, TBaseFuncType*, TStdFunc*, TParFunc*, TPolFunc*, TPointSeries*, TRelation*, TTextLabel*, TTangent*, TShading*
{
  if(!$1->DataValid())
      SWIG_exception_fail(SWIG_RuntimeError, "Element is not valid");
}

//Unfortunately the typemap(check) above will also add a check to the destructor. But we want to allow the destructor even though the object is not valid.
//This will remove the check from the destructor. I assume it is because this reopens the class and the last time the class 
//definition is finished the destructor is generated.
%define CLEAR_DELETE_CHECK(ClassName)
  %extend ClassName 
  {
    %typemap(check) ClassName* ""
  }
%enddef

%nodefaultctor TGraphElem;
%GraphAttribute(TGraphElem, int, Visible, GetVisible, SetVisible, 1);
%GraphAttribute(TGraphElem, bool, ShowInLegend, GetShowInLegend, SetShowInLegend, 1);
%GraphAttribute(TGraphElem, bool, ShowInFunctionList, GetShowInFunctionList, SetShowInFunctionList, 1);
%GraphAttribute(TGraphElem, std::wstring, LegendText, GetLegendText, SetLegendText, 0);
%attributestring(TGraphElem, TGraphElemPtr, Parent, GetParent);
%attributestring(TGraphElem, std::wstring, Caption, MakeLegendText);
%attribute2(TGraphElem, TStringMap, _PluginData, GetPluginData)

class TGraphElem
{
public:
  std::wstring MakeLegendText() const;
  std::wstring MakeText() const;
  TGraphElemPtr Clone();
};


%extend TGraphElem
{
  int ThisPtr() const {return reinterpret_cast<int>(self);}
  %pythoncode %{
    def __eq__(self, rhs):
      if not isinstance(rhs, TGraphElem): return False
      return self.ThisPtr() == rhs.ThisPtr()
  %}
}

enum TDrawType {dtAuto, dtDots, dtLines};
HANDLE_ENUM(TPenStyle)

%nodefaultctor TBaseFuncType;
//%attributeval(TBaseFuncType, %arg(std::pair<double,double>), CurrentRange, GetCurrentRange);
%GraphAttribute(TBaseFuncType, TTextValue, Steps, GetSteps, SetSteps, 0);
%attributestring(TBaseFuncType, std::wstring, Variable, GetVariable);
%GraphAttribute(TBaseFuncType, TColor, Color, GetColor, SetColor, 1);
%GraphAttribute(TBaseFuncType, unsigned, Size, GetSize, SetSize, 0);
%GraphAttribute(TBaseFuncType, TDrawType, DrawType, GetDrawType, SetDrawType, 0);
%GraphAttribute(TBaseFuncType, TPenStyle, Style, GetStyle, SetStyle, 0);
%GraphAttribute2(TBaseFuncType, %arg(std::pair<unsigned,unsigned>), EndpointStyle, GetEndpointStyle, SetEndpointStyle, 0);
%GraphAttribute(TBaseFuncType, TTextValue, From, GetFrom, SetFrom, 0);
%GraphAttribute(TBaseFuncType, TTextValue, To, GetTo, SetTo, 0);
%attribute(TBaseFuncType, const std::vector<TPoint>&, Points, GetPoints);
%attribute(TBaseFuncType, const std::vector<unsigned>&, PointNum, GetPointNum);
%attribute(TBaseFuncType, const std::vector<Func32::TCoordSet<> >&, CoordList, GetCoordList);
class TBaseFuncType : public TGraphElem
{
public: 
  boost::shared_ptr<TBaseFuncType> MakeDifFunc();
  Func32::TCoord<long double> Eval(long double t) const throw(Func32::EFuncError);
  long double CalcArea(long double From, long double To) const throw(Func32::EFuncError);
  long double CalcArcLength(long double From, long double To) const throw(Func32::EFuncError);
};
typedef boost::shared_ptr<TBaseFuncType> TBaseFuncPtr;

%nodefaultctor TTopGraphElem;
class TTopGraphElem : public TGraphElem
{
};

//Add exception handling to the Equation get/set functions
%exception Equation
%{
  try
  {
    $action
  }
  catch(Func32::EFuncError &E)
  {
    PyErr_SetString(Python::PyEFuncError, ToString(GetErrorMsg(E)).c_str());
    SWIG_fail;
  }
%}
%allowexception; //Allow exception handling for variables and get/set functions


%attributestring(TStdFunc, std::wstring, Text, GetEquation); //For backward compatibility
%GraphAttribute(TStdFunc, std::wstring, Equation, GetEquation, SetEquation, 1); 
class TStdFunc : public TBaseFuncType
{
};
typedef boost::shared_ptr<TStdFunc> TStdFuncPtr;

%extend TStdFunc {
  TStdFunc(const std::wstring &Str) throw(Func32::EFuncError)
  {
    TStdFunc *F = new TStdFunc(Str, Form1->Data.CustomFunctions.SymbolList, Form1->Data.Axes.Trigonometry);
    F->SetData(&Form1->Data);
    return F;
  }
}

%nodefaultctor TParFunc;
%GraphAttribute2(TParFunc, %arg(std::pair<std::wstring,std::wstring>), Equation, GetEquation, SetEquation, 1); 
class TParFunc : public TBaseFuncType
{
};
typedef boost::shared_ptr<TParFunc> TParFuncPtr;

%extend TParFunc {
  TParFunc(const std::wstring &xStr, const std::wstring &yStr) throw(Func32::EFuncError)
  {
    TParFunc *F = new TParFunc(xStr, yStr, Form1->Data.CustomFunctions.SymbolList, Form1->Data.Axes.Trigonometry);
    F->SetData(&Form1->Data);
    return F;
  }
}

%nodefaultctor TPolFunc;
%GraphAttribute(TPolFunc, std::wstring, Equation, GetEquation, SetEquation, 1); 
class TPolFunc : public TBaseFuncType
{
};

%extend TPolFunc {
  TPolFunc(const std::wstring &Str) throw(Func32::EFuncError)
  {
    TPolFunc *F = new TPolFunc(Str, Form1->Data.CustomFunctions.SymbolList, Form1->Data.Axes.Trigonometry);
    F->SetData(&Form1->Data);
    return F;
  }
}

enum TTangentType {ttTangent, ttNormal};
%attribute(TTangent, bool, Valid, IsValid);
%GraphAttribute(TTangent, TTextValue, Pos, GetPos, SetPos, 0);
%GraphAttribute(TTangent, TTangentType, TangentType, GetTangentType, SetTangentType, 0);
class TTangent : public TBaseFuncType
{
};

//All elements must have Data set; Otherwise they are assumed to be on undo/redo stack and cannot be accessed from Python
%extend TTangent {
  TTangent()
  {
    TTangent *T = new TTangent();
    T->SetData(&Form1->Data);
    return T;
  }
}


enum TErrorBarType {ebtNone, ebtFixed, ebtRelative, ebtCustom};
enum TInterpolationAlgorithm {iaLinear, iaCubicSpline, iaHalfCosine, iaCubicSpline2};
enum TPointType {ptCartesian, ptPolar};
enum TTrendType {ttLinear, ttLogarithmic,  ttPolynomial, ttPower, ttExponential};
namespace Graph
{
enum TLabelPosition {lpAbove, lpBelow, lpLeft, lpRight, lpAboveLeft, lpAboveRight, lpBelowLeft, lpBelowRight};
}

HANDLE_FPU(TPointSeries::GetDblPoint)


%GraphAttribute(TPointSeries, TErrorBarType, xErrorBarType, GetxErrorBarType, SetxErrorBarType, 0);
%GraphAttribute(TPointSeries, TErrorBarType, yErrorBarType, GetyErrorBarType, SetyErrorBarType, 0);
%GraphAttribute(TPointSeries, double, xErrorValue, GetxErrorValue, SetxErrorValue, 0);
%GraphAttribute(TPointSeries, double, yErrorValue, GetyErrorValue, SetyErrorValue, 0);
%GraphAttribute(TPointSeries, TColor, FillColor, GetFillColor, SetFillColor, 0);
%GraphAttribute(TPointSeries, TColor, LineColor, GetLineColor, SetLineColor, 0);
%GraphAttribute(TPointSeries, TColor, FrameColor, GetFrameColor, SetFrameColor, 0);
%GraphAttribute(TPointSeries, unsigned, Size, GetSize, SetSize, 0);
%GraphAttribute(TPointSeries, unsigned, Style, GetStyle, SetStyle, 0);
%GraphAttribute(TPointSeries, unsigned, LineSize, GetLineSize, SetLineSize, 0);
%GraphAttribute(TPointSeries, TPenStyle, LineStyle, GetLineStyle, SetLineStyle, 0);
%GraphAttribute(TPointSeries, TInterpolationAlgorithm, Interpolation, GetInterpolation, SetInterpolation, 0);
%GraphAttribute(TPointSeries, bool, ShowLabels, GetShowLabels, SetShowLabels, 0);
%attribute(TPointSeries, TFont*, Font, GetFont);
%GraphAttribute(TPointSeries, Graph::TLabelPosition, LabelPosition, GetLabelPosition, SetLabelPosition, 0);
%GraphAttribute(TPointSeries, TPointType, PointType, GetPointType, SetPointType, 0);
class TPointSeries : public TGraphElem
{
public:
  const Func32::TDblPoint& GetDblPoint(int Index) const throw(std::out_of_range);
  const TPointSeriesPoint& GetPoint(unsigned Index) const throw(std::out_of_range);
  unsigned PointCount() const;

  void InsertDblPoint(Func32::TDblPoint Point, int Index) throw(std::out_of_range);
  void InsertPoint(TPointSeriesPoint Point, int Index) throw(std::out_of_range);
  void ReplaceDblPoint(Func32::TDblPoint Point, unsigned Index) throw(std::out_of_range);
  void ReplacePoint(TPointSeriesPoint Point, unsigned Index) throw(std::out_of_range);
  void DeletePoint(unsigned Index, unsigned Count=1) throw(std::out_of_range);
};

%extend TPointSeries {
  TPointSeries()
  {
    TPointSeries *P = new TPointSeries();
    P->SetData(&Form1->Data);
    return P;
  }

  TBaseFuncPtr CreateTrendline(TTrendType TrendType, unsigned Order=2)
  {
    TTrendData Trend;
    Trend.Type = TrendType;
    Trend.Order = Order;
    TBaseFuncPtr Func = CreateTrendline(self, Trend);
    Func->SetData(&Form1->Data);
    return Func;
  }
  
  TBaseFuncPtr CreateMovingAverage(unsigned Period)
  {
    TTrendData Trend;
    Trend.Type = ttMovingAverage;
    Trend.Period = Period;
    TBaseFuncPtr Func = CreateTrendline(self, Trend);
    Func->SetData(&Form1->Data);
    return Func;
  }
  
  TBaseFuncPtr CreateModelTrendline(const std::wstring &Model, PyObject *Dict=Py_None) throw(Exception)
  {
    TTrendData Trend;
    Trend.Type = ttUserModel;
    Trend.Model = Model;
    PyObject *Key, *Value;
    Py_ssize_t Pos = 0;
    if(Dict != Py_None)
    {
      if(!PyDict_Check(Dict))
        throw Exception("Second argument must be a dictionary");
      while(PyDict_Next(Dict, &Pos, &Key, &Value)) 
      {
        Trend.Defaults[Python::FromPyObject<std::wstring>(Key)] = Python::FromPyObject<double>(Value);
        if(PyErr_Occurred())
          throw EAbort(""); //Python exception already set
      }
    }
    TBaseFuncPtr Func = CreateTrendline(self, Trend);
    Func->SetData(&Form1->Data);
    return Func;
  }
}


enum TLabelPlacement {lpUserTopLeft, lpAboveX, lpBelowX, lpLeftOfY, lpRightOfY, lpUserTopRight, lpUserBottomLeft, lpUserBottomRight};

%attribute(TTextLabel, TRect, Rect, GetRect);
%GraphAttribute2(TTextLabel, %arg(std::pair<TTextValue,TTextValue>), Pos, GetPos, SetPos, 0);
%GraphAttribute(TTextLabel, std::string, Text, GetText, SetText, 1);
%GraphAttribute(TTextLabel, TColor, BackgroundColor, GetBackgroundColor, SetBackgroundColor, 0);
%GraphAttribute(TTextLabel, TLabelPlacement, Placement, GetPlacement, SetPlacement, 0);
%GraphAttribute(TTextLabel, unsigned, Rotation, GetRotation, SetRotation, 0);
class TTextLabel : public TGraphElem
{
public:
  void Scale(double xSizeMul, double ySizeMul);
  TMetafile* GetImage() const {return Metafile;}
};

//All elements must have Data set; Otherwise they are assumed to be on undo/redo stack and cannot be accessed from Python
%extend TTextLabel {
  TTextLabel()
  {
    TTextLabel *L = new TTextLabel();
    L->SetData(&Form1->Data);
    return L;
  }
}


enum TShadeStyle {ssAbove, ssBelow, ssXAxis, ssYAxis, ssBetween, ssInside};
%GraphAttribute(TShading, TColor, Color, GetColor, SetColor, 1);
%GraphAttribute(TShading, TShadeStyle, ShadeStyle, GetShadeStyle, SetShadeStyle, 0);
%GraphAttribute(TShading, TBrushStyle, BrushStyle, GetBrushStyle, SetBrushStyle, 1);
%GraphAttribute(TShading, TBaseFuncPtr, Func2, GetFunc2, SetFunc2, 0);
%GraphAttribute(TShading, bool, MarkBorder, GetMarkBorder, SetMarkBorder, 0);
%GraphAttribute(TShading, TTextValue, Min, GetMin, SetMin, 0);
%GraphAttribute(TShading, TTextValue, Max, GetMax, SetMax, 0);
%GraphAttribute(TShading, TTextValue, Min2, GetMin2, SetMin2, 0);
%GraphAttribute(TShading, TTextValue, Max2, GetMax2, SetMax2, 0);
%GraphAttribute(TShading, bool, ExtendMinToIntercept, GetExtendMinToIntercept, SetExtendMinToIntercept, 0);
%GraphAttribute(TShading, bool, ExtendMaxToIntercept, GetExtendMaxToIntercept, SetExtendMaxToIntercept, 0);
%GraphAttribute(TShading, bool, ExtendMin2ToIntercept, GetExtendMin2ToIntercept, SetExtendMin2ToIntercept, 0);
%GraphAttribute(TShading, bool, ExtendMax2ToIntercept, GetExtendMax2ToIntercept, SetExtendMax2ToIntercept, 0);
class TShading : public TGraphElem
{
};

//All elements must have Data set; Otherwise they are assumed to be on undo/redo stack and cannot be accessed from Python
%extend TShading {
  TShading()
  {
    TShading *S = new TShading();
    S->SetData(&Form1->Data);
    return S;
  }
}


enum TRelationType {rtEquation, rtInequality};

%GraphAttribute(TRelation, TColor, Color, GetColor, SetColor, 1);
%GraphAttribute(TRelation, unsigned, Alpha, GetAlpha, SetAlpha, 0);
%GraphAttribute(TRelation, TBrushStyle, BrushStyle, GetBrushStyle, SetBrushStyle, 1);
%GraphAttribute(TRelation, TPenStyle, LineStyle, GetLineStyle, SetLineStyle, 0);
%attribute(TRelation, TRelationType, RelationType, GetRelationType);
%attributestring(TRelation, std::wstring, Text, GetText);
%attributestring(TRelation, std::wstring, Constraints, GetConstraints);
%GraphAttribute(TRelation, unsigned, Size, GetSize, SetSize, 0);
class TRelation : public TGraphElem
{
public:
  long double Eval(long double x, long double y) throw(Func32::EFuncError);
};

%extend TRelation {
  TRelation(const std::wstring &Str, const std::wstring &ConstraintsStr) throw(Func32::EFuncError)
  {
    TRelation *R = new TRelation(Str, ConstraintsStr, Form1->Data.CustomFunctions.SymbolList, Form1->Data.Axes.Trigonometry);
    R->SetData(&Form1->Data);
    return R;
  }
  TRelation(const std::wstring &Str) throw(Func32::EFuncError)
  {
    TRelation *R = new TRelation(Str, L"", Form1->Data.CustomFunctions.SymbolList, Form1->Data.Axes.Trigonometry);
    R->SetData(&Form1->Data);
    return R;
  }
}

%nodefaultctor TAxesView;
class TAxesView : public TGraphElem
{
};

CLEAR_DELETE_CHECK(TGraphElem)
CLEAR_DELETE_CHECK(TBaseFuncType)
CLEAR_DELETE_CHECK(TStdFunc)
CLEAR_DELETE_CHECK(TParFunc)
CLEAR_DELETE_CHECK(TPolFunc)
CLEAR_DELETE_CHECK(TTangent)
CLEAR_DELETE_CHECK(TRelation)
CLEAR_DELETE_CHECK(TPointSeries)
CLEAR_DELETE_CHECK(TTextLabel)
CLEAR_DELETE_CHECK(TShading)


%pythoncode
{
  import vcl
  import collections
  Point = collections.namedtuple("Point", ("x","y"))
  
  def GraphElemRepr(self):
    return '%s("%s")' % (self.__class__.__name__, self.MakeText())
  TStdFunc.__repr__ = GraphElemRepr
  TParFunc.__repr__ = GraphElemRepr
  TPolFunc.__repr__ = GraphElemRepr
  TTangent.__repr__ = GraphElemRepr
  TPointSeries.__repr__ = GraphElemRepr
  TRelation.__repr__ = GraphElemRepr
  TTextLabel.__repr__ = GraphElemRepr
  TShading.__repr__ = GraphElemRepr
  TAxesView.__repr__ = GraphElemRepr
  TTopGraphElem.__repr__ = GraphElemRepr
  TStringMap.__repr__ = lambda self: repr(dict(self))
  def SetPoints(self, L): TPointList(self)[:] = L
  def SetPointData(self, L): TPointDataList(self)[:] = L
  TPointSeries.__swig_setmethods__["Points"] = SetPoints
  TPointSeries.__swig_getmethods__["Points"] = lambda self: TPointList(self)
  TPointSeries.__swig_setmethods__["PointData"] = SetPointData
  TPointSeries.__swig_getmethods__["PointData"] = lambda self: TPointDataList(self)

  def UnpackSlice(key, count):
    step = 1 if key.step is None else key.step
    if key.start is None:
      start = count-1 if not key.step is None and key.step < 0 else 0
    else:  
      start = (max(count+key.start, 0) if key.start < 0 else key.start)
    if key.stop is None:
      stop = -1 if not key.step is None and key.step < 0 else count
    else:
      stop = (max(count+key.stop, 0) if key.stop < 0 else min(key.stop, count))
    return start, stop, step

  import collections
  class MutableSequenceWithSlice(collections.MutableSequence):
    def __getitem__(self, key):
      Size = len(self)
      if isinstance(key, slice):
        start, stop, step = UnpackSlice(key, Size)
        return [self._get(i) for i in range(start, stop, step)]
      if -Size <= key < Size:
        return self._get(Size + key if key < 0 else key)
      raise IndexError("Index out of range")

    def __setitem__(self, key, value):
      if isinstance(key, slice):
        start, stop, step = UnpackSlice(key, len(self))
        if step == 1 or step == -1:
          if len(self) > 0:
            self._del(min(start, stop), abs(stop - start))
          for x in zip(range(start, start + len(value), step), value):
            self.insert(x[0], x[1])
        elif len(range(start, stop, step)) == len(value):
          for x in zip(range(start, stop, step), value):
            self._set(x[0], x[1])
        else:
          raise ValueError("attempt to assign sequence of size %d to extended slice of size %d" % (len(value), len(range(key.start, key.stop, key.step))))
      else:
        self._set(self.PointSeries.PointCount() + key if key < 0 else key, value)
    
    def __delitem__(self, key):
      if isinstance(key, slice):
        start, stop, step = UnpackSlice(key, len(self))
        if step == 1 or step == -1:
          self._del(min(start, stop), abs(stop - start))
        else:
          for i in range(start, stop, step) if step < 0 else reversed(range(start, stop, step)):
            self._del(i, 1)
      else:
        self._del(len(self) + key if key < 0 else key, 1)

    def insert(self, key, value):
      if key >= len(self):
        self.append(value)
      else:
        self._insert(max(0, len(self) + key) if key < 0 else key, value)

    def __repr__(self):
      return repr(list(self))
    
    def __eq__(self, other):
      if self is other:
        return True
      if len(other) !=  len(self):
        return False
      for i in range(len(other)):
        if other[i] != self._get(i):
          return False
      return True

    def __ne__(self, other):
      return not self == other

  class TPointList(MutableSequenceWithSlice):
    def __init__(self, PointSeries):
      self.PointSeries = PointSeries
    def _get(self, key):
      return Point._make(self.PointSeries.GetDblPoint(key))
    def __len__(self):
      return self.PointSeries.PointCount()
    def _insert(self, key, value):
      self.PointSeries.InsertDblPoint(value, key)
    def append(self, value):
      self.PointSeries.InsertDblPoint(value, -1)
    def _set(self, key, value):
      self.PointSeries.ReplaceDblPoint(value, key)
    def _del(self, index, count):
      self.PointSeries.DeletePoint(index, count)

  class TPointDataList(MutableSequenceWithSlice):
    def __init__(self, PointSeries):
      self.PointSeries = PointSeries
    def _get(self, key):
      return self.PointSeries.GetPoint(key)
    def __len__(self):
      return self.PointSeries.PointCount()
    def _insert(self, key, value):
      if len(value) == 2: value = value[0], value[1], "", ""
      self.PointSeries.InsertPoint(value, key)
    def append(self, value):
      if len(value) == 2: value = value[0], value[1], "", ""
      self.PointSeries.InsertPoint(value, -1)
    def _set(self, key, value):
      if len(value) == 2: value = value[0], value[1], "", ""
      self.PointSeries.ReplacePoint(value, key)
    def _del(self, index, count):
      self.PointSeries.DeletePoint(index, count)
}


