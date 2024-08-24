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
#include "GuiHelper.h"
#include "PointSelect.h"
#include "Unit1.h"
#include <float.h>
//---------------------------------------------------------------------------
//////////////
// TAddView //
//////////////
TTreeNode* TAddView::AddNode(TGraphElem &Elem, int ImageIndex)
{
  //Replace Hyphen-Minus with Minus Sign
  std::wstring Str = Elem.MakeLegendText();
  std::replace(Str.begin(), Str.end(), L'-', L'\x2212');

  TTreeNode *Node = Elem.GetParent() ?
    Form1->TreeView->Items->AddChildObject(Form1->GetNode(Elem.GetParent()), Str.c_str(), &Elem) :
    Form1->TreeView->Items->AddObject(NULL, Str.c_str(), &Elem);

  Node->ImageIndex = ImageIndex;
  Node->SelectedIndex = ImageIndex;
  switch(Elem.GetVisible())
  {
    case -1: Node->StateIndex =  iiGrayed; break;
    case 0:  Node->StateIndex =  iiUnChecked; break;
    case 1:  Node->StateIndex =  iiChecked; break;
  }
  Elem.SetCustomData(Node);
  return Node;
}
//---------------------------------------------------------------------------
int TAddView::AddFuncImage(TColor Color)
{
  std::map<TColor,int>::iterator Iter = FuncIconMap.find(Color);
  if(Iter == FuncIconMap.end())
    return FuncIconMap[Color] = Form1->AddFuncImage(Color);
  return Iter->second;
}
//---------------------------------------------------------------------------
void TAddView::Visit(TBaseFuncType &Func)
{
  AddNode(Func, AddFuncImage(Func.GetColor()));
  for(unsigned I = 0; I < Func.ChildCount(); I++)
    Func.GetChild(I)->Accept(*this);
}
//---------------------------------------------------------------------------
void TAddView::Visit(TTangent &Tan)
{
  AddNode(Tan, Tan.GetTangentType() == ttTangent ? iiTanNode : iiNormalNode);
}
//---------------------------------------------------------------------------
void TAddView::Visit(TShading &Shade)
{
  AddNode(Shade, Form1->AddImage(Shade.GetColor(), Shade.GetBrushStyle()));
}
//---------------------------------------------------------------------------
void TAddView::Visit(TPointSeries &Series)
{
  std::auto_ptr<Graphics::TBitmap> Bitmap(new Graphics::TBitmap);
  Bitmap->Width = Form1->TreeView->Images->Width;
  Bitmap->Height = Form1->TreeView->Images->Height;
  Bitmap->Canvas->Brush->Color = clWhite;
  Bitmap->Canvas->FillRect(TRect(0, 0, Bitmap->Width, Bitmap->Height));

  TContext Context(Bitmap->Canvas);
  Context.SetSmoothingMode(smAntiAlias);
  TDrawElem::DrawPoint(
    Context,
    TPoint(Bitmap->Width / 2, Bitmap->Height /2),
    Series.GetStyle(),
    Series.GetFrameColor(),
    Series.GetFillColor(),
    (5 * Bitmap->Width) / 16);
  AddNode(Series, Form1->AddImage(Bitmap.get(), clWhite));
}
//---------------------------------------------------------------------------
void TAddView::Visit(TTextLabel &Label)
{
  AddNode(Label, iiLabelNode);
}
//---------------------------------------------------------------------------
void TAddView::Visit(TRelation &Relation)
{
  int ImageIndex;
  if(Relation.GetRelationType() == rtInequality)
    ImageIndex = Form1->AddImage(Relation.GetColor(), Relation.GetBrushStyle());
  else
    ImageIndex = AddFuncImage(Relation.GetColor());
  AddNode(Relation, ImageIndex);
}
//---------------------------------------------------------------------------
void TAddView::Visit(TAxesView &AxesView)
{
  AddNode(AxesView, iiAxesNode)->Text = LoadRes(RES_AXES);
}
//---------------------------------------------------------------------------
//////////////
// TZoomFit //
//////////////
TZoomFit::TZoomFit(const TData &AData, const TDraw &ADraw)
 : Data(AData), Draw(ADraw)
{
  //Always show axes when Axes style is Crossed
  xMin = Data.Axes.AxesStyle == asCrossed ? Data.Axes.yAxis.AxisCross : INF;
  xMax = Data.Axes.AxesStyle == asCrossed ? Data.Axes.yAxis.AxisCross : -INF;
  yMin = Data.Axes.AxesStyle == asCrossed ? Data.Axes.xAxis.AxisCross : INF;
  yMax = Data.Axes.AxesStyle == asCrossed ? Data.Axes.xAxis.AxisCross : -INF;
}
//---------------------------------------------------------------------------
bool TZoomFit::IsChanged() const
{
  return xMin != xMax && yMin != yMax && _finite(xMin) && _finite(xMax) && _finite(yMin) && _finite(yMax);
}
//---------------------------------------------------------------------------
void TZoomFit::Visit(TBaseFuncType &Func)
{
  if(TStdFunc *StdFunc = dynamic_cast<TStdFunc*>(&Func))
  {
    Func32::ECalcError E;
    if(std::_finite(StdFunc->GetFrom().Value))
      if(Func.GetFunc().Calc(StdFunc->GetFrom().Value, E), E.ErrorCode == Func32::ecNoError) //Note comma operator
        if(StdFunc->GetFrom().Value < xMin)
          xMin = StdFunc->GetFrom().Value;
    if(std::_finite(StdFunc->GetTo().Value))
      if(Func.GetFunc().Calc(StdFunc->GetTo().Value, E), E.ErrorCode == Func32::ecNoError) //Note comma operator
        if(StdFunc->GetTo().Value)
          xMax = StdFunc->GetTo().Value;
  }

  const std::vector<Func32::TCoordSet<> > &CoordList = Func.GetCoordList();
  for(std::vector<Func32::TCoordSet<> >::const_iterator Iter = CoordList.begin(); Iter != CoordList.end(); ++Iter)
  {
    //Ignore points outside range -10000..10000 to avoid showing huge axes ranges.
    if(Iter->x > -10000 && Iter->x < 10000 && Iter->y > -10000 && Iter->y < 10000)
    {
      if(Iter->x < xMin)
        xMin = Iter->x;
      if(Iter->x > xMax)
        xMax = Iter->x;
      if(Iter->y < yMin)
        yMin = Iter->y;
      if(Iter->y > yMax)
        yMax = Iter->y;
    }
  }
}
//---------------------------------------------------------------------------
void TZoomFit::Visit(TPointSeries &Series)
{
  const TPointSeries::TPointList &PointList = Series.GetPointList();
  for(TPointSeries::TPointList::const_iterator Point = PointList.begin(); Point != PointList.end(); ++Point)
    //Check if point is valid
    if((!Data.Axes.xAxis.LogScl || Point->x > 0) && (!Data.Axes.yAxis.LogScl || Point->y > 0))
    {
      if(Point->x < xMin)
        xMin = Point->x;
      if(Point->x > xMax)
        xMax = Point->x;
      if(Point->y < yMin)
        yMin = Point->y;
      if(Point->y > yMax)
        yMax = Point->y;
    }
}
//---------------------------------------------------------------------------
void TZoomFit::Visit(TRelation &Relation)
{
  const std::vector<TPoint> &Points = Relation.GetPolylinePoints();
  int XMin = MAXINT;
  int XMax = 0;
  int YMin = MAXINT;
  int YMax = 0;

  for(std::vector<TPoint>::const_iterator Iter = Points.begin(); Iter != Points.end(); ++Iter)
  {
    if(Iter->x < XMin)
      XMin = Iter->x;
    if(Iter->x > XMax)
      XMax = Iter->x;
    if(Iter->y < YMin)
      YMin = Iter->y;
    if(Iter->y > YMax)
      YMax = Iter->y;
  }

  xMin = std::min<double>(xMin, Draw.xCoord(XMin));
  xMax = std::max<double>(xMax, Draw.xCoord(XMax));
  yMin = std::min<double>(yMin, Draw.yCoord(YMax));
  yMax = std::max<double>(yMax, Draw.yCoord(YMin));
}
//---------------------------------------------------------------------------
/////////////////
// TFindColors //
/////////////////
void TFindColors::Visit(TBaseFuncType &Func)
{
  Colors.insert(Func.GetColor());
}
//---------------------------------------------------------------------------
void TFindColors::Visit(TShading &Shade)
{
  Colors.insert(Shade.GetColor());
}
//---------------------------------------------------------------------------
void TFindColors::Visit(TPointSeries &Series)
{
  Colors.insert(clBlack);
  Colors.insert(Series.GetFillColor());
  Colors.insert(Series.GetLineColor());
  Colors.insert(Series.GetFrameColor());
}
//---------------------------------------------------------------------------
void TFindColors::Visit(TTextLabel &Label)
{
  Colors.insert(Label.GetBackgroundColor());
}
//---------------------------------------------------------------------------
void TFindColors::Visit(TRelation &Relation)
{
  Colors.insert(Relation.GetColor());
}
//---------------------------------------------------------------------------
void TFindColors::Visit(TAxesView &AxesView)
{
  Colors.insert(AxesView.GetAxes().AxesColor);
  Colors.insert(AxesView.GetAxes().GridColor);
  Colors.insert(AxesView.GetAxes().BackgroundColor);
}
//---------------------------------------------------------------------------
void Graph::FindColors(const TData &Data, std::set<TColor> &Colors)
{
  TFindColors ColorFinder(Colors);
  for(unsigned I = 0; I < Data.ElemCount(); I++)
    if(Data.GetElem(I)->GetVisible())
      Data.GetElem(I)->Accept(ColorFinder);
}
//---------------------------------------------------------------------------

