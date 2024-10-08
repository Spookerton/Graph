/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#ifndef ContextH
#define ContextH
#include <vector>
#include <string>
#include <boost/scoped_ptr.hpp>

namespace Graph
{
typedef void (__closure *TClipCallback)(const TPoint *Points, unsigned Size);
enum TOutCode {ocInside=0, ocTop=1, ocBottom=2, ocLeft=4, ocRight=8};
enum TEndCap {ecRound, ecSquare, ecFlat};
enum TPenStyleJoin {psjBevel, psjMiter, psjRound};

//Smoothing mode constants copied from GDI+
enum TSmoothingMode {smInvalid=-1, smModeDefault=0, smHighSpeed=1, smHighQuality=2, smNone, smAntiAlias, smAntiAlias8x4=smAntiAlias, smAntiAlias8x8};

class TContext
{
  struct TContextImpl;
  TContextImpl *Impl;
  ULONG_PTR Token;
  Graphics::TCanvas *Canvas;
  HDC Handle;
  TPenStyle PenStyle;
  TColor PenColor;
  int PenWidth;
  TSmoothingMode SmoothingMode;
  bool UseGdiPlus;

  TContext(const TContext&); //Not implemented
  TContext& operator=(const TContext&); //Not implemented
  void DrawPolyline(const std::vector<TPoint> &Points);
  void CheckHandle();

public:
  TContext(Graphics::TCanvas *ACanvas, bool AUseGdiPlus=true);
  ~TContext();
  TCanvas* GetCanvas() {return Canvas;}
  void SetCanvas(TCanvas *ACanvas);

  static void ClipLine(TPoint &P1, TPoint &P2, const TRect &Rect);
  static TPoint ClipLine(const TPoint &P1, const TPoint &P2, const TRect &Rect);

  void DrawPolyline(const TPoint *Points, unsigned Size);
  void DrawPolyline(const TPoint *Points, unsigned Size, const TRect &Rect);
  void DrawPolyPolyline(const std::vector<TPoint> &Points, const std::vector<int> &Counts);

  void DrawPolygon(const std::vector<TPoint> &Points);
  void DrawPolygon(const std::vector<TPoint> &Points, const TRect &Rect);
  void DrawPolygon(const TPoint *Points, unsigned Size);
  void DrawPolygon(const TPoint *Points, unsigned Size, const TRect &Rect);
  void DrawPolyPolygon(const std::vector<TPoint> &Points, const std::vector<int> &Counts);

  void DrawLine(int X1, int Y1, int X2, int Y2);
  void DrawLine(const TPoint &P1, const TPoint &P2);
  void DrawText(const std::string &Str, int X, int Y);
  void DrawText(const std::wstring &Str, int X, int Y);
  void DrawText(const std::wstring &Str, int X, int Y, const TRect &Rect);
  void DrawRectangle(const TRect &Rect);
  void DrawRectangle(int Left, int Top, int Right, int Bottom);
  void DrawPolydots(const TPoint *Points, unsigned Size, TColor Color);
  void DrawPolyBezier(const TPoint *Points, unsigned Count);

  void DrawEllipse(int X1, int Y1, int X2, int Y2);
  void DrawEllipse(const TRect &R);
  void DrawArc(int X1, int Y1, int X2, int Y2, int X3, int Y3, int X4, int Y4);

  void DrawRegion(const class TRegion &Region);
  void DrawFrameRegion(const TRegion &Region, unsigned Width);

  void SetFont(TFont *Font);
  void SetFont(const std::string &Name, unsigned Size, TColor Color, TFontStyles Style = TFontStyles());
  void SetFontColor(TColor Color);
  void SetFontName(const std::string &Name);
  void SetBrush(TBrushStyle Style, TColor Color = clWhite, unsigned Alpha=100);
  void SetPen(TPenStyle Style, TColor Color, int Width, TEndCap EndCap=ecRound, TPenStyleJoin Join=psjRound);
  void SetGridPen(TColor Color, unsigned Width);
  void SetSmoothingMode(TSmoothingMode SmoothingMode);
  TSmoothingMode GetSmoothingMode() const {return SmoothingMode;}

  void FillRect(const TRect &Rect);

  TSize GetTextExtent(const std::string &Str);
  TSize GetTextExtent(const std::wstring &Str);
  int GetTextHeight(const std::string &Str);
  int GetTextHeight(const std::wstring &Str);
  int GetTextWidth(const std::string &Str);
  int GetTextWidth(const std::wstring &Str);

  void DestroyClipRect();
  void SetClipRegion(const TRegion &Region);
  void IntersectClipRect(const TRect &Rect);
  void ExcludeClipRect(const TRect &Rect);
  void SetDeviceOrigin(int X, int Y);

  void SetPixelColor(int X, int Y, TColor Color);
  template<typename TIter>
  void SetPixelColor(TIter Begin, TIter End, TColor Color)
  {
    HDC Handle = Canvas->Handle;
    COLORREF C = ColorToRGB(Color);
    for(TIter Iter = Begin; Iter != End; ++Iter)
      SetPixelV(Handle, Iter->x, Iter->y, C);
    Changed();
  }

  void Lock();
  void Unlock();
  void Changed();
};


class TContextLock
{
  TContext &Context;
  TContextLock(const TContextLock&); //Not implemented
  TContextLock& operator=(const TContextLock&); //NotImplemented
public:
  TContextLock(TContext &AContext) : Context(AContext) {Context.Lock();}
  ~TContextLock() {Context.Unlock();}
};

class TRegion
{
  friend TContext;
  HRGN Handle;

  TRegion(const TRegion&); //Not defined

public:
  TRegion(const std::vector<TPoint> &Points, int Mode=WINDING);
  TRegion(const TPoint *Points, unsigned Count, int Mode=WINDING);
  TRegion(const std::vector<TPoint> &Points, const std::vector<int> &Counts, int Mode=WINDING);
  TRegion(const TRect &Rect);
  TRegion(const std::vector<TRect> &Data);
  ~TRegion() {DeleteObject(Handle);}
  void Offset(int X, int Y);
  bool PointInRegion(const TPoint &Point) const;
  bool RectInRegion(const TRect &Rect) const;
  TRect GetBoundingRect() const;
  void GetData(std::vector<TRect> &Data);
  void Widen(int X1, int Y1, int X2, int Y2);
  void SetRegion(const std::vector<TRect> &Data);

  bool operator==(const TRegion &Region) const {return EqualRgn(Handle, Region.Handle);}
  TRegion& operator=(const TRegion&);
  TRegion& operator-=(const TRegion&);
  TRegion& operator&=(const TRegion&);
  TRegion& operator|=(const TRegion&);
  TRegion& operator^=(const TRegion&);
};
} //namespace Graph
//---------------------------------------------------------------------------
#endif
