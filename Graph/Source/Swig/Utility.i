%module Utility

%include "std_wstring.i"
%include "std_string.i"
%include "std_complex.i"
%include "Types.i"

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
#define SWIG_module SWIG_module_Utility
%}

%inline %{
  static void BeginMultiUndo() {UndoList.BeginMultiUndo();}
  static void EndMultiUndo() {UndoList.EndMultiUndo();}
  static void LoadDefault() { Form1->LoadDefault();}
  static bool LoadFromFile(const std::wstring &FileName, bool AddToRecent = true, bool ShowErrorMessages = true)
  {
    return Form1->LoadFromFile(FileName.c_str(), AddToRecent, ShowErrorMessages);
  }

  static bool SaveToFile(const std::wstring &FileName, bool Remember = true) {return Form1->Data.Save(FileName, Remember);}
  static void ImportPointSeries(const std::wstring &FileName, char Separator = 0) {Form1->Data.ImportPointSeries(FileName, Separator);}
  static void ImportPointSeriesText(const std::string &Str, char Separator = 0) {std::istringstream Stream(Str); Form1->Data.ImportPointSeries(Stream, Separator);}
  static void Import(const std::wstring &FileName) {Form1->Data.Import(FileName);}
  static std::wstring GetText(const wchar_t *Str) {return Str[0] == 0 ? L"" : gettext(Str).c_str();}
  static void ChangeLanguage(const std::wstring &Language) {Form1->ChangeLanguage(ToUString(Language));}
  static void UpdateMenu() {Form1->UpdateMenu();}
  static void ClearCache() {Form1->Data.ClearCache();}
%}

enum TComplexFormat;
String ComplexToString(const std::complex<double> &C, unsigned Decimals, TComplexFormat ComplexFormat);
void SmoothResize(TPngImage *Image, int Width, int Height) throw(Exception);
void ConvertTransparentToAlpha(TPngImage *Image, TColor TransparentColor);
void TranslateProperties(TObject *Object);
