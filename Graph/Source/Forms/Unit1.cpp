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
#include "Unit2.h"
#include "Unit3.h"
#include "Unit4.h"
#include "Unit5.h"
#include "Unit6.h"
#include "Unit7.h"
#include "Unit9.h"
#include "Unit11.h"
#include "Unit12.h"
#include "Unit13.h"
#include "Unit14.h"
#include "Unit15.h"
#include "Unit16.h"
#include "Unit17.h"
#include "Unit18.h"
#include "Unit19.h"
#include "Unit21.h"
#include "Unit22.h"
#include <Clipbrd.hpp>
#include <Registry.hpp>
#include <Printers.hpp>
#include <limits>
#include <cmath>
#include <float.h>
#include "IGraphic.h"
#include "Clipboard.h"
#include "VersionInfo.h"
#include "GuiHelper.h"
#ifndef _WIN64
#include "OleServerImpl.h"
#endif
#include "PrintForm.h" //Needed to access TPrintFrm for translation
#include <Menus.hpp>
#include <fstream>
#include <iomanip>
#include <jpeg.hpp>
#include "IThread.h"
#include "Debug.h"
#include <iostream>
#include "ConfigFile.h"
#include "ConfigRegistry.h"
#include <TypInfo.hpp>
#include "PyGraph.h"
#include "Encode.h"
#include "ICompCommon.h"
#include "Images.h"
#include <HtmlHelpViewer.hpp>
#include <IdHTTP.hpp>
#include <System.StrUtils.hpp>
//---------------------------------------------------------------------------
#pragma link "TRecent"
#pragma link "Cross"
#pragma link "MySplitter"
#pragma link "FocusPanel"
#pragma link "IRichEdit"
#pragma link "IPolygon"
#pragma link "IColorSelect"
#pragma link "OpenPreviewDialog"
#pragma link "IPrintDialog"
#pragma link "SaveDialogEx"
#ifndef _WIN64
#pragma link "Wininet.lib" //Used for InternetGetConnectedState()
#endif
#pragma link "ScaledImageList"
#pragma resource "*.dfm"

#ifdef _WIN64
#define OleServerRunning() false
#endif
TForm1 *Form1;
const TCursor crMoveHand1 = static_cast<TCursor>(1);
const TCursor crMoveHand2 = static_cast<TCursor>(2);
Thread::TMutex GlobalMutex(L"Graph running"); //Global Mutex object indicating Graph is running (Checked by installation program)
const unsigned iiBullet = 57; //Index to bullet in ImageList2
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: IsResizing(0), TForm(Owner), Updating(0), StatusIcon(-1), CursorState(csIdle),
    Draw(Image1->Canvas, &Data, "DrawThread", true),
    AbortPrinting(false), ConsoleHeight(150), FunctionListModified(false), UndoFlag(false)
{
	if(0)
  	(new std::ctype<char>)->is((short)0,' '); //Woraround for linker problem
#ifdef _DEBUG
  std::ofstream out(ChangeFileExt(Application->ExeName, ".log").c_str(), std::ios_base::app);
  if(out)
  {
    out << std::endl << std::endl;
		out << "PID: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << GetCurrentProcessId() << std::endl;
    out << "Date: " << ::ToString(DateTimeToStr(Now())) << std::endl;
    out << "CmdLine: " << ::ToString(CmdLine) << std::endl;
  }

  SetCompTranslateFunc(gettext);
	InitDebug();
#endif

  Draw.SetOnComplete(&UpdateEval);
	Font->Name = "MS Shell Dlg";
	Data.SetAbortUpdateEvent(&Draw.AbortUpdate);
 	Icon->LoadFromResourceName((THandle)MainInstance, L"MAINICON");
#ifndef _WIN64
	//Set "AllowOLE" to 1 in the Registry to use OLE with instances not started with "-Embedding"
	if(!FindCmdLineSwitch(L"EMBEDDING") && !GetRegValue(REGISTRY_KEY, L"AllowOLE", HKEY_CURRENT_USER, 0))
		//Prevent OLE instantiation if not started for use with OLE
		_Module.RevokeClassObjects();
#endif
	TThread::NameThreadForDebugging(AnsiString("Main"));
	DefaultInstance->OnDebugLine = DebugLine;

	Initialize();

	//Don't update when system settings are changed (especially DecimalSeparator)
	Application->UpdateFormatSettings = false;

	//Use dot as decimal separator
	FormatSettings.DecimalSeparator = '.';

	//Set bitmap to screen size; Maximum possible size
	Image1->Picture->Bitmap->Height = Screen->Height;
	Image1->Picture->Bitmap->Width = Screen->Width;

	//Save list of buttons on toolbar before they are loaded from registry
	DefaultToolBar = GetToolBar();

	//Accept draged files from explorer
	DragAcceptFiles(Handle, true);

	//Maximum time in milliseconds hint is shown
	Application->HintHidePause = 10000;

	BOOST_ASSERT(TreeView->Items->Count == 0);

	LoadSettings();

	//Don't create Form9 before settings are loaded. Scaling and other settings are needed in the constructor.
	Form9.reset(new TForm9(this));

	//Center main window on the working area
	CenterForm(this);

	//Set shortcuts that depends on the locale settings
	ZoomInAction->ShortCut = ShortCut(VkKeyScan('+') & 0xFF, TShiftState() << ssCtrl);
	ZoomInAction->SecondaryShortCuts->AddObject("Ctrl+Shift++", (TObject*)ShortCut(VkKeyScan('+') & 0xFF, TShiftState() << ssCtrl << ssShift));
	ZoomInAction->SecondaryShortCuts->AddObject("Ctrl++", (TObject*)ShortCut(VK_ADD, TShiftState() << ssCtrl));
	ZoomInAction->SecondaryShortCuts->AddObject("Ctrl+Shift++", (TObject*)ShortCut(VK_ADD, TShiftState() << ssCtrl << ssShift));

	ZoomOutAction->ShortCut = ShortCut(VkKeyScan('-') & 0xFF, TShiftState() << ssCtrl);
	ZoomOutAction->SecondaryShortCuts->AddObject("Ctrl+Shift+-", (TObject*)ShortCut(VkKeyScan('-') & 0xFF, TShiftState() << ssCtrl << ssShift));
	ZoomOutAction->SecondaryShortCuts->AddObject("Ctrl+-", (TObject*)ShortCut(VK_SUBTRACT, TShiftState() << ssCtrl));
	ZoomOutAction->SecondaryShortCuts->AddObject("Ctrl+Shift+-", (TObject*)ShortCut(VK_SUBTRACT, TShiftState() << ssCtrl << ssShift));

	HCURSOR MoveHand2 = LoadCursor(HInstance, L"MOVECURSOR1");
	HCURSOR MoveHand1 = LoadCursor(HInstance, L"MOVECURSOR2");
  Screen->Cursors[crMoveHand1] = MoveHand1;
  Screen->Cursors[crMoveHand2] = MoveHand2;

	Recent1->FileMenu = ActionMainMenuBar1->ActionClient->Items->ActionClients[0]->Items;

	//We need to use something in HTMLHelpViewer.pas to make sure it is linked in
	ViewerName = "HTML Help Viewer";

  StatusBar1->Panels->Items[1]->Width = GuiScale(StatusBar1->Panels->Items[1]->Width);

//	BOOST_ASSERT(MoveHand1);
//	BOOST_ASSERT(MoveHand2);
  Func32::Init();
  Data.Axes.TitleFont->OnChange = AxesFontChanged;
  Data.Axes.NumberFont->OnChange = AxesFontChanged;
  Data.Axes.LabelFont->OnChange = AxesFontChanged;
  Data.Axes.LegendFont->OnChange = AxesFontChanged;
  Draw.SetSizeMul(Screen->PixelsPerInch / 96.0);
}
//---------------------------------------------------------------------------
void TForm1::HandleCommandLine()
{
	//Do not initialize data when OLE is used. This is done through InitNew() and Load() in IPersistStorage
	if(!OleServerRunning())
	{
		if(!ParamCount() || ParamStr(1)[1] == '/' || ParamStr(1)[1] == '-' || !LoadFromFile(ParamStr(1)))
			LoadDefault();
    Data.SetNeedDrawing();

    if(ParamCount() > 1)
    {
      typedef std::map<String,String> TCommandList;
      TCommandList CommandList;
      for(int I = 1; I <= ParamCount(); I++)
      {
        String Str = ParamStr(I);
        if(Str[1] == '-' || Str[1] == '/')
        {
					int I = Str.Pos('=');
          if(I == -1)
            CommandList[Str.SubString(2, MAXINT).LowerCase()]; //Just add empty command
          else
            CommandList[Str.SubString(2, I - 2).LowerCase()] = Str.SubString(I + 1, MAXINT-1);
        }
      }

      //Example of command line: Graph.exe File.grf /si=Image.png /width=600 /height=500
      TCommandList::iterator Iter = CommandList.find("si");
      if(Iter != CommandList.end())
      {
        int ImageWidth = CommandList["width"].ToIntDef(Image1->Width);
        int ImageHeight = CommandList["height"].ToIntDef(Image1->Height);
				TImageOptions ImageOptions(ImageWidth, ImageHeight);
        ImageOptions.UseCustomSize = true;
        String FileName = Iter->second;
        SaveAsImage(FileName, ImageOptions);
				Close();
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormShow(TObject *Sender)
{
	//Load file passed as the first argument to the program
	//This cannot be done from the constructor, because the main form must be fully created.
	//When loading labels TIRichEdit are using the mainform to find the size
	//For this to work Form1->Visible must be set to False at design time
	BOOST_ASSERT(Application->MainForm);

  //Form9 must be created before it can be docked
  //Wait with docking until the form is shown. Else the COM client will blink in the title when we are running as COM server
  if(GetRegValue(REGISTRY_KEY, L"DockCalcForm", HKEY_CURRENT_USER, true))
  {
    if(Form9)
      Form9->ManualDock(Panel4);
    Panel1->Height = 0;
  }

  HandleCommandLine();

	if(Options.CheckForUpdate)
  {
    ShowStatusError(LoadRes(520), clGreen, 0);
		Thread::CallFromThread(&CheckForUpdate, true, &CancelStatusError);
  }
  PostMessage(Handle, WM_USER, 0, 0);

  TreeView->SetFocus();
  if(Form22)
    Form22->ManualDock(Panel6);
  Panel6->Height = Panel6->VisibleDockClientCount ? ConsoleHeight : 0;
  Splitter2->Visible = false;

  //Stupid workaround for bug that hides size grip
  StatusBar1->Visible = false;
  StatusBar1->Visible = true;
}
//---------------------------------------------------------------------------
void TForm1::Initialize()
{
  //Prevent flicker from Image1 and Panel1 when the form is resized
  ControlStyle = ControlStyle << csOpaque;

  //Prevent flicker
  Panel2->ControlStyle = Panel2->ControlStyle << csOpaque;
  Panel2->DoubleBuffered = true;
  TreeView->ControlStyle = TreeView->ControlStyle << csOpaque;
  TreeView->DoubleBuffered = true;

	//Prevent flicker in status bar
  StatusBar1->DoubleBuffered = true;

  Recent1->RegistryKey = REGISTRY_KEY; //Set key for data in registry

  //Be careful about using TreeView->Items before OnShow. It has probably not been streamed yet
}
//---------------------------------------------------------------------------
  struct TComponentInfo
  {
    TPersistent *Component;
    String Caption;
    String Hint;
    TComponentInfo(TActionClientItem *AComponent)
      : Component(AComponent), Caption(AComponent->Caption) {}
    TComponentInfo(TMenuItem *AComponent)
      : Component(AComponent), Caption(AComponent->Caption), Hint(AComponent->Hint) {}
    TComponentInfo(TAction *AComponent)
      : Component(AComponent), Caption(AComponent->Caption), Hint(AComponent->Hint) {}
  };

void TForm1::Translate()
{
  static std::vector<TComponentInfo> TranslateList;
  if(TranslateList.empty())
  {
    TActionClients *MenuItems = ActionMainMenuBar1->ActionClient->Items;
    for(int I = 0; I < MenuItems->Count; I++)
      TranslateList.push_back(MenuItems->ActionClients[I]);

		//Translate File->Import
    TranslateList.push_back(ActionMainMenuBar1->ActionClient->Items->ActionClients[0]->Items->ActionClients[7]);
    //Translate Help->Internet
    TranslateList.push_back(ActionMainMenuBar1->ActionClient->Items->ActionClients[6]->Items->ActionClients[5]);

    TranslateList.push_back(Tree_Export);
    TranslateList.push_back(Tree_ShowInLegend);
    TranslateList.push_back(Tree_Placement);
    TranslateList.push_back(Tree_Rotation);
    TranslateList.push_back(Tree_Above);
    TranslateList.push_back(Tree_Below);
    TranslateList.push_back(Tree_Left);
    TranslateList.push_back(Tree_Right);
    TranslateList.push_back(Tree_Placement_Custom);
    TranslateList.push_back(Tree_Rotation_Custom);

    TranslateList.push_back(Label_Edit);
    TranslateList.push_back(Label_Delete);
    TranslateList.push_back(Label_Placement);
    TranslateList.push_back(Label_Rotation);
    TranslateList.push_back(Label_Above);
    TranslateList.push_back(Label_Below);
    TranslateList.push_back(Label_Left);
    TranslateList.push_back(Label_Right);
    TranslateList.push_back(Label_Placement_Custom);
    TranslateList.push_back(Label_Rotation_Custom);

    TranslateList.push_back(ToolBar_Reset);
		TranslateList.push_back(ToolBar_Customize);

    TranslateList.push_back(Legend_Show);
    TranslateList.push_back(Legend_Font);
    TranslateList.push_back(Legend_Placement);
    TranslateList.push_back(Legend_TopRight);
    TranslateList.push_back(Legend_BottomRight);
    TranslateList.push_back(Legend_TopLeft);
    TranslateList.push_back(Legend_BottomLeft);

    //Translate actions
    for(int I = 0; I < ActionManager->ActionCount; I++)
      TranslateList.push_back(static_cast<TAction*>(ActionManager->Actions[I]));
  }
/*
  //As a workaround on a bug in Windows XP set MainMenu->Images to NULL while the menu is changed.
  //Else the menu will change color from grey to white.
  //Warning: Graph will throw EMenuError at the line "MainMenu->Images = NULL;" under startup when run in Wine under Linux.
  //I have no ide why. It looks like we can detect Wine by looking for the registry key HKCU\Software\Wine.
  if(!RegKeyExists(L"Software\\Wine", HKEY_CURRENT_USER))
    MainMenu->Images = NULL;
*/
  for(unsigned I = 0; I < TranslateList.size(); I++)
  {
    TPersistent *Component = TranslateList[I].Component;
    SetStrProp(Component, GetPropInfo(Component, "Caption"), gettext(StripHotkey(TranslateList[I].Caption)));
    if(!TranslateList[I].Hint.IsEmpty())
      SetStrProp(Component, GetPropInfo(Component, "Hint"), gettext(TranslateList[I].Hint));
	}
//  MainMenu->Images = ImageList2; //Remember to restore the images

  UpdateTreeView(); //Translate "Axes"

  Recent1->Hint = LoadStr(RES_OPEN_FILE); //Do not use LoadRes. We want to keep %s in the string.
  SaveDialogEx1->HelpCaption = LoadRes(RES_SAVE_OPTIONS); //Translate the Options dialog button
  if(Form9)
    Form9->Translate();
  if(Form22)
    Form22->Translate();
}
//---------------------------------------------------------------------------
__fastcall TForm1::~TForm1()
{
  //Calling Py_Finalize() might crash Graph.
	//Python::UnloadPlugin();
  //WARNING: DefaultActionBars is not destroyed by the VCL
  //Delete it to prevent Code Guard from reporting a lot of memory leaks
//  delete ActionManager->DefaultActionBars;
}
//---------------------------------------------------------------------------
void TForm1::Redraw(void)
{
  //Check if we are able to perform an update
  if(Application->Terminated || Image1->Width == 0)
    return;

  Draw.AbortUpdate();
  //Set width and height
	Draw.SetSize(Image1->Width, Image1->Height);
  //Make sure background is drawn
  Image1->Canvas->Brush->Style = bsSolid;
  //Set background color
  Image1->Canvas->Brush->Color = Data.Axes.BackgroundColor;
  //Remove clipping region and clear area
  SelectClipRgn(Image1->Canvas->Handle, NULL);
  Image1->Canvas->FillRect(Rect(0,0,Image1->Width,Image1->Height));
  Data.SetNeedDrawing(false);
  Draw.DrawAll();
#ifndef _WIN64
  SendOleAdvise(acDataChanged);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Image1MouseDown(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y)
{
  MouseDownPos = TPoint(X, Y);
  if(Shift.Contains(ssDouble))
    return;

  switch(Button)
  {
		case mbLeft:
			switch(CursorState)
			{
				case csZoomWindow:
					Shape1->SetBounds(X, Y + ActionToolBar1->Height, 0, 0);
					Shape1->Visible = true;
					xZoom = X;
					yZoom = Y;
					break;

        case csMove:
					UndoList.Push(TUndoAxes(Data));
          MovePos = Func32::TDblPoint(Draw.xCoord(X), Draw.yCoord(Y));
          SetCursorState(csMoving);
          break;

        case csIdle:
          if(Shift.Contains(ssCtrl))
          {
            boost::shared_ptr<TGraphElem> Elem = GetGraphElem(TreeView->Selected);
            if(TPointSeriesPtr Series = boost::dynamic_pointer_cast<TPointSeries>(Elem))
            {
							Series->InsertDblPoint(Draw.xyCoord(X, Y));
							UndoList.Push(TUndoAddPoint(Series));
              Python::ExecutePluginEvent(Python::peChanged, Elem);
							Redraw(); //We need to redraw everything; Smooth lines may have changed
					    UpdateMenu();
            }
          }
          else
          {
            boost::shared_ptr<TTextLabel> Label = Data.FindLabel(X, Y);
            if(Draw.InsideLegend(X, Y))
            {
              Draw.AbortUpdate();
              SetCursorState(csMoveLegend);
              TRect Rect = Draw.GetLegendRect();

              Image2->SetBounds(Rect.Left, Rect.Top, Rect.Width(), Rect.Height());
              Image2->Picture->Bitmap->SetSize(Rect.Width(), Rect.Height());
              Graph::TContext Context(Image2->Canvas);
              Context.SetSmoothingMode(static_cast<TSmoothingMode>(PlotSettings.SmoothingMode));

              TRect ImageRect(0, 0, Rect.Width(), Rect.Height());
              Image2->Canvas->Brush->Color = Data.Axes.BackgroundColor;
              Image2->Canvas->FillRect(ImageRect);
              Draw.DrawLegend(Context, ImageRect);
              Image2->Transparent = false;
              Image2->Visible = true;
              ImagePos = TPoint(Image2->Left, Image2->Top);
              LegendPlacement = Data.Axes.LegendPlacement;

              Data.Axes.ShowLegend = false;
              Redraw();
            }
            else if(Label)
            {
              Draw.AbortUpdate();
              SetCursorState(csMoveLabel);
              MovingLabel = Label;
              MovingLabelPlacement = Label->GetPlacement();

              Image2->Top = Label->GetRect().Top;
              Image2->Left = Label->GetRect().Left;
							Image2->Picture->Assign(Label->GetImage());
              Image2->Visible = true;
              ImagePos = TPoint(Image2->Left, Image2->Top);

              Label->ChangeVisible(); //Make label invisible
              Redraw();
              break;
            }


            if(EvalAction->Checked)
              SetCross(X, Y);
            else if(AreaAction->Checked || PathAction->Checked)
            {
              Form9->StartValueChanged(X, Y);
              Form9->EndValueChanged(X, Y);
            }
          }
          break;

          default:
            break;
      }
      break;

    case mbRight:
      if(CursorState != csMoveLabel && CursorState != csMoveLegend)
      {
        TPoint Pos = Image1->ClientToScreen(TPoint(X, Y));
        SetCursorState(csIdle); //Popup() will not return until the context menu is closed again
        if(Data.FindLabel(X, Y))
          PopupMenu3->Popup(Pos.x, Pos.y);
        else if(Draw.InsideLegend(X, Y))
          PopupMenu4->Popup(Pos.x, Pos.y);
      }
      break;

    case mbMiddle:
      UndoList.Push(TUndoAxes(Data));
      MovePos = Func32::TDblPoint(Draw.xCoord(X), Draw.yCoord(Y));
      SetCursorState(csMoving);
      if(TControl *Control = dynamic_cast<TControl*>(Sender))
        SetCaptureControl(Control);
      break;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Image1MouseMove(TObject *Sender, TShiftState Shift,
	int X, int Y)
{
  if(LastMousePos == TPoint(X, Y))
    return;

  switch(CursorState)
  {
    case csZoomWindow:
      if(Shift.Contains(ssLeft))
      {
        //Check if we are moving outside canvas
        if(X >= Image1->Width)
          X = Image1->Width-1;
        else if(X < 0)
          X = 0;
        if(Y >= Image1->Height)
          Y = Image1->Height-1;
        else if(Y < 0)
          Y = 0;
        Shape1->Width = abs(X-xZoom);
        Shape1->Height = abs(Y-yZoom);
        if(X < xZoom)
					Shape1->Left = X;
        else
          Shape1->Left = xZoom;
        if(Y < yZoom)
          Shape1->Top = Y;
        else
          Shape1->Top = yZoom;
          ShowStatusMessage("(" + RoundToStr(Draw.xCoord(xZoom)) + " ; " +
            RoundToStr(Draw.yCoord(yZoom)) + ")->(" + RoundToStr(Draw.xCoord(X)) + " ; " +
            RoundToStr(Draw.yCoord(Y)) + ")", true);
      }
      else if(X >= 0 && X < Image1->Width && Y >= 0 && Y < Image1->Height)
        ShowStatusMessage("(" + RoundToStr(Draw.xCoord(X)) + " ; " +
          RoundToStr(Draw.yCoord(Y)) + ")", true);
      break;

    case csMoving:
    {
      TAxes &Axes = Data.Axes;
      double xMin = Axes.xAxis.Min;
      double xMax = Axes.xAxis.Max;
      double yMin = Axes.yAxis.Min;
      double yMax = Axes.yAxis.Max;
      if(Axes.xAxis.LogScl)
      {
        double xScale = MovePos.x / Draw.xCoord(X);
        xMin *= xScale;
        xMax *= xScale;
			}
      else
      {
        double DeltaX = MovePos.x - Draw.xCoord(X);
        xMin += DeltaX,
        xMax += DeltaX;
      }

      if(Axes.yAxis.LogScl)
      {
        double yScale = MovePos.y / Draw.yCoord(Y);
        yMin *= yScale;
        yMax *= yScale;
      }
      else
      {
        double DeltaY = MovePos.y - Draw.yCoord(Y);
        yMin += DeltaY;
        yMax += DeltaY;
      }
      ZoomWindow(xMin, xMax, yMin, yMax, true, false);
      break;
    }

    case csIdle:
      if(Shift.Contains(ssLeft))
      {
        if(EvalAction->Checked)
					SetCross(X,Y);
        else if(AreaAction->Checked || PathAction->Checked)
          Form9->EndValueChanged(X, Y);
      }
      else
        if(!!Data.FindLabel(X, Y) || Draw.InsideLegend(X, Y))
          Panel2->Cursor = crSizeAll;
        else
          Panel2->Cursor = crDefault;
      break;

    case csMoveLabel:
      MoveAndSnapLabel(X - LastMousePos.x, Y - LastMousePos.y, !Shift.Contains(ssCtrl));
      break;

    case csMoveLegend:
      MoveAndSnapLegend(X - LastMousePos.x, Y - LastMousePos.y, !Shift.Contains(ssCtrl));
      break;

    case csMove:
      break;
  }

  LastMousePos.x = X;
  LastMousePos.y = Y;

  SetPanelCoordText();
}
//---------------------------------------------------------------------------
void TForm1::SetPanelCoordText()
{
  const TAxis &xAxis = Data.Axes.xAxis;
  const TAxis &yAxis = Data.Axes.yAxis;
	int xDigits = xAxis.LogScl ? 1 - std::log10(std::abs(xAxis.Min)) : 4 - std::log10(std::abs(xAxis.Max - xAxis.Min));
  int xPrecision = xDigits > 0 ? 100 : 4;
  if(xDigits < 0)
    xDigits = 0;
  int yDigits = yAxis.LogScl ? 1 - std::log10(std::abs(yAxis.Min)) : 4 - std::log10(std::abs(yAxis.Max - yAxis.Min));
  int yPrecision = yDigits > 0 ? 100 : 4;
  if(yDigits < 0)
    yDigits = 0;

  Func32::TDblPoint Coord = Draw.xyCoord(LastMousePos.x, LastMousePos.y);

  boost::shared_ptr<TGraphElem> Item = GetGraphElem(TreeView->Selected);
  String Str;
  if(dynamic_cast<TPolFunc*>(Item.get()))
  {
    std::pair<long double,long double> Polar = GetPolarCoord(Coord, Data.Axes.Trigonometry);

    Str = FloatToStrF(Polar.first, ffFixed, xDigits < yDigits ? xPrecision : yPrecision, xDigits < yDigits ? xDigits : yDigits) +
      L'\x2220' + FloatToStrF(Polar.second, ffFixed, 5, 2);
    if(Data.Axes.Trigonometry == Func32::Degree)
      Str += L'\xB0';
  }
  else
    Str = L"x = " + FloatToStrF(Coord.x, ffFixed, xPrecision, xDigits) + L"    y = " +
      FloatToStrF(Coord.y, ffFixed, yPrecision, yDigits);

  Str = ReplaceStr(Str, L'-', L'\x2212');
  StatusBar1->Panels->Items[1]->Text = Str;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Image1MouseUp(TObject *Sender, TMouseButton Button,
	TShiftState Shift, int X, int Y)
{
  if(Shift.Contains(ssDouble))
    return;

  switch(Button)
  {
    case mbLeft:
      switch(CursorState)
      {
        case csMoving:
          SetCursorState(csMove);
          Python::ExecutePluginEvent(Python::peZoom);
          break;

        case csMoveLabel:
        {
          Image2->Visible = false;
          MovingLabel->ChangeVisible(); //Works because we can only move a visible label

          //Only if label was actually moved
          if(Image2->Left != MovingLabel->GetRect().Left || Image2->Top != MovingLabel->GetRect().Top)
          {
            int X = Image2->Left;
            int Y = Image2->Top;
            if(MovingLabelPlacement == lpUserTopRight || MovingLabelPlacement == lpUserBottomRight)
              X += Image2->Width;
            if(MovingLabelPlacement == lpUserBottomLeft || MovingLabelPlacement == lpUserBottomRight)
              Y += Image2->Height;

            UndoList.Push(TUndoChange(MovingLabel));
            MovingLabel->SetPlacement(MovingLabelPlacement);
            MovingLabel->SetPos(std::make_pair(TTextValue(Draw.xCoord(X)), TTextValue(Draw.yCoord(Y))));
            MovingLabel->UpdateRect(Image2->Left, Image2->Top); //Needed so we don't have to wait for label to be redrawn
            Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(MovingLabel));
            MovingLabel.reset();
          }

          UpdateMenu();
          Redraw();
          SetCursorState(csIdle);
          break;
        }

        case csMoveLegend:
          Image2->Visible = false;
          Data.Axes.ShowLegend = true;
          UndoList.Push(TUndoAxes(Data));
          Data.Axes.LegendPlacement = LegendPlacement;
          Data.Axes.LegendPos = Draw.xyCoord(Image2->Left, Image2->Top);
          Data.SetModified();
          Redraw();
          SetCursorState(csIdle);
          break;

        case csZoomWindow:
          SetCursorState(csIdle);
          if(X != xZoom && Y != yZoom)
            //Zoom to user selected window;don't change coordinates
            ZoomWindow(Draw.xCoord(Shape1->Left), Draw.xCoord(Shape1->Left+Shape1->Width), Draw.yCoord(Shape1->Top+Shape1->Height), Draw.yCoord(Shape1->Top));
          break;

        case csMove:
        case csIdle:
          break;
      }
      break;

    case mbMiddle:
      SetCursorState(csIdle);
      SetCaptureControl(NULL);
      break;

    case mbRight:
      break;
  }
}
//---------------------------------------------------------------------------
bool TForm1::AskSave(void)
{
  bool Result = true;
  if(Data.IsModified())
  {
    String Str = Data.GetFileName().empty() ? LoadRes(RES_SAVE_CHANGES) :
      LoadRes(RES_SAVE_CHANGES_IN, ExtractFileName(ToUString(Data.GetFileName())));
    switch(MessageBox(Str, NAME, MB_YESNOCANCEL | MB_ICONEXCLAMATION))
    {
      case IDYES:
        Result = DoSave(Data.GetFileName().c_str(), true);
        break;
      case IDNO:
				Result = true;
        break;
      case IDCANCEL:
        Result = false;
        break;
    }
  }
  if(Result)
    Draw.AbortUpdate();
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCloseQuery(TObject *Sender, bool &CanClose)
{
  DWORD SaveOption = OLECLOSE_SAVEIFDIRTY;

  //Some other programs may ask us to terminate more than once
  //When runing as OLE server only save when using linked object
  //Don't save when running as automation object
  if(!Application->Terminated && (!OleServerRunning() || !Data.GetFileName().empty()))
  {
    CanClose = AskSave();
    SaveOption = OLECLOSE_NOSAVE; //Don't save again
  }
#ifndef _WIN64
  if(OleServerRunning() && CanClose)
  {
    OleServerImpl->AddRef();
		OleServerImpl->Close(SaveOption);
    OleServerImpl->Release();
  }
#endif
}
//---------------------------------------------------------------------------
void TForm1::ScaleImages()
{
  int OldIconWidth = TreeViewImageList->Width;
  int FontScale = PixelsPerInch * Options.FontScale;
  int IconWidth = MulDiv(16, FontScale, 96 * 100);  //Width of GUI icons

  if(TreeViewImageList->Width != IconWidth)
  {
    TreeViewImageList->SetSize(IconWidth, IconWidth);
    TScaledImageList::ScaleImageList(ImageList1, TreeViewImageList);
  }
  ScaledImageList3->SetSize(IconWidth, IconWidth);

  int OldIconWidth1 = ScaledImageList1->Width;
  int OldIconWidth2 = ScaledImageList2->Width;

  int IconWidth1 = MulDiv(GuiSettings.MenuIconSize, FontScale, 96 * 100); //Width of icons in the menu
  int IconWidth2 = MulDiv(GuiSettings.ToolBarIconSize, FontScale, 96 * 100); //Width of icons on the toolbar

  ScaledImageList1->SetSize(IconWidth1, IconWidth1);
  ScaledImageList2->SetSize(IconWidth2, IconWidth2);

  ActionToolBar1->VertMargin = MulDiv(2, FontScale, 96 * 100);
  ActionToolBar1->Spacing = MulDiv(5, FontScale, 96 * 100);

  TreeView->Indent = MulDiv(18, FontScale, 9600);
  ActionMainMenuBar1->Spacing = IconWidth1 / 2;
  StatusBar1->Font->Height = MulDiv(-11, FontScale, 96 * 100);
  StatusBar1->Height = MulDiv(19, FontScale, 96 * 100);
  TreeView->Font->Height = MulDiv(-11, FontScale, 96 * 100);
  Screen->MenuFont->Height = MulDiv(-12, FontScale, 96 * 100);
  if(Visible)
    ActionMainMenuBar1->ScaleBy(IconWidth1, OldIconWidth1);
  if(IconWidth2 < OldIconWidth2)
    ActionToolBar1->ScaleBy(IconWidth2, OldIconWidth2);
  Realign();
}
//---------------------------------------------------------------------------
void TForm1::LoadSettings(void)
{
  TConfigRegistry Registry;

  if(Registry.KeyExists(REGISTRY_KEY L"\\Property"))
    Registry.OpenKey(REGISTRY_KEY L"\\Property");
  Options.Read(Registry);

  //We must scale after FontScale has been read and before the form size has been set
	ScaleForm(this);

  Registry.OpenKey(REGISTRY_KEY);
  Recent1->MaxFiles = Registry.Read("Recents", 4);
	Application->ShowHint = Registry.Read("ShowHint", true);
	TreeView->ToolTips = Application->ShowHint;
  Width = Registry.Read("Width", MulDiv(900, Screen->PixelsPerInch, 96)/*Width*/); //Note: Calls FormConstrainedResize()
  StdWidth = Width;

  Height = Registry.Read("Height", MulDiv(600, Screen->PixelsPerInch, 96)/*Height*/); //Note: Calls FormConstrainedResize()
  StdHeight = Height;

  Panel3->Width = Registry.Read("SplitterPos", MulDiv(160, Screen->PixelsPerInch, 96));
  Panel1->Width = Panel3->Width;
  ConsoleHeight = Registry.Read("ConsoleHeight", 150);

  WindowState = Registry.Read("Maximized", false) ? wsMaximized : wsNormal;
  IPrintDialog1->LeftMargin = Registry.Read("LeftMargin", IPrintDialog1->LeftMargin);
  IPrintDialog1->TopMargin = Registry.Read("TopMargin", IPrintDialog1->TopMargin);
  IPrintDialog1->CenterOnPage = Registry.Read("CenterOnPage", true);
  IPrintDialog1->Scale = Registry.Read("PrinterScale", 100);
  IPrintDialog1->Orientation = Registry.Read("Orientation", poPortrait);
	UndoList.SetMaxUndo(Registry.Read("MaxUndo", 50));

  if(Registry.ValueExists(L"Language"))
    Options.Language = Registry.Read(L"Language", L"English");
  else
    Options.Language = GetRegValue(REGISTRY_KEY, L"Language", HKEY_LOCAL_MACHINE, L"English");

  ScaleImages();
  ChangeLanguage(ToUString(Options.Language), Options.Language != L"English");
  Python::InitPlugins();

  if(Registry.ValueExists(L"ToolBar"))
    CreateToolBar(Registry.Read(L"ToolBar", L"").c_str());

  StartToolBar = GetToolBar(); //Save shown toolbar

  const wchar_t *Models =  L"[Hyperbolic fit]\nModel=$a+$b/x\n$a=1\n$b=1\n"
                          "[Sinusoidal]\nModel=$a+$b*sin($c*x+$d)\n$a=1\n$b=1\n$c=1\n$d=1\n"
                          "[Rational function]\nModel=($a+$b*x)/(1+$c*x+$d*x^2)\n$a=1\n$b=1\n$c=0\n$d=0\n"
                          "[Saturation-Growth rate]\nModel=$a*x/($b+x)\n$a=1\n$b=1\n"
                          "[BET model]\nModel=x/($a+$b*x-($a+$b)*x^2)\n$a=1\n$b=1\n"
                          "[Reciprocal]\nModel=1/($a*x+$b)\n$a=1\n$b=1\n"
                          "[Exponential association]\nModel=$a+$b*exp($c*x)\n$a=0\n$b=1\n$c=-1\n";

  Data.ImportUserModels(Registry.Read(L"UserModels", Models));
}
//---------------------------------------------------------------------------
void TForm1::SaveSettings(void)
{
  TConfigRegistry Registry;
  if(Registry.CreateKey(REGISTRY_KEY))
  {
    Registry.Write("Recents", Recent1->MaxFiles);
    Registry.Write("ShowHint", Application->ShowHint);

    Registry.Write("LeftMargin", IPrintDialog1->LeftMargin);
    Registry.Write("TopMargin", IPrintDialog1->TopMargin);
    Registry.Write("CenterOnPage", IPrintDialog1->CenterOnPage);
    Registry.Write("PrinterScale", IPrintDialog1->Scale);
    Registry.Write("Orientation", static_cast<int>(IPrintDialog1->Orientation));

    Registry.Write("DockCalcForm", Panel4->DockClientCount);
    Registry.Write(L"UserModels", Data.ExportUserModels());

    if(Options.SavePos)
    {
      Registry.Write("Maximized", WindowState == wsMaximized);
      if(WindowState == wsNormal)
      {
        StdWidth = Width;
        StdHeight = Height;
      }
      Registry.Write("Width", StdWidth);
      Registry.Write("Height", StdHeight);
      Registry.Write("SplitterPos", TreeView->Width);
      Registry.Write("ConsoleHeight", ConsoleHeight);
		}

    Registry.Write("MaxUndo", UndoList.GetMaxUndo());

    String ToolBar = GetToolBar();
    if(StartToolBar != GetToolBar())
      Registry.Write(L"ToolBar", ToolBar.c_str());

    if(Registry.CreateKey(REGISTRY_KEY "\\Property"))
      Options.Write(Registry);
  }
}
//---------------------------------------------------------------------------
void TForm1::UpdateMenu()
{
  //It may be called after the destructor
  if(Application->Terminated)
    return;

  if(dynamic_cast<TPointSeries*>(Selected.get()))
	{
    Tree_InsTrend->Visible = true;
    Tree_Export->Visible = true;
  }
  else
  {
    Tree_InsTrend->Visible = false;
    Tree_Export->Visible = false;
  }

  if(Selected)
  {
    Tree_ShowInLegend->Checked = Selected->GetShowInLegend();
  }

	if(dynamic_cast<TTextLabel*>(Selected.get()) || dynamic_cast<TAxesView*>(Selected.get()))
    Tree_ShowInLegend->Visible = false;
  else
    Tree_ShowInLegend->Visible = true;

  Tree_Placement->Visible = dynamic_cast<TTextLabel*>(Selected.get());
  Tree_Rotation->Visible = Tree_Placement->Visible;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
  SaveSettings();

  if(Printer()->Printing)
  {
    if(MessageBox(LoadRes(521), NAME, MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
      Action = caNone;
    else
      AbortPrinting = true;
  }
	if(Python::ExecutePluginEvent(Python::peClose))
    Action = caNone;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormConstrainedResize(TObject *Sender,
      int &MinWidth, int &MinHeight, int &MaxWidth, int &MaxHeight)
{
	if(WindowState == wsNormal && Top > 0)
  {
    StdWidth = Width;
    StdHeight = Height;
  }
}
//---------------------------------------------------------------------------
//This function sets the min, max and scl values
bool TForm1::ZoomWindow(long double xMin, long double xMax, long double yMin, long double yMax, bool Update, bool SaveUndo)
{
  //Check for out of range
  if(xMax-xMin < 1E-10 || yMax-yMin < 1E-10)
  {
    ShowStatusError(LoadRes(RES_MAX_ZOOM));
    return false;
  }

  //Check if we have zoomed so much out that log10(xMin/xMax) can no longer be evaluated
  if(xMax>1E15 || yMax>1E15 || xMin < -1E15 || yMin < -1E15)
  {
    ShowStatusError(LoadRes(RES_MIN_ZOOM));
    return false;
  }

  Cross->Hide();

	Draw.AbortUpdate();
  if(SaveUndo)
    UndoList.Push(TUndoAxes(Data));

  //Set min and max for axes
  Data.Axes.xAxis.Min = xMin;
  Data.Axes.xAxis.Max = xMax;
  Data.Axes.yAxis.Min = yMin;
  Data.Axes.yAxis.Max = yMax;

//  Data.Axes.HandleZoomSquare(Draw.GetScaledYAxis());

  Data.ClearCache();
  if(Update)
  {
    Data.SetModified();
    Redraw();
    if(!MoveAction->Checked)
      UpdateEval();
    UpdateMenu();
  }
  return true;
}
//---------------------------------------------------------------------------
//This function is called then a file is dragged from explorer
void __fastcall TForm1::WMDropFiles(TMessage &Message)
{
	//Get needed size for filename
  int BufSize = DragQueryFile(HDROP(Message.WParam), 0, NULL, 0) + 1;

  //Allocate memory for filename
  std::vector<wchar_t> FileName(BufSize);
  DragQueryFile(HDROP(Message.WParam), 0, &FileName[0], BufSize);

  //Release memory
  DragFinish(HDROP(Message.WParam));

  if(AskSave())
  {
    LoadFromFile(&FileName[0]);
    Redraw();
  }
}
//---------------------------------------------------------------------------
//This function is called then the form, and therefore the
//satusbar is resized
void __fastcall TForm1::StatusBar1Resize(TObject *Sender)
{
  StatusBar1->Panels->Items[0]->Width = SysLocale.MiddleEast ? 40 : StatusBar1->Width - StatusBar1->Panels->Items[1]->Width - 40;
  StatusBar1->Invalidate();
}
//---------------------------------------------------------------------------
//This function is called then the statusbar should be redrawn
void __fastcall TForm1::StatusBar1DrawPanel(TStatusBar *StatusBar,
      TStatusPanel *Panel, const TRect &Rect)
{
  if((SysLocale.MiddleEast && Panel->Index == 2) ||
     (!SysLocale.MiddleEast && Panel->Index == 0))
  {
    StatusBar1->Canvas->FillRect(Rect);
    StatusBar1->Canvas->Font->Color = StatusErrorColor;
    StatusBar1->Canvas->TextOut(5, 4, StatusError);
  }
  else if((SysLocale.MiddleEast && Panel->Index == 0) ||
         (!SysLocale.MiddleEast && Panel->Index == 2))
    if(StatusIcon != -1)
      //Draw icon number StatusIcon from the image list on the statusbar
      ScaledImageList3->Draw(StatusBar->Canvas, Rect.Left+2, Rect.Top, StatusIcon, true);
}
//---------------------------------------------------------------------------
//This shows a message in the stausbar
void TForm1::ShowStatusMessage(const String &Str, bool AutoHint)
{
  //Do not disable timer; Warnings may not be overwritten
  if(StatusError.IsEmpty())
  {
    //Don't show hints if Str not empty
    StatusBar1->AutoHint = Str.IsEmpty() || AutoHint;
    //Show message
    StatusBar1->Panels->Items[SysLocale.MiddleEast ? 2 : 0]->Text = Str;
    StatusBar1->Panels->Items[SysLocale.MiddleEast ? 2 : 0]->Style = psText;
    StatusBar1->Update();
  }
}
//---------------------------------------------------------------------------
//Show error message in status bar. This overwrite a normal message.
//Timeout indicates how long the message is shown
//Timeout=0 will show the message until CancelStatusError() is called
void TForm1::ShowStatusError(const String &Str, TColor Color, unsigned Timeout)
{
  //Set statusbar to owner drawn; don't show hints, show error message
  StatusBar1->Panels->Items[SysLocale.MiddleEast ? 2 : 0]->Style = psOwnerDraw;
  //Save message; written by Statusbar1DrawPanel()
  StatusError = Str;
  StatusErrorColor = Color;
  Timer1->Enabled = false;
  Timer1->Interval = Timeout;
  if(Timeout)
    Timer1->Enabled = true; //Enable timer
  StatusBar1->Update(); //Update message in status bar
}
//---------------------------------------------------------------------------
void TForm1::CancelStatusError()
{
  StatusError = "";
  Timer1->Enabled = false;
  StatusBar1->AutoHint = true;
  StatusBar1->Panels->Items[SysLocale.MiddleEast ? 2 : 0]->Text = "";
  StatusBar1->Panels->Items[SysLocale.MiddleEast ? 2 : 0]->Style = psText;
  StatusBar1->Update();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
  CancelStatusError();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ApplicationEventsShowHint(String &HintStr,
      bool &CanShow, THintInfo &HintInfo)
{
  //Maximum length in pixels of hint text before line wrap
  if(HintInfo.HintControl != TreeView)
    HintInfo.HintMaxWidth = GuiScale(300);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::WMEnterSizeMove(TMessage &Message)
{
  IsResizing++;
  TForm::Dispatch(&Message);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::WMExitSizeMove(TMessage &Message)
{
  IsResizing--;
  if(Reseized)
  {
    Draw.AbortUpdate();
    Data.ClearCache();
#ifndef _WIN64
    SendOleAdvise(acDataChanged);
#endif
		Redraw(); //Activates thread; must be done after OLE update
    Reseized = false;
  }
  TForm::Dispatch(&Message);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewCollapsing(TObject *Sender,
      TTreeNode *Node, bool &AllowCollapse)
{
  AllowCollapse = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel2Resize(TObject *Sender)
{
  //Check if bitmap exists
  if(Image1->Picture->Bitmap && Visible && !Application->Terminated)
  {
    if(IsResizing)
    {
      IPolygon1->Visible = false;
      Draw.AbortUpdate();
//      if(Data.Axes.ZoomSquare)
//        ZoomWindow(Data.Axes.xAxis.Min, Data.Axes.xAxis.Max, Data.Axes.yAxis.Min, Data.Axes.yAxis.Max, false, false);
      //Set width and height
      Draw.SetSize(Image1->Width,Image1->Height);
      //Make sure background is drawn
      Image1->Canvas->Brush->Style = bsSolid;
      //Set background color
			Image1->Canvas->Brush->Color = Data.Axes.BackgroundColor;
      //Clear area
      Image1->Canvas->FillRect(Rect(0,0,Image1->Width,Image1->Height));
      //Only draw the axes, not the functions.
      Draw.RedrawAxes();
      Draw.DrawAxes();
      Reseized = true;
      Cross->Visible = false;
    }
    else
    {
      Draw.AbortUpdate();
      Data.ClearCache();
      Data.SetNeedDrawing();
      UpdateEval();
    }
#ifndef _WIN64
    if(OleServerRunning())
      OleServerImpl->SetSize(Image1->Width, Image1->Height);
#endif
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewContextPopup(TObject *Sender,
      TPoint &MousePos, bool &Handled)
{
  if(TreeView->Selected == NULL)
    Handled = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewDblClick(TObject *Sender)
{
  if(TreeView->Selected)
    Tree_Properties->OnClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
  TTreeNode *Node = TreeView->GetNodeAt(X,Y);
  if(Node == NULL)
    return;

  if(Button == mbRight)
    TreeView->Selected = Node;
  else if(Button == mbLeft)
    if(TreeView->GetHitTestInfoAt(X, Y).Contains(htOnStateIcon))
      ChangeVisible(GetGraphElem(Node));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormMouseWheelUp(TObject *Sender,
      TShiftState Shift, const TPoint &MousePos, bool &Handled)
{
  //Check for mouse inside Image1
  if(MousePos.x < Image1->ClientOrigin.x || MousePos.x > Image1->ClientOrigin.x + Image1->Width ||
     MousePos.y < Image1->ClientOrigin.y || MousePos.y > Image1->ClientOrigin.y + Image1->Height)
       return;

  //Zoom in at the point where the cursor is.
  ZoomPoint(Image1->ScreenToClient(Mouse->CursorPos), GuiSettings.MouseZoomIn, false);
  Handled = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormMouseWheelDown(TObject *Sender,
      TShiftState Shift, const TPoint &MousePos, bool &Handled)
{
  //Check for mouse inside Image1
  if(MousePos.x < Image1->ClientOrigin.x || MousePos.x > Image1->ClientOrigin.x + Image1->Width ||
     MousePos.y < Image1->ClientOrigin.y || MousePos.y > Image1->ClientOrigin.y + Image1->Height)
       return;

  //Zoom out at the point where the cursor is
  ZoomPoint(Image1->ScreenToClient(Mouse->CursorPos), GuiSettings.MouseZoomOut, false);
  Handled = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Splitter1DblClick(TObject *Sender)
{
  int MaxWidth = Splitter1->MinSize;

  SCROLLINFO ScrollInfo = {sizeof(ScrollInfo), SIF_RANGE};
  SCROLLBARINFO ScrollBarInfo = {sizeof(ScrollBarInfo)};

  // To find the whole (scrollable) width of the tree control,
  // we determine the range of the scrollbar.
  // Unfortunately when a scrollbar isn't needed (and is invisible),
  // its range isn't zero (but rather 0 to 100),
  // so we need to specifically ignore it then.
  // Only if the scrollbar is displayed
  if(GetScrollInfo(TreeView->Handle, SB_HORZ, &ScrollInfo) &&
     GetScrollBarInfo(TreeView->Handle, OBJID_HSCROLL, &ScrollBarInfo) &&
    (ScrollBarInfo.rgstate[0] & STATE_SYSTEM_INVISIBLE) == 0)
  {
    MaxWidth = ScrollInfo.nMax - ScrollInfo.nMin + 6;
  }
  else
  {
    for(int I = 0; I < TreeView->Items->Count; I++)
    {
      TTreeNode *Node = TreeView->Items->Item[I];
      int StrWidth = Node->DisplayRect(true).Right;
      if(StrWidth > MaxWidth)
        MaxWidth = StrWidth;
    }
    MaxWidth += 5;
  }

  //If the vertical scroll bar is visible, add the width of this scroll bar
  if(GetScrollBarInfo(TreeView->Handle, OBJID_VSCROLL, &ScrollBarInfo) &&
    (ScrollBarInfo.rgstate[0] & STATE_SYSTEM_INVISIBLE) == 0)
    MaxWidth += GetSystemMetrics(SM_CXVSCROLL);

  if(MaxWidth > Width / 2)
    MaxWidth = Width / 2;
  Panel3->Width = MaxWidth;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewChange(TObject *Sender, TTreeNode *Node)
{
  static TGraphElemPtr LastSelected; //Used to avoid executing events unnecessary
  UpdateMenu();
  TGraphElemPtr Elem = GetGraphElem(Node);
  //Necessary because Form9 may not have been loaded yet
  if(Form9)
    Form9->FuncChanged(Elem);
  if(Elem != LastSelected)
    Python::ExecutePluginEvent(Python::peSelect, Elem);
  Selected = Elem;
  LastSelected = Elem;
}
//---------------------------------------------------------------------------
void TForm1::ChangeLanguage(const String &Lang, bool Force)
{
  if(Lang.IsEmpty())
    return;

  LoadLanguage(Lang);

  int DefaultBiDiMode = GetRegValue(REGISTRY_KEY, L"BiDiMode", HKEY_CURRENT_USER, bdLeftToRight);
  TBiDiMode Mode = static_cast<TBiDiMode>(ToIntDef(DefaultInstance->GetTranslationProperty("BiDiMode"), DefaultBiDiMode));
  if(Mode != Application->BiDiMode)
    FlipChildren(true);
  SysLocale.MiddleEast = Mode;
  Application->BiDiMode = Mode;

  if(Force || Lang != ToUString(Options.Language))
    Translate();

  //Necesarry to update all hot keys
  ActionMainMenuBar1->ActionClient->Items->AutoHotKeys = false;
  ActionMainMenuBar1->ActionClient->Items->AutoHotKeys = true;

  //Workaround for bug in TActionClientsCollection.InternalRethinkHotkeys()
  for(int I = 0; I < ActionMainMenuBar1->ActionClient->Items->Count; I++)
    ActionMainMenuBar1->ActionClient->Items->ActionClients[I]->Control->Caption = ActionMainMenuBar1->ActionClient->Items->ActionClients[I]->Caption;

  Options.Language = ToWString(Lang);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
  if(Shift == TShiftState())
    switch(Key)
    {
      case VK_F5:
        Draw.AbortUpdate();
        Data.ClearCache();
        Data.Update();
        UpdateTreeView();
        Redraw(); //Activates thread; must be done after OLE update
        break;

      case VK_F9:
        Draw.AbortUpdate();
        Data.Axes.xAxis.ShowNegativeArrow = !Data.Axes.xAxis.ShowNegativeArrow;
        Data.Axes.yAxis.ShowNegativeArrow = !Data.Axes.yAxis.ShowNegativeArrow;
        Data.Axes.xAxis.NumberPlacement = Data.Axes.xAxis.NumberPlacement == npCenter ? npBefore : npCenter;
        Data.Axes.yAxis.NumberPlacement = Data.Axes.yAxis.NumberPlacement == npCenter ? npBefore : npCenter;
        Redraw(); //Activates thread; must be done after OLE update
        break;

      case VK_F11:
        if(Form22)
          Form22->Show();
        break;
    }

  else if(Shift == TShiftState() << ssShift)
    switch(Key)
    {
      case VK_SHIFT:
        if(CursorState == csIdle)
          SetCursorState(csMove);
        break;
    }
  else if(Shift == TShiftState() << ssCtrl << ssAlt)
    switch(Key)
    {
      case VK_F12: //Simulate crash (division by zero
      {
        double x=0;
        double y=5/x;
        break;
      }
      case VK_F16:
        throw EAbort("");
      case VK_F17: //Test uncaught exception
        throw Exception("Test Exception");
      case VK_F18:  //Test Access Violation
      {
        int *Ptr = NULL;
        *Ptr = 5;
        break;
      }
      case VK_F19:
        throw std::runtime_error("Test std::exception");
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormKeyPress(TObject *Sender, char &Key)
{
  switch(Key)
  {
    case 27:
      Draw.AbortUpdate(); //Abort updating
      Key = 0;

      //If text label is being moved: Hide image and redraw with the old label visible
      if(CursorState == csMoveLabel)
      {
        Image2->Visible = false;
        MovingLabel->ChangeVisible();
        Redraw();
      }
      else if(CursorState == csMoveLegend)
      {
        Image2->Visible = false;
        Data.Axes.ShowLegend = true;
        Redraw();
      }
      SetCursorState(csIdle);
      break;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewMouseMove(TObject *Sender,
			TShiftState Shift, int X, int Y)
{
	if(StatusBar1->AutoHint)
	{
		TTreeNode *Node = TreeView->GetNodeAt(X,Y);
		boost::shared_ptr<TGraphElem> GraphElem = GetGraphElem(Node);
		if(GraphElem)
		{
			String Str = ToUString(GraphElem->MakeText());
			String LegendText = ToUString(GraphElem->MakeLegendText());
			if(!LegendText.IsEmpty() && Str != LegendText)
				Str += L"    \"" + LegendText + L'\"';
			ShowStatusMessage(Str, true);
		}
    else
			ShowStatusMessage(L"", true);
	}
}
//---------------------------------------------------------------------------
void TForm1::SetSelected(const TGraphElemPtr &Elem)
{
  Selected = Elem;
  if(!FunctionListModified)
    TreeView->Selected = GetNode(Elem);
}
//---------------------------------------------------------------------------
void TForm1::UpdateTreeView(const boost::shared_ptr<TGraphElem> &ASelected)
{
  FunctionListModified = true;
  Selected = ASelected;
}
//---------------------------------------------------------------------------
void TForm1::RecreateTreeView()
{
  TreeView->OnChange = NULL;  //Prevent a lot of change notifications
  TreeView->Repaint();
  //Prevent the scrollbar from jumping
  SendMessage(TreeView->Parent->Handle, WM_SETREDRAW, false, 0);
  int Pos = GetScrollPos(TreeView->Handle, SB_VERT);

  int Index = TreeView->Selected ? TreeView->Selected->AbsoluteIndex : -1;
  TreeView->Items->Clear();

  while(TreeViewImageList->Count > ImageList1->Count)
    TreeViewImageList->Delete(TreeViewImageList->Count-1);

  Graph::TAddView AddView;
  for(unsigned I = 0; I < Data.ElemCount(); I++)
  {
    TGraphElemPtr Elem = Data.GetElem(I);
    if(Elem->GetShowInFunctionList())
      Elem->Accept(AddView);
    else
      Elem->SetCustomData(NULL);
  }
  TreeView->OnChange = TreeViewChange; //Enable change notification again

  TreeView->FullExpand();

  //Enable and redraw the scrollbar again
  SetScrollPos(TreeView->Handle, SB_VERT, Pos, false);
  SendMessage(TreeView->Parent->Handle, WM_SETREDRAW, true, 0);
	RedrawWindow(TreeView->Parent->Handle, NULL, 0, RDW_INVALIDATE | RDW_ALLCHILDREN);
  if(TreeView->Selected)
    PostMessage(TreeView->Handle, TVM_ENSUREVISIBLE, 0, (LONG)TreeView->Selected->ItemId); //SendMessage doesn't work
  //If the item is wider than the tree view, TVM_ENSUREVISIBLE will center the tree view.
  //PostMessage WM_HSCROLL will move the scroll bar back.
  PostMessage(TreeView->Handle, WM_HSCROLL, SB_TOP, 0);

  FunctionListModified = false;
  Python::ExecutePluginEvent(Python::peFunctionListUpdated);

  if(Selected)
    TreeView->Selected = GetNode(Selected);
  else if(Index != -1 && TreeView->Items->Count)
    TreeView->Selected = TreeView->Items->Item[Index < TreeView->Items->Count ? Index : TreeView->Items->Count-1];
}
//---------------------------------------------------------------------------
void TForm1::ChangeVisible(TGraphElemPtr GraphElem)
{
  if(GraphElem)
  {
    UndoList.Push(TUndoChangeGraphElemSetting<int>(GraphElem, &GraphElem->GetVisible, &GraphElem->SetVisible));
    GraphElem->ChangeVisible();
    switch(GraphElem->GetVisible())
    {
      case -1: TreeView->Selected->StateIndex =  iiGrayed; break;
      case 0:  TreeView->Selected->StateIndex =  iiUnChecked; break;
      case 1:  TreeView->Selected->StateIndex =  iiChecked; break;
    }
    Python::ExecutePluginEvent(Python::peChanged, GraphElem);
    UpdateMenu();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewKeyPress(TObject *Sender, char &Key)
{
  switch(Key)
  {
		case 13:
      Tree_Properties->OnClick(Sender);
      Key = 0;
      break;

    case ' ':
      ChangeVisible(GetGraphElem(TreeView->Selected));
      break;
  }
  Key = 0;
}
//---------------------------------------------------------------------------
void TForm1::SetCross(int X, int Y)
{
  if(!Draw.Updating() && !!Form9)
    Form9->StartValueChanged(X, Y);
}
//---------------------------------------------------------------------------
void TForm1::SetCrossPos(long double x, long double y)
{
  Cross->SetPos(Draw.xPoint(x), Draw.yPoint(y));
  Cross->Show();               
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UpdateEval()
{
  if(Form9)
    Form9->Edit1Change(NULL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormResize(TObject *Sender)
{
  if(Panel3->Width > ClientWidth - Splitter1->MinSize)
    Panel3->Width = ClientWidth - Splitter1->MinSize;

  //We need to set Align when form is maximized to update height of the toolbar in case it has changed.
  if(WindowState == wsMaximized)
    ActionToolBar1->Align = ActionToolBar1->Align;

  if(Panel6->VisibleDockClientCount && Splitter2->Top < Splitter2->MinSize)
    Panel6->Height = Panel5->Height - Splitter2->MinSize;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::BeginUpdate()
{
  //Start timer to delay the cursor update to avoid blinking.
  if(Updating++ == 0)
    Timer3->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::EndUpdate()
{
  if(--Updating == 0)
  {
    Timer3->Enabled = false;
    Screen->Cursor = crDefault;
    ShowStatusMessage("");
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ApplicationEventsActivate(TObject *Sender)
{
  UpdateMenu();
}
//---------------------------------------------------------------------------
//Use to zoom in at a given pixel position
bool TForm1::Zoom(TPoint Pos, double ZoomRate, bool ChangeUnits)
{
  return Zoom(Draw.xCoord(Pos.x), Draw.yCoord(Pos.y), ZoomRate, ZoomRate, ChangeUnits);
}
//---------------------------------------------------------------------------
//Use to zoom in and center at current center
bool TForm1::Zoom(double ZoomRate, bool ChangeUnits)
{
  return Zoom(ZoomRate, ZoomRate, ChangeUnits);
}
//---------------------------------------------------------------------------
bool TForm1::Zoom(double xZoomRate, double yZoomRate, bool ChangeUnits)
{
  const TAxes &Axes = Data.Axes;
  double x, y;
  if(Axes.xAxis.LogScl)
    x = std::exp((std::log(Axes.xAxis.Max) + std::log(Axes.xAxis.Min)) / 2);
  else
    x = (Axes.xAxis.Max + Axes.xAxis.Min) / 2;

  if(Axes.yAxis.LogScl)
    y = std::exp((std::log(Axes.yAxis.Max) + std::log(Axes.yAxis.Min)) / 2);
  else
    y = (Axes.yAxis.Max + Axes.yAxis.Min) / 2;

  return Zoom(x, y, xZoomRate, yZoomRate, ChangeUnits);
}
//---------------------------------------------------------------------------
//Use this function to zoom in at a given position
//ZoomRate = sqrt(Z), where Z is the relative window size
//If you want the new window to be 1/4 of the current:  Z=0.25 => ZoomRate=0.5
//If you want the new window to be 4 times the current: Z=4    => ZoomRate=2
bool TForm1::Zoom(double x, double y, double xZoomRate, double yZoomRate, bool ChangeUnits)
{
	TAxes &Axes = Data.Axes;
	double xMin,xMax,yMin,yMax;

	xZoomRate /= 2;
	yZoomRate /= 2;
	if(Axes.xAxis.LogScl)
	{
		xMin = std::exp(std::log(x) - std::log(Axes.xAxis.Max / Axes.xAxis.Min) * xZoomRate);
		xMax = std::exp(std::log(x) + std::log(Axes.xAxis.Max / Axes.xAxis.Min) * xZoomRate);
	}
	else
	{
		xMin = x - (Axes.xAxis.Max - Axes.xAxis.Min) * xZoomRate;
		xMax = x + (Axes.xAxis.Max - Axes.xAxis.Min) * xZoomRate;
	}
	if(Axes.yAxis.LogScl)
	{
		yMin = std::exp(std::log(y) - std::log(Axes.yAxis.Max / Axes.yAxis.Min) * yZoomRate);
		yMax = std::exp(std::log(y) + std::log(Axes.yAxis.Max / Axes.yAxis.Min) * yZoomRate);
	}
	else
	{
		yMin = y - (Axes.yAxis.Max - Axes.yAxis.Min) * yZoomRate;
		yMax = y + (Axes.yAxis.Max - Axes.yAxis.Min) * yZoomRate;
	}

  if(ZoomWindow(xMin, xMax, yMin, yMax, false))
  {
    if(ChangeUnits)
    {
      Axes.xAxis.TickUnit = Axes.xAxis.LogScl ? std::pow(Axes.xAxis.TickUnit, 2*xZoomRate) : Axes.xAxis.TickUnit * 2*xZoomRate;
      Axes.yAxis.TickUnit = Axes.yAxis.LogScl ? std::pow(Axes.yAxis.TickUnit, 2*yZoomRate) : Axes.yAxis.TickUnit * 2*yZoomRate;
      Axes.xAxis.GridUnit = Axes.xAxis.LogScl ? std::pow(Axes.xAxis.GridUnit, 2*xZoomRate) : Axes.xAxis.GridUnit * 2*xZoomRate;
      Axes.yAxis.GridUnit = Axes.yAxis.LogScl ? std::pow(Axes.yAxis.GridUnit, 2*yZoomRate) : Axes.yAxis.GridUnit * 2*yZoomRate;
    }

    Data.SetModified();
    Redraw();
    UpdateMenu();

    if(!MoveAction->Checked)
      UpdateEval();
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
//Use this function to zoom in at a given position, where the given position stay fixed.
//ZoomRate = sqrt(Z), where Z is the relative window size
//If you want the new window to be 1/4 of the current:  Z=0.25 => ZoomRate=0.5
//If you want the new window to be 4 times the current: Z=4    => ZoomRate=2
bool TForm1::ZoomPoint(const TPoint &Pos, double ZoomRate, bool ChangeUnits)
{
	TAxes &Axes = Data.Axes;
	double xMin,xMax,yMin,yMax;
  double x = Draw.xCoord(Pos.x);
  double y = Draw.yCoord(Pos.y);

	if(Axes.xAxis.LogScl)
	{
		xMin = std::exp(std::log(x) - std::log(x / Axes.xAxis.Min) * ZoomRate);
		xMax = std::exp(std::log(x) + std::log(Axes.xAxis.Max / x) * ZoomRate);
	}
	else
	{
		xMin = x - (x - Axes.xAxis.Min) * ZoomRate;
		xMax = x + (Axes.xAxis.Max - x) * ZoomRate;
	}
	if(Axes.yAxis.LogScl)
	{
		yMin = std::exp(std::log(y) - std::log(y / Axes.yAxis.Min) * ZoomRate);
		yMax = std::exp(std::log(y) + std::log(Axes.yAxis.Max / y) * ZoomRate);
	}
	else
	{
		yMin = y - (y - Axes.yAxis.Min) * ZoomRate;
		yMax = y + (Axes.yAxis.Max - y) * ZoomRate;
	}

  if(ZoomWindow(xMin, xMax, yMin, yMax, false))
  {
    if(ChangeUnits)
    {
      Axes.xAxis.TickUnit = Axes.xAxis.LogScl ? std::pow(Axes.xAxis.TickUnit, ZoomRate) : Axes.xAxis.TickUnit * ZoomRate;
      Axes.yAxis.TickUnit = Axes.yAxis.LogScl ? std::pow(Axes.yAxis.TickUnit, ZoomRate) : Axes.yAxis.TickUnit * ZoomRate;
      Axes.xAxis.GridUnit = Axes.xAxis.LogScl ? std::pow(Axes.xAxis.GridUnit, ZoomRate) : Axes.xAxis.GridUnit * ZoomRate;
      Axes.yAxis.GridUnit = Axes.yAxis.LogScl ? std::pow(Axes.yAxis.GridUnit, ZoomRate) : Axes.yAxis.GridUnit * ZoomRate;
    }

    Data.SetModified();
    Redraw();
    UpdateMenu();
    SetPanelCoordText();

    if(!MoveAction->Checked)
      UpdateEval();
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel2MouseLeave(TObject *Sender)
{
  //Clear mouse cursor coordinate in status bar if mouse is outside Panel2.
  //This event may also happen if we are over another control inside Panel2, eg. Image2
  //Don't use Mouse->CursorPos. A user (Veet) has reported it to fail sometimes)
	DWORD P = GetMessagePos();
  POINTS Points = MAKEPOINTS(P);
  TPoint Pos = Panel2->ScreenToClient(TPoint(Points.x, Points.y));
  if(!InsideRect(Panel2->ClientRect, Pos))
    StatusBar1->Panels->Items[1]->Text = "";
}
//---------------------------------------------------------------------------
//Create a function icon with the specified color and add it to the image list
int TForm1::AddFuncImage(TColor Color)
{
  //The sine is plottet as 4 bezier curves
  //See http://stackoverflow.com/questions/13932704/how-to-draw-sine-waves-with-svg-js
  int Size = TreeViewImageList->Width;
  TPoint Data[] = {
    TPoint(0,Size/2),
    TPoint(Size*0.08153, Size*(0.5-1.5*0.08153)), TPoint(Size*0.15915, Size*(0.5-1.5*0.15915)), TPoint(Size/4, Size*(0.5-1.5*0.15915)),
    TPoint(Size*(0.25+0.08153), Size*(0.5-1.5*0.15915)), TPoint(Size*(0.25+0.15915), Size*(0.5-1.5*0.08153)), TPoint(Size/2, Size/2),
    TPoint(Size*(0.5+0.08153), Size*(0.5+1.5*0.08153)), TPoint(Size*(0.5+0.15915), Size*(0.5+1.5*0.15915)), TPoint(Size*0.75, Size*(0.5+1.5*0.15915)),
    TPoint(Size*(0.75+0.08153), Size*(0.5+1.5*0.15915)), TPoint(Size*(0.75+0.15915), Size*(0.5+1.5*0.08153)), TPoint(Size, Size/2)
  };

  std::auto_ptr<Graphics::TBitmap> Bitmap(new Graphics::TBitmap);
  Bitmap->SetSize(Size, Size);
  Bitmap->Canvas->Brush->Color = clWhite;
  Bitmap->Canvas->FillRect(TRect(0, 0, Bitmap->Width, Bitmap->Height));

  Graph::TContext Context(Bitmap->Canvas);
  Context.SetSmoothingMode(smAntiAlias);
  Context.SetPen(psSolid, Color, std::ceil(Size/8.0));
  Context.DrawPolyBezier(Data, 13);
  return TreeViewImageList->AddMasked(Bitmap.get(), clWhite);
}
//---------------------------------------------------------------------------
int TForm1::AddImage(TColor Color, TBrushStyle Style)
{
  std::auto_ptr<Graphics::TBitmap> Bitmap(new Graphics::TBitmap);
  Bitmap->Width = TreeViewImageList->Width;
  Bitmap->Height = TreeViewImageList->Height;
  Bitmap->Canvas->Brush->Color = Color;
  Bitmap->Canvas->Brush->Style = Style;
  SetBkColor(Bitmap->Canvas->Handle, clWhite);
  SetBkMode(Bitmap->Canvas->Handle, OPAQUE);
  Bitmap->Canvas->FillRect(TRect(0, 0, Bitmap->Width, Bitmap->Height));
  return TreeViewImageList->AddMasked(Bitmap.get(), clWhite);
}
//---------------------------------------------------------------------------
int TForm1::AddImage(TBitmap *Bitmap, TColor BackgroundColor)
{
  return TreeViewImageList->AddMasked(Bitmap, BackgroundColor);
}
//---------------------------------------------------------------------------
bool __fastcall TForm1::Recent1LoadFile(TRecent *Sender, String FileName)
{
  if(AskSave())
  {
    LoadFromFile(FileName);
    if(Form9)
      Form9->Clear();

    if(TreeView->Items->Count)
    {
      TreeView->Select(TreeView->Items->Item[0]);
      Form9->FuncChanged(Data.GetElem(0));
    }
    else
			Form9->FuncChanged(boost::shared_ptr<TGraphElem>());
    Redraw();
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::NewActionExecute(TObject *Sender)
{
  if(AskSave())
  {
    if(Form9)
      Form9->Clear();
    LoadDefault();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OpenActionExecute(TObject *Sender)
{
  if(AskSave())
  {
    if(Form9)
      Form9->Clear();
    OpenPreviewDialog1->Filter = LoadRes(RES_GRAPH_FILTER);
    if(OpenPreviewDialog1->Execute())
    {
      LoadFromFile(OpenPreviewDialog1->FileName);
      if(TreeView->Items->Count)
        TreeView->Items->Item[0]->Selected = true;
      Redraw();
    }
  }
}
//---------------------------------------------------------------------------
bool TForm1::DoSave(const String &FileName, bool Remember)
{
  bool Result = false;
  if(FileName.IsEmpty())
  {
    SaveDialog->FileName = "";
    SaveDialog->DefaultExt = "grf";
    SaveDialog->Filter = LoadRes(RES_FILE_FILTER);
    if(SaveDialog->Execute())
    {
      Result = Data.Save(SaveDialog->FileName.c_str(), Remember);
      if(Result)
        Recent1->FileUsed(SaveDialog->FileName);
      else
        MessageBox(LoadRes(RES_FILE_ACCESS, SaveDialog->FileName), LoadRes(RES_WRITE_FAILED), MB_ICONSTOP);
    }
  }
  else
  {
    Result = Data.Save(FileName.c_str(), Remember);
    if(Result)
      Recent1->FileUsed(FileName);
    else
      MessageBox(LoadRes(RES_FILE_ACCESS, FileName), LoadRes(RES_WRITE_FAILED), MB_ICONSTOP);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SaveActionExecute(TObject *Sender)
{
  DoSave(Data.GetFileName().c_str(), true);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SaveAsActionExecute(TObject *Sender)
{
  DoSave("", true);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SaveAsImageActionExecute(TObject *Sender)
{
  //Remove file extention
  SaveDialogEx1->FileName = ChangeFileExt(ToUString(Data.GetFileName()), "");
  SaveDialogEx1->DefaultExt = "emf";
  SaveDialogEx1->Filter =
    "Windows Enhanced Metafile [*.emf]|*.emf|"
//    "Encapsulated PostScript [*.eps]|*.eps|"
    "Scalable Vector Graphics [*.svg]|*.svg|"
    "Windows Bitmap [*.bmp]|*.bmp|"
    "Portable Network Graphics [*.png]|*.png|"
    "Joint Photographic Experts Group [*.jpeg,*.jpg]|*.jpeg;*.jpg|"
    "Portable Document Format [*.pdf]|*.pdf";
  SaveDialogEx1->FilterIndex = GetRegValue(REGISTRY_KEY "\\SaveAsImage", L"FilterIndex", HKEY_CURRENT_USER, 1);

  ActionImageOptions.reset(new TImageOptions(Image1->Width, Image1->Height));
  ActionImageOptions->LoadSettings();
  if(SaveDialogEx1->Execute())
    try
    {
      String FileName = SaveDialogEx1->FileName; //SaveAsImage() calls ProcessMessages() and may change SaveDialogEx1
			SaveAsImage(FileName, SaveDialogEx1->FilterIndex, *ActionImageOptions);
      SetRegValue(REGISTRY_KEY "\\SaveAsImage", L"FilterIndex", HKEY_CURRENT_USER, SaveDialogEx1->FilterIndex);
    }
    catch(Exception &E)
    {
      MessageBox(E.Message, LoadRes(RES_ERROR), MB_OK | MB_ICONSTOP);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PrintActionExecute(TObject *Sender)
{
  if(Printer()->Printing)
  {
    if(MessageBox(LoadRes(521), NAME, MB_OKCANCEL | MB_ICONEXCLAMATION) != IDCANCEL)
      AbortPrinting = true;
    return;
  }

  IPrintDialog1->ImageWidth = Image1->Width;
  IPrintDialog1->ImageHeight = Image1->Height;

  //Throws EPrinter on error
  try
  {
    if(!IPrintDialog1->Execute())
      return; //User pressed Cancel
  }
  catch(EPrinter &E)
	{
    MessageBox(E.Message, LoadRes(RES_PRINTER_ERROR), MB_ICONSTOP);
    return;
  }

  SetStatusIcon(iiPrint);  //Show printer icon in statusbar
//  TCallOnRelease Dummy(&SetStatusIcon, -1); //May cause ICE in C++ Builder XE
  try
  {
    Printer()->Title = Caption; //Set document title
    Printer()->BeginDoc();      //Start sending to print spooler

    HDC ScreenDC = GetDC(NULL);
    double xMM = GetDeviceCaps(ScreenDC, HORZSIZE); //Get horizontal width of screen in mm
    double yMM = GetDeviceCaps(ScreenDC, VERTSIZE); //Get vertical width of screen in mm
    double xScreenResolution = Screen->Width * 25.4 / xMM; //Get horizontal resolution of screen in pixels per inch
    double yScreenResolution = Screen->Height * 25.4 / yMM; //Get horizontal resolution of screen in pixels per inch
    ReleaseDC(NULL, ScreenDC);
    double xPrinterResolution = GetDeviceCaps(Printer()->Handle, LOGPIXELSX); //Horizontal pixels per inch on printer
    double yPrinterResolution = GetDeviceCaps(Printer()->Handle, LOGPIXELSY); //Vertical pixels per inch on printer
    double xSizeMul = xPrinterResolution / xScreenResolution;
    double ySizeMul = yPrinterResolution / yScreenResolution;

    unsigned Copies = IPrintDialog1->Copies;
    TRect Area = IPrintDialog1->DrawingArea;

    Draw.AbortUpdate();
    TData PrintData(Data);

    //We need to update size of all text labels
    //This is only used for printing, and this is not a good way to find the size; To make it work I scaled by 1.1
    for(unsigned I = 0; I < PrintData.ElemCount(); I++)
      if(TTextLabel *Label = dynamic_cast<TTextLabel*>(PrintData.GetElem(I).get()))
        Label->Scale(xSizeMul, ySizeMul);

    TDraw PrintDraw(Printer()->Canvas, &PrintData, "Printing DrawThread", false);
    PrintDraw.SetArea(Area);  //Set drawing area
    PrintDraw.SetSizeMul(xSizeMul);

    AbortPrinting = false;
    for(unsigned I = 0; I < Copies && !AbortPrinting; I++)
    {
      if(I > 0)
        Printer()->NewPage();
      PrintDraw.DrawAll();           //Draw graphs
      while(PrintDraw.Updating())
      { //Process messages while waiting for draw thread to finish
        Sleep(100);
        Application->ProcessMessages();
        //OnClose set AbortPrinting to signal that we want to close the application
        if(AbortPrinting)
          PrintDraw.AbortUpdate();
      }
    }

    PrintDraw.Wait();
    if(AbortPrinting)
      Printer()->Abort();
    else
      Printer()->EndDoc();             //End printing
  }
  catch(...)
  {
    SetStatusIcon(-1);
    throw;
  }
  SetStatusIcon(-1);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ExitActionExecute(TObject *Sender)
{
  Close();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UndoActionExecute(TObject *Sender)
{
  Draw.AbortUpdate();
  UndoList.Undo();
  Data.SetModified();
  UpdateTreeView();
  UpdateMenu();
  Redraw();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RedoActionExecute(TObject *Sender)
{
  Draw.AbortUpdate();
  UndoList.Redo();
  Data.SetModified();
  UpdateTreeView();
  UpdateMenu();
  Redraw();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CutActionExecute(TObject *Sender)
{
  //Copy from the edit control if it has focus
  if(TEdit *Edit = dynamic_cast<TEdit*>(ActiveControl))
  {
    Edit->CutToClipboard();
		return;
  }

  if(!TreeView->Selected)
    return;

  boost::shared_ptr<TGraphElem> Elem = GetGraphElem(TreeView->Selected);
  GraphClipboard.Copy(Data, Elem);
  DeleteGraphElem(Elem);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CopyActionExecute(TObject *Sender)
{
  //Copy from the edit control if it has focus
  if(TCustomEdit *Edit = dynamic_cast<TCustomEdit*>(ActiveControl))
  {
    Edit->CopyToClipboard();
    return;
  }

  TGraphElemPtr Elem = GetGraphElem(TreeView->Selected);
  GraphClipboard.Copy(Data, Elem);
  UpdateMenu();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PasteActionExecute(TObject *Sender)
{
  Draw.AbortUpdate();
	GraphClipboard.Paste(Data);
  Python::ExecutePluginEvent(Python::peNewElem, Data.Back());
  UpdateTreeView(Data.Back());
  UpdateMenu();
  UpdateEval();
  Redraw();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CopyImageActionExecute(TObject *Sender)
{
  try
  {
#ifndef _WIN64
    IDataObject *Dummy = NULL;
    OleCheck(OleInitialize(NULL));
    if(!OleServerRunning())
      OleCheck(TOleServerImpl::_CreatorClass::CreateInstance(NULL, IID_IDataObject, &(void*&)Dummy));

    if(FAILED(OleSetClipboard(OleServerImpl)))
      CopyAsImageToClipboard(); //Only copy bitmap and metafile
    else
      OleCheck(OleFlushClipboard());

    if(Dummy)
      OleCheck(Dummy->Release());

    OleUninitialize();
#else
    CopyAsImageToClipboard(); //Only copy bitmap and metafile
#endif
  }
  catch(Exception &E)
  {
    ShowErrorMsg(E);
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AxesActionExecute(TObject *Sender)
{
	if(CreateForm<TForm3>(Data)->ShowModal() == mrOk)
  {
    Draw.AbortUpdate();
    Cross->Hide();
    Data.ClearCache();
    Data.SetModified();
    UpdateTreeView(); //Update the check box for AxesView
    Python::ExecutePluginEvent(Python::peAxesChanged);
    Redraw();
    UpdateEval();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OptionsActionExecute(TObject *Sender)
{
  if(CreateForm<TForm4>(Data)->ShowModal()==mrOk)
  {
    Draw.AbortUpdate();
    Data.ClearCache();
    Data.Update();
    Python::ExecutePluginEvent(Python::peOptionsChanged);
    Redraw();
    UpdateMenu();
    ScaleImages();
    UpdateTreeView();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertFunctionActionExecute(TObject *Sender)
{
	if(CreateForm<TForm5>(Data)->ShowModal() == mrOk)
  {
    Python::ExecutePluginEvent(Python::peNewElem, Data.Back());
    UpdateTreeView(Data.Back());
    TreeView->SetFocus();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertTangentActionExecute(TObject *Sender)
{
  boost::shared_ptr<TBaseFuncType> Func = boost::dynamic_pointer_cast<TBaseFuncType>(GetGraphElem(TreeView->Selected));
  if(Func)
    if(CreateForm<TForm12>(Data)->InsertTan(Func) == mrOk)
    {
      Python::ExecutePluginEvent(Python::peNewElem, Func->GetChild(Func->ChildCount()-1));
      UpdateTreeView(Func->GetChild(Func->ChildCount()-1));
      Redraw();
      UpdateMenu();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertShadingActionExecute(TObject *Sender)
{
  boost::shared_ptr<TBaseFuncType> Func = boost::dynamic_pointer_cast<TBaseFuncType>(Selected);
  if(Func)
    if(CreateForm<TForm16>(Data)->InsertShade(Func) == mrOk)
    {
      Python::ExecutePluginEvent(Python::peNewElem, Func->GetChild(Func->ChildCount()-1));
      UpdateTreeView(Func->GetChild(Func->ChildCount()-1));
      Redraw();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertPointsActionExecute(TObject *Sender)
{
  if(CreateForm<TForm14>(Data)->ShowModal() == mrOk)
  {
    Python::ExecutePluginEvent(Python::peNewElem, Data.Back());
    UpdateTreeView(Data.Back());
    Redraw();
    UpdateMenu();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertTrendlineActionExecute(TObject *Sender)
{
  boost::shared_ptr<TPointSeries> Series = boost::dynamic_pointer_cast<TPointSeries>(Selected);
  if(Series)
    if(CreateForm<TForm13>(Data)->InsertTrendline(Series) == mrOk)
    {
      Python::ExecutePluginEvent(Python::peNewElem, Data.Back());
      UpdateTreeView(Data.Back());
      Redraw();
      UpdateMenu();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::EditActionExecute(TObject *Sender)
{
  TGraphElemPtr Item = GetGraphElem(TreeView->Selected);

  if(Python::ExecutePluginEvent(Python::peEdit, Item))
    return;

  TModalResult Result = mrNone;
  if(boost::shared_ptr<TTangent> Tan = boost::dynamic_pointer_cast<TTangent>(Item))
    Result = CreateForm<TForm12>(Data)->EditTan(Tan);
  else if(boost::shared_ptr<TBaseFuncType> Func = boost::dynamic_pointer_cast<TBaseFuncType>(Item))
    Result = CreateForm<TForm5>(Data)->EditFunc(Func);
  else if(boost::shared_ptr<TPointSeries> PointSeries = boost::dynamic_pointer_cast<TPointSeries>(Item))
    Result = CreateForm<TForm14>(Data)->EditPointSeries(PointSeries);
  else if(boost::shared_ptr<TShading> Shade = boost::dynamic_pointer_cast<TShading>(Item))
    Result = CreateForm<TForm16>(Data)->EditShade(Shade);
  else if(boost::shared_ptr<TTextLabel> Label = boost::dynamic_pointer_cast<TTextLabel>(Item))
    EditLabel(Label);
  else if(boost::shared_ptr<TRelation> Relation = boost::dynamic_pointer_cast<TRelation>(Item))
    Result = CreateForm<TForm11>(Data)->EditRelation(Relation);
  else if(boost::shared_ptr<TAxesView> AxesView = boost::dynamic_pointer_cast<TAxesView>(Item))
    AxesAction->Execute();

  if(Result == mrOk)
  {
    UpdateTreeView();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void TForm1::DeleteGraphElem(const boost::shared_ptr<TGraphElem> &GraphElem)
{
  Draw.AbortUpdate();
  Python::ExecutePluginEvent(Python::peDelete, GraphElem);

  UndoList.BeginMultiUndo();
  if(dynamic_cast<const TBaseFuncType*>(GraphElem.get()))
  {
    //Delete all shades bound to the function
    for(unsigned I = 0; I < Data.ElemCount(); I++)
    {
      const TGraphElemPtr &Elem = Data.GetElem(I);
      unsigned ChildCount = Elem->ChildCount();
      for(unsigned J = 0; J < ChildCount; J++)
        if(boost::shared_ptr<TShading> Shade = boost::dynamic_pointer_cast<TShading>(Elem->GetChild(J)))
          if(Shade->GetFunc2() == GraphElem)
          {
            UndoList.Push(TUndoDel(Shade, Shade->GetParent(), J));
            Data.Delete(Shade);
          }
    }
  }

  GraphElem->ClearCache();
  UndoList.Push(TUndoDel(GraphElem, GraphElem->GetParent(), Data.GetIndex(GraphElem)));
  UndoList.EndMultiUndo();

  Data.Delete(GraphElem);
  UpdateTreeView();
  UpdateMenu();
  Redraw();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DeleteActionExecute(TObject *Sender)
{
  TTreeNode *Node = TreeView->Selected;
  if(MessageBox(LoadRes(RES_DELETE, Node->Text), LoadRes(RES_CAPTION_DELETE), MB_ICONQUESTION|MB_YESNO) == IDYES)
    DeleteGraphElem(GetGraphElem(Node));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertDifActionExecute(TObject *Sender)
{
  if(boost::shared_ptr<TBaseFuncType> Func = boost::dynamic_pointer_cast<TBaseFuncType>(Selected))
    if(CreateForm<TForm7>(Data)->InsertDif(Func) == mrOk)
    {
      Python::ExecutePluginEvent(Python::peNewElem, Data.Back());
      UpdateTreeView(Data.Back());
      Redraw();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomInActionExecute(TObject *Sender)
{
  //Zoom in; New window is 1/4 of the original
  Zoom(GetKeyState(ssShift) ? GuiSettings.MajorZoomIn : GuiSettings.MinorZoomIn, false);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomOutActionExecute(TObject *Sender)
{
  //Zoom out; New window is the double of the original
  Zoom(GetKeyState(ssShift) ? GuiSettings.MajorZoomOut : GuiSettings.MinorZoomOut, false);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomWindowActionExecute(TObject *Sender)
{
  SetCursorState(csZoomWindow);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomStandardActionExecute(TObject *Sender)
{
  UndoList.Push(TUndoAxes(Data));
  TAxis xAxis, yAxis;

  std::wstring Str = GetRegValue(REGISTRY_KEY, L"DefaultAxes", HKEY_CURRENT_USER, L"");
  TConfigFile ConfigFile;
  ConfigFile.LoadFromString(Str.c_str());
  xAxis.ReadFromIni(ConfigFile.Section(L"Axes"), L"x");
  yAxis.ReadFromIni(ConfigFile.Section(L"Axes"), L"y");

	Data.Axes.xAxis.Min = xAxis.Min;
	Data.Axes.xAxis.Max = xAxis.Max;
	Data.Axes.xAxis.TickUnit = xAxis.TickUnit;
	Data.Axes.xAxis.GridUnit = xAxis.GridUnit;
	Data.Axes.xAxis.AutoTick = xAxis.AutoTick;
	Data.Axes.xAxis.AutoGrid = xAxis.AutoGrid;
	Data.Axes.xAxis.LogScl = xAxis.LogScl;

	Data.Axes.yAxis.Min = yAxis.Min;
	Data.Axes.yAxis.Max = yAxis.Max;
	Data.Axes.yAxis.TickUnit = yAxis.TickUnit;
	Data.Axes.yAxis.GridUnit = yAxis.GridUnit;
	Data.Axes.yAxis.AutoTick = yAxis.AutoTick;
	Data.Axes.yAxis.AutoGrid = yAxis.AutoGrid;
	Data.Axes.yAxis.LogScl = yAxis.LogScl;

	Data.ClearCache();
  Data.SetModified();
  Python::ExecutePluginEvent(Python::peZoom);
  Redraw();
  if(!MoveAction->Checked)
    UpdateEval();
  UpdateMenu();  
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PathActionExecute(TObject *Sender)
{
  PathAction->Checked = !PathAction->Checked;
  EvalAction->Checked = false;
  AreaAction->Checked = false;
  if(PathAction->Checked)
    Form9->SetEvalType(etArc);
  else
    Form9->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AreaActionExecute(TObject *Sender)
{
  AreaAction->Checked = !AreaAction->Checked;
  PathAction->Checked = false;
  EvalAction->Checked = false;
  if(AreaAction->Checked)
    Form9->SetEvalType(etArea);
  else
    Form9->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::EvalActionExecute(TObject *Sender)
{
  EvalAction->Checked = !EvalAction->Checked;
  PathAction->Checked = false;
  AreaAction->Checked = false;
  if(EvalAction->Checked)
  {
    Form9->SetEvalType(etEval);
    Form9->FuncChanged(GetGraphElem(TreeView->Selected));
  }
  else
    Form9->Visible = false;

  if(MoveAction->Checked)
	MoveAction->Execute();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TableActionExecute(TObject *Sender)
{
  if(TBaseFuncType *Func = dynamic_cast<TBaseFuncType*>(Selected.get()))
    CreateForm<TForm15>()->ShowTable(Func);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ContentsActionExecute(TObject *Sender)
{
  ShowHelp("");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ListActionExecute(TObject *Sender)
{
  ShowHelp("ListOfFunctions.html");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FaqActionExecute(TObject *Sender)
{
  ShowHelp("FAQ.html");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::HomePageActionExecute(TObject *Sender)
{
  ShellExecute(Handle, NULL, HOMEPAGE, NULL, NULL, SW_SHOWMAXIMIZED);
}
//---------------------------------------------------------------------------
void TForm1::CheckForUpdate(bool StartupCheck)
{
  unsigned long Flags;
  InternetGetConnectedState(&Flags, 0);
  if((Flags & INTERNET_CONNECTION_MODEM) && StartupCheck)
	return; //If user has a modem, he will be asked if he will connect to the Internet if we continue

  try
  {
		TVersionInfo Info;
		std::auto_ptr<TMemoryStream> Stream(new TMemoryStream);
		std::auto_ptr<TIdHTTP> IdHTTP(new TIdHTTP(NULL));

		IdHTTP->Request->UserAgent = (L"Mozilla/3.0 (compatible; Graph " + Info.StringValue(L"FileVersion") + L')').c_str();
		IdHTTP->HandleRedirects = true;

		std::wstring Url = GetRegValue(REGISTRY_KEY, L"InfFile", HKEY_CURRENT_USER, INF_FILE);
		Url += L"?Version=" + Info.StringValue(L"ProductVersion");
		if(Info.FileFlags() & ffDebug)
			Url += L"&Build=" + ToWString(Info.FileVersion().Build);
		Url += L"&Language=" + Options.Language;
		IdHTTP->Get(Url.c_str(), Stream.get());

		Stream->Position = 0;
    std::auto_ptr<TStringList> StringList(new TStringList);
    StringList->LoadFromStream(Stream.get());
    std::auto_ptr<TMemIniFile> IniFile(new TMemIniFile(""));
    IniFile->SetStrings(StringList.get());
    int Major = IniFile->ReadInteger("Graph", "Major", 0);
    int Minor = IniFile->ReadInteger("Graph", "Minor", 0);
    int Release = IniFile->ReadInteger("Graph", "Release", 0);
    int Build = IniFile->ReadInteger("Graph", "Build", 0);
    String DownloadPage = IniFile->ReadString("Graph", "DownloadPage", "http://www.padowan.dk");

    //Check if a newer version is available, or if this is a debug version one with the same version
    if(Info.InfoAvailable())
    {
      TVersion Version = Info.ProductVersion();
      if(Major > Version.Major || (Major == Version.Major && (Minor > Version.Minor ||
        (Minor == Version.Minor && (Release > Version.Release ||
        (Release == Version.Release && (Build > Version.Build)))))))
      {
        String VersionString = String(Major) + '.' + Minor;
        if(Release)
          VersionString += "." + String(Release);
        String Str = LoadRes(541, VersionString);
        if(StartupCheck)
          Str += LoadRes(550);
        if(MessageBox(Str, LoadRes(540), MB_YESNO) == ID_YES)
          ShellExecute(Handle, NULL, DownloadPage.c_str(), NULL, NULL, SW_SHOWMAXIMIZED);
      }
        else if(!StartupCheck)
        MessageBox(LoadRes(543), LoadRes(542), MB_ICONINFORMATION);
    }
  }
  catch(EIdSocketError &E)
  {
    if(!StartupCheck)
      MessageBox(LoadRes(544), LoadRes(545, E.LastError), MB_ICONSTOP);
  }
  catch(Exception &E)
  {
    if(!StartupCheck)
      MessageBox(E.Message, LoadRes(RES_ERROR), MB_ICONSTOP);
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UpdateActionExecute(TObject *Sender)
{
  TWaitCursor Dummy;
  ShowStatusError(LoadRes(520), clGreen, 0);
  CheckForUpdate(false);
  CancelStatusError();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AboutActionExecute(TObject *Sender)
{
  ShowForm<TForm2>();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MoveRightActionExecute(TObject *Sender)
{
  const TAxes &Axes = Data.Axes;
  double StepSize = GetKeyState(ssShift) ? GuiSettings.MajorStepSize : GuiSettings.MinorStepSize;
  double MoveWidth=(Axes.xAxis.Max-Axes.xAxis.Min) * StepSize;
  if(Axes.xAxis.LogScl)
  {
    double xMul = std::pow(Axes.xAxis.Max/Axes.xAxis.Min, StepSize);
    ZoomWindow(Axes.xAxis.Min*xMul, Axes.xAxis.Max*xMul, Axes.yAxis.Min, Axes.yAxis.Max);
  }
  else
    ZoomWindow(Axes.xAxis.Min+MoveWidth, Axes.xAxis.Max+MoveWidth, Axes.yAxis.Min, Axes.yAxis.Max);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MoveLeftActionExecute(TObject *Sender)
{
  const TAxes &Axes = Data.Axes;
  double StepSize = GetKeyState(ssShift) ? GuiSettings.MajorStepSize : GuiSettings.MinorStepSize;
  double MoveWidth=(Axes.xAxis.Max - Axes.xAxis.Min) * StepSize;
  if(Axes.xAxis.LogScl)
  {
    double xDiv = std::pow(Axes.xAxis.Max/Axes.xAxis.Min, StepSize);
    ZoomWindow(Axes.xAxis.Min/xDiv, Axes.xAxis.Max/xDiv, Axes.yAxis.Min, Axes.yAxis.Max);
  }
  else
    ZoomWindow(Axes.xAxis.Min-MoveWidth, Axes.xAxis.Max-MoveWidth, Axes.yAxis.Min, Axes.yAxis.Max);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MoveUpActionExecute(TObject *Sender)
{
  const TAxes &Axes = Data.Axes;
  double StepSize = GetKeyState(ssShift) ? GuiSettings.MajorStepSize : GuiSettings.MinorStepSize;
  double MoveHeight=(Axes.yAxis.Max-Axes.yAxis.Min) * StepSize;
  if(Axes.yAxis.LogScl)
  {
    double yMul = std::pow(Axes.yAxis.Max/Axes.yAxis.Min, StepSize);
    ZoomWindow(Axes.xAxis.Min, Axes.xAxis.Max, Axes.yAxis.Min*yMul, Axes.yAxis.Max*yMul);
  }
  else
    ZoomWindow(Axes.xAxis.Min, Axes.xAxis.Max, Axes.yAxis.Min+MoveHeight, Axes.yAxis.Max+MoveHeight);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MoveDownActionExecute(TObject *Sender)
{
  const TAxes &Axes = Data.Axes;
  double StepSize = GetKeyState(ssShift) ? GuiSettings.MajorStepSize : GuiSettings.MinorStepSize;
  double MoveHeight=(Axes.yAxis.Max - Axes.yAxis.Min) * StepSize;
  if(Axes.yAxis.LogScl)
  {
    double yDiv = std::pow(Axes.yAxis.Max / Axes.yAxis.Min, StepSize);
    ZoomWindow(Axes.xAxis.Min, Axes.xAxis.Max, Axes.yAxis.Min / yDiv, Axes.yAxis.Max / yDiv);
  }
  else
    ZoomWindow(Axes.xAxis.Min, Axes.xAxis.Max, Axes.yAxis.Min - MoveHeight, Axes.yAxis.Max - MoveHeight);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SeparatorActionUpdate(TObject *Sender)
{
  for(int I = 0; I < ActionToolBar1->ActionClient->Items->Count;  I++)
  {
    TActionClientItem *Item = ActionToolBar1->ActionClient->Items->ActionClients[I];
    if(Item->Action == SeparatorAction)
    {
      Item->Action = NULL;
      Item->Caption = "-";
    }
  }
}
//---------------------------------------------------------------------------
void TForm1::CreateToolBar(String Str)
{
  int Pos;
  String LastName;
  ActionToolBar1->ActionClient->Items->Clear();
  while((Pos = Str.Pos(',')) != 0)
  {
    String ButtonName = Str.SubString(1, Pos - 1);
    Str.Delete(1, Pos);
    if(ButtonName == '-')
    {
      if(LastName != '-')
      {
        TActionClientItem *Item = ActionToolBar1->ActionClient->Items->Add();
        Item->Action = SeparatorAction;
        Item->InitiateAction(); //This is need to avoid an empty second line and make sure the separators are shown.
      }
    }
    else
      for(int I = 0; I < ActionManager->ActionCount; I++)
        if(ButtonName == ActionManager->Actions[I]->Name)
        {
          TActionClientItem *Item = ActionToolBar1->ActionClient->Items->Add();
          Item->Action = ActionManager->Actions[I];
          break;
        }
    LastName = ButtonName;
  }

  ActionToolBar1->Align = alTop; //Makes sure a second line is removed if empty
}
//---------------------------------------------------------------------------
String TForm1::GetToolBar()
{
  String Str;
  for(int I = 0; I < ActionToolBar1->ActionClient->Items->Count; I++)
  {
    TContainedAction *Action = ActionToolBar1->ActionClient->Items->ActionClients[I]->Action;
    if(Action == NULL)
      Str += "-,";
    else if(!Action->Name.IsEmpty())
      Str += Action->Name + ',';
  }
  return Str;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MoveActionExecute(TObject *Sender)
{
  SetCursorState(MoveAction->Checked ? csMove : csIdle);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToolBar_CustomizeClick(TObject *Sender)
{
  CustomizeDlg->Show();
  TCustomizeFrm *Form = CustomizeDlg->CustomizeForm;
  TranslateProperties(Form);
  Form->Icon->Assign(Icon);
  Form->Tabs->Pages[1]->Caption = LoadRes(552);
  Form->ActionsActionsLbl->Caption = LoadRes(552) + L':';
  Form->CatList->Items->Strings[Form->CatList->Items->Count-1] = LoadRes(553);
  Form->InfoLbl->Caption = LoadRes(554);
  Form->ActionsCatLbl->Caption = LoadRes(557);
  Form->ToolbarsTab->TabVisible = false;
  Form->OptionsTab->TabVisible = false;
  Form->SeparatorBtn->Visible = false;
  Form->CatList->ItemIndex = Form->CatList->Items->Count - 1;
  Form->CatList->OnClick(Form->CatList);
  SetAccelerators(Form);
  ScaleForm(Form);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToolBar_ResetClick(TObject *Sender)
{
  CreateToolBar(DefaultToolBar);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CustomizeDlgClose(TObject *Sender)
{
  UpdateMenu();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ApplicationEventsSettingChange(TObject *Sender,
      int Flag, const String Section, int &Result)
{
  //If local options has changed
  if(Flag == 0)
    FormatSettings.DecimalSeparator = '.';
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomFitAllActionExecute(TObject *Sender)
{
  TZoomFit ZoomFit(Data, Draw);

  for(unsigned I = 0; I < Data.ElemCount(); I++)
    if(Data.GetElem(I)->GetVisible())
      Data.GetElem(I)->Accept(ZoomFit);

  if(!ZoomFit.IsChanged())
    return;

  double xMin = ZoomFit.xMin;
  double xMax = ZoomFit.xMax;
  double yMin = ZoomFit.yMin;
  double yMax = ZoomFit.yMax;

  double xMin2 = Data.Axes.xAxis.LogScl ? xMin * std::pow(xMin / xMax, 0.1) : xMin - (xMax - xMin) / 10;
  double xMax2 = Data.Axes.xAxis.LogScl ? xMax * std::pow(xMax / xMin, 0.1) : xMax + (xMax - xMin) / 10;
  double yMin2 = Data.Axes.yAxis.LogScl ? yMin * std::pow(yMin / yMax, 0.1) : yMin - (yMax - yMin) / 10;
  double yMax2 = Data.Axes.yAxis.LogScl ? yMax * std::pow(yMax / yMin, 0.1) : yMax + (yMax - yMin) / 10;

  ZoomWindow(xMin2, xMax2, yMin2, yMax2);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel4DockDrop(TObject *Sender,
      TDragDockObject *Source, int X, int Y)
{
  Panel1->Height = Form9->GetFrameHeight() ? Form9->GetFrameHeight() + DockCaptionHeight : 100;
//  Panel1->Height = Form9->Height;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel4UnDock(TObject *Sender, TControl *Client,
      TWinControl *NewTarget, bool &Allow)
{
  Panel1->Height = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel4DockOver(TObject *Sender,
      TDragDockObject *Source, int X, int Y, TDragState State,
      bool &Accept)
{
  // Modify the DockRect to preview dock area.
  int FormHeight = Form9->GetFrameHeight() ? Form9->GetFrameHeight() + DockCaptionHeight : 100;
  TPoint TopLeft = Panel1->ClientToScreen(TPoint(0, -FormHeight));
  TPoint BottomRight = Panel1->ClientToScreen(TPoint(Panel1->Width, Panel1->Height));
  Source->DockRect = TRect(TopLeft, BottomRight);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomFitActionExecute(TObject *Sender)
{
  if(!TreeView->Selected)
    return;
  boost::shared_ptr<TGraphElem> Elem = GetGraphElem(TreeView->Selected);

  TZoomFit ZoomFit(Data, Draw);
  Elem->Accept(ZoomFit);

  long double xMin = ZoomFit.xMin;
  long double xMax = ZoomFit.xMax;
  long double yMin = ZoomFit.yMin;
  long double yMax = ZoomFit.yMax;

  if(_finitel(xMin) && _finitel(xMax) && _finitel(yMin) && _finitel(yMax))
  {
    long double xMin2 = Data.Axes.xAxis.LogScl ? xMin * std::pow(xMin / xMax, 0.1L) : xMin - (xMax - xMin) / 10;
    long double xMax2 = Data.Axes.xAxis.LogScl ? xMax * std::pow(xMax / xMin, 0.1L) : xMax + (xMax - xMin) / 10;
    long double yMin2 = Data.Axes.yAxis.LogScl ? yMin * std::pow(yMin / yMax, 0.1L) : yMin - (yMax - yMin) / 10;
    long double yMax2 = Data.Axes.yAxis.LogScl ? yMax * std::pow(yMax / yMin, 0.1L) : yMax + (yMax - yMin) / 10;

    ZoomWindow(xMin2, xMax2, yMin2, yMax2);
  }
}
//---------------------------------------------------------------------------
//Workaround for bug in VCL
//Without this, a menu item is sometimes shown in the status bar when it is double buffered,
//See http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&oe=UTF-8&selm=3caf8e6a_1%40dnews&rnum=4
void __fastcall TForm1::WndProc(Messages::TMessage &Message)
{
  if(Message.Msg == WM_DRAWITEM)
    if(StatusBar1->Handle == reinterpret_cast<HWND>(Message.WParam))
      reinterpret_cast<DRAWITEMSTRUCT*>(Message.LParam)->CtlType = ODT_STATIC;
  TForm::WndProc(Message);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomSquareActionExecute(TObject *Sender)
{
  TAxes &Axes = Data.Axes;
  if(Axes.xAxis.LogScl != Axes.yAxis.LogScl)
    return; //Invalid

  UndoList.Push(TUndoAxes(Data));
//  Axes.ZoomSquare = !Axes.ZoomSquare;

  //Don't save undo info. We have already done that
//  if(Axes.ZoomSquare)
  Data.Axes.HandleZoomSquare(Draw.GetScaledYAxis());
  ZoomWindow(Axes.xAxis.Min, Axes.xAxis.Max, Axes.yAxis.Min, Axes.yAxis.Max, true, false);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Tree_ShowInLegendClick(TObject *Sender)
{
  if(TreeView->Selected)
  {
    TGraphElemPtr GraphElem = GetGraphElem(TreeView->Selected);
    UndoList.Push(TUndoChangeShowInLegend(GraphElem));
    GraphElem->SetShowInLegend(Tree_ShowInLegend->Checked);
    Python::ExecutePluginEvent(Python::peChanged, GraphElem);
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertLabelActionExecute(TObject *Sender)
{
  std::auto_ptr<TForm6> Form6(new TForm6(Application, Options.DefaultLabelFont, NAME, Data.GetFileName().c_str()));
  if(Form6->ShowModal() == mrOk && !Form6->IsEmpty())
  {
    Options.DefaultLabelFont = Form6->GetFont();
    boost::shared_ptr<TTextLabel> Label(new TTextLabel(
      Form6->GetText().c_str(),
      lpUserTopLeft,
      TTextValue(Draw.xCoord(Image1->Width / 2)),
      TTextValue(Draw.yCoord(Image1->Height /2)),
      Form6->GetBackgroundColor(),
      0,
      Form6->GetOleLink()
    ));
    Data.Insert(Label);
    Label->Update();
    Redraw();
    UndoList.Push(TUndoAdd(Label));
    Python::ExecutePluginEvent(Python::peNewElem, Data.Back());
    UpdateTreeView(Data.Back());
    UpdateMenu();
  }
}
//---------------------------------------------------------------------------
void TForm1::SetCursorState(TCursorState State)
{
  if(State != csMove && State != csMoving)
    MoveAction->Checked = false;
  if(State != csZoomWindow)
    ZoomWindowAction->Checked = false;
  switch(State)
  {
    case csIdle:
    {
      MoveAction->Checked = false;
      Shape1->Visible = false;
      if(CursorState == csZoomWindow)
        ShowStatusMessage("", true); //Only if state has changed
      TPoint Pos = Image1->ScreenToClient(Mouse->CursorPos);
      if(!!Data.FindLabel(Pos.x, Pos.y) || Draw.InsideLegend(Pos.x, Pos.y))
        Panel2->Cursor = crSizeAll;
      else
        Panel2->Cursor = crDefault;
      break;
    }

		case csMoveLabel:
    case csMoveLegend:
      Panel2->Cursor = crSizeAll;
      break;

    case csZoomWindow:
      Panel2->Cursor = crCross;
      break;

    case csMove:
      Panel2->Cursor = crMoveHand1;
      break;

    case csMoving:
      Panel2->Cursor = crMoveHand2;
      break;
  }

  CursorState = State;

  //Necesarry if the cursor is inside the area,
  //else cursor is not changed when captured; It will also change the cursor instantly
  if(Screen->Cursor == crDefault &&
     Panel2->ClientRect.Contains(Panel2->ScreenToClient(Mouse->CursorPos)))
    ::SetCursor(Screen->Cursors[Panel2->Cursor]);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Image1DblClick(TObject *Sender)
{
  EditLabel(Data.FindLabel(LastMousePos.x, LastMousePos.y));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Label_EditClick(TObject *Sender)
{
  //Don't use LastMousePos as OnMouseMove is called just before this event
  TPoint P = Image1->ScreenToClient(PopupMenu3->PopupPoint);
  EditLabel(Data.FindLabel(P.x, P.y));
}
//---------------------------------------------------------------------------
void TForm1::EditLabel(const boost::shared_ptr<TTextLabel> &Label)
{
  if(Label)
  {
    std::auto_ptr<TForm6> Form6(new TForm6(Application, Options.DefaultLabelFont, NAME, Data.GetFileName().c_str()));
    Form6->SetText(Label->GetText().c_str());
    Form6->SetBackgroundColor(Label->GetBackgroundColor());
    Form6->Caption = LoadRes(528);
    if(Form6->ShowModal() == mrOk)
    {
      //If text is empty, remove label
      if(Form6->IsEmpty())
      {
        DeleteGraphElem(Label);
      }
      else
      {
        Options.DefaultLabelFont = Form6->GetFont();
        boost::shared_ptr<TTextLabel> NewLabel(new TTextLabel(
          Form6->GetText().c_str(),
          Label->GetPlacement(),
          Label->GetPos().first,
          Label->GetPos().second,
          Form6->GetBackgroundColor(),
          Label->GetRotation(),
          Form6->GetOleLink()
        ));
        Label->Swap(*NewLabel);
        UndoList.Push(TUndoChange(Label, NewLabel));
        Label->Update();
        Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(Label));
        UpdateTreeView();
        UpdateMenu();
        Redraw();
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Label_DeleteClick(TObject *Sender)
{
  //Don't use LastMousePos as OnMouseMove is called just before this event
  TPoint P = Image1->ScreenToClient(PopupMenu3->PopupPoint);
  boost::shared_ptr<TTextLabel> Label = Data.FindLabel(P.x, P.y);
  if(Label)
  {
    UndoList.Push(TUndoDel(Label, Label->GetParent(), Data.GetIndex(Label)));
    Data.Delete(Label);
    UpdateTreeView();
    Redraw();
    UpdateMenu();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DonateActionExecute(TObject *Sender)
{
  ShellExecute(Handle, NULL, DONATE_PAGE, NULL, NULL, SW_SHOWMAXIMIZED);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormKeyUp(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
  switch(Key)
  {
    case VK_SHIFT:
      if(CursorState == csMove || CursorState == csMoving)
        if(!MoveAction->Checked)
          SetCursorState(csIdle);
      break;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SaveCopyAsActionExecute(TObject *Sender)
{
  DoSave("", false);
}
//---------------------------------------------------------------------------
//Disables menu items that shouldn't be available in the OLE server
void TForm1::ActivateOleUserInterface()
{
  NewAction->Enabled = false;
  OpenAction->Enabled = false;
  if(Data.GetFileName().empty())
    SaveAction->Enabled = false;
  SaveAsAction->Enabled = false;
  Recent1->Enabled = false;

//  SaveCopyAsAction->Visible = true;
  //Make "File->Save copy as..." visible
  TActionClients *MenuItems = ActionMainMenuBar1->ActionClient->Items;
  TActionClientItem *FilesItem = MenuItems->ActionClients[0];
  TActionClientItem *Item = FilesItem->Items->ActionClients[4];
  Item->Visible = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
  TTreeNode *Node = TreeView->Selected;
  if(Node && Shift.Contains(ssAlt))
    switch(Key)
    {
      //Move selected item up
      case VK_UP:
      {
        Key = 0;
        TTreeNode *PrevNode = Node->getPrevSibling();
        if(!PrevNode)
          return; //Node already at top

        TGraphElemPtr Elem = GetGraphElem(Node);
        TGraphElemPtr Parent = Elem->GetParent();

        int OldIndex = Parent->GetChildIndex(Elem);
        int NewIndex = Parent->GetChildIndex(GetGraphElem(PrevNode));
        UndoList.Push(TUndoMove(Data, Elem, OldIndex));
        Parent->RemoveChild(OldIndex);
        Parent->InsertChild(Elem, NewIndex);
        Python::ExecutePluginEvent(Python::peMoved, Elem);
        UpdateTreeView(Elem);
        Redraw();
        break;
      }

      //Move selected item down
      case VK_DOWN:
      {
        Key = 0;
        TTreeNode *NextNode = Node->getNextSibling();
        if(!NextNode)
          return; //Node already at bottom

        TGraphElemPtr Elem = GetGraphElem(Node);
        TGraphElemPtr Parent = Elem->GetParent();

        int OldIndex = Parent->GetChildIndex(Elem);
        int NewIndex = Parent->GetChildIndex(GetGraphElem(NextNode));
        UndoList.Push(TUndoMove(Data, Elem, OldIndex));
        Parent->RemoveChild(OldIndex);
        Parent->InsertChild(Elem, NewIndex);
        Python::ExecutePluginEvent(Python::peMoved, Elem);
        UpdateTreeView(Elem);
        Redraw();
        break;
      }
    }
    else if(Shift.Contains(ssCtrl))
      switch(Key)
      {
        case 'C':
          CopyAction->Execute();
          break;

        case 'X':
          CutAction->Execute();
          break;

        case 'V':
          PasteAction->Execute();
          break;
      }
}
//---------------------------------------------------------------------------
bool TForm1::LoadFromFile(const String &FileName, bool AddToRecent, bool ShowErrorMessages)
{
  Draw.AbortUpdate();
  try
  {
    Data.LoadFromFile(FileName.c_str());
  }
  catch(std::exception &E)
  {
    if(ShowErrorMessages)
      ShowErrorMsg(E);
    //Parts of Data might have changed. Better change this to Strong guarantee!!
    UpdateTreeView();
    return false;
  }

  RecreateTreeView();
  Caption = String(NAME) + L" - " + FileName;
  if(AddToRecent)
    Recent1->FileUsed(FileName);
  UndoList.Clear();

  Application->Title = String(NAME) + L" - " + ExtractFileName(FileName);
  UpdateMenu();
  Python::ExecutePluginEvent(Python::peLoad);
  return true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OpenPreviewDialog1PreviewFile(
      TOpenPreviewDialog *Dialog, const String &FileName,
      TCanvas *Canvas, const TRect &Rect)
{
  PreviewDraw->AbortUpdate();
/*
  TConfigFile IniFile;
  if(IniFile.LoadFromFile(FileName.c_str()))
  {
    std::string PngStr = IniFile.Read("Image", "Png", "");
    if(!PngStr.empty())
    {
      std::vector<BYTE> Data((PngStr.size()*3)/4);
      Base64Decode(PngStr, &Data[0]);
      std::auto_ptr<Graphics::TBitmap> Bitmap(new Graphics::TBitmap);
      Bitmap->Handle = CreateBitmapFromPNGMemory(&Data[0], Data.size());
      if(Bitmap->Handle != NULL)
      {
        Canvas->CopyRect(Rect, Bitmap->Canvas, TRect(0, 0, Bitmap->Width, Bitmap->Height));
        return true;
      }
    }
  }
*/
  //Make sure background is drawn
  Canvas->Brush->Style = bsSolid;
  //Set background color
  Canvas->Brush->Color = PreviewData.Axes.BackgroundColor;
  //Clear area
  Canvas->FillRect(Rect);

  if(!PreviewDraw)
    return;

  try
  {
    PreviewData.LoadFromFile(FileName.c_str());
  }
  catch(std::exception &E)
  { //Ignore errors
    PreviewDraw->AbortUpdate();
    Canvas->Brush->Style = bsSolid;
    Canvas->Brush->Color = clWhite;
    Canvas->FillRect(Rect);
    return;
  }

  PreviewData.Axes.ShowLegend = false; //Always disable legend for preview

  //Make sure background is drawn
  Canvas->Brush->Style = bsSolid;
  //Set background color
  Canvas->Brush->Color = PreviewData.Axes.BackgroundColor;
  //Clear area
  Canvas->FillRect(Rect);

  PreviewDraw->SetCanvas(Canvas);
  PreviewDraw->SetSize(Rect.Width(), Rect.Height());
  PreviewDraw->DrawAll();
}
//---------------------------------------------------------------------------
void TForm1::LoadDefault()
{
  std::wstring Str = GetRegValue(REGISTRY_KEY, L"DefaultAxes", HKEY_CURRENT_USER, L"");
  TConfigFile ConfigFile(Str);

  if(Str.empty())
    Data.LoadDefault();
  else
    try
    {
      Data.Load(ConfigFile);
    }
    catch(std::exception &E)
    {
      ShowErrorMsg(E);
      Data.LoadDefault();
    }

  UndoList.Clear();
  Python::ExecutePluginEvent(Python::peNew);
  UpdateTreeView(Data.GetElem(0));
  Caption = NAME;
  Application->Title = NAME;
  UpdateMenu();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OpenPreviewDialog1Show(TObject *Sender)
{
  PreviewDraw.reset(new TDraw(NULL, &PreviewData, "Preview DrawThread", true));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OpenPreviewDialog1Close(TObject *Sender)
{
  PreviewDraw.reset();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Tree_ExportClick(TObject *Sender)
{
  if(!TreeView->Selected)
    return;

  if(TPointSeries *Series = dynamic_cast<TPointSeries*>(GetGraphElem(TreeView->Selected).get()))
  {
    SaveDialog->Filter = LoadRes(RES_EXPORT_DATA_FILTER);
    if(SaveDialog->Execute())
      if(!ExportPointSeries(Series, SaveDialog->FileName.c_str(), SaveDialog->FilterIndex == 1 ? ';' : '\t', Options.DecimalSeparator))
        MessageBox(LoadRes(RES_FILE_ACCESS, SaveDialog->FileName), LoadRes(RES_WRITE_FAILED), MB_ICONSTOP);
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::IPrintDialog1PaintSample(TIPrintDialog *Sender,
      TCanvas *PageCanvas, const TRect &PaintRect, const TRect &PaperSize)
{
  int x = (PaintRect.Right + PaintRect.Left) / 2;
	int y = (PaintRect.Bottom + PaintRect.Top) / 2;
	int Right = PaintRect.Right;
	int Top = PaintRect.Top;
	PageCanvas->Pen->Width = 2;
	PageCanvas->Pen->Color = clBlue;
	PageCanvas->Brush->Color = clBlue;
	PageCanvas->MoveTo(x, PaintRect.Top+1);
	PageCanvas->LineTo(x, PaintRect.Bottom);
	PageCanvas->MoveTo(PaintRect.Left, y);
	PageCanvas->LineTo(PaintRect.Right, y);

	PageCanvas->Pen->Width = 1;
	TPoint Arrow1[] = {TPoint(Right-1, y-1), TPoint(Right-6, y-6), TPoint(Right-6, y+5), TPoint(Right-1, y)};
	TPoint Arrow2[] = {TPoint(x-1, Top+1), TPoint(x-6, Top+6), TPoint(x+5, Top+6), TPoint(x, Top+1)};
	PageCanvas->Polygon(Arrow1, 3);
	PageCanvas->Polygon(Arrow2, 3);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::IPrintDialog1Show(TObject *Sender)
{
  if(IPrintDialog1->PrintForm)
  {
    TPrintFrm *Form = IPrintDialog1->PrintForm;
    TranslateProperties(Form);  
    ScaleForm(Form);
    SetAccelerators(Form);
  }                                  
}
//---------------------------------------------------------------------------
boost::shared_ptr<TGraphElem> TForm1::GetGraphElem(TTreeNode *Node)
{
  static boost::shared_ptr<TGraphElem> Empty;
  if(Node == NULL)
    return Empty;

  return static_cast<TGraphElem*>(Node->Data)->shared_from_this();
}
//---------------------------------------------------------------------------
TTreeNode* TForm1::GetRootNode(unsigned Index)
{
  TTreeNode *Node = TreeView->Items->GetFirstNode();
  for(unsigned I = 0; I < Index; I++)
    Node = Node->getNextSibling();
  return Node;
}                                       
//---------------------------------------------------------------------------
TTreeNode* TForm1::GetNode(const TGraphElemPtr &Elem)
{
  //WARNING: TreeView->Items->Item conatins *ALL* nodes, not just the root nodes
  if(!Elem)
    throw Exception("No element given");
  const TGraphElemPtr &Parent = Elem->GetParent();
  if(!Parent)
    return NULL;
  return static_cast<TTreeNode*>(Elem->GetCustomData());
}
//---------------------------------------------------------------------------
//Copy metafile and bitmap to clipboard; Only used when not possible to register
//as OLE server
void TForm1::CopyAsImageToClipboard()
{
  Clipboard()->Open();

  //Copy metafile to clipboard
  std::auto_ptr<TMetafile> Metafile(new TMetafile());
  Metafile->Width = Image1->Width;
  Metafile->Height = Image1->Height;
  std::auto_ptr<TMetafileCanvas> MetaCanvas(new TMetafileCanvas(Metafile.get(), 0));
  TDraw DrawMeta(MetaCanvas.get(), &Data, "Metafile DrawThread", false);

  //Set width and height
  DrawMeta.SetSize(Image1->Width, Image1->Height);
  DrawMeta.SetSizeMul(Screen->PixelsPerInch / 96.0);
  DrawMeta.DrawAll();
  DrawMeta.Wait();
  MetaCanvas.reset();
  Clipboard()->Assign(Metafile.get());

  //Copy bitmap to clipboard
  std::auto_ptr<Graphics::TBitmap> Bitmap(new Graphics::TBitmap);
  Bitmap->Width = Image1->Width;
  Bitmap->Height = Image1->Height;
  Bitmap->Canvas->CopyRect(TRect(0,0,Image1->Width,Image1->Height), Image1->Canvas,Image1->ClientRect);
  Clipboard()->Assign(Bitmap.get());

  GraphClipboard.CopyPngData(Bitmap.get());

  Clipboard()->Close();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewDragOver(TObject *Sender, TObject *Source,
      int X, int Y, TDragState State, bool &Accept)
{
  Accept = false;
  TTreeNode *Node= TreeView->GetNodeAt(X, Y);
  if(Node && TreeView->Selected)
    Accept = Node->Parent == TreeView->Selected->Parent;

  Timer2->Enabled = Y < 10 || Y > TreeView->Height - 15;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewDragDrop(TObject *Sender, TObject *Source,
      int X, int Y)
{
  TTreeNode *Node = TreeView->GetNodeAt(X, Y);
  if(TreeView->Selected && Node != TreeView->Selected)
  {
    boost::shared_ptr<TGraphElem> Elem = GetGraphElem(TreeView->Selected);
    Draw.AbortUpdate();
    const TGraphElemPtr &Parent = Elem->GetParent();
    int OldIndex = Parent->GetChildIndex(Elem);
    int NewIndex = Parent->GetChildIndex(GetGraphElem(Node));
    UndoList.Push(TUndoMove(Data, Elem, OldIndex));
    Parent->RemoveChild(OldIndex);
    Parent->InsertChild(Elem, NewIndex);
    UpdateTreeView(Elem);
    Python::ExecutePluginEvent(Python::peMoved, Elem);
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::LegendPlacementClick(TObject *Sender)
{
  if(TMenuItem *MenuItem = dynamic_cast<TMenuItem*>(Sender))
  {
    UndoList.Push(TUndoAxes(Data));
    Data.Axes.LegendPlacement = static_cast<TLegendPlacement>(MenuItem->MenuIndex + 1);
    Data.SetModified();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Legend_ShowClick(TObject *Sender)
{
  UndoList.Push(TUndoAxes(Data));
  Data.Axes.ShowLegend = Legend_Show->Checked;
  Data.SetModified();
  Redraw();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PopupMenu4Popup(TObject *Sender)
{
  //Warning: RadioItem does not work when Images are assigned
  Legend_Show->Checked = Data.Axes.ShowLegend;
  int Index = Data.Axes.LegendPlacement > lpCustom &&
    Data.Axes.LegendPlacement <= Legend_Placement->Count ? Data.Axes.LegendPlacement - 1 : -1;
  for(int I = 0; I < Legend_Placement->Count; I++)
    Legend_Placement->Items[I]->ImageIndex = (I == Index) ? iiBullet : -1;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SupportActionExecute(TObject *Sender)
{
  ShellExecute(Handle, NULL, SUPPORT_PAGE, NULL, NULL, SW_SHOWMAXIMIZED);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomXInActionExecute(TObject *Sender)
{
  //Zoom in on x-axis only
  Zoom(GetKeyState(ssShift) ? GuiSettings.MajorZoomIn : GuiSettings.MinorZoomIn, 1.0, false);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomXOutActionExecute(TObject *Sender)
{
  //Zoom out on x-axis only
  Zoom(GetKeyState(ssShift) ? GuiSettings.MajorZoomOut : GuiSettings.MinorZoomOut, 1.0, false);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomYInActionExecute(TObject *Sender)
{
  //Zoom in on y-axis only
  Zoom(1.0, GetKeyState(ssShift) ? GuiSettings.MajorZoomIn : GuiSettings.MinorZoomIn, false);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomYOutActionExecute(TObject *Sender)
{
  //Zoom out on y-axis only
  Zoom(1.0, GetKeyState(ssShift) ? GuiSettings.MajorZoomOut : GuiSettings.MinorZoomOut, false);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertRelationActionExecute(TObject *Sender)
{
  if(CreateForm<TForm11>(Data)->ShowModal() == mrOk)
  {
    TreeView->SetFocus();
    Python::ExecutePluginEvent(Python::peNewElem, Data.Back());
    UpdateTreeView(Data.Back());
    UpdateMenu();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CustomFunctionsActionExecute(TObject *Sender)
{
  CreateForm<TForm17>(Data)->ShowModal();
  UpdateMenu();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SaveDialogEx1Help(TObject *Sender)
{
  CreateForm<TForm18>()->EditOptions(*ActionImageOptions, static_cast<TImageFormat>(SaveDialogEx1->FilterIndex), Image1->Width, Image1->Height);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PlacementClick(TObject *Sender)
{
  if(TMenuItem *MenuItem = dynamic_cast<TMenuItem*>(Sender))
  {
    Draw.AbortUpdate();
    boost::shared_ptr<TTextLabel> TextLabel;
    if(MenuItem->Parent == Tree_Placement)
      TextLabel = boost::dynamic_pointer_cast<TTextLabel>(GetGraphElem(TreeView->Selected));
    else
    {
      //Don't use LastMousePos as OnMouseMove is called just before this event
      TPoint P = Image1->ScreenToClient(PopupMenu3->PopupPoint);
      TextLabel = Data.FindLabel(P.x, P.y);
    }
    if(!TextLabel)
      return;

    if(MenuItem->MenuIndex == 4)
    {
      if(!CreateForm<TForm21>(Data, Draw, TextLabel)->ShowModal() == mrOk)
        return;
    }
    else
    {
      UndoList.Push(TUndoChange(TextLabel));
      TextLabel->SetPlacement(static_cast<TLabelPlacement>(MenuItem->MenuIndex + 1));
      TextLabel->SetPos(std::make_pair(0.0, 0.0));
      TextLabel->Update();
      Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(TextLabel));
    }

    UpdateMenu();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PopupMenu3Popup(TObject *Sender)
{
  TPoint Pos = Image1->ScreenToClient(PopupMenu3->PopupPoint);
  if(boost::shared_ptr<TTextLabel> TextLabel = Data.FindLabel(Pos.x, Pos.y))
  {
    int Index = (TextLabel->GetPlacement() == lpUserTopLeft || TextLabel->GetPlacement() >= lpUserTopRight) ? 4 : TextLabel->GetPlacement() - 1;
    for(int I = 0; I < Label_Placement->Count; I++)
      Label_Placement->Items[I]->ImageIndex = (I == Index) ? iiBullet : -1;

    double Rotation = TextLabel->GetRotation();
    if(TextLabel->GetRotation() == static_cast<int>(Rotation))
      Index = static_cast<int>(Rotation) % 90 == 0 ? static_cast<int>(Rotation) / 90 : 4;
    else
      Index = 4;
    for(int I = 0; I < Label_Rotation->Count; I++)
      Label_Rotation->Items[I]->ImageIndex = (I == Index) ? iiBullet : -1;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PopupMenu1Popup(TObject *Sender)
{
  //Warning: RadioItem does not work when Images are assigned
  if(boost::shared_ptr<TTextLabel> TextLabel = boost::dynamic_pointer_cast<TTextLabel>(GetGraphElem(TreeView->Selected)))
  {
    int Index = (TextLabel->GetPlacement() == lpUserTopLeft || TextLabel->GetPlacement() >= lpUserTopRight) ? 4 : TextLabel->GetPlacement() - 1;
    for(int I = 0; I < Tree_Placement->Count; I++)
      Tree_Placement->Items[I]->ImageIndex = (I == Index) ? iiBullet : -1;

    double Rotation = TextLabel->GetRotation();
    if(TextLabel->GetRotation() == static_cast<int>(Rotation))
      Index = static_cast<int>(Rotation) % 90 == 0 ? static_cast<int>(Rotation) / 90 : 4;
    else
      Index = 4;
    for(int I = 0; I < Tree_Rotation->Count; I++)
      Tree_Rotation->Items[I]->ImageIndex = (I == Index) ? iiBullet : -1;
  }
}
//---------------------------------------------------------------------------
void TForm1::MoveAndSnapLegend(int dx, int dy, bool Snap)
{
  ImagePos.x += dx;
  ImagePos.y += dy;
  Image2->Left = ImagePos.x;
  Image2->Top = ImagePos.y;
  unsigned SnapDist = 10;
  const TRect &Rect = Draw.GetAxesRect();

  if(!Snap)
  {
    LegendPlacement = lpCustom;
    Image2->Left = ImagePos.x;
    Image2->Top = ImagePos.y;
    return;
  }

  unsigned LeftDist = std::abs(Image2->Left - (int)Rect.Left);
  unsigned TopDist = std::abs(Image2->Top - (int)Rect.Top);
  unsigned RightDist = std::abs(Image2->Left + Image2->Width - (int)Rect.Right);
  unsigned BottomDist = std::abs(Image2->Top + Image2->Height - (int)Rect.Bottom);

  //Check for snap to upper left corner
  if(LeftDist < SnapDist && TopDist < SnapDist)
  {
    Image2->Left = Rect.Left;
    Image2->Top = Rect.Top;
    LegendPlacement = lpTopLeft;
  }

  //Check for snap to upper right corner
  else if(RightDist < SnapDist && TopDist < SnapDist)
  {
    Image2->Left = Rect.Right - Image2->Width;
    Image2->Top = Rect.Top;
    LegendPlacement = lpTopRight;
  }

  //Check for snap to lower left corner
  else if(LeftDist < SnapDist && BottomDist < SnapDist)
  {
    Image2->Left = Rect.Left;
    Image2->Top = Rect.Bottom - Image2->Height;
    LegendPlacement = lpBottomLeft;
  }

  //Check for snap to lower right corner
  else if(RightDist < SnapDist && BottomDist < SnapDist)
  {
    Image2->Left = Rect.Right - Image2->Width;
    Image2->Top = Rect.Bottom - Image2->Height;
    LegendPlacement = lpBottomRight;
  }

  // Legend has a custom position
  else
    LegendPlacement = lpCustom;
}
//---------------------------------------------------------------------------
void TForm1::MoveLabel(int X, int Y, TLabelPlacement Placement, bool Snap)
{
  Image2->Left = X;
  Image2->Top = Y;
  if(Snap)
    MovingLabelPlacement = Placement;
  else
    switch(MovingLabel->GetPlacement())
    {
      case lpUserTopRight:
      case lpUserBottomLeft:
      case lpUserBottomRight:
        MovingLabelPlacement = MovingLabel->GetPlacement();
        break;
      default:
        MovingLabelPlacement = lpUserTopLeft;
    }
}
//---------------------------------------------------------------------------
void TForm1::MoveAndSnapLabel(int dx, int dy, bool Snap)
{
  ImagePos.x += dx;
  ImagePos.y += dy;
  int SnapDist = 15;
  const TRect &Rect = Draw.GetAxesRect();

  if(!Snap)
  {
    MoveLabel(ImagePos.x, ImagePos.y, lpUserTopLeft, false);
    return;
  }

  int RightDist = std::abs((int)ImagePos.x + Image2->Width - Rect.Width()) + 2;
  int TopDist = std::abs((int)ImagePos.y - (int)Rect.Top + 1);
  int xAxesCoord = Draw.xPoint(Data.Axes.yAxis.AxisCross);
  int yAxesCoord = Draw.yPoint(Data.Axes.xAxis.AxisCross);

  //Check for label above x-axis
  if(RightDist < SnapDist && std::abs(ImagePos.y + Image2->Height -4 - yAxesCoord) < SnapDist)
    MoveLabel(Rect.Width() - Image2->Width + 2, yAxesCoord - Image2->Height - 4, lpAboveX, true);

  //Check for label below x-axis
  else if(RightDist < SnapDist && std::abs(ImagePos.y - yAxesCoord) < SnapDist)
    MoveLabel(Rect.Width() - Image2->Width + 2, yAxesCoord, lpBelowX, true);

  //Check for label left of y-axis
  else if(TopDist < SnapDist && std::abs(ImagePos.x + Image2->Width - xAxesCoord) < SnapDist)
    MoveLabel(xAxesCoord - Image2->Width, Rect.Top + 1, lpLeftOfY, true);

  //Check for label right of y-axis
  else if(TopDist < SnapDist && std::abs(ImagePos.x - 12 - xAxesCoord) < SnapDist)
    MoveLabel(xAxesCoord + 12, Rect.Top + 1, lpRightOfY, true);
  else
  {
    //Snap to nearest point in point series
/*    unsigned MinDist = MAXINT;
    std::vector<TGraphElemPtr>::const_iterator End = Data.End();
    TPointSeries *MinSeries = NULL;
    TPoint MinPoint;
    for(std::vector<TGraphElemPtr>::const_iterator Iter = Data.Begin(); Iter != End; ++Iter)
      if(TPointSeries *Series = dynamic_cast<TPointSeries*>(Iter->get()))
        for(unsigned I = 0; I < Series->PointList.size(); I++)
        {
          TPoint Point;
          Point.x = Draw.xPoint(Series->PointList[I].x.Value);
          Point.y = Draw.yPoint(Series->PointList[I].y.Value);
          unsigned Dist = Sqr(ImagePos.x - Point.x) + Sqr(ImagePos.y - Point.y);
          if(Dist < MinDist)
          {
            MinDist = Dist;
            MinSeries = Series;
            MinPoint = Point;
          }
        }

    if(MinSeries && MinDist <= 150)
    {
      Image2->Left = MinPoint.x;
      Image2->Top = MinPoint.y;
    }*/

    MoveLabel(ImagePos.x, ImagePos.y, lpUserTopLeft, false);
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RotationClick(TObject *Sender)
{
  if(TMenuItem *MenuItem = dynamic_cast<TMenuItem*>(Sender))
  {
    Draw.AbortUpdate();
    boost::shared_ptr<TTextLabel> TextLabel;
    if(MenuItem->Parent == Label_Rotation)
    {
      //Don't use LastMousePos as OnMouseMove is called just before this event
      TPoint P = Image1->ScreenToClient(PopupMenu3->PopupPoint);
      TextLabel = Data.FindLabel(P.x, P.y);  //Right click on label menu
    }
    else
      TextLabel = boost::dynamic_pointer_cast<TTextLabel>(GetGraphElem(TreeView->Selected)); //Context menu in function list

    if(!TextLabel)
      return;

    double Rotation = TextLabel->GetRotation();
    if(MenuItem->MenuIndex == 4)
    {
      if(!InputQuery(LoadStr(RES_ROTATION), LoadStr(RES_DEGREES) + ":", Rotation))
        return;
      Rotation = std::fmod(Rotation, 360);
      if(Rotation < 0)
        Rotation += 360;
    }
    else
      Rotation = MenuItem->MenuIndex * 90;

    UndoList.Push(TUndoChange(TextLabel));
    TextLabel->SetRotation(Rotation);
    TextLabel->Update();
    Python::ExecutePluginEvent(Python::peChanged, TGraphElemPtr(TextLabel));
    UpdateMenu();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DebugLine(System::TObject* Sender, const String Line, bool &Discard)
{
  OutputDebugString(Line.c_str());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AnimateActionExecute(TObject *Sender)
{
  CreateForm<TForm19>(Data, Image1->Width, Image1->Height)->ShowModal();
}
//---------------------------------------------------------------------------
void CallScriptNewElem(const TGraphElemPtr &Elem)
{
  unsigned Count = Elem->ChildCount();
  for(unsigned I = 0; I < Count; I++)
  {
    TGraphElemPtr Child = Elem->GetChild(I);
    Python::ExecutePluginEvent(Python::peNewElem, Child);
    CallScriptNewElem(Child);
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ImportGraphFileActionExecute(TObject *Sender)
{
  OpenPreviewDialog1->Filter = LoadRes(RES_GRAPH_FILTER);
  if(OpenPreviewDialog1->Execute())
  {
    unsigned Count = Data.ElemCount();
    try
    {
      Data.Import(OpenPreviewDialog1->FileName.c_str());
      UndoList.BeginMultiUndo();
      for(unsigned I = Count; I < Data.ElemCount(); I++)
        UndoList.Push(TUndoAdd(Data.GetElem(I)));
      UndoList.EndMultiUndo();
      CallScriptNewElem(Data.GetTopElem());
      UpdateTreeView();
      UpdateMenu();
      Redraw();
    }
    catch(std::exception &Error)
    {
      ShowErrorMsg(Error);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ImportPointSeriesActionExecute(TObject *Sender)
{
	OpenDialog->Filter = LoadRes(RES_DATA_FILTER);
	const char Separators[] = {';',',','\t',' ',0};
	if(OpenDialog->Execute())
	{
		for(int I = 0; I < OpenDialog->Files->Count; I++)
			if(!Data.ImportPointSeries(OpenDialog->Files->Strings[I].c_str(), Separators[OpenDialog->FilterIndex-1]))
        return;

    UpdateTreeView();
    UpdateMenu();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Legend_FontClick(TObject *Sender)
{
  std::auto_ptr<TFontDialog> FontDialog(new TFontDialog(NULL));
  FontDialog->Options = FontDialog->Options << fdForceFontExist;
  FontDialog->Font->Assign(Data.Axes.LegendFont);
  if(FontDialog->Execute())
  {
    UndoList.Push(TUndoAxes(Data));
    Data.Axes.LegendFont->Assign(FontDialog->Font);
    Data.SetModified();
    Redraw();
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel6UnDock(TObject *Sender, TControl *Client,
      TWinControl *NewTarget, bool &Allow)
{
  Panel6->Height = 0;
  Splitter2->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel6DockDrop(TObject *Sender,
      TDragDockObject *Source, int X, int Y)
{
  if(Form22->Visible)
  {
    Panel6->Height = ConsoleHeight;
    Splitter2->Visible = true;
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel6DockOver(TObject *Sender,
      TDragDockObject *Source, int X, int Y, TDragState State,
      bool &Accept)
{
  // Modify the DockRect to preview dock area.
  int FormHeight = ConsoleHeight;// - StatusBar1->Height;
  TPoint TopLeft = Panel6->ClientToScreen(TPoint(0, -FormHeight));
  TPoint BottomRight = Panel6->ClientToScreen(TPoint(Panel6->Width, Panel6->Height));
  Source->DockRect = TRect(TopLeft, BottomRight);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel6GetSiteInfo(TObject *Sender,
      TControl *DockClient, TRect &InfluenceRect, TPoint &MousePos,
      bool &CanDock)
{
  CanDock = DockClient == Form22;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Panel4GetSiteInfo(TObject *Sender,
      TControl *DockClient, TRect &InfluenceRect, TPoint &MousePos,
      bool &CanDock)
{
  CanDock = DockClient == Form9.get();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Timer2Timer(TObject *Sender)
{
  TPoint Pos = TreeView->ScreenToClient(Mouse->CursorPos);
  if(Pos.y < 10)
  {
    if(TTreeNode *Node = TreeView->TopItem->getPrevSibling())
    {
      TreeView->TopItem = Node;
      TreeView->Invalidate();
    }
  }
  else if(Pos.y > TreeView->Height - 15)
    if(TTreeNode *Node = TreeView->TopItem->getNextSibling())
    {
      TreeView->TopItem = Node;
      TreeView->Invalidate();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewEndDrag(TObject *Sender, TObject *Target,
      int X, int Y)
{
  Timer2->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ExecuteFunction(TMessage &Message)
{
  typedef int (*TFunction)(int Arg);
  TFunction Function = (TFunction)Message.WParam;
  Message.Result = Function(Message.LParam);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MainMenuChange(TObject *Sender, TMenuItem *Source,
      bool Rebuild)
{
  //This seems to be necesarry when Windows is configured to Arabic for non-Unicode
  //and Arabic is selected as language in Graph.
/*  if(SysLocale.MiddleEast)
  {
    MENUITEMINFO  mii;
    wchar_t szBuffer [80];
    //get current menu alignment
    mii.cbSize = sizeof (MENUITEMINFO);
    mii.fMask = MIIM_TYPE;
    mii.dwTypeData= szBuffer;
    mii.cch = sizeof(szBuffer)/sizeof(szBuffer[0]);
    GetMenuItemInfo(MainMenu->Handle, 0, true, &mii);
    //update menu alignment
    mii.fMask = MIIM_TYPE;
    mii.fType |= MFT_RIGHTJUSTIFY | MFT_RIGHTORDER;
    SetMenuItemInfo(MainMenu->Handle, 0, true, &mii);
    DrawMenuBar(Handle);
  }*/
}
//---------------------------------------------------------------------------
void __fastcall TForm1::StatusBar1Hint(TObject *Sender)
{
  //Add spaces to the end when Panel2 is used to avoid the size grip
  if(SysLocale.MiddleEast)
    StatusBar1->Panels->Items[2]->Text = L"     " + Application->Hint;
  else
    StatusBar1->Panels->Items[0]->Text = Application->Hint;
}
//---------------------------------------------------------------------------
void TForm1::SetStatusIcon(int AStatusIcon)
{
  StatusBar1->Panels->Items[SysLocale.MiddleEast ? 0 : 2]->Style = psOwnerDraw;
  StatusIcon = AStatusIcon;
  StatusBar1->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TreeViewMouseLeave(TObject *Sender)
{
  if(StatusBar1->AutoHint)
    ShowStatusMessage(L"", true);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SaveDialogEx1Close(TObject *Sender)
{
; //Dummy to force old dialog
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ScriptDocActionExecute(TObject *Sender)
{
  ShowHelp("", GetBaseDir() + "\\Help\\PluginDoc.chm");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ApplicationEventsModalBegin(TObject *Sender)
{
  //Something seems to sometimes change the FPU Control Word so we reset it back to the default
  //once in a while. It only happens on very few computers.
  //See http://forum.padowan.dk/viewtopic.php?pid=2438#p2438
  SET_DEFAULT_FPU_MASK();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ConsoleFormShown(TObject*)
{
	Splitter2->Visible = Panel6->VisibleDockClientCount > 0;
	Panel6->Height = Panel6->VisibleDockClientCount ? ConsoleHeight : 0;}
//---------------------------------------------------------------------------
void __fastcall TForm1::Splitter2Moved(TObject *Sender)
{
  if(Panel6->VisibleDockClientCount)
    ConsoleHeight = Panel6->Height;
}
//---------------------------------------------------------------------------
void TForm1::RecreateToolBar()
{
  CreateToolBar(StartToolBar);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UndoActionUpdate(TObject *Sender)
{
  UndoAction->Enabled = UndoList.CanUndo();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RedoActionUpdate(TObject *Sender)
{
  RedoAction->Enabled = UndoList.CanRedo();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AxesFontChanged(TObject *Sender)
{
  //We should push the axes to the undo stack, but it must be done before the change and there is no event for that.
  if(Sender == Data.Axes.TitleFont)
    Data.ClearCache();
  Data.SetModified();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Timer3Timer(TObject *Sender)
{
  Timer3->Enabled = false;
  if(Panel2->Cursor == crDefault)
    Screen->Cursor = crAppStart;
  ShowStatusMessage(LoadRes(RES_UPDATE));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CustomizeDlgShow(TObject *Sender)
{
  //The customize dialog uses ScaledImageList1->Handle; Make sure images are up to date.
  ScaledImageList1->EnsureUpdate();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertDifActionUpdate(TObject *Sender)
{
  InsertDifAction->Enabled = dynamic_cast<TBaseFuncType*>(Selected.get()) && !dynamic_cast<TTangent*>(Selected.get());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertShadingActionUpdate(TObject *Sender)
{
  InsertShadingAction->Enabled = dynamic_cast<TBaseFuncType*>(Selected.get()) && !dynamic_cast<TTangent*>(Selected.get());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertTrendlineActionUpdate(TObject *Sender)
{
  InsertTrendlineAction->Enabled = dynamic_cast<TPointSeries*>(Selected.get());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertTangentActionUpdate(TObject *Sender)
{
  InsertTangentAction->Enabled = dynamic_cast<TBaseFuncType*>(Selected.get()) && !dynamic_cast<TTangent*>(Selected.get());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TableActionUpdate(TObject *Sender)
{
  TableAction->Enabled = dynamic_cast<TBaseFuncType*>(Selected.get()) && !dynamic_cast<TTangent*>(Selected.get());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CopyActionUpdate(TObject *Sender)
{
    CopyAction->Enabled = TreeView->Selected && TreeView->Selected->Level == 0 && dynamic_cast<TAxesView*>(Selected.get()) == NULL;
    CutAction->Enabled = CopyAction->Enabled;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::EditActionUpdate(TObject *Sender)
{
  EditAction->Enabled = Selected;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DeleteActionUpdate(TObject *Sender)
{
  DeleteAction->Enabled = dynamic_cast<TAxesView*>(Selected.get()) == NULL;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PasteActionUpdate(TObject *Sender)
{
	PasteAction->Enabled = GraphClipboard.HasData();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomSquareActionUpdate(TObject *Sender)
{
  ZoomSquareAction->Enabled = Data.Axes.xAxis.LogScl == Data.Axes.yAxis.LogScl;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomFitActionUpdate(TObject *Sender)
{
  ZoomFitAction->Enabled = dynamic_cast<TBaseFuncType*>(Selected.get()) || dynamic_cast<TPointSeries*>(Selected.get()) || dynamic_cast<TRelation*>(Selected.get());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AnimateActionUpdate(TObject *Sender)
{
  //Enable animation if there are any constants defined
  AnimateAction->Enabled = false;
  for(TCustomFunctions::TConstIterator Iter = Data.CustomFunctions.Begin(); Iter != Data.CustomFunctions.End(); ++Iter)
    if(Iter->Arguments.empty())
    {
      AnimateAction->Enabled = true;
      break;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ApplicationEventsShortCut(TWMKey &Msg, bool &Handled)
{
  //We use TApplicationEvent::OnShortCut instead of TForm1::OnShortCut because the later is called twixe for stupid reasons!
  //Instead if just check if Form1 is active
  if(IsWindowEnabled(Handle))
  {
    TShiftState ShiftState = KeyDataToShiftState(Msg.KeyData);
    Handled = Python::ExecutePluginFunc<bool>("HandleShortCut", Msg.CharCode, ShiftState.ToInt());
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ApplicationEventsMessage(tagMSG &Msg, bool &Handled)
{
  //Check if we need to update the tree view every time a message is handled.
  //We need it updated before the user tries to do something with it.
  UndoFlag = false;
  if(Data.NeedDrawing())
    Redraw();
  if(FunctionListModified)
    RecreateTreeView();
}
//---------------------------------------------------------------------------

