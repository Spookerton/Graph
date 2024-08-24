# Plugin for Graph for creating circles.
# The plugin will create an action and add it to the main menu. When executed, the action will show a dialog where you can enter radius and center of the circle.
# When the dialog is closed it will create the circle as a parametric function and add it to the function list.
# In addition the plugin hooks into Edit command and shows the same dialog when a circle previously  created by the plugin is edited.
import Graph
import Gui
from Gui import ScaledVcl
import os.path

TranslationTexts = { 
  "Danish": {"Center:": "Centrum:", "Insert circle" : "Indsæt cirkel", "Create circle from center and radius." : "Indsæt cirkel givet ved centrum og radius."}
}
_ = Graph.CreateTranslations(TranslationTexts).gettext        

PluginName = _("Circle plotter")
PluginVersion = "0.2"
PluginDescription = _("Creates a menu item for inserting a circle given (x,y) coordinates and radius.")


class CircleDialog(Gui.SimpleDialog):
    def __init__(self, CircleItem=None):
        import ctypes
        Gui.SimpleDialog.__init__(self)
        
        self.CircleItem = CircleItem
        self.Caption = _("Insert circle")
        self.ClientHeight = 190
        self.ClientWidth = 340
        CircleData = CircleItem.PluginData["Circle"] if CircleItem else ("","","")

        self.Label1 = ScaledVcl.TLabel(None, Parent = self.Panel, Caption = _("Radius:"), Top = 12, Left = 8)
        self.Label2 = ScaledVcl.TLabel(None, Parent = self.Panel, Caption = _("Center:"), Top = 44, Left = 8)
        self.Label3 = ScaledVcl.TLabel(None, Parent = self.Panel, Caption = "X:", Top = 44, Left = 55)
        self.Label4 = ScaledVcl.TLabel(None, Parent = self.Panel, Caption = "Y:", Top = 44, Left = 180)
        self.Label5 = ScaledVcl.TLabel(None, Parent = self.Panel, Caption = _("Line style:"), Top = 76, Left = 8)
        self.Label6 = ScaledVcl.TLabel(None, Parent = self.Panel, Caption = _("Color:"), Top = 76, Left = 180)
        self.Label7 = ScaledVcl.TLabel(None, Parent = self.Panel, Caption = _("Width:"), Top = 108, Left = 8)
        self.Edit1 = ScaledVcl.TEdit(None, Parent = self.Panel, Top = 8, Left = 70, Width = 100, Text = CircleData[0])
        self.Edit2 = ScaledVcl.TEdit(None, Parent = self.Panel, Top = 40, Left = 70, Width = 100, Text = CircleData[1])
        self.Edit3 = ScaledVcl.TEdit(None, Parent = self.Panel, Top = 40, Left = 215, Width = 100, Text = CircleData[2])
        self.LineSelect = ScaledVcl.TLineSelect(None, Parent = self.Panel, Top = 72, Left = 70, Width = 100, LineStyle = CircleItem.Style if CircleItem else 0)
        self.ColorBox = ScaledVcl.TExtColorBox(None, Parent = self.Panel, Top = 72, Left = 215, Width = 100, Selected = CircleItem.Color if CircleItem else 0x0000FF)
        self.Edit4 = ScaledVcl.TEdit(None, Parent = self.Panel, Top = 104, Left = 70, Width = 100, Text = str(CircleItem.Size) if CircleItem else "1")
        
    def OnOk(self, Sender):
        CircleData = (self.Edit1.Text, self.Edit2.Text, self.Edit3.Text)
        Func = Graph.TParFunc("{1}+{0}*cos t".format(*CircleData), "{2}+{0}*sin t".format(*CircleData))
        Func.PluginData["Circle"] = CircleData
        Func.LegendText = _("Circle: radius={0}, center=({1},{2})").format(*CircleData)
        Func.Color = self.ColorBox.Selected
        Func.Style = self.LineSelect.LineStyle
        Func.Size = int(self.Edit4.Text)
        Func.From = 0
        Func.To = 6.28318530717

        if self.CircleItem:
            Graph.FunctionList[Graph.FunctionList.index(self.CircleItem)] = Func
        else:
            Graph.FunctionList.append(Func)
        self.Close()

def ShowCircleDialog(Sender):
    d = CircleDialog()
    d.ShowModal()

def OnEdit(Item):
    if "Circle" in Item.PluginData:
        d = CircleDialog(Item)
        d.ShowModal()
        return True

Action = Graph.CreateAction(Caption=_("Insert circle") + "...", OnExecute=ShowCircleDialog, Hint=_("Create circle from center and radius."), ShortCut="Ctrl+Shift+C", IconFile="Circle.png")
Graph.AddActionToMainMenu(Action)
Graph.OnEdit.append(OnEdit)
