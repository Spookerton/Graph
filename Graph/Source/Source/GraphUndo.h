/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#ifndef GraphUndoH
#define GraphUndoH
#include "Undo.h"
#include "PyGraph.h"

namespace Graph
{
class TUndoAdd
{
	TGraphElemPtr GraphElem;
public:
	TUndoAdd(const TGraphElemPtr &AGraphElem)
    : GraphElem(AGraphElem) {}
  void operator()(TUndoList &UndoList) const;
};

class TUndoReplace
{
  TGraphElemPtr OldElem;
	TGraphElemPtr NewElem;
public:
	TUndoReplace(const TGraphElemPtr &AOldElem, const TGraphElemPtr &ANewElem)
		: OldElem(AOldElem), NewElem(ANewElem)
	{
    OldElem->SetData(NULL); //Makes elem invalid from plugins
	}

	void operator()(TUndoList &UndoList) const;
};

class TUndoChange
{
  TGraphElemPtr Elem;
	TGraphElemPtr ElemCopy;
public:
	TUndoChange(const TGraphElemPtr &AElem)
		: Elem(AElem), ElemCopy(AElem->Clone())
	{}
	TUndoChange(const TGraphElemPtr &AElem, const TGraphElemPtr &AElemCopy)
		: Elem(AElem), ElemCopy(AElemCopy)
	{}

	void operator()(TUndoList &UndoList) const;
  const TGraphElemPtr& GetElem() const {return Elem;}
};


class TUndoDel
{
	int Index;
	TGraphElemPtr Elem;
	TGraphElemPtr Parent;
public:
	TUndoDel(const TGraphElemPtr &AElem, const TGraphElemPtr &AParent, int AIndex)
		: Index(AIndex), Elem(AElem), Parent(AParent)  {Elem->SetData(NULL);}  //Makes elem invalid from plugins
	void operator()(TUndoList &UndoList) const;
};

template<class T>
class TUndoObject
{
  T &OrgObject;
	T Object;
public:
  TUndoObject(T &AOrgObject) : OrgObject(AOrgObject), Object(AOrgObject) {}
  void operator()(TUndoList &UndoList)
  {
    UndoList.Push(TUndoObject<T>(OrgObject));
    OrgObject = Object;
  }
};

class TUndoAxes
{
  TAxes Axes;
  TData &Data;
public:
  TUndoAxes(TData &AData) : Data(AData), Axes(AData.Axes) {}
  void operator()(TUndoList &UndoList);
};

class TUndoCustomFunctions
{
  boost::shared_ptr<TCustomFunctions> CustomFunctions;
  TData &Data;
public:
  TUndoCustomFunctions(TData &AData) //This will steal the custom functions from AData
    : Data(AData), CustomFunctions(new TCustomFunctions)
  {
    CustomFunctions->Swap(AData.CustomFunctions);
  }
	void operator()(TUndoList &UndoList);
};

class TUndoSetCustomFunction
{
  TData &Data;
  std::wstring Name;
  Func32::TArgType Args;
  std::wstring Equation;
public:
  TUndoSetCustomFunction(TData &AData, const std::wstring &AName);
	void operator()(TUndoList &UndoList);
};

class TUndoMove
{
  TData &Data;
  TGraphElemPtr Elem;
  unsigned Index;
public:
  TUndoMove(TData &AData, const TGraphElemPtr &AElem, unsigned AIndex)
    : Data(AData), Elem(AElem), Index(AIndex) {}
	void operator()(TUndoList &UndoList);
};

class TUndoAddPoint
{
	TPointSeriesPtr Series;
public:
	TUndoAddPoint(TPointSeriesPtr ASeries) : Series(ASeries) {}
	void operator()(TUndoList &UndoList);
};

class TUndoDelPoint
{
	TPointSeriesPtr Series;
	TPointSeriesPoint Point;
public:
	TUndoDelPoint(TPointSeriesPtr ASeries, const TPointSeriesPoint &APoint)
		: Series(ASeries), Point(APoint) {}
	void operator()(TUndoList &UndoList);
};

template<class T>
TUndoObject<T> MakeUndoObject(T &OrgObject)
{
  return TUndoObject<T>(OrgObject);
}

class TUndoChangeShowInLegend
{
  TGraphElemPtr Elem;
  bool ShowInLegend;
public:
  TUndoChangeShowInLegend(const TGraphElemPtr &AElem)
    : Elem(AElem), ShowInLegend(AElem->GetShowInLegend()) {}
  void operator()(TUndoList &UndoList);
};

template<typename T>
class TUndoChangeGraphElemSetting
{
  typedef T (__closure *TGetFunc)();
  typedef void (__closure *TSetFunc)(T);

  TGetFunc GetFunc;
  TSetFunc SetFunc;
  TGraphElemPtr Elem;
  T Value;
public:
  TUndoChangeGraphElemSetting(const TGraphElemPtr &AElem, const TGetFunc &AGetFunc, const TSetFunc &ASetFunc)
    : Elem(AElem), GetFunc(AGetFunc), SetFunc(ASetFunc)
  {
    Value = GetFunc();
  }
  void operator()(TUndoList &UndoList)
  {
    UndoList.Push(TUndoChangeGraphElemSetting<T>(Elem, GetFunc, SetFunc));
    SetFunc(Value);
    Python::ExecutePluginEvent(Python::peChanged, Elem);
  }
};

extern TUndoList UndoList;
//---------------------------------------------------------------------------
} //namespace Graph
#endif
