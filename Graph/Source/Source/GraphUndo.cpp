/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2013 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#include "Graph.h"
#pragma hdrstop
#include "GraphUndo.h"
#include "PyGraph.h"
//---------------------------------------------------------------------------
namespace Graph
{
TUndoList UndoList(50);
//---------------------------------------------------------------------------
void TUndoReplace::operator()(TUndoList &UndoList) const
{
  OldElem->SetData(&NewElem->GetData());
  TData::Replace(NewElem, OldElem);
  UndoList.Push(TUndoReplace(NewElem, OldElem));
  Python::ExecutePluginEvent(Python::peChanged, OldElem);
}
//---------------------------------------------------------------------------
void TUndoChange::operator()(TUndoList &UndoList) const
{
  Elem->Swap(*ElemCopy);
  UndoList.Push(*this);
  Elem->Update();
  Python::ExecutePluginEvent(Python::peChanged, Elem);
}
//---------------------------------------------------------------------------
void TUndoDel::operator()(TUndoList &UndoList) const
{
  Elem->SetData(&Parent->GetData());
  UndoList.Push(TUndoAdd(Elem));
  Parent->InsertChild(Elem, Index);
  Python::ExecutePluginEvent(Python::peNewElem, Elem);
}
//---------------------------------------------------------------------------
void TUndoAdd::operator()(TUndoList &UndoList) const
{
  Python::ExecutePluginEvent(Python::peDelete, GraphElem);
  UndoList.Push(TUndoDel(GraphElem, GraphElem->GetParent(), TData::GetIndex(GraphElem)));
  TData::Delete(GraphElem);
}
//---------------------------------------------------------------------------
void TUndoAxes::operator()(TUndoList &UndoList)
{
  UndoList.Push(TUndoAxes(Data));
  Data.Axes = Axes;
  Data.ClearCache();
  Python::ExecutePluginEvent(Python::peAxesChanged);
  Data.Update(); //In case trigonmetry has changed
}
//---------------------------------------------------------------------------
void TUndoCustomFunctions::operator()(TUndoList &UndoList)
{
  Data.CustomFunctions.Swap(*CustomFunctions);
  UndoList.Push(*this);
  Python::ExecutePluginEvent(Python::peCustomFunctionsChanged);
  Data.Update();
  Data.ClearCache();
}
//---------------------------------------------------------------------------
TUndoSetCustomFunction::TUndoSetCustomFunction(TData &AData, const std::wstring &AName)
    : Data(AData), Name(AName)
{
  TCustomFunctions::TConstIterator Iter = Data.CustomFunctions.GetIter(AName);
  if(Iter != Data.CustomFunctions.End())
  {
    Args = Iter->Arguments;
    Equation = Iter->Text;
  }
}
//---------------------------------------------------------------------------
void TUndoSetCustomFunction::operator()(TUndoList &UndoList)
{
  UndoList.Push(TUndoSetCustomFunction(Data, Name));
  if(Equation.empty())
    Data.CustomFunctions.Delete(Name);
  else
    Data.CustomFunctions.Add(Name, Args, Equation);
  Python::ExecutePluginEvent(Python::peCustomFunctionsChanged);
  Data.Update();
  Data.ClearCache();
}
//---------------------------------------------------------------------------
void TUndoMove::operator()(TUndoList &UndoList)
{
  const TGraphElemPtr &Parent = Elem->GetParent();
  int OldIndex = Parent->GetChildIndex(Elem);
  UndoList.Push(TUndoMove(Data, Elem, OldIndex));
  Parent->RemoveChild(OldIndex);
  Parent->InsertChild(Elem, Index);
  Python::ExecutePluginEvent(Python::peMoved, Elem);
}
//---------------------------------------------------------------------------
void TUndoDelPoint::operator()(TUndoList &UndoList)
{
  Series->InsertPoint(Point);
  UndoList.Push(TUndoAddPoint(Series));
  Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(Series));
}
//---------------------------------------------------------------------------
void TUndoAddPoint::operator()(TUndoList &UndoList)
{
	UndoList.Push(TUndoDelPoint(Series, Series->GetPoint(Series->PointCount()-1)));
	Series->DeletePoint(Series->PointCount()-1);
  Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(Series));
}
//---------------------------------------------------------------------------
void TUndoChangeShowInLegend::operator()(TUndoList &UndoList)
{
  UndoList.Push(TUndoChangeShowInLegend(Elem));
  Elem->SetShowInLegend(ShowInLegend);
  Python::ExecutePluginEvent(Python::peChanged, Elem);
}
//---------------------------------------------------------------------------
} //namespace Graph
