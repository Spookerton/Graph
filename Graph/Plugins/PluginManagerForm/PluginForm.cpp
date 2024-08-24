//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "PluginForm.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TPluginFrm *PluginFrm;
//---------------------------------------------------------------------------
__fastcall TPluginFrm::TPluginFrm(TComponent* Owner)
  : TForm(Owner)
{
}
//---------------------------------------------------------------------------
