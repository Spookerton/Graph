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
#include "Unit1.h"
#include "DrawThread.h"
#include <complex>
#include <cfloat>
#include <values.h>
#include <algorithm>
#include <ctime>
#include <cfloat>
#include <set>
#include "IGraphic.h"
#pragma warn -8072 //Disable warning: Suspicous pointer arithmetic
#include <boost/tr1/complex.hpp>

namespace Graph
{
const unsigned MaxPixelDist = 10;
const unsigned MaxExtraPoints = 100;
//---------------------------------------------------------------------------
using std::real;
using std::imag;
#ifdef _WIN64
inline long double imag(long double v) {return 0;}
inline long double real(long double v) {return v;}
#endif
//---------------------------------------------------------------------------
template<typename T> inline bool TDrawThread::IsError(Func32::TErrorCode ErrorCode, const Func32::TCoord<T> &Coord)
{
  return ErrorCode != Func32::ecNoError || std::abs(imag(Coord.x)) > MIN_ZERO || std::abs(imag(Coord.y)) > MIN_ZERO ||
    (Data->Axes.xAxis.LogScl && real(Coord.x) <= 0) || (Data->Axes.yAxis.LogScl && real(Coord.y) <= 0);
}
//---------------------------------------------------------------------------
inline bool TDrawThread::InsideImage(const TPoint &P)
{
  return P.x >= AxesRect.Left && P.y >= AxesRect.Top && P.x <= AxesRect.Right && P.y <= AxesRect.Bottom;
}
//---------------------------------------------------------------------------
__fastcall TDrawThread::TDrawThread(TDraw *ADraw)
 : Thread::TIThread(true),
   Data(ADraw->Data), AxesRect(ADraw->AxesRect), Aborted(false),
   SizeMul(ADraw->SizeMul), Draw(ADraw), Context(ADraw->Context), Axes(ADraw->Axes)
{
}
//---------------------------------------------------------------------------
void __fastcall TDrawThread::Execute()
{
	randomize();
  bool Init = true;

  while(!Terminated)
  {
    if(Aborted)
      ClearMessageQueue();
    Aborted = false;
    if(!HasMessage())
      Draw->IncThreadInIdle(Init);
    Init = false;
    TMessage Message;
    GetMessage(Message);

    switch(Message.Msg)
    {
      case dmDrawAll:
        DrawAll();
        break;

      case dmTerminate:
        return;
    }
  }
}
//---------------------------------------------------------------------------
bool TDrawThread::EvalNext()
{
  TGraphElemPtr Elem = Draw->GetNextEvalElem();
  if(!Elem)
    return false;
  if(Elem->GetVisible() && !Aborted)
    Elem->Accept(*this);

  unsigned Count = Elem->ChildCount();
  for(unsigned N = 0; N < Count && !Aborted; N++)
  {
    const TGraphElemPtr &Child = Elem->GetChild(N);
    if(Child->GetVisible())
      Child->Accept(*this);
  }

  if(!Aborted)
    Elem->SetUpdateFinished();
  return !Aborted && !Terminated;
}
//---------------------------------------------------------------------------
void TDrawThread::DrawAll()
{
  while(EvalNext())
    Queue(&Draw->DrawElem.DrawNext);
}
//---------------------------------------------------------------------------
void TDrawThread::PrepareFunction(TBaseFuncType *F)
{
  if(F->GetPoints().empty())
  {
    //Use axes range for standard functions if the axes range is smaller than the function range
    double Min, Max, ds;
    F->GetCurrentRange(Min, Max, ds);
    if(!_finite(Min) || !_finite(Max) || Min > Max)
      return; //No range or evaluation of Min or Max failed

    bool LogScl = Axes.xAxis.LogScl && ds == 0;
    if(ds == 0)
      ds = LogScl ? std::exp(1/Draw->xScale) : 1/Draw->xScale;

    if(ds <= 0)
      ds = 1; //ds must always be a positive number

    //Avoid calculations on an empty range  
    if(!IsZero(Max - Min))
    {
      if(Data->Axes.CalcComplex)
        CalcFunc<Func32::TComplex>(*F, Min, Max, ds, LogScl);
      else
        CalcFunc<long double>(*F, Min, Max, ds, LogScl);
    }
  }
}
//---------------------------------------------------------------------------
void TDrawThread::Visit(TBaseFuncType &Func)
{
  PrepareFunction(&Func);
}
//---------------------------------------------------------------------------
//This function draws a mathematical function on the canvas
//Func is the function to draw; sMin and sMax are the range
//ds is the increment between the calculations
template<typename T>
void TDrawThread::CalcFunc(TBaseFuncType &F, double sMin, double sMax, double ds, bool LogScl)
{
  try
  {
    std::vector<TPoint> Points;
    std::vector<unsigned> PointNum;
    std::vector<Func32::TCoordSet<> > CoordList;
    Func32::ECalcError CalcError;
    const Func32::TBaseFunc &Func = F.GetFunc();
    Func32::TCoord<T> Coord = Func.Calc(T(sMin), CalcError); //Calc first x- and y-coordinate
    TPoint Pos;

    //We allow points to be just outside the AxesRect as a thick line just outside might be visible inside
    TRect ClipRect = AxesRect;
    ClipRect.Inflate(F.GetSize() / 2, F.GetSize() / 2);

    //Calculate position in pixel coordinates
    TPoint LastPos;
    if(!CalcError.ErrorCode)
      LastPos = TPoint(xPoint(real(Coord.x)), yPoint(real(Coord.y)));
    unsigned OldSize = 0;
    int Err = 4; //0=no error; 1=calc error; 2=croped; 3=crossing image boundary; 4=start (first point)

    bool Finished = false;
    double sLast = sMin;
    //WARNING: Because of bug in Bcc 5.6.4 we need to put sLast = s inside the ?: operator instead of before
    for(double s = sMin; !Finished && !Aborted; LogScl ? (sLast = s, s*=ds) : (sLast = s, s+=ds))
    {
      //Last point will always be exactly at sMax
      if(s > sMax)
      {
        s = sMax;
        Finished = true;
      }

      try
      {
        Coord = Func.Calc(T(s), CalcError);    //Calc x- and y-coordinate

        Pos.x = xPoint(real(Coord.x));
        Pos.y = yPoint(real(Coord.y));

        //If an error occurred or x or y have an imaginary part
        if(IsError(CalcError.ErrorCode, Coord))
        {
          //If we just went outside defined range
          //Use binary search to find the place between the last defined point and
          //the undefined point where the pixel position no longer changes.
          if(!Err && F.GetDrawType() == dtAuto)
          {
            double s2 = sLast;   //The last defined point
            Pos = LastPos;       //Current pixel position

            do
            {
              double sMiddle = LogScl ? std::sqrt(s*s2) : (s + s2) / 2; // Average between s and s2
              Coord = Func.Calc(T(sMiddle), CalcError);
              Err = IsError(CalcError.ErrorCode, Coord);
              if(Err)
                s = sMiddle;
              else
              {
                s2 = sMiddle;
                LastPos = Pos;
                Pos.x = xPoint(real(Coord.x));
                Pos.y = yPoint(real(Coord.y));
             }
            }
            while((Err || (LastPos != Pos && InsideImage(Pos))) && std::abs(s-s2) > 1E-12);

            //If we have a new position, add it. We do not check for error here.
            //Even if the last calculation was an error, we may have a better position.
            if(Pos != Points.back())
            {
              Points.push_back(Pos);
              CoordList.push_back(Func32::TCoordSet<>(s2, real(Coord.x), real(Coord.y)));
            }
          }
          Err = true;
          continue;
        }

        //If no error occured: Check if Pos and LastPos are not outside the image on the same side
        if(!Err && F.GetDrawType() == dtAuto)
          if(!((Pos.x < 0 && LastPos.x < 0) || (Pos.x > ClipRect.Right && LastPos.x > ClipRect.Right) || (Pos.y < 0 && LastPos.y < 0) || (Pos.y > ClipRect.Bottom && LastPos.y > ClipRect.Bottom)))
          {
            unsigned Count = MaxExtraPoints; //Use a max loop count to prevent the algorithm from running wild
            //If there is more than 10 pixels between the two points, calculate a point in the middle.
            //We use virtical and horizontal dist to avoid overflow in square calculation
            while(--Count && MaxDist(Pos, LastPos) > MaxPixelDist)
            {
              double sMiddle = LogScl ? std::sqrt(s*sLast) : (s + sLast) / 2; // Average between s and sLast
              Coord = Func.Calc(T(sMiddle), CalcError);
              Err = IsError(CalcError.ErrorCode, Coord);
              if(Err)
                break;

              //Cosider the new point to be part of the line it is closest to
              TPoint P = Draw->xyPoint(Coord);
              if(MaxDist(P, LastPos) < MaxDist(P, Pos))
              {
                if(P == LastPos)
                {
                  Err = 3; //Simulate error at LastPos to make Pos part of a new line
                  break;
                }
                sLast = sMiddle;
                LastPos = P;
                Points.push_back(LastPos);
                CoordList.push_back(Func32::TCoordSet<>(sLast, real(Coord.x), real(Coord.y)));
              }
              else
              {
                if(P == Pos)
                {
                  Err = 3; //Simulate error at LastPos to make Pos part of a new line
                  break;
                }
                s = sMiddle;
                Pos = P;
                Finished = false;
              }

              //If both points are outside the image, simulate an error at LastPos to break the line between LastPos and Pos
              if(!InsideRect(ClipRect, LastPos) && !InsideRect(ClipRect, Pos))
              {
                Err = 3; //Simulate error at LastPos to make Pos part of a new line
                break;
              }
            }
          }

        if(Err)             //If an error happened last but not now
        {
          //If points has been added since last calculation
          if(Points.size() > OldSize)
            //Add number of points in piece of line
            PointNum.push_back(Points.size() - OldSize);

          OldSize = Points.size();

          //If line was split because of we were out of image area
          if(Err == 2)
          {
             //Add last point
             Points.push_back(LastPos);
             Func32::TCoord<T> Coord = Func.Calc(T(s-ds), CalcError);
             CoordList.push_back(Func32::TCoordSet<>(s - ds, real(Coord.x), real(Coord.y)));
          }
          else if(Err == 1 && F.GetDrawType() == dtAuto)   //If calculation error
          {
            //If we just went inside defined range
            //Use binary tree to find the place between the last undefined point and
            //the defined point where the pixel position no longer changes.
            double s1 = sLast;  //The last undefined point
            double s2 = s;      //The first defined point
            TPoint Pos2 = Pos;  //Pixel position at s2
            LastPos = Pos2;
            Func32::TCoord<T> Coord2;

            do
            {
              double sMiddle = LogScl ? std::sqrt(s1 * s2) : (s1 + s2) / 2; //Average between s1 and s2
              Coord2 = Func.Calc(T(sMiddle), CalcError);

              Err = IsError(CalcError.ErrorCode, Coord2);
              if(Err)
                s1 = sMiddle;
              else
              {
                s2 = sMiddle;
                LastPos = Pos2;
                Pos2.x = xPoint(real(Coord2.x));
                Pos2.y = yPoint(real(Coord2.y));
              }
            }
            while((Err || (LastPos != Pos2 && InsideImage(LastPos))) && std::abs(s1-s2) > 1E-12);

            //Add point if we have a new one. Don't check Err. Even if the last calculation
            //was an error, Pos2 might be better than Pos
            if(Pos2 != Pos)
            {
              Points.push_back(Pos2);
              CoordList.push_back(Func32::TCoordSet<>(s2, real(Coord2.x), real(Coord2.y)));
            }
          }

          //Add point to vector
          Points.push_back(Pos);
          CoordList.push_back(Func32::TCoordSet<>(s, real(Coord.x), real(Coord.y)));
          Err = false;            //Indicate no error
        }
        else //No errors
        {
          //Add point to vector
          if(Pos != LastPos)
          {
            Points.push_back(Pos);
            CoordList.push_back(Func32::TCoordSet<>(s, real(Coord.x), real(Coord.y)));
          }
        }
        LastPos = Pos; //Save point
      }
      catch(EOverflow&)         //"e^1000" results in EOverflow when casting from long double to double
      {                         //Note: The exception is not thrown until xPoint(x)
        Err = true;
        continue;
      }
    }
    if(Points.size() > OldSize)
      PointNum.push_back(Points.size() - OldSize);

    //Clear points if aborted
    if(Aborted)
      F.ClearCache();
    else
      F.SetPoints(Points, PointNum, CoordList);
  }
  catch(Exception &E)
  {
    ShowStatusError("Internal error. Unexpected exception: " + E.Message);
  }
  catch(std::exception &E)
  {
    ShowStatusError("Internal error. Unexpected exception: " + String(E.what()));
  }
  catch(...)
  {
    ShowStatusError("Internal error. Unknown exception");
  }
}
//---------------------------------------------------------------------------
void TDrawThread::Visit(TShading &Shade)
{
  if(Shade.GetRegion() == NULL)
    CreateShade(Shade);
}
//---------------------------------------------------------------------------
int ComparePoint(const TPoint &P1, const TPoint &P2, TShadeStyle Style)
{
  switch(Style)
  {
    case ssAbove:
    case ssBelow:
    case ssXAxis:
      return P2.x == P1.x ? 0 : (P2.x > P1.x ? 1 : -1);

    case ssYAxis:
      return P2.y == P1.y ? 0 : (P2.y > P1.y ? 1 : -1);

    default:
      return 0;
  }
}
//---------------------------------------------------------------------------
TPoint TDrawThread::GetFixedPoint(const TShading &Shade, const TPoint &P)
{
  switch(Shade.GetShadeStyle())
  {
    case ssAbove:
      return TPoint(P.x, AxesRect.Top - 1);
    case ssBelow:
      return TPoint(P.x, AxesRect.Bottom + 1);
    case ssXAxis:
      return TPoint(P.x, Draw->yPixelCross);
    case ssYAxis:
      return TPoint(Draw->xPixelCross, P.y);
    default:
      return P;
  }
}
//---------------------------------------------------------------------------
void TDrawThread::CreateShade(TShading &Shade)
{
  TBaseFuncType *F = dynamic_cast<TBaseFuncType*>(Shade.GetParent().get());
  const std::vector<TPoint> &P1 = F->GetPoints();
  const std::vector<unsigned> &PointNum = F->GetPointNum();
  const std::vector<Func32::TCoordSet<> > &CoordList = F->GetCoordList();

  //If the shading is attached to a Func2, we continue evaluating until
  //Func2 has been evaluated so we can use the data from it.
  if(Shade.GetFunc2())
    while(!Shade.GetFunc2()->IsUpdateFinished())
      if(!EvalNext())
      {
        //If there are no more to do, wait for Func2 to finish
        if(Aborted)
          return;
        else
          Sleep(10);
      }

	if(CoordList.empty() || (Shade.GetFunc2().get() && Shade.GetFunc2()->GetCoordList().empty()))
    return;

  //The intervals can go both ways. Make sure sMin is always less than sMax
  double sMin = std::min(Shade.GetMin().Value, Shade.GetMax().Value);
  double sMax = std::max(Shade.GetMin().Value, Shade.GetMax().Value);
  double Min, Max;

  //If shade is outside the function area, nothing to draw
  if(sMax < CoordList.front().t || sMin > CoordList.back().t)
    return;

  if(dynamic_cast<const TStdFunc*>(F))
  {
    //If Min or Max is outside coordinate system: Make sure they are just one pixel outside
    Min = Range(std::max(Draw->xCoord(-1), F->GetFrom().Value), sMin, Draw->xCoord(AxesRect.Right+1));
    Max = Range(Draw->xCoord(-1), sMax, std::min(Draw->xCoord(AxesRect.Right+1), F->GetTo().Value));
  }
  else
  {
    Min = std::_finite(sMin) ? sMin : F->GetFrom().Value;
    Max = std::_finite(sMax) ? sMax : F->GetTo().Value;
  }

  //Find starting and ending points in point vector
  unsigned N1 = std::lower_bound(CoordList.begin(), CoordList.end() - 1, Min, TCompCoordSet()) - CoordList.begin();
  unsigned N2 = std::upper_bound(CoordList.begin() + N1, CoordList.end(), Max, TCompCoordSet()) - CoordList.begin() - 1;

	if(N2 <= N1)
		return; //This may happen because we subtracted 1 when we calculated N2

  //Position of x-axis on the y-axis
  int yAxisPixel = Range<int>(AxesRect.Top, yPoint(Axes.xAxis.AxisCross), AxesRect.Bottom);

  //Position of y-axis on the x-axis
  int xAxisPixel = Range<int>(AxesRect.Left, xPoint(Axes.yAxis.AxisCross), AxesRect.Right);

  if(Shade.GetExtendMinToIntercept())
  {
    switch(Shade.GetShadeStyle())
    {
      case ssAbove:
      case ssBelow:
      case ssXAxis:
        int CrossLine;
        if(Shade.GetShadeStyle() == ssAbove)
          CrossLine = 0;
        else if(Shade.GetShadeStyle() == ssBelow)
          CrossLine = AxesRect.Bottom;
        else
          CrossLine = yAxisPixel;

        //Decrease N1 until P1[N1] intercepts with the x-axis, top or bottom of image
        bool Above;
        Above = P1[N1].y < CrossLine;
        while(N1 && (Above ? P1[N1].y < CrossLine : P1[N1].y > CrossLine))
          --N1;
        break;
      case ssYAxis:
        //Decrease N1 until P1[N1] intercepts with the y-axis
        bool Left;
        Left = P1[N1].x < xAxisPixel;
        while(N1 && (Left ? P1[N1].x < xAxisPixel : P1[N1].x > xAxisPixel))
          --N1;
        break;
      case ssBetween:
        N1 = P1.rend() - FindCrossing(P1.rend() - (N1+1), P1.rend(), Shade.GetFunc2()->GetPoints().begin(), Shade.GetFunc2()->GetPoints().end());
        if(N1)
          N1--;
        break;
      case ssInside:
        N1 = P1.rend() - FindCrossing(P1.rend() - (N1+1), P1.rend(), P1.begin() + N2, P1.end());
        if(N1)
          N1--;
    }
    Min = CoordList[N1].t;
  }

  if(Shade.GetExtendMaxToIntercept())
  {
    switch(Shade.GetShadeStyle())
    {
      case ssAbove:
      case ssBelow:
      case ssXAxis:
        int CrossLine;
        if(Shade.GetShadeStyle() == ssAbove)
          CrossLine = 0;
        else if(Shade.GetShadeStyle() == ssBelow)
          CrossLine = AxesRect.Bottom;
        else
          CrossLine = yAxisPixel;

        //Increase N2 until P1[N2] intercepts with the x-axis, top or bottom of image
        bool Above;
        Above = P1[N2].y < CrossLine;
        while(N2 < P1.size() && (Above ? P1[N2].y < CrossLine : P1[N2].y > CrossLine))
          ++N2;
        break;

      case ssYAxis:
        //Increase N2 until P1[N2] intercepts with the y-axis
        bool Left;
        Left = P1[N2].x < xAxisPixel;
        while(N2 < P1.size() && (Left ? P1[N2].x < xAxisPixel : P1[N2].x > xAxisPixel))
          ++N2;
        break;

      case ssBetween:
        N2 = FindCrossing(P1.begin() + N2, P1.end(), Shade.GetFunc2()->GetPoints().begin(), Shade.GetFunc2()->GetPoints().end()) - P1.begin();
        break;

      case ssInside:
        N2 = FindCrossing(P1.begin() + N2, P1.end(), P1.rend() - N1, P1.rend()) - P1.begin();
    }
    if(N2)
      N2--;
    Max = CoordList[N2].t;
  }

  //Find start and end points of function; May not have been drawn, and may not be valid
  int x1, y1, x2, y2;
  try
  {
    x1 = xPoint(F->GetFunc().CalcX(Min));
    y1 = yPoint(F->GetFunc().CalcY(Min));
  }
  catch(...)
  {
    x1 = P1[N1].x;
    y1 = P1[N1].y;
  }

  try
  {
    x2 = xPoint(F->GetFunc().CalcX(Max));
    y2 = yPoint(F->GetFunc().CalcY(Max));
  }
  catch(...)
  {
    x2 = P1[N2].x;
    y2 = P1[N2].y;
  }

  if(x1 == AxesRect.Left)
    x1--;
  if(x2 == AxesRect.Right)
    x2++;
  if(y1 == AxesRect.Bottom)
    y1++;
  if(y2 == AxesRect.Top)
    y2--;

  boost::shared_ptr<TRegion> Region(new TRegion(TRect(0,0,0,0)));
  unsigned CountIndex = 0;
  unsigned Sum = 0;
  while(Sum <= N1)
    Sum += PointNum[CountIndex++];

  switch(Shade.GetShadeStyle())
  {
    case ssAbove:
    case ssBelow:
    case ssXAxis:
    case ssYAxis:
    {
      std::vector<TPoint> Points;
      //Set start point
      Points.push_back(GetFixedPoint(Shade, TPoint(x1, y1)));
      Points.push_back(TPoint(x1, y1));
      int y = P1[N1].y;
      unsigned I = N1+1;
      bool Pos = ComparePoint(P1[N1], P1[I], Shade.GetShadeStyle()) >= 0;
      while(I < N2+1)
      {
        while(I < N2+1 && I < Sum)
        {
          int Pos2 = ComparePoint(P1[I-1], P1[I], Shade.GetShadeStyle());
          if(Pos ? Pos2 < 0 : Pos2 > 0)
            break;
          I++;
        }
        if(I == Sum)
        {
          if(CountIndex < PointNum.size())
            Sum += PointNum[CountIndex++];
        }
        else
          Pos = !Pos;
        Points.insert(Points.end(), P1.begin()+N1, P1.begin()+I);
        if(I < N2+1)
        {
          Points.push_back(GetFixedPoint(Shade, Points.back()));
          try
          {
            *Region |= TRegion(Points);
          }
          catch(...)
          { //Ignore errors; It looks like some impossible regions will throw an exception.
          }
          Points.clear();
          N1 = I;
          Points.push_back(GetFixedPoint(Shade, P1[I]));
        }
      }
      Points.push_back(TPoint(x2, y2));
      Points.push_back(GetFixedPoint(Shade, TPoint(x2, y2)));
      try
      {
        *Region |= TRegion(Points);
      }
      catch(...)
      { //Ignore errors; It looks like some impossible regions will throw an exception.
      }
      Region->Widen(0, 0, 1, 1);
      Points.clear();
      break;
    }

    case ssInside:
      try
      {
        if(N1 != N2)
          *Region |= TRegion(&P1[N1], N2-N1);
      }
      catch(...)
      { //Ignore errors; It looks like some impossible regions will throw an exception.
      }
      break;

    case ssBetween:
    {
      //Func2 may be NULL after a copy-paste of the function
      if(!Shade.GetFunc2())
        return;
      std::vector<TPoint> Points;
      Points.push_back(TPoint(x1, y1));
      Points.insert(Points.end(), P1.begin()+N1, P1.begin()+N2+1);
      Points.push_back(TPoint(x2, y2));

      const std::vector<TPoint> &P2 = Shade.GetFunc2()->GetPoints();

      //The intervals can go both ways. Make sure sMin is always less than sMax
      double sMin2 = std::min(Shade.GetMin2().Value, Shade.GetMax2().Value);
      double sMax2 = std::max(Shade.GetMin2().Value, Shade.GetMax2().Value);

      double Min2, Max2;

      if(dynamic_cast<const TStdFunc*>(Shade.GetFunc2().get()))
      {
        Min2 = Range(Draw->xCoord(-1), sMin2, Draw->xCoord(AxesRect.Right+1));
        Max2 = Range(Draw->xCoord(-1), sMax2, Draw->xCoord(AxesRect.Right+1));
      }
      else
      {
        Min2 = std::_finite(sMin2) ? sMin2 : Shade.GetFunc2()->GetFrom().Value;
        Max2 = std::_finite(sMax2) ? sMax2 : Shade.GetFunc2()->GetTo().Value;
      }

      const std::vector<Func32::TCoordSet<> > &CoordList2 = Shade.GetFunc2()->GetCoordList();
      //Find starting and ending points in point vector
      //If list is empty: M1=-1, M2=-1
      int M1 = std::lower_bound(CoordList2.begin(), CoordList2.end() - 1, Min2, TCompCoordSet()) - CoordList2.begin();
      int M2 = std::upper_bound(CoordList2.begin()+M1, CoordList2.end(), Max2, TCompCoordSet()) - CoordList2.begin() - 1;

      //Find start and end points of function; May not have been drawn, and may not be valid
      int X1, Y1, X2, Y2;
      try
      {
        X1 = xPoint(Shade.GetFunc2()->GetFunc().CalcX(Min2));
        Y1 = yPoint(Shade.GetFunc2()->GetFunc().CalcY(Min2));
      }
      catch(...)
      {
        X1 = P2[M1].x;
        Y1 = P2[M1].y;
      }

      try
      {
        X2 = xPoint(Shade.GetFunc2()->GetFunc().CalcX(Max2));
        Y2 = yPoint(Shade.GetFunc2()->GetFunc().CalcY(Max2));
      }
      catch(...)
      {
        X2 = P2[M2].x;
        Y2 = P2[M2].y;
      }

      //Check if Max is actually less than min for one of the functions
      bool SwapMinMax = (Shade.GetMin().Value < Shade.GetMax().Value) != (Shade.GetMin2().Value < Shade.GetMax2().Value);

      if(X1 == AxesRect.Left)
        X1--;
      if(X2 == AxesRect.Right)
        X2++;
      if(Y1 == AxesRect.Bottom)
        Y1++;
      if(Y2 == AxesRect.Top)
        Y2--;

      //Add start point to the polygon points list
      if(!Shade.GetExtendMax2ToIntercept())
        Points.push_back(SwapMinMax ? TPoint(X1, Y1) : TPoint(X2, Y2));

      if(Shade.GetExtendMin2ToIntercept())
      {
        M1 = P2.rend() - FindCrossing(P2.rend() - (M1+1), P2.rend(), P1.begin(), P1.end());
        if(M1)
          M1--;
      }

      if(Shade.GetExtendMax2ToIntercept())
      {
        M2 = FindCrossing(P2.begin() + M2, P2.end(), P1.begin(), P1.end()) - P2.begin();
        if(M2)
          M2--;
      }

      //Add points on the second function to the polygon points list
      if(M2 > M1)
      {
        if(SwapMinMax)
          Points.insert(Points.end(), P2.begin()+M1, P2.begin()+M2);
        else
          Points.insert(Points.end(), P2.rend()-M2-1, P2.rend()-M1);
      }

      //Add end point to the polygon points list
      //Add start point to the polygon points list
      if(!Shade.GetExtendMin2ToIntercept())
        Points.push_back(SwapMinMax ? TPoint(X2, Y2) : TPoint(X1, Y1));
      try
      {
        *Region |= TRegion(Points);
      }
      catch(...)
      { //Ignore errors; It looks like some impossible regions will throw an exception.
      }
      break;
    }
  }

  Shade.SetRegion(Region);
}
//---------------------------------------------------------------------------
void TDrawThread::Visit(TTextLabel &Label)
{
}
//---------------------------------------------------------------------------
void TDrawThread::Visit(TPointSeries &PointSeries)
{
}
//---------------------------------------------------------------------------
void TDrawThread::Visit(TAxesView &AxesView)
{
}
//---------------------------------------------------------------------------
struct TCompY
{
  inline bool operator()(const TPoint &P1, const TPoint &P2) const
  {
    return P1.y == P2.y ? P1.x < P2.x : P1.y < P2.y;
  }
};
//---------------------------------------------------------------------------
struct TCompX
{
  inline bool operator()(const TPoint &P1, const TPoint &P2) const
  {
    return P1.x == P2.x ? P1.y < P2.y : P1.x < P2.x;
  }
};
//---------------------------------------------------------------------------
void RemoveSharedVertice(std::set<TPoint,TCompY> &Points, const TPoint &P)
{
  std::set<TPoint,TCompY>::iterator Iter = Points.find(P);
  if(Iter != Points.end())
    Points.erase(Iter);
  else
    Points.insert(P);
}
//---------------------------------------------------------------------------
//Converted from Python script found at
//http://stackoverflow.com/questions/13746284/merging-multiple-adjacent-rectangles-into-one-polygon
void RegionToPolygons(const std::vector<TRect> &Region, std::vector<TPoint> &PolygonPoints, std::vector<int> &PolygonCount)
{
  std::set<TPoint,TCompY> yPoints;
  for(std::vector<TRect>::const_iterator RectIter = Region.begin(); RectIter != Region.end(); ++RectIter)
  {
    RemoveSharedVertice(yPoints, RectIter->TopLeft());
    RemoveSharedVertice(yPoints, RectIter->BottomRight());
    RemoveSharedVertice(yPoints, TPoint(RectIter->Right, RectIter->Top));
    RemoveSharedVertice(yPoints, TPoint(RectIter->Left, RectIter->Bottom));
  }

  std::map<TPoint,TPoint,TCompY> hEdges;
  std::set<TPoint,TCompY>::const_iterator Iter = yPoints.begin();
  while(Iter != yPoints.end())
  {
    int Y = Iter->y;
    while(Iter != yPoints.end() && Iter->y == Y)
    {
      const TPoint &P = *Iter;
      ++Iter;
      hEdges.insert(std::make_pair(P, *Iter));
      hEdges.insert(std::make_pair(*Iter, P));
      ++Iter;
    }
  }

  std::map<TPoint,TPoint,TCompX> vEdges;
  std::set<TPoint,TCompX> xPoints(yPoints.begin(), yPoints.end());
  std::set<TPoint,TCompX>::const_iterator Iter2 = xPoints.begin();
  while(Iter2 != xPoints.end())
  {
    int X = Iter2->x;
    while(Iter2 != xPoints.end() && Iter2->x == X)
    {
      const TPoint &P = *Iter2;
      ++Iter2;
      vEdges.insert(std::make_pair(P, *Iter2));
      vEdges.insert(std::make_pair(*Iter2, P));
      ++Iter2;
    }
  }

  //Get all the polygons
  while(!hEdges.empty())
  {
    std::vector<std::pair<TPoint,int> > Polygon;
    Polygon.push_back(std::make_pair(hEdges.begin()->first,0)); //We can start with any point.
    hEdges.erase(hEdges.begin());

    while(true)
    {
      if(Polygon.back().second == 0)
      {
        std::map<TPoint,TPoint,TCompX>::iterator NextVertex = vEdges.find(Polygon.back().first);
        Polygon.push_back(std::make_pair(NextVertex->second, 1));
        vEdges.erase(NextVertex);
      }
      else
      {
        std::map<TPoint,TPoint,TCompY>::iterator NextVertex = hEdges.find(Polygon.back().first);
        Polygon.push_back(std::make_pair(NextVertex->second, 0));
        hEdges.erase(NextVertex);
      }

      if(Polygon.front() == Polygon.back())
      {
        //Closed polygon
        Polygon.erase(Polygon.end()-1);
        break;
      }
    }
    //Remove implementation-markers from the polygon.
    PolygonCount.push_back(Polygon.size());
    for(unsigned I = 0; I < Polygon.size(); I++)
    {
      PolygonPoints.push_back(TPoint(Polygon[I].first.x, Polygon[I].first.y));
      hEdges.erase(Polygon[I].first);
      vEdges.erase(Polygon[I].first);
    }
  }
}
//---------------------------------------------------------------------------
void TDrawThread::CreateInequality(TRelation &Relation)
{
  bool xLogScl = Axes.xAxis.LogScl;
  bool yLogScl = Axes.yAxis.LogScl;
  int dX = (Draw->Width > 1500) ? (Draw->Width > 2400 ? Draw->Width / 120 : 2 ) : 1;
  std::exp(1.0); //Workaround for stupid bug in bcc 5.6.4
  double dx = xLogScl ? std::exp(dX/Draw->xScale) : dX/Draw->xScale;
  double dx2 = xLogScl ? std::exp(1/Draw->xScale) : 1/Draw->xScale;
  double dy = yLogScl ? std::exp(-1/Draw->yScale) : -1/Draw->yScale;
  std::exp(1.0); //Workaround for stupid bug in bcc 5.6.4

  std::vector<TRect> Points;
  Points.reserve(500);
  int XStart;
  std::vector<long double> Args(2);
  Func32::ECalcError CalcError;

  double y = Axes.yAxis.Max;
  for(int Y = AxesRect.Top; Y < AxesRect.Bottom + 1; Y++)
  {
    Args[1] = y;
    bool LastResult = false;
    double x;
    x = xLogScl ? Axes.xAxis.Min / std::sqrt(dx2) : Axes.xAxis.Min - dx2/2;
    for(int X = AxesRect.Left - 1; X < AxesRect.Right + dX; X += dX)
    {
      Args[0] = x;
      long double Temp = Relation.Eval(Args, CalcError);
      bool Result = !_isnanl(Temp) && Temp != 0;
      if(Result != LastResult)
      {
        double x2 = xLogScl ? x / dx * dx2 : x - dx + dx2;
        for(int X2 = X - dX + 1; X2 <= X; X2++)
        {
          Args[0] = x2;
          Temp = Relation.Eval(Args, CalcError);
          Result = !_isnanl(Temp) && Temp != 0;
          if(Result != LastResult)
          {
            if(Result)
              XStart = X2;
            else
              Points.push_back(TRect(
                XStart <= AxesRect.Left ? -100 : XStart,
                Y <= AxesRect.Top ? -100 : Y,
                X2,
                Y >= AxesRect.Bottom ? AxesRect.Bottom + 100 : Y + 1));
            break;
          }
          xLogScl ? x2 *= dx2 : x2 += dx2; //Don't place inside for() because of bug in CB2009
        }
      }
      LastResult = Result;
      xLogScl ? x *= dx : x += dx;
    }

    if(Aborted)
      return;
    if(LastResult)
      Points.push_back(TRect(
        XStart <= AxesRect.Left ? -100 : XStart,
        Y <= AxesRect.Top ? -100 : Y,
        AxesRect.Right + 100,
        Y >= AxesRect.Bottom ? AxesRect.Bottom + 100 : Y + 1));
    yLogScl ? y *= dy : y += dy;
  }

  std::vector<TPoint> PolygonPoints;
  std::vector<int> PolygonCounts;
  RegionToPolygons(Points, PolygonPoints, PolygonCounts);
  Relation.SetPoints(PolygonPoints, PolygonCounts);
}
//---------------------------------------------------------------------------
//Check if there is a possibility for a zero point in f(x,y).
//It may not be certain
bool TDrawThread::CheckResult1(const long double Result[3])
{
  //If the sign has changed, at least one of the current and last result is valid, or
  //the increase/decrease state has changed, which might indicate an asymptote.
  return (((Result[1] < 0) == (Result[2] > 0) && !(_isnanl(Result[1]) && _isnanl(Result[2]))) ||
    (Result[0] < Result[1]) != (Result[1] < Result[2]));/* ||
    _isnanl(Result[1]) != _isnanl(Result[2]);*/
}
//---------------------------------------------------------------------------
//Check if there actually is a zero point in f(x,y) for certain.
bool TDrawThread::CheckResult2(const long double Result[3])
{
  //If the sign has changed, both last and current result is valid, and both last and current
  //value is increasing/decreasing (to avoid false vertical line at y=1/x)
  return (Result[1] < 0) == (Result[2] > 0) && !_isnanl(Result[1]) && !_isnanl(Result[2]) &&
    (Result[0] < Result[1]) == (Result[1] < Result[2]);
}
//---------------------------------------------------------------------------
/** Mark a point in the equation bitmap as used by the equation.
 */
inline void TDrawThread::AddPoint(const TPoint &P, std::vector<BYTE> &EquationBitmap)
{
  EquationBitmap[(P.y-AxesRect.Top)*AxesRect.Width()+(P.x-AxesRect.Left)] = 1;
}
//---------------------------------------------------------------------------
/** Mark all pixels for the equation in the EquationBitmap.
 *  param Loop: false: Loop through every x-pixel while scaning y-pixels for change in sign; true: Loop through every y-pixel while scanning x-pixels for change in sign.
 */
void TDrawThread::EquationLoop(TRelation &Relation, std::vector<BYTE> &EquationBitmap, bool Loop)
{
  std::vector<long double> Args(2);
  Func32::ECalcError CalcError;
  double ds1 = Loop ? 1/Draw->xScale : -1/Draw->yScale;
  double ds2 = Loop ? -1/Draw->yScale : 1/Draw->xScale;
  bool LogScl1 = Loop ? Axes.xAxis.LogScl : Axes.yAxis.LogScl;
  bool LogScl2 = Loop ? Axes.yAxis.LogScl : Axes.xAxis.LogScl;

  double s1Min = Loop ? Axes.xAxis.Min : Axes.yAxis.Max;
  double s2Min = Loop ? Axes.yAxis.Max : Axes.xAxis.Min;

  int S1Min = Loop ? AxesRect.Left - 1 : AxesRect.Top - 1;
  int S2Min = Loop ? AxesRect.Top : AxesRect.Left;

  int dS1 = Draw->Width > 1200 ? Draw->Width / 120 : 5;

  int S1Max = Loop ? AxesRect.Right + dS1 : AxesRect.Bottom + dS1;
  int S2Max = Loop ? AxesRect.Bottom : AxesRect.Right;

  int S3Min = S1Min;
  int S3Max = Loop ? AxesRect.Right : AxesRect.Bottom;

  const int dS2 = 1;

  LogScl1 ? s1Min /= std::exp(ds1/2) : s1Min -= ds1/2;

  double ds3 = LogScl1 ? std::exp(ds1) : ds1;
  ds1 *= dS1;
  ds2 *= dS2;
  if(LogScl1)
    ds1 = std::exp(ds1);
  if(LogScl2)
    ds2 = std::exp(ds2);

  double s2 = s2Min;
  for(int S2 = S2Min; S2 < S2Max && !Aborted; S2 += dS2)
  {
    Args[Loop] = s2;
    long double Result[3] = {NAN, NAN}; //Don't use plain double as it might cause overflow with large numbers
    double s1 = s1Min;
    for(int S1 = S1Min; S1 < S1Max; S1 += dS1)
    {
      Args[!Loop] = s1;
      //Only evaluate relation and not constraints here to be able to find change in sign close to the constraints
      Result[2] = Relation.EvalRelation(Args, CalcError);
      if(CheckResult1(Result))
      {
        double s3 = LogScl1 ? s1 / ds1 * ds3 : s1 - ds1 + ds3;
        for(int S3 = S1 - dS1 + 1; S3 <= S1; S3++)
        {
          Args[!Loop] = s3;
          Result[2] = Relation.EvalRelation(Args, CalcError);
          if(CheckResult2(Result) && S3 > S3Min && S3 < S3Max) //Filter out points outside the axes; Could probably be done better
          {
            if(Relation.EvalConstraints(Args, CalcError))
            {
              if(Loop)
                AddPoint(TPoint(S3, S2), EquationBitmap);
              else
                AddPoint(TPoint(S2, S3), EquationBitmap);
            }
          }
          Result[0] = Result[1];
          Result[1] = Result[2];
          if(LogScl1)
            s3 *= ds3;
          else
            s3 += ds3;
        }
      }
      else
        Result[0] = Result[1], Result[1] = Result[2];

      if(LogScl1)
        s1 *= ds1;
      else
        s1 += ds1;
    }
    if(LogScl2)
      s2 *= ds2;  //Don't integrate in for loop. It will trigger a bug in CB2009
    else
      s2 += ds2;
  }
}
//---------------------------------------------------------------------------
void DumpPoints(const std::deque<TPoint> &Points)
{
  if(Points.empty())
    return;
  String Str;
  for(unsigned I = 0; I < Points.size(); I++)
    Str += "(" + String(Points[I].x) + "," + Points[I].y + "), ";
  OutputDebugString(Str.c_str());
}
//---------------------------------------------------------------------------
/** Convert an index into the equation bitmap to a point.
 */
inline TPoint TDrawThread::IndexToPoint(unsigned I)
{
  return TPoint(I % AxesRect.Width() + AxesRect.Left, I / AxesRect.Width() + AxesRect.Top);
}
//---------------------------------------------------------------------------
/** Convert a point to an index into the equation bitmap.
 */
inline unsigned TDrawThread::PointToIndex(const TPoint &P)
{
  return (P.y - AxesRect.Top) * AxesRect.Width() + (P.x - AxesRect.Left);
}
//---------------------------------------------------------------------------
/** Convert the equation bitmap to polylines.
 *  1. Search for the first pixel in the EquationBitmap. Store it in the Line dequeue, which will hold one polyline.
 *  2. Keep checking if there is a point next to the back of Line and add it to the back.
 *  3. Keep checking if there is a point next to the front of the Line and add it to the front.
 *  4. If the front and back of the Line are close, add the front point to the back to close the loop.
 *  5. Repeat back to 1 as long as there are still points in the EquationBitmap.
 *
 *  \param EquationBitmap: Vector with 1 byte for every pixel.
 *  \param PolylinePoints: Vector to fil? with all the points found.
 *  \param: PolylineCounts: Vector with size of every polyline in PolylinePoints.
 */
void TDrawThread::BitmapToPolylines(std::vector<BYTE> &EquationBitmap, std::vector<TPoint> &PolylinePoints, std::vector<int> &PolylineCounts)
{
  std::deque<TPoint> Line;
  int BitmapWidth = AxesRect.Width();
  int LastIndex = -1;
  int EquationBitmapSize = EquationBitmap.size();
  //List of index offsets for poxels that are close to the current pixel index, i.e. the pixel below Index will be at Index+BitmapWidth
  int OffsetList[] = {-1, 1, -BitmapWidth, BitmapWidth, -BitmapWidth+1, -BitmapWidth-1, BitmapWidth+1, BitmapWidth-1};
  while(true)
  {
    int Index;
    //Find the first pixel in the equation and use it as the first point in the line
    for(Index = LastIndex+1; Index < EquationBitmapSize; Index++)
      if(EquationBitmap[Index])
      {
        Line.push_back(IndexToPoint(Index));
        EquationBitmap[Index] = 0;
        LastIndex = Index;
        break;
      }
    if(Index == EquationBitmapSize)
      return;

    //Continue adding points to the end if a pixel is found close to the last one
    bool Found;
    int Index1 = PointToIndex(Line.back());
    do
    {
      Found = false;
      for(unsigned I = 0; I < sizeof(OffsetList)/sizeof(OffsetList[0]); I++)
      {
        int NewIndex = Index1 + OffsetList[I];
        if(NewIndex >= 0 && NewIndex < EquationBitmapSize && EquationBitmap[NewIndex])
        {
          Line.push_back(IndexToPoint(NewIndex));
          EquationBitmap[NewIndex] = 0;
          Index1 = NewIndex;
          Found = true;
          break;
        }
      }
    } while(Found);

    //Continue adding points to the front if a pixel is found close to the last one
    int Index2 = PointToIndex(Line.front());
    do
    {
      Found = false;
      for(unsigned I = 0; I < sizeof(OffsetList)/sizeof(OffsetList[0]); I++)
      {
        int NewIndex = Index2 + OffsetList[I];
        if(NewIndex >= 0 && NewIndex < EquationBitmapSize && EquationBitmap[NewIndex])
        {
          Line.push_front(IndexToPoint(NewIndex));
          EquationBitmap[NewIndex] = 0;
          Index2 = NewIndex;
          Found = true;
          break;
        }
      }
    } while(Found);

    //If the first and last point are close together, add the first point as the last to close the loop
    for(unsigned I = 0; I < sizeof(OffsetList)/sizeof(OffsetList[0]); I++)
      if(Index1 + OffsetList[I] == Index2)
      {
        Line.push_back(Line.front());
        break;
      }

//    DumpPoints(Line);
    PolylinePoints.insert(PolylinePoints.end(), Line.begin(), Line.end());
    PolylineCounts.push_back(Line.size());
    Line.clear();
  }
}

//---------------------------------------------------------------------------
/** Create data to draw a relation that is an equation.
 *  1. Create an array with 1 byte per pixel.
 *  2. Loop through every row while scanning x-pixels for change in sign. Mark all pixels found.
 *  3. Loop through every column while scanning y-pixels for change in sign. Mark all pixels found.
 *  4. Convert the pixels found to polylines.
 */
void TDrawThread::CreateEquation(TRelation &Relation)
{
//  LARGE_INTEGER Start, End;
//  QueryPerformanceCounter(&Start);
  //EquationBitmap contains 1 byte pixel in the image.
  //The byte is set to 1 if it is part of the equation.
  std::vector<BYTE> EquationBitmap(AxesRect.Width()*AxesRect.Height());

  EquationLoop(Relation, EquationBitmap, 1);
  EquationLoop(Relation, EquationBitmap, 0);
  if(Aborted)
    return;

  std::vector<TPoint> PolylinePoints;
  std::vector<int> PolylineCounts;
  PolylinePoints.reserve(10000);
  BitmapToPolylines(EquationBitmap, PolylinePoints, PolylineCounts);

  Relation.SetPoints(PolylinePoints, PolylineCounts);
//  QueryPerformanceCounter(&End);
//  int Diff = End.QuadPart - Start.QuadPart;
//  OutputDebugString(("Time: " + IntToStr(Diff)).c_str());
}
//---------------------------------------------------------------------------
void TDrawThread::Visit(TRelation &Relation)
{
  if(Relation.GetPolylinePoints().empty())
  {
    if(Relation.GetRelationType() == rtEquation)
      CreateEquation(Relation);
    else
      CreateInequality(Relation);
  }
}
//---------------------------------------------------------------------------
void TDrawThread::SynchronizedShowStatusError(const String &Str)
{
  Form1->ShowStatusError(Str);
}
//---------------------------------------------------------------------------
void TDrawThread::ShowStatusError(const String &Str)
{
  //Synchronizing directly to Form1->ShowStatusError will sometimes
  //cause an ICE on C++ Builder XE
  Synchronize(&SynchronizedShowStatusError, Str);
}
//---------------------------------------------------------------------------
} //namespace Graph

