/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "ScaledImageList.h"
#include <Vcl.Imaging.pngimage.hpp>
#pragma package(smart_init)
#pragma warn -8022 //Remove warning in gdiplus.h
#include "gdiplus.h"
#pragma warn .8022 //Set warning back to default
#ifdef _WIN64
#pragma link "gdiplus.a"
#else
#pragma link "gdiplus.lib"
#endif
//---------------------------------------------------------------------------
// ValidCtrCheck is used to assure that the components created do not have
// any pure virtual functions.
//
static inline void ValidCtrCheck(TScaledImageList *)
{
  new TScaledImageList(NULL);
}
//---------------------------------------------------------------------------
namespace Scaledimagelist
{
  void __fastcall PACKAGE Register()
  {
     TComponentClass classes[1] = {__classid(TScaledImageList)};
     RegisterComponents(L"IComp", classes, 0);
  }
}
//---------------------------------------------------------------------------
__fastcall TScaledImageList::TScaledImageList(TComponent* Owner)
  : TCustomImageList(Owner), FImages(NULL), Changed(false), FGrayScale(1), DisableChange(0), FAutoUpdate(true)
{
  Gdiplus::GdiplusStartupInput StartupInput;
  Gdiplus::GdiplusStartup(&Token, &StartupInput, NULL);
  ChangeLink = new TChangeLink;
  ChangeLink->OnChange = ImageListChange;
  ColorDepth = cd32Bit;
  Masked = false;
}
//---------------------------------------------------------------------------
__fastcall TScaledImageList::~TScaledImageList()
{
  Gdiplus::GdiplusShutdown(Token);
  delete ChangeLink;
}
//---------------------------------------------------------------------------
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   Gdiplus::GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}
//---------------------------------------------------------------------------
void TScaledImageList::RecreateImageList()
{
  Changed = false;
  if(Images == NULL)
    return;

  if(Width == Images->Width && Height == Images->Height)
  {
    DisableChange++; //Avoid recursive calls as Assign calls Change()
    Assign(Images);
    DisableChange--;
  }
  else
    InternalScaleImageList(Images, this);
}
//---------------------------------------------------------------------------
void TScaledImageList::ScaleImageList(TCustomImageList *Images, TCustomImageList *DestImages)
{
  InternalScaleImageList(Images, DestImages);

  //Force a call to TCustomImage::Change() which is protected
  DestImages->BeginUpdate();
  TDrawingStyle Style = DestImages->DrawingStyle;
  DestImages->DrawingStyle = static_cast<TDrawingStyle>(0xFF);
  DestImages->DrawingStyle = Style;
  DestImages->EndUpdate();
}
//---------------------------------------------------------------------------
/** Scale the imagelist Images and write the result to DestImages.
 *  Notice: It looks like the alpha channel may not be premultiplied for bitmaps added
 *  to the imagelist. However it also looks like the alpha channel is premultiplied when
 *  the bitmap is read from the imagelist.
 */
void TScaledImageList::InternalScaleImageList(TCustomImageList *Images, TCustomImageList *DestImages)
{
  if(Images->ColorDepth != cd32Bit)
    return;

  if(DestImages->Width == Images->Width && DestImages->Height == Images->Height)
  {
    DestImages->Assign(Images);
    return;
  }

  int Count = Images->Count;
  DestImages->ColorDepth = cd32Bit; //This may clear the image list
  ImageList_SetImageCount(reinterpret_cast<HIMAGELIST>(DestImages->Handle), Count);
  if(Count == 0)
    return;

  //Get access to the data in the bitmap in the imagelist.
  BITMAP bm;
  Win32Check(GetObject(Images->GetImageBitmap(), sizeof(BITMAP), &bm));
  BYTE* SourceBytes = static_cast<BYTE*>(bm.bmBits);
  if(SourceBytes == NULL)
    return;

  //Find the needed height; The retrieved bitmap may be much larger than needed
  int IconsPerLine = (bm.bmWidth / Images->Width);
  int Height = Images->Height * ((Count + IconsPerLine - 1) / IconsPerLine);

  //It looks like the alpha channel has been premultiplied when the bitmap is retrieved.
  Gdiplus::Bitmap B(bm.bmWidth, Height, PixelFormat32bppPARGB);

  // Get access to the Gdiplus::Bitmap's pixel data
  Gdiplus::BitmapData bmd;
  Gdiplus::Rect R2(0, 0, bm.bmWidth, Height);
  B.LockBits(&R2, Gdiplus::ImageLockMode::ImageLockModeWrite, PixelFormat32bppPARGB, &bmd);

  // Copy source pixel data to destination Bitmap (one is 'upside down' relative to the other)
  int LineSize = bm.bmWidth * sizeof(DWORD);
  BYTE* DestBytes = static_cast<BYTE*>(bmd.Scan0);
  for(int y = 0; y < Height; y++)
    memcpy(DestBytes + (y * LineSize), SourceBytes + ((bm.bmHeight - y - 1) * LineSize), LineSize);
  B.UnlockBits(&bmd);

  CLSID pngClsid;
  GetEncoderClsid(L"image/png", &pngClsid);
//  B.Save((L"ImageList_" + DestImages->Name + ".png").c_str(), &pngClsid, NULL);

  int OldWidth = Images->Width;
  int OldHeight = Images->Height;

  Gdiplus::Bitmap B2(DestImages->Width, DestImages->Height, PixelFormat32bppARGB);
  Gdiplus::Graphics G(&B2);

  // set high quality rendering modes
  G.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
  G.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
  G.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
  G.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

  Gdiplus::RectF R(0, 0, DestImages->Width, DestImages->Height);
  Gdiplus::Color ClearColor(0, 0, 0, 0);
  int Top = 0;
  int Left = 0;

  std::auto_ptr<TBitmap> B3(new TBitmap);
  B3->PixelFormat= pf32bit;
  B3->SetSize(DestImages->Width, DestImages->Height);
  Gdiplus::Rect R3(0, 0, DestImages->Width, DestImages->Height);

  for(int I = 0; I < Count; I++)
  {
    G.Clear(ClearColor);
    //Subtract 0.5 to prevent some the top of the next icon to blend into this
    G.DrawImage(&B, R, Left, Top + 0.5, OldWidth, OldHeight - 0.5, Gdiplus::UnitPixel);
//    B2.Save((DestImages->Name + "_" + IntToStr(I) + ".png").c_str(), &pngClsid, NULL);

    //Copy data from B2 to a TBitmap to avoid premultiplcation of alpha channel.
    //B2.GetHBITMAP returns a bitmap that has premultiplied alpha or maybe its a DDB.
    Gdiplus::BitmapData bmd;
    B2.LockBits(&R3, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, &bmd);

    LineSize = bmd.Width * sizeof(DWORD);
    BYTE* SrcBytes = static_cast<BYTE*>(bmd.Scan0);
    for(unsigned y = 0; y < bmd.Height; y++)
      memcpy(B3->ScanLine[y], SrcBytes + (y * LineSize), LineSize);

    B2.UnlockBits(&bmd);

    ImageList_Replace(reinterpret_cast<HIMAGELIST>(DestImages->Handle), I, B3->Handle, NULL);
    Left += OldWidth;
    if(Left >= bm.bmWidth)
    {
      Top += OldHeight;
      Left = 0;
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TScaledImageList::SetImages(TCustomImageList *Value)
{
  if(FImages)
    FImages->UnRegisterChanges(ChangeLink);
  FImages = Value;
  if(FImages)
    FImages->RegisterChanges(ChangeLink);
  Change();
}
//---------------------------------------------------------------------------
void __fastcall TScaledImageList::DefineProperties(TFiler *Filer)
{
  //Prevent save of images in dfm file by not calling TCustomImageList::DefineProperties()
  TComponent::DefineProperties(Filer);
}
//---------------------------------------------------------------------------
void __fastcall TScaledImageList::SetGrayScale(double Value)
{
  if(Value != FGrayScale)
  {
    FGrayScale = Value;
    Change();
  }
}
//---------------------------------------------------------------------------
void __fastcall TScaledImageList::ImageListChange(TObject *Sender)
{
  Change();
}
//---------------------------------------------------------------------------
void __fastcall TScaledImageList::Change()
{
  if(DisableChange > 0 || !HandleAllocated())
    return;

  int Count = Images ? Images->Count : 0;
  ImageList_SetImageCount(reinterpret_cast<HIMAGELIST>(Handle), Count);
  if((ComponentState.Contains(csDesigning) || FAutoUpdate) && !ComponentState.Contains(csLoading))
    RecreateImageList(); //Make sure Handle always contains valid images in the designer
  else
    Changed = true;

  TCustomImageList::Change();
}
//---------------------------------------------------------------------------
void TScaledImageList::Bitmap2GrayScale(TBitmap *Bitmap)
{
  for(int y = 0; y < Bitmap->Height; y++)
  {
    TRGBQuad *Row = static_cast<TRGBQuad*>(Bitmap->ScanLine[y]);
    for(int x = 0; x < Bitmap->Width; x++)
    {
      //Ignore completely transparent pixels
      if(Row[x].rgbReserved > 0)
      {
        //All pixels are premultiplied with alpha and will be unpremultiplied when drawn.
        //That means that Windows will do NewRed=(Red*255)/Alpha.
        //Make sure this doesn't result in a value greater than 255 and cause wrap around.
        double Gray = (0.299 * Row[x].rgbRed + 0.587 * Row[x].rgbGreen + 0.114 * Row[x].rgbBlue);
        //If it is nearly black, change to dark gray
        if(Gray < 100)
          Gray = 100 * Row[x].rgbReserved / 255.0;
        Gray *= FGrayScale;
        //If it becomes white when unpremultiplied, change it to light grey
        if(Gray > 220 * Row[x].rgbReserved / 255.0)
          Gray = 220 * Row[x].rgbReserved / 255.0;
        Row[x].rgbRed   = Gray;
        Row[x].rgbGreen = Gray;
        Row[x].rgbBlue  = Gray;
      }
    }
  }
}
//---------------------------------------------------------------------------
//Draw a scaled image.
//If anything has changed, all images are recreated.
//If the image is drawn disabled, we make it gray scale. The code for this is copied from TCustomImageList::DoDraw()
//and modified. GrayScale is a double that replaces GrascaleFactor, which was a byte.
//GrayScale can be lower than 1 to make the grayed icon darker and higher than 1 to make it lighter.
//Colors that are too dark (black) will be changed to dark gray.
void __fastcall TScaledImageList::DoDraw(int Index, Vcl::Graphics::TCanvas* Canvas, int X, int Y, unsigned Style, bool Enabled)
{
  EnsureUpdate();
  if(Enabled || FGrayScale == 0)
  {
    ImageList_DrawEx(reinterpret_cast<HIMAGELIST>(Handle), Index, Canvas->Handle,
      X, Y, 0, 0, CLR_NONE, CLR_NONE, ILD_TRANSPARENT);
  }
  else
  {
    std::auto_ptr<TBitmap> GrayBitmap(new TBitmap);
    GrayBitmap->PixelFormat = pf32bit;
    GrayBitmap->SetSize(Width, Height);
    GrayBitmap->HandleType = bmDIB;
    GrayBitmap->IgnorePalette = true;
    GrayBitmap->AlphaFormat = afPremultiplied;

    void *P = GrayBitmap->ScanLine[GrayBitmap->Height - 1];
    memset(P, 0, BytesPerScanline(GrayBitmap->Width, 32, 32) * GrayBitmap->Height);

    ImageList_DrawEx(reinterpret_cast<HIMAGELIST>(Handle), Index, GrayBitmap->Canvas->Handle,
      0, 0, 0, 0, CLR_NONE, CLR_NONE, ILD_TRANSPARENT);

    Bitmap2GrayScale(GrayBitmap.get());
    Canvas->Draw(X, Y, GrayBitmap.get());
  }
}
//---------------------------------------------------------------------------
void TScaledImageList::EnsureUpdate()
{
  if(Changed)
    RecreateImageList();
}
//---------------------------------------------------------------------------
void __fastcall TScaledImageList::Loaded()
{
  TCustomImageList::Loaded();
  if(ComponentState.Contains(csDesigning) || FAutoUpdate)
    RecreateImageList(); //Make sure Handle always contains valid images in the designer
}
//---------------------------------------------------------------------------
void TScaledImageList::GetPngImage(int AIndex, TPngImage *ADestPNG)
{
  if(AIndex < 0 || AIndex >= Count || ADestPNG == NULL)
    return;

  std::auto_ptr<TBitmap> ContentBmp(new TBitmap);
  ContentBmp->SetSize(ADestPNG->Width, ADestPNG->Height);
  ContentBmp->PixelFormat = pf32bit;

  // Allocate zero alpha-channel
  for(int Y = 0; Y < ContentBmp->Height; Y++)
  {
    TRGBQuad *RowInOut = static_cast<TRGBQuad*>(ContentBmp->ScanLine[Y]);
    for(int X = 0; X < ContentBmp->Width; X++)
      RowInOut[X].rgbReserved = 0;
  }
  ContentBmp->AlphaFormat = afDefined;

  // Copy image
  ImageList_DrawEx(reinterpret_cast<HIMAGELIST>(Handle), AIndex, ContentBmp->Canvas->Handle,
      0, 0, 0, 0, CLR_NONE, CLR_NONE, ILD_TRANSPARENT);

  // Now ContentBmp has premultiplied alpha value, but it will
  // make bitmap too dark after converting it to PNG. Setting
  // AlphaFormat property to afIgnored helps to unpremultiply
  // alpha value of each pixel in bitmap.
  ContentBmp->AlphaFormat = afIgnored;

  // Copy graphical data and alpha-channel values
  ADestPNG->Assign(ContentBmp.get());
  ADestPNG->CreateAlpha();
  for(int Y = 0; Y < ContentBmp->Height; Y++)
  {
    TRGBQuad *RowInOut = static_cast<TRGBQuad*>(ContentBmp->ScanLine[Y]);
    BYTE *RowAlpha = &(*ADestPNG->AlphaScanline[Y])[0];
    for(int X = 0; X < ContentBmp->Width; X++)
      RowAlpha[X] = RowInOut[X].rgbReserved;
  }
}
//---------------------------------------------------------------------------

