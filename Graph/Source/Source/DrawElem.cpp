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
#include "Unit1.h"
#include "DrawElem.h"
#include <complex>
#include <cfloat>
#include <values.h>
#include <algorithm>
#include <ctime>
#include <float.h>
#include "PointSelect.h"
#include "IGraphic.h"
#include <boost/format.hpp>
#pragma warn -8072 //Disable warning: Suspicous pointer arithmetic
#include <boost/tr1/complex.hpp>

namespace Graph
{
using std::real;
using std::imag;
#ifdef _WIN64
inline long double imag(long double v) {return 0;}
inline long double real(long double v) {return v;}
#endif
//---------------------------------------------------------------------------
template<typename T> inline bool TDrawElem::IsError(Func32::TErrorCode ErrorCode, const Func32::TCoord<T> &Coord)
{
  return ErrorCode != Func32::ecNoError || std::abs(imag(Coord.x)) > MIN_ZERO || std::abs(imag(Coord.y)) > MIN_ZERO ||
    (Axes.xAxis.LogScl && real(Coord.x) <= 0) || (Axes.yAxis.LogScl && real(Coord.y) <= 0);
}
//---------------------------------------------------------------------------
TDrawElem::TDrawElem(TDraw *ADraw, const TRect &AAxesRect, const TAxes &AAxes, TContext &AContext)
 : AxesRect(AAxesRect),
   Draw(ADraw), Context(AContext), Axes(AAxes), PlotIndex(0)
{
}
//---------------------------------------------------------------------------
inline int TDrawElem::xPoint(long double x) const {return Draw->xPoint(x);}
inline int TDrawElem::yPoint(long double y) const {return Draw->yPoint(y);}
inline TPoint TDrawElem::xyPoint(const Func32::TDblPoint &P) const {return Draw->xyPoint(P);}
inline int TDrawElem::SizeScale(int I){return I*Draw->SizeMul+0.5;}
//---------------------------------------------------------------------------
void TDrawElem::DrawNext()
{
  try
  {
    const TTopGraphElemPtr &Top = Draw->Data->GetTopElem();
    unsigned Count = Top->ChildCount();
    while(PlotIndex < Count)
    {
      TGraphElemPtr Elem = Top->GetChild(PlotIndex);
      if(Elem->IsUpdateFinished())
      {
        unsigned Count = Elem->ChildCount();
        for(unsigned N = 0; N < Count; N++)
        {
          const TGraphElemPtr &Child = Elem->GetChild(N);
          if(Child->GetVisible())
            Child->Accept(*this);
        }
        if(Elem->GetVisible())
          Elem->Accept(*this);
        PlotIndex++;

        //Ensure interface gets updated
        Application->ProcessMessages();
      }
      else
        break;
    }
  }
  catch(EPrinter &)
  {
    ShowStatusError(LoadRes(RES_PRINTING_ABORTED));
  }
}
//---------------------------------------------------------------------------
void TDrawElem::Visit(TBaseFuncType &Func)
{
  Draw->SetClippingRegion();

  const std::vector<TPoint> &Points = Func.GetPoints();
  const std::vector<unsigned> &PointNum = Func.GetPointNum();
  if(Func.GetDrawType() == dtDots)
    Draw->DrawPolydots(Points.begin(), Points.end(), Func.GetSize(), Func.GetColor());
  else
  {
    std::vector<TPoint>::const_iterator iter = Points.begin();
    for(unsigned I = 0; I < PointNum.size(); I++)
    {
      Draw->DrawPolyline(iter, iter + PointNum[I], Func.GetStyle(), Func.GetSize(), Func.GetColor());
      iter += PointNum[I];
    }
  }

  DrawEndPoints(Func);
}
//---------------------------------------------------------------------------
void TDrawElem::Visit(TShading &Shade)
{
	if(!Shade.GetRegion())
	  return;
  Draw->SetClippingRegion();

  Context.SetBrush(Shade.GetBrushStyle(), Shade.GetColor());

  //Draw marked area, and clip at AxesRect
  Context.DrawRegion(*Shade.GetRegion());
  Draw->SetClippingRegion(); //Set clipping region again; DrawRegion() seems to remove it

  Context.SetBrush(bsSolid, Shade.GetColor());
  if(Shade.GetMarkBorder())
    Context.DrawFrameRegion(*Shade.GetRegion(), 1);
}
//---------------------------------------------------------------------------
void TDrawElem::DrawEndPoints(const TBaseFuncType &Func)
{
  const std::vector<TPoint> &Points = Func.GetPoints();
  if(Func.GetEndpointStyle().first != 0)
  {
    long double t = Func.GetFrom().Value;
    if(_finitel(t))
      DrawEndPoint(Func, t, Func.GetEndpointStyle().first, true);
    else
    {
      //If a start value has not been specified, search for the first point inside the visible area
      //that is close to the border, and show the end point here.
      for(unsigned I = 0; I < Points.size(); I++)
        if(InsideRect(AxesRect, Points[I]))
        {
          if(!IsInRange(Points[I].x, AxesRect.Left + 5, AxesRect.Right - 5) ||
             !IsInRange(Points[I].y, AxesRect.Top + 5, AxesRect.Bottom - 5))
          {
            t = Func.GetCoordList()[I].t;
            TPoint Pos = Points[I];
            if(I > 0)
              //This only works because we know the function is a standard function
              Pos = TContext::ClipLine(Points[I-1], Points[I], AxesRect);
            DrawEndPoint(Func, t, Pos, Func.GetEndpointStyle().first, true);
          }
          break;
        }
    }
  }

  if(Func.GetEndpointStyle().second != 0)
  {
    long double t = Func.GetTo().Value;
    if(_finitel(t))
      DrawEndPoint(Func, t, Func.GetEndpointStyle().second, false);
    else if(!Points.empty())
    {
      //If a start value has not been specified, search for the first point inside the visible area
      //that is close to the border, and show the end point here.
      for(unsigned I = Points.size()-1; I > 0; I--)
        if(InsideRect(AxesRect, Points[I]))
        {
          if(!IsInRange(Points[I].x, AxesRect.Left + 5, AxesRect.Right - 5) ||
             !IsInRange(Points[I].y, AxesRect.Top + 5, AxesRect.Bottom - 5))
          {
            t = Func.GetCoordList()[I].t;
            TPoint Pos = Points[I];
            if(I < Points.size() - 1)
              //This only works because we know the function is a standard function
              Pos = TContext::ClipLine(Points[I+1], Points[I], AxesRect);
            DrawEndPoint(Func, t, Pos, Func.GetEndpointStyle().second, false);
          }
          break;
        }
    }
  }
}
//---------------------------------------------------------------------------
void TDrawElem::DrawEndPoint(const TBaseFuncType &Func, long double t, const TPoint &Pos, unsigned Style, bool InvertArrow)
{
  if(_finitel(t))
    try
    {
      Func32::TCoord<long double> Coord = Func.GetFunc().Calc(t);
      if(IsError(Func32::ecNoError, Coord))
        return; //Invalid coordinate because of log axes

      switch(Style)
      {
        case 1:
          DrawPoint(Context, Pos, 0, Func.GetColor(), Axes.BackgroundColor, SizeScale(Func.GetSize() + 3));
          break;

        case 2:
          DrawPoint(Context, Pos, 0, Func.GetColor(), Func.GetColor(), SizeScale(Func.GetSize() + 3));
          break;

        case 3:
          if(InvertArrow)
						DrawArrow(Pos, Func.GetFunc().CalcAngleSlope(t) + M_PI, Func.GetColor(), Func.GetSize());
					else
            DrawArrow(Pos, Func.GetFunc().CalcAngleSlope(t), Func.GetColor(), Func.GetSize());
          break;

        case 4:
          DrawHalfCircle(Pos, Func.GetFunc().CalcAngleSlope(t), Func.GetColor(), Func.GetSize());
          break;

        case 5:
          DrawHalfCircle(Pos, Func.GetFunc().CalcAngleSlope(t) + M_PI, Func.GetColor(), Func.GetSize());
          break;

        case 6:
          DrawSquareBracket(Pos, Func.GetFunc().CalcAngleSlope(t), Func.GetColor(), Func.GetSize());
          break;

        case 7:
          DrawSquareBracket(Pos, Func.GetFunc().CalcAngleSlope(t) + M_PI, Func.GetColor(), Func.GetSize());
          break;
      }
    }
    catch(Func32::EFuncError&)
    {
    }
}
//---------------------------------------------------------------------------
void TDrawElem::DrawEndPoint(const TBaseFuncType &Func, long double t, unsigned Style, bool InvertArrow)
{
  if(_finitel(t))
    try
    {
      Func32::TCoord<long double> Coord = Func.GetFunc().Calc(t);
      if(IsError(Func32::ecNoError, Coord))
        return; //Invalid coordinate because of log axes

      DrawEndPoint(Func, t, Draw->xyPoint(Coord), Style, InvertArrow);
    }
    catch(Func32::EFuncError&)
    {
    }
}
//---------------------------------------------------------------------------
void TDrawElem::Visit(TTextLabel &Label)
{
  if(_isnan(Label.GetPos().first.Value) || _isnan(Label.GetPos().second.Value))
    return;

  TPoint Pos;
  switch(Label.GetPlacement())
  {
    case lpAboveX:
      Pos = TPoint(AxesRect.Right - Label.GetRect().Width() + 2, Draw->yPixelCross - Label.GetRect().Height() - 4);
      break;

    case lpBelowX:
      Pos = TPoint(AxesRect.Right - Label.GetRect().Width() + 2, Draw->yPixelCross);
      break;

    case lpRightOfY:
      Pos = TPoint(Draw->xPixelCross + 12, AxesRect.Top + 1);
      break;

    case lpLeftOfY:
      Pos = TPoint(Draw->xPixelCross - Label.GetRect().Width(), AxesRect.Top + 1);
      break;

    case lpUserTopLeft:
    default:
      Pos = Draw->xyPoint(Label.GetPos().first.Value, Label.GetPos().second.Value);
      break;

    case lpUserTopRight:
      Pos = Draw->xyPoint(Label.GetPos().first.Value, Label.GetPos().second.Value);
      Pos.x -= Label.GetRect().Width();
      break;

    case lpUserBottomLeft:
      Pos = Draw->xyPoint(Label.GetPos().first.Value, Label.GetPos().second.Value);
      Pos.y -= Label.GetRect().Height();
      break;

    case lpUserBottomRight:
      Pos = Draw->xyPoint(Label.GetPos().first.Value, Label.GetPos().second.Value);
      Pos.x -= Label.GetRect().Width();
      Pos.y -= Label.GetRect().Height();
      break;
  }

  Label.UpdateRect(Pos.x, Pos.y);

  //Check for valid coordinate; The label may have a negative position on a log axis
  if(Pos.x != MAXINT && Pos.y != MAXINT)
  {
    Draw->SetClippingRegion();
    if(Draw->SizeMul != 1)
      Context.GetCanvas()->StretchDraw(TRect(Pos, Label.GetRect().Width(), Label.GetRect().Height()), Label.GetImage());
    else
      Context.GetCanvas()->Draw(Pos.x, Pos.y, Label.GetImage());
  }
}
//---------------------------------------------------------------------------
void TDrawElem::Visit(TPointSeries &PointSeries)
{
  unsigned StartTicks = GetTickCount();

  const int MinPixelDist = 5;
  Draw->SetClippingRegion();

  std::vector<TPoint> PointArray;
  const std::vector<TPointSeriesPoint> &PointData = PointSeries.GetPointData();
  const TPointSeries::TPointList PointList = PointSeries.GetPointList();
  TPointSeries::TPointList::const_iterator End = PointList.end();
  TPoint LastPos(MAXINT, MAXINT);

	for(TPointSeries::TPointList::const_iterator Iter = PointList.begin(); Iter != End; ++Iter)
  {
		//Check that point is valid (NAN is used to indicate invalid value)
		if((!Axes.xAxis.LogScl || Iter->x > 0) && (!Axes.yAxis.LogScl || Iter->y > 0) && _finite(Iter->x) && _finite(Iter->y))
		{
			TPoint P = xyPoint(*Iter);
      //No need to draw at same position as the last point.
      if(std::abs(P.x - LastPos.x) >= MinPixelDist || std::abs(P.y - LastPos.y) >= MinPixelDist)
      {
        LastPos = P;
				PointArray.push_back(P);
      }
		}
  }

  if(PointSeries.GetLineStyle() != psClear && PointSeries.GetLineSize() > 0)
  {
    std::vector<TPoint> SplinePoints;
    Interpolate(SplinePoints, PointArray, PointSeries.GetInterpolation());
    Draw->DrawPolyline(SplinePoints.begin(), SplinePoints.end(), PointSeries.GetLineStyle(), PointSeries.GetLineSize(), PointSeries.GetLineColor());
  }

  if(PointSeries.GetShowLabels())
  {
    Context.SetFont(PointSeries.GetFont());
  }

  std::wstring FormatStr;
  if(PointSeries.GetPointType() == ptPolar)
    FormatStr = Axes.Trigonometry == Func32::Degree ? GuiFormatSettings.DegreePointFormat : GuiFormatSettings.RadianPointFormat;
  else
    FormatStr = GuiFormatSettings.CartesianPointFormat;
  boost::wformat WFormat(FormatStr);

  //First draw all error bars and labels
  if(PointSeries.GetxErrorBarType() != ebtNone || PointSeries.GetyErrorBarType() != ebtNone ||  PointSeries.GetShowLabels())
    for(TPointSeries::TPointList::const_iterator Iter = PointList.begin(); Iter != End; ++Iter)
    {
      double x = Iter->x;
      double y = Iter->y;
      if(!_finite(x) || !_finite(y))
        continue;

      //Check that point is valid (NAN is used to indicate invalid value)
      if((!Axes.xAxis.LogScl || x > 0) && (!Axes.yAxis.LogScl || y > 0) )
      {
        TPoint Pos = xyPoint(*Iter);
        if(!InsideRect(AxesRect, Pos)) //We might get rounding errors if the point is too much outside the visible area
          continue;

        //Draw x error bar
        Context.SetPen(psSolid, clBlack, SizeScale(1));
        unsigned I = Iter - PointList.begin();
        double Delta = PointSeries.GetXError(I);
        if(Delta)
        {
          int X1 = xPoint(x - Delta);
          int X2 = xPoint(x + Delta);
          Context.DrawLine(X1, Pos.y, X2, Pos.y);
          Context.DrawLine(X1, Pos.y - 4, X1, Pos.y +5);
          Context.DrawLine(X2, Pos.y - 4, X2, Pos.y +5);
        }

        //Draw y error bar
        Delta = PointSeries.GetYError(I);
        if(Delta)
        {
          int Y1 = yPoint(y - Delta);
          int Y2 = yPoint(y + Delta);
          Context.DrawLine(Pos.x, Y1, Pos.x, Y2);
          Context.DrawLine(Pos.x - 4, Y1, Pos.x +5, Y1);
          Context.DrawLine(Pos.x - 4, Y2, Pos.x +5, Y2);
        }

        if(PointSeries.GetShowLabels())
        {
          std::wstring Str = str(WFormat % PointData[I].First % PointData[I].Second);
          int PointSize = SizeScale(PointSeries.GetSize());
          TDraw::DrawPointLabel(Context.GetCanvas(), Pos, PointSize, Str, PointSeries.GetLabelPosition());
        }
      }
    }

  //Only draw for point sizes >0
  if(PointSeries.GetSize() > 0)
  {
    //Remove clipping region and draw all markers.
    //This is done so you either is all the marker or none of it. InsideRect checks if it should be shown.
    Context.DestroyClipRect();
    for(unsigned I = 0; I < PointArray.size(); I++)
    {
      TPoint Pos = PointArray[I];
      if(!InsideRect(AxesRect, Pos)) //We might get rounding errors if the point is too much outside the visible area
        continue;

      //Check if we are inside the legend
      if(Axes.ShowLegend && InsideRect(Draw->LegendRect, Pos))
        continue;

      int PointSize = SizeScale(PointSeries.GetSize());
      TColor FillColor = PointSeries.GetFillColor();
      TColor FrameColor = PointSeries.GetFrameColor();

      if(PointSeries.GetStyle() != 7)
        DrawPoint(Context, Pos, PointSeries.GetStyle(), FrameColor, FillColor, PointSize); //Draw normalt markers
      else if(I != 0) //Draw arrow
      {
        //To be fixed: Avoid conversion to real coordinate. We should calculate angle in screen positions
        double Angle = InterpolationAngle(Draw->xyCoord(PointArray[std::max((int)I-2, 0)]), Draw->xyCoord(PointArray[I-1]),
        Draw->xyCoord(Pos), Draw->xyCoord(PointArray[std::min<unsigned>(I+1, PointArray.size()-1)]), PointSeries.GetInterpolation());

        DrawArrow(Pos, Angle, PointSeries.GetFillColor(), PointSeries.GetSize());
      }
    }
  }
  unsigned Ticks = GetTickCount() - StartTicks;
  String Str = String(Ticks) + " " + PointSeries.MakeLegendText().c_str() + " " + PointList.size() + " " + PointArray.size();
  OutputDebugString(Str.c_str());
}
//---------------------------------------------------------------------------
void TDrawElem::DrawArrow(const TPoint &P, long double Angle, TColor Color, unsigned Size)
{
  Context.SetPen(psSolid, Color, 0, ecFlat, psjMiter);
  Context.SetBrush(bsSolid, Color);

  long double Angle2 = RealToVisualAngle(Angle);
  int Length = SizeScale(6 + 2*Size);
  int dX1 = Length * std::cos(Angle2 + 2.8);
  int dY1 = Length * std::sin(Angle2 + 2.8);
  int dX2 = Length * std::cos(Angle2 - 2.8);
  int dY2 = Length * std::sin(Angle2 - 2.8);
  TPoint Arrow[] = {TPoint(P.x + dX1, P.y - dY1), P, TPoint(P.x + dX2, P.y - dY2)};
  Context.DrawPolygon(Arrow, 3);
}
//---------------------------------------------------------------------------
void TDrawElem::LineToAngle(int X, int Y, double Angle, double Length)
{
  int dX = Length * std::cos(Angle);
  int dY = Length * std::sin(Angle);

  Context.DrawLine(X, Y, X + dX, Y - dY);
}
//---------------------------------------------------------------------------
long double TDrawElem::RealToVisualAngle(long double Angle)
{
  //Adjust angle for visual coordinate system instead of real system
  long double Sin = std::sin(Angle) * Draw->yScale;
  long double Cos = std::cos(Angle) * Draw->xScale;
  return std::atan2(Sin, Cos);
}
//---------------------------------------------------------------------------
void TDrawElem::DrawHalfCircle(const TPoint &Point, long double Angle, TColor Color, unsigned Size)
{
  Context.SetPen(psSolid, Color, SizeScale(Size));
  long double Angle2 = RealToVisualAngle(Angle);

  int r = SizeScale(3 + Size);
  int a = r * std::sin(Angle2);
  int b = r * std::cos(Angle2);
  TPoint C(Point.x - b, Point.y + a);

  Context.DrawArc(C.x - r, C.y - r, C.x + r, C.y + r, C.x + a, C.y + b, C.x - a, C.y - b);
}
//---------------------------------------------------------------------------
void TDrawElem::DrawSquareBracket(const TPoint &P, long double Angle, TColor Color, unsigned Size)
{
  Context.SetPen(psSolid, Color, SizeScale(Size), ecFlat, psjMiter);
  long double Angle2 = RealToVisualAngle(Angle);
  double Length = SizeScale(3 + Size);
  double a = Length * std::cos(Angle2);
  double b = Length * std::sin(Angle2);
  TPoint Data[] = {TPoint(P.x - b - a + 0.5, P.y - a + b + 0.5), TPoint(P.x - b + 0.5, P.y - a + 0.5),
                   TPoint(P.x + b + 0.5, P.y + a + 0.5), TPoint(P.x + b - a + 0.5, P.y + a + b + 0.5)};
  Context.DrawPolyline(Data, 4);
}
//---------------------------------------------------------------------------
void TDrawElem::Visit(TAxesView &AxesView)
{
  Draw->DrawAxes();
}
//---------------------------------------------------------------------------
void TDrawElem::Visit(TRelation &Relation)
{
  Draw->SetClippingRegion();
  TColor Color = Relation.GetColor();
  Context.SetBrush(Relation.GetRelationType() == rtInequality ? Relation.GetBrushStyle() : bsSolid, Color);
  Context.SetPen(Relation.GetLineStyle(), Color,
    Relation.GetRelationType() == rtInequality && Relation.GetBrushStyle() == bsSolid ? 1 : SizeScale(Relation.GetSize()));

  if(Relation.GetRelationType() == rtInequality)
    Context.DrawPolyPolygon(Relation.GetPolylinePoints(), Relation.GetPolylineCounts());
  else
    Context.DrawPolyPolyline(Relation.GetPolylinePoints(), Relation.GetPolylineCounts());
}
//---------------------------------------------------------------------------
void TDrawElem::DrawPoint(TContext &Context, TPoint Pos, int Style, TColor FrameColor, TColor FillColor, unsigned Size)
{
	if(Size == 0)
	  return;
  Context.SetPen(psSolid, Size > 2 ? FrameColor : FillColor, Size/8 + 1);
  Context.SetBrush(bsSolid, FillColor);

  TRect Rect(Pos.x-Size, Pos.y-Size, Pos.x+Size+1, Pos.y+Size+1);

  switch(Style)
  {
    case 0: //Draw circle
      Context.DrawEllipse(Rect);
      break;

    case 1: //Draw square
      Context.DrawRectangle(Rect);
      break;

    case 2: //Draw square rotated 45 degrees
    {
      TPoint Points[] = {TPoint(Pos.x-Size, Pos.y), TPoint(Pos.x, Pos.y-Size),
                         TPoint(Pos.x+Size, Pos.y), TPoint(Pos.x, Pos.y+Size)};
      Context.DrawPolygon(Points, sizeof(Points)/sizeof(TPoint));
      break;
    }

    case 3: //Draw triangle
    {
      TPoint Points[] = {TPoint(Pos.x-Size, Pos.y+Size), TPoint(Pos.x, Pos.y-Size),
                         TPoint(Pos.x+Size, Pos.y+Size)};
      Context.DrawPolygon(Points, sizeof(Points)/sizeof(TPoint));
      break;
    }

    case 4: //Draw horizontal line
      Context.SetPen(psSolid, FillColor, Size);
      Context.DrawLine(Pos.x-Size, Pos.y, Pos.x+Size, Pos.y);
      break;

    case 5: //Draw square with a cross
    case 6: //Draw cross
      if(Style == 5)
        Context.FillRect(Rect);
      else
        Context.SetPen(psSolid, FillColor, Size / 2);
      Context.DrawLine(Pos.x, Pos.y, Pos.x - Size, Pos.y - Size);
      Context.DrawLine(Pos.x, Pos.y, Pos.x + Size, Pos.y - Size);
      Context.DrawLine(Pos.x, Pos.y, Pos.x - Size, Pos.y + Size);
      Context.DrawLine(Pos.x, Pos.y, Pos.x + Size, Pos.y + Size);
      break;

    case 7:
      TPoint Points[] = {TPoint(Rect.Left, Pos.y - (Size*2)/3), TPoint(Rect.Right, Pos.y),
                         TPoint(Rect.Left, Pos.y + (Size*2)/3)};
      Context.SetPen(psSolid, FillColor, 1);
      Context.DrawPolygon(Points, sizeof(Points)/sizeof(TPoint));
      break;
  }
}
//---------------------------------------------------------------------------
} //namespace Graph

