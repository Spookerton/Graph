/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2019 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#include "Graph.h"
#pragma hdrstop
#include "Unit1.h"
#include "TRelationFrame.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "TEvalFrame"
#pragma link "MyEdit"
#pragma resource "*.dfm"
TRelationFrame *RelationFrame;
//---------------------------------------------------------------------------
__fastcall TRelationFrame::TRelationFrame(TComponent* Owner)
  : TEvalFrame(Owner), LastPoint(0,0)
{
}
//---------------------------------------------------------------------------
/** Find the shown screen point in the equation closest to a given point.
 */
TPoint TRelationFrame::FindNearest(const TRelation *Relation, const TPoint &P, TDistType DistType)
{
  const std::vector<TPoint>& Points = Relation->GetPolylinePoints();
  unsigned MinDist = MAXUINT;
  TPoint Point(-1, -1);
  for(unsigned I = 0; I < Points.size(); I++)
  {
    unsigned Dist;
    if(DistType == dtXCoord)
    {
      if(Points[I].y != P.y)
        continue;
      Dist = std::abs(Points[I].x - P.x);
    }
    else if(DistType == dtYCoord)
    {
      if(Points[I].x != P.x)
        continue;
      Dist = std::abs(Points[I].y - P.y);
    }
    else
      Dist = Sqr(Points[I].x - P.x) + Sqr(Points[I].y - P.y);
    if(Dist < MinDist)
    {
      MinDist = Dist;
      Point = Points[I];
    }
  }
  return Point;
}
//---------------------------------------------------------------------------
/** Find the value in a vector closest to a given value.
 */
long double FindClosest(const std::vector<long double> &Values, long double Fixed)
{
  long double Result = NAN;
  long double MinDist = INF;
  for(unsigned I = 0; I < Values.size(); I++)
  {
    long double Dist = std::abs(Fixed - Values[I]);
    if(Dist < MinDist)
    {
      Result = Values[I];
      MinDist = Dist;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
/** Called when the user has entered a X- or Y-coordinate.
 *  1. Find out if a x- or y- coordinate has been entered.
 *  2. Find a list of values for the other part of the X/Y-set.
 *  3. Pick the value closest to the point where the use last clicked with the mouse (default: (0,0)).
 *  4. Show the value to the user.
 */
void TRelationFrame::Eval(const TGraphElem *Elem)
{
  if(const TRelation *Relation = dynamic_cast<const TRelation*>(Elem))
  {
    if(Edit1->Modified)
    {
      Edit1->Modified = false;
      ErrorPrefix = "x: ";
      SetEditText(Edit2, "");
      std::wstring xStr = ToWString(Edit1->Text);
      long double x = Form1->Data.Calc(xStr);
      long double LastPointY = Form1->Draw.yCoord(LastPoint.y);
      std::vector<long double> Values = Relation->Analyse(Form1->Data.Axes.yAxis.Min, Form1->Data.Axes.yAxis.Max, Form1->Draw.GetAxesRect().Height(), x, rtYValue);
      if(!Values.empty())
      {
        long double y = FindClosest(Values, Form1->Draw.yCoord(LastPoint.y));
        SetEditText(Edit2, RoundToStr(y));
        Form1->Cross->SetPos(Form1->Draw.xPoint(x), Form1->Draw.yPoint(y));
        Form1->Cross->Show();
      }
    }
    else
    {
      Edit2->Modified = false;
      ErrorPrefix = "y: ";
      SetEditText(Edit1, "");
      std::wstring yStr = ToWString(Edit2->Text);
      long double y = Form1->Data.Calc(yStr);
      std::vector<long double> Values = Relation->Analyse(Form1->Data.Axes.xAxis.Min, Form1->Data.Axes.xAxis.Max, Form1->Draw.GetAxesRect().Width(), y, rtXValue);
      if(!Values.empty())
      {
        long double x = FindClosest(Values, Form1->Draw.xCoord(LastPoint.x));
        SetEditText(Edit1, RoundToStr(x));
        Form1->Cross->SetPos(Form1->Draw.xPoint(x), Form1->Draw.yPoint(y));
        Form1->Cross->Show();
      }
    }
  }
}
//---------------------------------------------------------------------------
/** Called when the user clicks somewhere in the graphing area.
 *  This will find the nearest coordinate to the point clicked at and show to the user.
 *  It will also store the position found.
 */
void TRelationFrame::SetPoint(const TGraphElem *Elem, int X, int Y)
{
  if(const TRelation *Relation = dynamic_cast<const TRelation*>(Elem))
  {
    //Find the nearest known pixel in the equation to the selected point.
    //Use binary search to find a more accurate x-coordinate for the given y-coordinate.
    //If that fails, use binary search to find a more accurate y-coordinate for the given x-coordinate.
    TPoint Point = FindNearest(Relation, TPoint(X, Y), dtDist);

    if(Point != TPoint(-1, -1))
    {
      LastPoint = Point;
      long double x = Form1->Draw.xCoord(Point.x);
      long double y = Form1->Draw.yCoord(Point.y);
      long double x2 = Relation->Trace(Form1->Draw.xCoord(Point.x-1), Form1->Draw.xCoord(Point.x+1), y, rtXValue);
      if(!_finitel(x2))
      {
        long double y2 = Relation->Trace(Form1->Draw.yCoord(Point.y-1), Form1->Draw.yCoord(Point.y+1), x, rtYValue);
        if(!_finitel(y2))
        {
          SetEditText(Edit1, "");
          SetEditText(Edit2, "");
          Form1->Cross->Hide();
          return;
        }
        y = y2;
      }
      else
        x = x2;

      SetEditText(Edit1, RoundToStr(x));
      SetEditText(Edit2, RoundToStr(y));
      Form1->Cross->SetPos(Point.x, Point.y);
      Form1->Cross->Show();
    }
  }
}
//---------------------------------------------------------------------------

