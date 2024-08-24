import Graph
import vcl
import ctypes
import os.path
import time
import Gui

ImagePath = os.path.dirname(__file__) + "\\Images\\"

def GetForm(Name):
    for I in range(0, vcl.Screen.FormCount):
        if vcl.Screen.Forms[I].Name == Name:
            return vcl.Screen.Forms[I]

def GetFormShot():
    MakeFormShot()
    Gui.Delay(200)
    Bitmap = vcl.TBitmap()
    Bitmap.Assign(vcl.Clipboard)
    PngImage = vcl.TpngImage()
    PngImage.Assign(Bitmap)
    return PngImage

def MakeFormShot():
    ctypes.windll.user32.keybd_event(0x2C, 1, 0, 0)
    
def SaveFormShot(FileName):
    os.makedirs(ImagePath, exist_ok=True)
    GetFormShot().SaveToFile(ImagePath + FileName)

def DoMouseClick(X, Y):
    MOUSEEVENTF_ABSOLUTE = 0x8000
    MOUSEEVENTF_LEFTDOWN = 0x0002
    MOUSEEVENTF_LEFTUP = 0x0004
    ctypes.windll.user32.mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, X, Y, 0, 0)
    ctypes.windll.user32.mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, X, Y, 0, 0)

def HandleDialog(FormID):
    if FormID == 3: # Edit|Axes
        Form3 = GetForm("Form3")
        Form3.Button1.SetFocus()
        SaveFormShot("DialogAxes1.png",)
        Form3.PageControl1.ActivePageIndex = 2
        SaveFormShot("DialogAxes3.png",)
        Form3.PageControl1.ActivePageIndex = 3
        SaveFormShot("DialogAxes4.png",)
        Form3.Close()
    elif FormID == 4: # Edit|Axes
        Form4 = GetForm("Form4")
        Form4.RadioGroup1.SetFocus()
        SaveFormShot("DialogOptions.png",)
        Form4.Close()
    elif FormID == 5: # Function|Insert functions
        Form5 = GetForm("Form5")
        Form5.Edit1.Text = "sin(x)"
        Form5.Edit3.Text = "-5"
        Form5.Edit4.Text = "15"
        Form5.ComboBox2.ItemIndex = 1
        Form5.ComboBox3.ItemIndex = 2
        Form5.LineSelect1.ItemIndex = 1
        Form5.ExtColorBox1.Selected = 0x0000FF
        Form5.Edit6.Text = "3"        
        Form5.Button1.SetFocus()
        SaveFormShot("DialogFunction.png",)
        Form5.Close()
    elif FormID == 6: # Function|Insert label
        Form6 = GetForm("Form6")
        Form6.Height = 340
        P = Form6.IRichEdit1.ClientToScreen((110, 5))
        Gui.DelayCall(HandleDialog, 200, (106,))
        Form6.PopupMenu1.Popup(P.x, P.y)
    elif FormID == 106:
        Form6 = GetForm("Form6")
        P = Form6.PopupMenu1.PopupPoint
        P = vcl.TPoint(P.x + 3, P.y + 150)
        vcl.Mouse.CursorPos = P
        Gui.Delay(100)
        
        Cursor = vcl.TPngImage()
        Cursor.LoadFromFile(os.path.dirname(__file__) + "\\Cursor.png")
        Image = GetFormShot()
        Image.Canvas.Draw(P.x - Form6.Left, P.y - Form6.Top, Cursor)
        Image.SaveToFile(ImagePath + "DialogLabel.png",)

        P = Form6.ClientToScreen((-10,-10))
        DoMouseClick(P.x, P.y) # Removes popup menu; PopupMenu.CloseMenu() does not work
        Form6.Close()
    elif FormID == 7: # Function|Insert f'(x)
        Form7 = GetForm("Form7")
        Form7.Button1.SetFocus()
        SaveFormShot("DialogDiff.png",)
        Form7.Close()
    elif FormID == 9: # Calc|Evaluate
        SaveFormShot("DialogEval.png",)
    elif FormID == 11: # Function|Insert relation
        Form11 = GetForm("Form11")
        Form11.Edit1.Text = "x^2 + y^2 = 25"
        Form11.Edit2.Text = "x > 0"
        Form11.Edit3.Text = "Right part of circle"
        Form11.ShadeSelect1.ItemIndex = 4
        Form11.LineSelect1.ItemIndex = 0
        Form11.ExtColorBox1.Selected = 0x008000
        Form11.Edit4.Text = "4"        
        Form11.Button1.SetFocus()
        SaveFormShot("DialogRelation.png",)
        Form11.Close()
    elif FormID == 12: # Function|Insert tangent
        Form12 = GetForm("Form12")
        Form12.Edit1.Text = "-0.985"
        Form12.Button1.SetFocus()
        SaveFormShot("DialogTangent.png",)
        Form12.Close()
    elif FormID == 13: # Function|Insert trendline
        Form13 = GetForm("Form13")
        Form13.Button1.SetFocus()
        Form13.ListBox1.ItemIndex = 4
        Form13.ListBox1.OnClick(None)
        SaveFormShot("DialogTrendline1.png",)
        Form13.PageControl1.ActivePageIndex = 1
        SaveFormShot("DialogTrendline2.png",)
        Form13.Close()
    elif FormID == 14: # Function|Insert point series
        Form14 = GetForm("Form14")
        Form14.Button1.SetFocus()
        Form14.Grid.Selection = (0, 5, 1, 12, (0,5), (1,12))
        P = Form14.Grid.ClientToScreen(Form14.Grid.CellRect(0, 5).TopLeft)
        Gui.DelayCall(HandleDialog, 300, (114,))
        Form14.PopupMenu1.Popup(P.x + 50, P.y + 6)
    elif FormID == 114:
        Form14 = GetForm("Form14")
        Cursor = vcl.TPngImage()
        Cursor.LoadFromFile(os.path.dirname(__file__) + "\\Cursor.png")
        Image = GetFormShot()
        P = Form14.PopupMenu1.PopupPoint
        Image.Canvas.Draw(P.x - Form14.Left, P.y - Form14.Top, Cursor)
        Image.SaveToFile(ImagePath + "DialogPoints.png",)
        P = Form14.ClientToScreen((-10,-10))
        DoMouseClick(P.x, P.y) # Removes popup menu; PopupMenu.CloseMenu() does not work
        Form14.Close()
    elif FormID == 15: # Calc|Table
        Form15 = GetForm("Form15")
        Form15.Button2.SetFocus()
        Form15.Button2.Click()
        Gui.Delay(200)
        SaveFormShot("DialogTable.png",)
        Form15.Close()        
    elif FormID == 16: # Function|Insert Shading
        Form16 = GetForm("Form16")
        Form16.Button1.SetFocus()
        Form16.ListBox1.ItemIndex = 0
        SaveFormShot("DialogShading1.png",)
        Form16.RadioButton6.Checked = True
        Form16.PageControl1.ActivePageIndex = 1
        SaveFormShot("DialogShading2.png",)
        Form16.PageControl1.ActivePageIndex = 2
        SaveFormShot("DialogShading3.png",)
        Form16.Close()
    elif FormID == 17: # Function|Custom functions/constants
        Form17 = GetForm("Form17")
        SaveFormShot("DialogCustomFunctions.png",)
        Form17.Close()
    elif FormID == 19: # Calc|Animate
        Form19 = GetForm("Form19")
        Form19.Edit1.Text = "1"
        Form19.Edit2.Text = "10"
        Form19.Edit3.Text = "1"
        Form19.Edit4.Text = "320"
        Form19.Edit5.Text = "320"
        SaveFormShot("DialogAnimate.png",)
        Form19.Close()
    elif FormID == 22: # Python interpreter
        Form22 = GetForm("Form22")
        Form22.Clear1.Click()
        Text = 'F=Graph.StdFunc("sin(x)+x")\r\nF=Graph.TStdFunc("sin(x)+x")\r\nGraph.FunctionList.append(F)\r\n'
        vcl.Clipboard.AsText = Text
        Form22.IRichEdit1.OnKeyDown(None, ord("V"), "ssCtrl")
        SaveFormShot("DialogInterpreter.png",)
   
def DoScreenShot(FormID=None):
    if FormID is None:
        FormList = [1, 3, 4, 5, 6, 7, 9, 11, 12, 13, 14, 15, 16, 17, 19, 22]
        for FormID in FormList:
            DoScreenShot(FormID)
            Gui.Delay(1000)
        
    elif FormID == 1: # Main Window
        Graph.LoadFromFile(os.path.dirname(__file__) + "\\Main.grf")
        Form1 = Graph.Form1
        Form9 = GetForm("Form9")
        if not Form1.EvalAction.Checked:
            Form1.EvalAction.Execute()
        Form9.ManualDock(Form1.Panel4, None, "alNone")
        Form1.Width = 850
        Form1.Height = 550
        Form1.Splitter1.OnDblClick(None)
        Graph.Form22.Visible = False
        Graph.FunctionList.Selected = Graph.FunctionList[2]
        Form9.StdFuncFrame1.Edit1.Text = "4.1538"
        Gui.Delay(500)
        SaveFormShot("DialogMain.png")
        
    elif FormID == 3: # Edit|Axes
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.AxesAction.Execute()
    elif FormID == 4: # Edit|Options
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.OptionsAction.Execute()
    elif FormID == 5: # Function|Insert function
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.InsertFunctionAction.Execute()
    elif FormID == 6: # Function|Insert label
        Graph.LoadFromFile(os.path.dirname(__file__) + "\\PointSeries.grf")
        Graph.FunctionList.Selected = Graph.FunctionList[2]
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.EditAction.Execute()
    elif FormID == 7: # Function|Insert 
        Func = Graph.TStdFunc("sin x")
        Graph.FunctionList.append(Func)
        Graph.FunctionList.Selected = Func
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.InsertDifAction.Execute()
    elif FormID == 9: # Calc|Evaluate
        Func = Graph.TStdFunc("sin x")
        Graph.FunctionList.append(Func)
        Graph.FunctionList.Selected = Func
        if not Graph.Form1.EvalAction.Checked:
            Graph.Form1.EvalAction.Execute()
        Form9 = GetForm("Form9")
        Form9.ManualFloat((200, 200, 200+Form9.UndockWidth, 200+Form9.UndockHeight))
        Form9.BorderStyle = "bsDialog"
        Form9.Left = 400
        Gui.Delay(100)
        Form9.AlphaBlend = False
        Form9.StdFuncFrame1.Edit1.Text = "2.3932"
        Form9.StdFuncFrame1.ComboBox1.SetFocus()
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
    elif FormID == 11: # Function|Insert relation
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.InsertRelationAction.Execute()
    elif FormID == 12: # Function|Insert tangent
        Func = Graph.TStdFunc("sin x")
        Graph.FunctionList.append(Func)
        Graph.FunctionList.Selected = Func
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.InsertTangentAction.Execute()
    elif FormID == 13: # Function|Insert trendline
        PointSeries = Graph.TPointSeries()
        PointSeries.Points = [(1,2), (3,4), (5,6)]
        Graph.FunctionList.append(PointSeries)
        Graph.FunctionList.Selected = PointSeries
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.InsertTrendlineAction.Execute()
    elif FormID == 14: # Function|Insert point series
        Graph.LoadFromFile(os.path.dirname(__file__) + "\\PointSeries.grf")
        Graph.FunctionList.Selected = Graph.FunctionList[1]
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.EditAction.Execute()
    elif FormID == 15: # Calc|Table
        Func = Graph.TStdFunc("5sin x+x")
        Graph.FunctionList.append(Func)
        Graph.FunctionList.Selected = Func
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.TableAction.Execute()
    elif FormID == 16: # Function | Insert shading
        Graph.LoadDefault()
        Graph.FunctionList.append(Graph.TStdFunc("x^2"))
        Graph.FunctionList.append(Graph.TParFunc("t^2", "3t"))
        Graph.FunctionList.append(Graph.TPolFunc("t+3"))
        Func = Graph.TStdFunc("sin x")
        Graph.FunctionList.append(Func)
        Graph.FunctionList.Selected = Func
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.InsertShadingAction.Execute()
    elif FormID == 17: # Function|Custom functions/constants
        Graph.Constants.clear()
        
        Graph.Constants["sinc"] = ("if(x=0, 1, sin(x)/x)", "x")
        Graph.Constants["g"] = 9.80665
        Graph.Constants["R"] = 8.31
        Graph.Constants["kilroy"] = ("ln abs sinc x", "x")
        Graph.Constants["E1"] = ("x^2*e^x/(e^x-1)^2", "x")
        Graph.Constants["E2"] = ("x/(e^x-1)", "x")
        Graph.Constants["E3"] = ("ln(1-e^-x)", "x")
        Graph.Constants["E4"] = ("E2 - E3(x)", "x")
        Graph.Constants["fact2"] = (0, "x")
        Graph.Constants["fact2"] = ("if(x <= 0, 1, x*fact2(x-1))", "x")

        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.CustomFunctionsAction.Execute()    
    elif FormID == 19: # Calc|Animate
        Graph.Constants.clear()
        Graph.Constants["a"] = 1
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
        Graph.Form1.AnimateAction.Execute()
    elif FormID == 22: # Python interpreter
        Form22 = Graph.Form22
        Form22.Visible = True
        Form22.ManualFloat((500, 500, 500+600, 500+250))
        Form22.BorderStyle = "bsDialog"
        Gui.DelayCall(HandleDialog, 500, Args=(FormID,))
               
Action = Graph.CreateAction(Caption="Create screen shots", OnExecute=lambda S: DoScreenShot())  
Graph.AddActionToMainMenu(Action)  
    
