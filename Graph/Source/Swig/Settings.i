%module Settings
%include "std_wstring.i"
%include "attribute.i"
%import "Types.i"

%begin %{
#define WRAP_PYOBJECTS
#include "Graph.h"
#include "Unit1.h"
#include "Python.hpp"
#include "PythonBind.h"
#include "PyGraph.h"
#include "PyVclObject.h"
#pragma warn -8060

//Bug in BCB XE6 64 bit: several extern "C" functions with static variables with the same name seems to be merged.
#define SWIG_module SWIG_module_Settings
%}

//Don't allow users to add attributes to proxy objects
%pythonnondynamic;

%inline %{
static Graph::TData* GetData() {return &Form1->Data;}
%}

%define VAR_WITH_SET_CODE(class, type, member, pre_code, post_code)
%extend class
{
  type member; 
}

%{
  #define class ## _ ## member ## _get(self) self->member
  #define class ## _ ## member ## _set(self, value) {pre_code} self->member = value; {post_code}  
%}
%enddef

namespace Func32
{
enum TTrigonometry {Radian, Degree};
}

namespace Graph
{
enum TAxesStyle {asNone, asCrossed, asBoxed};
enum TLegendPlacement {lpCustom, lpTopRight, lpBottomRight, lpTopLeft, lpBottomLeft};
enum TNumberPlacement {npCenter, npBefore};
}

#define VAR_CLEAR_CACHE(class, type, member) VAR_WITH_SET_CODE(class, type, member, PushUndoAxes(); Form1->Data.ClearCache();,)
#define VAR_PUSH_UNDO_AXES(class, type, member)  VAR_WITH_SET_CODE(class, type, member, PushUndoAxes();,)
VAR_CLEAR_CACHE(TAxis, double, Min)
VAR_CLEAR_CACHE(TAxis, double, Max)
VAR_CLEAR_CACHE(TAxis, bool, LogScl)
VAR_PUSH_UNDO_AXES(TAxis, bool, MultipleOfPi)
VAR_PUSH_UNDO_AXES(TAxis, bool, ShowLabel)
VAR_PUSH_UNDO_AXES(TAxis, bool, ShowNumbers)
VAR_PUSH_UNDO_AXES(TAxis, bool, ShowTicks)
VAR_PUSH_UNDO_AXES(TAxis, bool, ShowGrid)
VAR_PUSH_UNDO_AXES(TAxis, bool, AutoTick)
VAR_PUSH_UNDO_AXES(TAxis, bool, AutoGrid)
VAR_PUSH_UNDO_AXES(TAxis, std::wstring, Label)
VAR_PUSH_UNDO_AXES(TAxis, double, AxisCross)
VAR_PUSH_UNDO_AXES(TAxis, double, TickUnit)
VAR_PUSH_UNDO_AXES(TAxis, double, GridUnit)
VAR_PUSH_UNDO_AXES(TAxis, bool, Visible)
VAR_PUSH_UNDO_AXES(TAxis, bool, ShowPositiveArrow)
VAR_PUSH_UNDO_AXES(TAxis, bool, ShowNegativeArrow)
VAR_PUSH_UNDO_AXES(TAxis, Graph::TNumberPlacement, NumberPlacement)

%nodefault TAxis;
%nodefault TAxes;
struct TAxis
{};

enum TGridStyle
{
  gsLines, gsDots
};

VAR_WITH_SET_CODE(TAxes, Func32::TTrigonometry, Trigonometry, PushUndoAxes(); Form1->Data.ClearCache();, Form1->Data.Update();)
VAR_CLEAR_CACHE(TAxes, std::wstring, Title)
VAR_PUSH_UNDO_AXES(TAxes, TColor, AxesColor)
VAR_PUSH_UNDO_AXES(TAxes, TColor, GridColor)
VAR_PUSH_UNDO_AXES(TAxes, TColor, BackgroundColor)
VAR_PUSH_UNDO_AXES(TAxes, bool, ShowLegend)
VAR_CLEAR_CACHE(TAxes, Graph::TAxesStyle, AxesStyle)
VAR_PUSH_UNDO_AXES(TAxes, Graph::TLegendPlacement, LegendPlacement)
VAR_PUSH_UNDO_AXES(TAxes, Func32::TDblPoint, LegendPos)
VAR_CLEAR_CACHE(TAxes, bool, CalcComplex)
VAR_PUSH_UNDO_AXES(TAxes, TGridStyle, GridStyle)

struct TAxes
{
  TAxis xAxis;
  TAxis yAxis;
  TFont*const NumberFont;
  TFont*const LabelFont;
  TFont*const LegendFont;
  TFont*const TitleFont;
};

enum TComplexFormat {cfReal,cfRectangular,cfPolar};

#define VAR_PUSH_UNDO_PROPERTY(class, type, member)  VAR_WITH_SET_CODE(class, type, member, PushUndoOptions();,)
VAR_PUSH_UNDO_PROPERTY(TOptions, int, RoundTo) 
VAR_PUSH_UNDO_PROPERTY(TOptions, bool, SavePos)
VAR_PUSH_UNDO_PROPERTY(TOptions, TComplexFormat, ComplexFormat)
VAR_PUSH_UNDO_PROPERTY(TOptions, bool, CheckForUpdate)
VAR_PUSH_UNDO_PROPERTY(TOptions, TDefaultData, DefaultFunction)
VAR_PUSH_UNDO_PROPERTY(TOptions, TDefaultData, DefaultPoint)
VAR_PUSH_UNDO_PROPERTY(TOptions, TDefaultData, DefaultPointLine)
VAR_PUSH_UNDO_PROPERTY(TOptions, TDefaultData, DefaultShade)
VAR_PUSH_UNDO_PROPERTY(TOptions, TDefaultData, DefaultTrendline)
VAR_PUSH_UNDO_PROPERTY(TOptions, TDefaultData, DefaultRelation)
VAR_PUSH_UNDO_PROPERTY(TOptions, TDefaultData, DefaultTangent)
VAR_PUSH_UNDO_PROPERTY(TOptions, TDefaultData, DefaultDif)
VAR_PUSH_UNDO_PROPERTY(TOptions, std::wstring, Language)
VAR_WITH_SET_CODE(TOptions, unsigned, FontScale,PushUndoOptions();,Form1->ScaleImages();)
VAR_PUSH_UNDO_PROPERTY(TOptions, bool, CustomDecimalSeparator) 
VAR_PUSH_UNDO_PROPERTY(TOptions, wchar_t, DecimalSeparator)

%nodefault TOptions;
struct TOptions
{
  TFont*const DefaultPointLabelFont;
  TFont*const DefaultLabelFont;
};

%nodefault TGuiFormatSettings;
struct TGuiFormatSettings
{
  std::wstring CartesianPointFormat;
  std::wstring DegreePointFormat;
  std::wstring RadianPointFormat;
};

#define VAR_SET_MODIFIED(class, type, member) VAR_WITH_SET_CODE(class, type, member, Form1->Data.AbortUpdate();, Form1->Data.SetModified();)
VAR_SET_MODIFIED(TPlotSettings, int, AxisWidth)
VAR_SET_MODIFIED(TPlotSettings, int, GridWidth)
VAR_SET_MODIFIED(TPlotSettings, int, xNumberDist)
VAR_SET_MODIFIED(TPlotSettings, int, yNumberDist)
VAR_SET_MODIFIED(TPlotSettings, int, TickWidth)
VAR_SET_MODIFIED(TPlotSettings, int, TickLength)
VAR_WITH_SET_CODE(TPlotSettings, int, ThreadCount,,if(value >= 1) Form1->Draw.SetThreadCount(value);)
VAR_SET_MODIFIED(TPlotSettings, int, SmoothingMode)

%nodefault TPlotSettings;
struct TPlotSettings
{
};

VAR_WITH_SET_CODE(TGuiSettings, unsigned, MenuIconSize,,Form1->ScaleImages();)
VAR_WITH_SET_CODE(TGuiSettings, unsigned, ToolBarIconSize,,Form1->ScaleImages();)
%nodefault TGuiSettings;
struct TGuiSettings
{
  double MajorZoomIn;
  double MinorZoomIn;
  double MajorZoomOut;
  double MinorZoomOut;
  double MajorStepSize;
  double MinorStepSize;
  double MouseZoomIn;
  double MouseZoomOut;
  TFont*const Font;
};

const TOptions &Options;
const TGuiFormatSettings &GuiFormatSettings;
const TPlotSettings &PlotSettings;
const TGuiSettings &GuiSettings;

%nodefaultctor TData;
%nodefaultdtor TData;
struct TData
{
  TAxes Axes;
};

%{
  PyObject* ToPyObject(TData &Data)
  {
    Python::TLockGIL Dummy;
    return Python::IsPythonInstalled() ? SWIG_NewPointerObj(SWIG_as_voidptr(&Data), SWIGTYPE_p_TData, 0) : NULL;
  }
%}

%{
  static void PushUndoAxes()
  {
    Form1->Data.AbortUpdate();
    if(Form1->ForceUndo() || UndoList.Peek<TUndoAxes>() == NULL)
  		UndoList.Push(TUndoAxes(Form1->Data));
		Form1->Data.SetModified();
  }

  static void PushUndoOptions()
  {
    if(Form1->ForceUndo() || UndoList.Peek<TUndoObject<TOptions> >() == NULL)
  		UndoList.Push(MakeUndoObject(Options));
  }
%}

%pythoncode
{
  import vcl
  Data = GetData()
  
  def SwigRepr(self):
    First = True
    S = self.__class__.__module__ + "." + self.__class__.__name__ + "("
    for Name in self.__swig_getmethods__:
      if not First:
        S += ", "
      S += Name + "=" + repr(self.__swig_getmethods__[Name](self))
      First = False
    S += ")"
    return S
    
  TAxes.__repr__ = SwigRepr
  TAxis.__repr__ = SwigRepr
  TOptions.__repr__ = SwigRepr
  TPlotSettings.__repr__ = SwigRepr
  TGuiSettings.__repr__ = SwigRepr
  TGuiFormatSettings.__repr__ = SwigRepr
}
