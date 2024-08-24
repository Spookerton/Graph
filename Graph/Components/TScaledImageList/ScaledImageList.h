//---------------------------------------------------------------------------
#ifndef ScaledImageListH
#define ScaledImageListH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Classes.hpp>
#include <ImgList.hpp>
//---------------------------------------------------------------------------
namespace Vcl {namespace Imaging {namespace Pngimage {class TPngImage;}}}
class PACKAGE TScaledImageList : public TCustomImageList
{
private:
  ULONG_PTR Token;
  TCustomImageList *FImages;
  TChangeLink *ChangeLink;
  bool Changed;
  double FGrayScale;
  int DisableChange;
  bool FAutoUpdate;

  void RecreateImageList();
  void __fastcall SetImages(TCustomImageList *Value);
  void Bitmap2GrayScale(TBitmap *BitMap);
  void __fastcall SetGrayScale(double Value);

protected:
  void __fastcall DefineProperties(TFiler *Filer);
  void __fastcall ImageListChange(TObject *Sender);
  void __fastcall DoDraw(int Index, Vcl::Graphics::TCanvas* Canvas, int X, int Y, unsigned Style, bool Enabled);
  static void InternalScaleImageList(TCustomImageList *Images, TCustomImageList *DestImages);
  DYNAMIC void __fastcall Change();
  void __fastcall Loaded();

public:
  __fastcall TScaledImageList(TComponent* Owner);
  __fastcall ~TScaledImageList();
  static void ScaleImageList(TCustomImageList *Images, TCustomImageList *DestImages);
  void EnsureUpdate();
  void GetPngImage(int AIndex, Vcl::Imaging::Pngimage::TPngImage *ADestPng);

__published:
  __property TCustomImageList *Images = {read=FImages, write=SetImages};
  __property double GrayScale = {read=FGrayScale, write=SetGrayScale, default=1};
  __property bool AutoUpdate = {read=FAutoUpdate, write=FAutoUpdate, default=true};
  __property Width;
  __property Height;
  __property DrawingStyle;
};
//---------------------------------------------------------------------------
#endif
