//---------------------------------------------------------------------------

#ifndef PluginFormH
#define PluginFormH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TPluginFrm : public TForm
{
__published:	// IDE-managed Components
  TListView *ListView;
  TSplitter *Splitter1;
  TPanel *Panel1;
  TMemo *Memo;
  TEdit *PathEdit;
  TButton *ImportButton;
  TButton *UninstallButton;
  TButton *CloseButton3;
  TLabel *Label1;
private:	// User declarations
public:		// User declarations
  __fastcall TPluginFrm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TPluginFrm *PluginFrm;
//---------------------------------------------------------------------------
#endif
