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
#include <algorithm>
#include "float.h"
#include "Unit1.h"
#include "TAreaFrame.h"
//---------------------------------------------------------------------------
#pragma link "MyEdit"
#pragma link "UpDownEx"
#pragma link "TEvalFrame"
#pragma link "GridPanelEx"
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
__fastcall TAreaFrame::TAreaFrame(TComponent* Owner)
        : TEvalFrame(Owner)
{
}
//---------------------------------------------------------------------------
void TAreaFrame::EvalArea(const TGraphElem *GraphElem)
{
  Edit3->Text = "";
  Form1->IPolygon1->Clear();

  long double From = Form1->Data.Calc(ToWString(Edit1->Text));
  long double To = Form1->Data.Calc(ToWString(Edit2->Text));

  if(const TBaseFuncType *Func = dynamic_cast<const TBaseFuncType*>(GraphElem))
    EvalArea(Func, From, To);
  else if(const TPointSeries *PointSeries = dynamic_cast<const TPointSeries*>(GraphElem))
    EvalArea(PointSeries, From, To);

  Form1->IPolygon1->PolygonType = ptPolygon;
	Form1->IPolygon1->Visible = true;
}
//---------------------------------------------------------------------------
void TAreaFrame::EvalArea(const TBaseFuncType *Func, long double From, long double To)
{
  Edit3->Text = RoundToStr(Func->CalcArea(From, To));
  const std::vector<Func32::TCoordSet<> > &CoordList = Func->GetCoordList();

  //Don't try to acces CoordList when function may be updated.
  if(!Func->GetVisible() || CoordList.empty() || Form1->Draw.Updating())
    return;

  if(From > To)
    std::swap(From, To);

  unsigned N1 = std::lower_bound(CoordList.begin(), CoordList.end(), From, TCompCoordSet()) - CoordList.begin();
  unsigned N2 = std::lower_bound(CoordList.begin() + N1, CoordList.end(), To, TCompCoordSet()) - CoordList.begin();

  Func32::TCoord<long double> Min, Max;
  try
  {
    if(_finitel(From))
      Min = Func->Eval(From);
    else
      Min = CoordList[N1];
  }
  catch(Func32::ECalcError &E)
  {
    Min = CoordList[N1];
  }

	try
  {
    if(_finitel(To))
      Max = Func->Eval(To);
    else
      Max = CoordList[N2-1];
  }
  catch(Func32::ECalcError &E)
  {
    Max = CoordList[N2-1];
  }

  if(N1 != N2)
  {
    Form1->IPolygon1->AddPoint(Form1->Draw.xyPoint(Min.x, Min.y));
    Form1->IPolygon1->AddPoints(&Func->GetPoints()[N1], N2 - N1);
    Form1->IPolygon1->AddPoint(Form1->Draw.xyPoint(Max.x, Max.y));
  }

  if(const TPolFunc *PolFunc = dynamic_cast<const TPolFunc*>(Func))
  {
    if(N1 != N2)
      Form1->IPolygon1->AddPoint(TPoint(Form1->Draw.xyPoint(0, 0)));
  }
  else
  {
    Form1->IPolygon1->AddPoint(Form1->Draw.xyPoint(Max.x, 0));
    Form1->IPolygon1->AddPoint(Form1->Draw.xyPoint(Min.x, 0));
  }

  Form1->IPolygon1->Pen->Width = Func->GetSize();
}
//---------------------------------------------------------------------------
void TAreaFrame::EvalArea(const TPointSeries *PointSeries, long double From, long double To)
{
  TPointSeries::TPointList::const_iterator FromIter = PointSeries->FindPoint(From);
  TPointSeries::TPointList::const_iterator ToIter = PointSeries->FindPoint(To);
  Func32::TDblPoint FromCoord = FindCoord(FromIter, From);
  Func32::TDblPoint ToCoord = FindCoord(ToIter, To);

  Form1->IPolygon1->AddPoint(Form1->Draw.xyPoint(Func32::TDblPoint(FromCoord.x, 0)));
  Form1->IPolygon1->AddPoint(Form1->Draw.xyPoint(FromCoord));
  if(ToIter > FromIter)
    ++FromIter, ++ToIter;

  for(TPointSeries::TPointList::const_iterator Iter = FromIter; Iter != ToIter; ToIter > FromIter ? ++Iter : --Iter)
    Form1->IPolygon1->AddPoint(Form1->Draw.xyPoint(*Iter));
  Form1->IPolygon1->AddPoint(Form1->Draw.xyPoint(ToCoord));
  Form1->IPolygon1->AddPoint(Form1->Draw.xyPoint(Func32::TDblPoint(ToCoord.x, 0)));

  //Area between x-axis and line from P1 to P2: A=(x2-x1)*(y2+y1)/2
  long double Area = 0;
  if(FromIter == ToIter)
    Area = (ToCoord.x - FromCoord.x) * (ToCoord.y + FromCoord.y) / 2;
  else
  {
    Area = (FromIter->x - FromCoord.x) * (FromIter->y + FromCoord.y) / 2;
    if(ToIter > FromIter)
		{
      --ToIter;
      for(TPointSeries::TPointList::const_iterator Iter = FromIter; Iter != ToIter; ++Iter)
        Area += ((Iter+1)->x - Iter->x) * ((Iter+1)->y + Iter->y) / 2;
    }
    else
    {
      ++ToIter;
      for(TPointSeries::TPointList::const_iterator Iter = FromIter; Iter != ToIter; --Iter)
        Area += ((Iter-1)->x - Iter->x) * ((Iter-1)->y + Iter->y) / 2;
    }
    Area += (ToCoord.x - ToIter->x) * (ToCoord.y + ToIter->y) / 2;
  }
  Edit3->Text = RoundToStr(Area);
}
//---------------------------------------------------------------------------
void TAreaFrame::EvalArc(const TGraphElem *GraphElem)
{
  Edit3->Text = "";
  Form1->IPolygon1->Clear();

  double Min = Form1->Data.Calc(ToWString(Edit1->Text));
  double Max = Form1->Data.Calc(ToWString(Edit2->Text));

  if(const TBaseFuncType *Func = dynamic_cast<const TBaseFuncType*>(GraphElem))
  {
    const std::vector<Func32::TCoordSet<> > &CoordList = Func->GetCoordList();
		Edit3->Text = RoundToStr(Func->CalcArcLength(Min, Max));

    //Don't try to acces sList when function may be updated.
    if(!GraphElem->GetVisible() || Form1->Draw.Updating())
      return;

    unsigned N1 = std::lower_bound(CoordList.begin(), CoordList.end(), Min, TCompCoordSet()) - CoordList.begin();
    unsigned N2 = std::upper_bound(CoordList.begin() + N1, CoordList.end(), Max, TCompCoordSet()) - CoordList.begin();
    if(N1 != N2)
      Form1->IPolygon1->AddPoints(&Func->GetPoints()[N1], N2 - N1);
    Form1->IPolygon1->Pen->Width = Func->GetSize();
  }

  Form1->IPolygon1->PolygonType = ptPolyline;
  Form1->IPolygon1->Visible = true;
}
//---------------------------------------------------------------------------
void TAreaFrame::SetPoint(const TGraphElem *Elem, int X, int Y)
{
  if(const TBaseFuncType *Func = dynamic_cast<const TBaseFuncType*>(Elem))
  {
    double t = FindNearestPoint(Func, X, Y); //Returns NAN if no point found
    if(_isnan(t))
    {
      Form1->Cross->Hide();
      Edit1->Text = "";
      Edit2->Text = "";
    }
    else
    {
      Edit1->Text = RoundToStr(t);
      Edit2->Text = Edit1->Text;
    }
  }
  else if(const TPointSeries *PointSeries = dynamic_cast<const TPointSeries*>(Elem))
    Edit1->Text = RoundToStr(Form1->Draw.xCoord(X));
}
//---------------------------------------------------------------------------
void TAreaFrame::SetEndPoint(const TGraphElem *Elem, int X, int Y)
{
  if(const TBaseFuncType *Func = dynamic_cast<const TBaseFuncType*>(Elem))
  {
    double t = FindNearestPoint(Func, X, Y);
    Edit2->Text = _isnan(t) ? String("") : RoundToStr(t);
  }
  else if(const TPointSeries *PointSeries = dynamic_cast<const TPointSeries*>(Elem))
    Edit2->Text = RoundToStr(Form1->Draw.xCoord(X));
}
//---------------------------------------------------------------------------

