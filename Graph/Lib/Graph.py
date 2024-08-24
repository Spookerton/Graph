# Interface for built-in functionlity in Graph
import os.path
import os
import sys
import traceback
import GraphImpl
import Settings
import Data
import Utility
import vcl
import xmlrpc.client
import getopt
import collections
import importlib
import winreg
import ast
import zlib
import importlib.machinery
import importlib.util
import collections
import gettext

# enum values
Radian = Settings.Radian
Degree = Settings.Degree
asNone = Settings.asNone
asCrossed = Settings.asCrossed
asBoxed = Settings.asBoxed
lpCustom = Settings.lpCustom
lpTopRight = Settings.lpTopRight
lpBottomRight = Settings.lpBottomRight
lpTopLeft = Settings.lpTopLeft
lpBottomLeft = Settings.lpBottomLeft
npCenter = Settings.npCenter
npBefore = Settings.npBefore
gsLines = Settings.gsLines
gsDots = Settings.gsDots
dtAuto = Data.dtAuto
dtDots = Data.dtDots
dtLines = Data.dtLines
ttTangent = Data.ttTangent
ttNormal = Data.ttNormal
iaLinear = Data.iaLinear
iaCubicSpline = Data.iaCubicSpline
iaHalfCosine = Data.iaHalfCosine
iaCubicSpline2 = Data.iaCubicSpline2
lpAbove = Data.lpAbove
lpBelow = Data.lpBelow
lpLeft = Data.lpLeft
lpRight = Data.lpRight
lpAboveLeft = Data.lpAboveLeft
lpAboveRight = Data.lpAboveRight
lpBelowLeft = Data.lpBelowLeft
lpBelowRight = Data.lpBelowRight
ptCartesian = Data.ptCartesian
ptPolar = Data.ptPolar
lpUserTopLeft = Data.lpUserTopLeft
lpAboveX = Data.lpAboveX
lpBelowX = Data.lpBelowX
lpLeftOfY = Data.lpLeftOfY
lpRightOfY = Data.lpRightOfY
lpUserTopRight = Data.lpUserTopRight
lpUserBottomLeft = Data.lpUserBottomLeft
lpUserBottomRight = Data.lpUserBottomRight
ssAbove = Data.ssAbove
ssBelow = Data.ssBelow
ssXAxis = Data.ssXAxis
ssYAxis = Data.ssYAxis
ssBetween = Data.ssBetween
ssInside = Data.ssInside
rtEquation = Data.rtEquation
rtInequality = Data.rtInequality

ttLinear = Data.ttLinear
ttLogarithmic = Data.ttLogarithmic
ttPolynomial = Data.ttPolynomial
ttPower = Data.ttPower
ttExponential = Data.ttExponential


Axes = Settings.Data.Axes
Options = Settings.Options
Property = Settings.Options #For backward compatibility
Redraw = Data.Redraw
AbortUpdate = Data.AbortUpdate
IsUpdating = Data.IsUpdating
GuiFormatSettings = Settings.GuiFormatSettings;
PlotSettings = Settings.PlotSettings
GuiSettings = Settings.GuiSettings
UpdateMenu = Utility.UpdateMenu
ClearCache = Utility.ClearCache

Selected = None
ModalForm = None
TGraphElem = Data.TGraphElem
TBaseFuncType = Data.TBaseFuncType
TStdFunc = Data.TStdFunc
TParFunc = Data.TParFunc
TPolFunc = Data.TPolFunc
TTangent = Data.TTangent
TTextLabel = Data.TTextLabel
TShading = Data.TShading
TRelation = Data.TRelation
TPointSeries = Data.TPointSeries
TAxesView = Data.TAxesView
TTopGraphElem = Data.TTopGraphElem
Form22 = GraphImpl.Form22
EFuncError = GraphImpl.EFuncError
EGraphError = GraphImpl.EGraphError

# Current version of Graph
VersionInfoType = collections.namedtuple("VersionInfoType", ["Major","Minor","Release","Build","ReleaseLevel"])
VersionInfoType.__eq__ = lambda self, other: self[:len(other)] == other if isinstance(other, tuple) else false
VersionInfo = VersionInfoType._make(GraphImpl.version_info)
Debug = GraphImpl.Debug

PluginMenuItems = []
AllowedExt = importlib.machinery.all_suffixes()

def LoadPlugin(Path, Name, IsDir, ForceLoad=False):
    if IsDir:
        ModuleName = Name
        if ForceLoad or not Name in sys.modules:
            if os.path.exists(Path + "\\" + Name + "\\__init__.py"):
                Spec = importlib.util.spec_from_file_location(Name, Path + "\\" + Name + "\\__init__.py")
            elif os.path.exists(Path + "\\" + Name + "\\__init__.pyc"):
                Spec = importlib.util.spec_from_file_location(Name, Path + "\\" + Name + "\\__init__.pyc")
            else:
                return
        else:
            return sys.modules[Name]
    else:
        ModuleName, Ext = os.path.splitext(Name)
        if not Ext in AllowedExt:
            return
        if not ForceLoad and ModuleName in sys.modules:
            return sys.modules[ModuleName]
        Spec = importlib.util.spec_from_file_location(ModuleName, Path + "\\" + Name)
    Module = importlib.util.module_from_spec(Spec)
    PluginModules.append(Module)
    sys.modules[ModuleName] = Module
    Spec.loader.exec_module(Module)
    return Module              


# Load all .py and .pyc files from the directory specified by Path.
# In addition load all packages, i.e. all __init__.py and __init__.pyc files from sub directories.
def LoadPlugins(Path):
    ModuleList = []
    sys.path.append(Path) # reload() only works if the module is in sys.path
    try:
        for Entry in os.scandir(Path):
            try:
                Module = LoadPlugin(Path, Entry.name, Entry.is_dir())
                if Module:
                    ModuleList.append(Module)
            except Exception:
                traceback.print_exc()
    except FileNotFoundError:
        pass # Skip if Path does not exist
    return ModuleList
        
def LoadAllPlugins():
    if 'LOCALAPPDATA' in os.environ:
        LoadPlugins(os.environ['LOCALAPPDATA'] + "\\Graph\\Plugins")
    LoadPlugins(PluginsDir)

        
# Function called when Graph is started. It will load the plugins in the Plugins directory. BaseDir is the directory where Graph is installed.
def InitPlugins(BaseDir):
    global PluginsDir
    global Form1
    global GraphDir
    GraphDir = BaseDir
    PluginsDir = BaseDir + '\\Plugins'
    Form1 = GraphImpl.Form1 # Form1 does not exist when Graph is started with /regserver
    LoadAllPlugins()


# Put the ClearConsole function into the module used in the console.
sys.modules["__main__"].clear = GraphImpl.ClearConsole

"""Create a new action.
   Caption is the caption used when the action is shown in a menu.
   OnExecute is the function called when the action is executed, e.g. the menu item is selected.
   Hint is an optional hint for the action which is shown in the menu.
   ShortCut is an optional shortcut as a string, e.g. "Ctrl+A".
   IconFile is an optional file name of an image file that will be used as icon.
   OnUpdate is an optinal function called when idle and may be used to update the action.
   AddToToolBar, which is default True, specifies if the action can be added to the tool bar. The user will have to add it himself. This only makes it available.
"""
def CreateAction(Caption, OnExecute, Hint="", ShortCut="", IconFile=None, OnUpdate=None, AddToToolBar=True, Name=None):
    # To avoid problems with file names containing invalid characters, use a checksum.
    # We use adler32() instead of hash() because hash() gives different result between sessions.
    if not Name:
        Name = "A_%08X" % zlib.adler32((OnExecute.__module__+"_"+OnExecute.__name__).encode("utf8"))
    Action = vcl.TAction(None, Name=Name, Caption=Caption, OnExecute=OnExecute, Hint=Hint, ShortCut=vcl.TextToShortCut(ShortCut), Category="Plugins", OnUpdate=OnUpdate)
    Action.ActionList = Form1.ActionManager if AddToToolBar else Form1.ActionList1
    if IconFile:
        if os.path.split(IconFile)[0] == "":
            IconFile = os.path.split(sys.modules[OnExecute.__module__].__file__)[0] + "\\" + IconFile
        Action.ImageIndex = LoadImage(IconFile)
    Action._owned = False # We keep the action, even if the proxy is destroyed
    return Action

TopMenuNames = ["File", "Edit", "Function", "Zoom", "Calc", "Plugins", "Help"]

def AddActionToMainMenu(Action, TopMenu="Plugins", SubMenus=""):
    MenuItems = Form1.ActionMainMenuBar1.ActionClient.Items
    ParentItem = MenuItems.Items[TopMenuNames.index(TopMenu)]
    if ParentItem.Items[0].Caption == "&Tahoma": ParentItem.Items[0].Visible = False #Workaround for bug in C++ Builder XE6; There must always be a visible sub item
    ParentItem.Visible = True
    
    for SubMenu in SubMenus:
        for I in range(ParentItem.Items.Count):
            if ParentItem.Items[I].Caption.replace("&", "") == SubMenu:
                ParentItem = ParentItem.Items[I]
                break
        else:
            ParentItem = ParentItem.Items.Add()
            ParentItem.Caption = SubMenu
            PluginMenuItems.append(ParentItem)
    
    Item = ParentItem.Items.Add()
    Item.Action = Action
    PluginMenuItems.append(Item)
    
    # Update the recent files list of we made changes to the Files menu
    if TopMenu == "File":
        Form1.Recent1.Enabled = False
        Form1.Recent1.Enabled = True
    return Item

def AddActionToContextMenu(Action):
    Item = vcl.TMenuItem(Form1)
    Item._owned = False
    Item.Action = Action
    Form1.PopupMenu1.Items.Insert(Form1.N10.MenuIndex, Item)
    PluginMenuItems.append(Item)
    return Item

def GetActionNames():
    Names = []
    for I in range(Form1.ActionManager.ActionCount):
        Names.append(Form1.ActionManager.Actions[I].Name)
    for I in range(Form1.ActionList1.ActionCount):
        Names.append(Form1.ActionList1.Actions[I].Name)
    return Names
    
def GetAction(Name):
    return Form1.FindComponent(Name)    
    
def ExecuteAction(Name):
    Action = Form1.FindComponent(Name)
    return not Action is None and Action.Execute()

def LoadImage(FileName, BkColor=0xFFFFFF):
    Picture = vcl.TPicture()
    Picture.LoadFromFile(FileName)
    Bitmap = vcl.TBitmap()
    if Picture.Graphic.ClassName() == "TBitmap":
        Picture.Graphic.PixelFormat = "pf32bit"
    if Picture.Height != 64 or Picture.Width != 64 or Picture.Graphic.ClassName == "TBitmap":
        PngImage = vcl.TPngImage()
        PngImage.Assign(Picture.Graphic)
        PngImage.CreateAlpha()
        if Picture.Graphic.ClassName() == "TBitmap":
            Utility.ConvertTransparentToAlpha(PngImage, BkColor)
        Utility.SmoothResize(PngImage, 64, 64)
        Bitmap.Assign(PngImage)
    else:
        Bitmap.Assign(Picture.Graphic)
    Bitmap.AlphaFormat = "afIgnored" #Bitmap may not be premultiplied with alpha when added to image list
    Result = Form1.ImageList64x64.Add(Bitmap, None)   
    return Result

class ConstantsType(collections.abc.MutableMapping):
    def keys(self):
        return GraphImpl.GetConstantNames()
    def __getitem__(self, name):
        return GraphImpl.GetConstant(name)
    def __setitem__(self, name, value):
        if value.__class__ == tuple:
            GraphImpl.SetConstant(name, value[1:], str(value[0]))
        else:
            GraphImpl.SetConstant(name, None, str(value))
    def __delitem__(self, name):
        GraphImpl.DelConstant(name)
    def __iter__(self):
        for k in self.keys(): yield k
    def __len__(self): return len(keys)
    def __repr__(self): return repr(dict(self))
    def clear(self): GraphImpl.ClearConstants()

class CustomFunctionsType(collections.abc.MutableMapping):
    def keys(self):
        return GraphImpl.GetCustomFunctionNames()
    def __getitem__(self, name):
        return GraphImpl.GetCustomFunction(name)
    def __setitem__(self, name, value):
        GraphImpl.SetCustomFunction(name, value)
    def __delitem__(self, name):
        GraphImpl.DelCustomFunction(name)
    def __iter__(self):
        for k in self.keys(): yield k
    def __len__(self): return len(keys)
    def __repr__(self): return repr(dict(self))

def ExecuteEvent(event, args):
    for action in EventList[event]:
        try:
            if action(*args):
                return True
        except:
            traceback.print_exc()

class ChildListType(Data.MutableSequenceWithSlice):
    def __init__(self, node):
        self.node = node
    def _get(self, key):
        return Data.GetChild(self.node, key)
    def __len__(self):
        return Data.ChildCount(self.node)
    def _insert(self, key, value):
        Data.InsertChild(self.node, value, key)
    def _set(self, key, value):
        Data.ReplaceChild(self.node, key, value)
    def append(self, value):
        Data.InsertChild(self.node, value, -1)
    def _del(self, key, count):
        for i in range(count):
          Data.RemoveChild(self.node, key)
    Selected = property(lambda self:Data.GetSelected(), lambda self,x:Data.SetSelected(x))

class PluginDataType(collections.abc.MutableMapping):
    def __init__(self, dict, elem=None): 
      self.data = dict
      self.elem = elem
    def __len__(self): 
      return self.data.size()
    def __getitem__(self, key): 
      return xmlrpc.client.loads(self.data[key])[0]
    def __setitem__(self, key, item): 
      if self.elem:
        Data.PushUndoElem(self.elem)
      self.data[key] = xmlrpc.client.dumps(item, allow_none=True).replace("\n", "")
    def __delitem__(self, key): 
      if self.elem:
        Data.PushUndoElem(self.elem)
      self.data.erase(key)
    def __iter__(self): return iter(self.data)
    def __contains__(self, key): return key in self.data
    def __repr__(self): return repr(dict(self))
    def copy(self): return dict(self)
    
TGraphElem.PluginData = property(lambda self: PluginDataType(self._PluginData, self))
TGraphElem.ChildList = property(lambda self: ChildListType(self))
PluginData = PluginDataType(Data.GetPluginData())

class GlobalPluginDataType(collections.abc.MutableMapping):
    def __init__(self):
        self.key = winreg.CreateKey(winreg.HKEY_CURRENT_USER, 'Software\\Ivan\\Graph\\PluginData')
    def __len__(self):
        return winreg.QueryInfoKey(self.key)[1]
    def __getitem__(self, key):
        try:
            return xmlrpc.client.loads("\n".join(winreg.QueryValueEx(self.key, key)[0]))[0]
        except:
            raise KeyError(key)
    def __setitem__(self, key, value):
        try:
            winreg.SetValueEx(self.key, key, 0, winreg.REG_MULTI_SZ, xmlrpc.client.dumps(value).split('\n'))
        except:
            raise KeyError(key)
    def __delitem__(self, key):
        try:
            winreg.DeleteValue(self.key, key)
        except:
            raise KeyError(key)
    def __iter__(self):
        for index in range(winreg.QueryInfoKey(self.key)[1]):
            yield winreg.EnumValue(self.key, index)[0]
    def __repr__(self):
        return repr(dict(self))
        
    def copy(self):
        return dict(self)

GlobalPluginData = GlobalPluginDataType()

Constants = ConstantsType()
CustomFunctions = CustomFunctionsType()
FunctionList = ChildListType(Data.GetTopElem())

Eval = GraphImpl.Eval
EvalComplex = GraphImpl.EvalComplex
SaveAsImage = GraphImpl.SaveAsImage
Update = GraphImpl.Update
InputQuery = GraphImpl.InputQuery

# TOptions
cfReal = Settings.cfReal
cfRectangular = Settings.cfRectangular
cfPolar = Settings.cfPolar

def HandleOnSelect(Elem):
    global Selected
    Selected = Elem

OnNew = []
OnLoad = []
OnSelect = [HandleOnSelect]
OnClose = []
OnEdit = []
OnAnimate = []
OnDelete = []
OnAxesChanged = []
OnZoom = []
OnOptionsChanged = []
OnCustomFunctions = []
OnNewElem = []
OnChanged = []
OnMoved = []
OnShowForm = []
OnCloseForm = []
OnFunctionListUpdated = []
OnShortCut = []
EventList = [OnNew, OnLoad, OnSelect, OnClose, OnEdit, OnAnimate, OnDelete, OnAxesChanged, 
  OnZoom, OnOptionsChanged, OnCustomFunctions, OnNewElem, OnChanged, OnMoved, OnShowForm, OnCloseForm, OnFunctionListUpdated, OnShortCut]

# Imports from Utility
BeginMultiUndo = Utility.BeginMultiUndo
EndMultiUndo = Utility.EndMultiUndo
LoadDefault = Utility.LoadDefault
LoadFromFile = Utility.LoadFromFile
SaveToFile = Utility.SaveToFile
Import = Utility.Import
ImportPointSeries = Utility.ImportPointSeries
ImportPointSeriesText = Utility.ImportPointSeriesText
GetText = Utility.GetText
ChangeLanguage = Utility.ChangeLanguage

PluginModules = []

class DictTranslations(gettext.NullTranslations):
    def __init__(self, Texts):
        gettext.NullTranslations.__init__(self)
        self.Texts = Texts

    def gettext(self, Message):
        Messages = self.Texts.get(Property.Language, {})
        tmsg = Messages.get(Message)
        if tmsg is None:
            if self._fallback:
                return self._fallback.gettext(Message)
            return Message
        return tmsg

class InternalTranslations(gettext.NullTranslations):
    def gettext(self, Message):
        return Utility.GetText(Message)
InternalTranslationsObject = InternalTranslations()

def CreateTranslations(Texts):
    t = DictTranslations(Texts)
    t.add_fallback(InternalTranslationsObject)
    return t

def GetLanguageList():
    L = GraphImpl.GetLanguageList()
    return list(L.Strings)

TextFormatFunc = {}

def AddTextFormatFunc(Name, Func):
    if Func is None:
        del TextFormatFunc[Name.lower()]
    else:
        TextFormatFunc[Name.lower()] = Func

def CallTextFormatFunc(Name, S):
    Name = Name.lower()
    if Name in TextFormatFunc:
    
        try:
            Args = ast.literal_eval(S + ",")
        except ValueError:
            raise ValueError("Malformed expression. Did you forget adding quotes?") from None
        return str(TextFormatFunc[Name](*Args))
    raise Exception('Text format function "' + Name + '" not registered.')

def FormatNumber(Number, Decimals=None, ComplexFormat=None):
    return Utility.ComplexToString(Number, Property.RoundTo if Decimals is None else Decimals, Property.ComplexFormat if ComplexFormat is None else ComplexFormat)

def Reload():
    global OnSelect
    global PluginMenuItems
    global TextFormatFunc
    for Event in EventList:
        Event.clear()
    OnSelect.append(HandleOnSelect)
    TextFormatFunc = {}

    # Remove all menu items added by plugins
    for Item in reversed(PluginMenuItems):
        Item.Free()
    PluginMenuItems = []

    for Module in PluginModules:
        importlib.reload(Module)

    # Load new plugins and plugins that failed to load
    LoadAllPlugins()

    GraphImpl.RecreateToolBar()

class UserModelsType(collections.abc.MutableMapping):
    def __len__(self): return len(GraphImpl.GetUserModelNames())
    def __getitem__(self, key): return GraphImpl.GetUserModel(key)
    def __setitem__(self, key, item): GraphImpl.SetUserModel(key, item[0], item[1])
    def __delitem__(self, key): GraphImpl.DelUserModel(key)
    def __iter__(self): return iter(GraphImpl.GetUserModelNames())
    def __repr__(self): return "User models: " + repr(dict(self))

UserModels = UserModelsType()


KeyNameMap = {
    0x01: "LButton",
    0x02: "RButton",
    0x03: "Cancel",
    0x04: "MButton",
    0x05: "X1",
    0x06: "X2",
    0x08: "Backspace",
    0x09: "Tab",
    0x0C: "Clear",
    0x0D: "Enter",
    0x10: "Shift",
    0x11: "Ctrl",
    0x12: "Alt",
    0x13: "Pause",
    0x14: "Caps Lock",
    0x1B: "Esc",
    0x20: "Space",
    0x21: "PgUp",
    0x22: "PgDn",
    0x23: "End",
    0x24: "Home",
    0x25: "Left",
    0x26: "Up",
    0x27: "Right",
    0x28: "Down",
    0x29: "Select",
    0x2A: "Print",
    0x2B: "Execute",
    0x2C: "PrtScr",
    0x2D: "Ins",
    0x2E: "Del",
    0x2F: "Help",
    0x5B: "LWin",
    0x5C: "RWin",
    0x5D: "App",
    0x5F: "Sleep",
    0x60: "Num 0",
    0x61: "Num 1",
    0x62: "Num 2",
    0x63: "Num 3",
    0x64: "Num 4",
    0x65: "Num 5",
    0x66: "Num 6",
    0x67: "Num 7",
    0x68: "Num 8",
    0x69: "Num 9",
    0x6A: "*",
    0x6B: "+",
    0x6C: "Separator",
    0x6D: "-",
    0x6E: ".",
    0x6F: "/",
    0x90: "Num Lock",
    0x91: "Scroll Lock",
    0xA0: "LShift",
    0xA1: "RShift",
    0xA2: "LCtrl",
    0xA3: "RCtrl",
    0xA4: "LAlt",
    0xA5: "RAlt",
    0xBA: "OEM 1",
    0xBB: "+",
    0xBC: ",",
    0xBD: "-",
    0xBE: ".",
    0xBF: "OEM 2",
    0xC0: "OEM 3",
    0xDB: "OEM 4",
    0xDC: "OEM 5",
    0xDD: "OEM 6",
    0xDE: "OEM 7",
    0xDF: "OEM 8",
    0xFA: "Play",
    0xFB: "Zoom",
}

def GetKeyName(Key):
    if 0x30 <= Key <= 0x5A:
        return chr(Key)
    if 0x70 <= Key <= 0x87:
        return "F" + str(Key - 0x6F)
    try:
        return KeyNameMap[Key] 
    except:
        return "0x%02X" % Key

def HandleShortCut(Key, KeyData):
    ShiftState = set()
    if KeyData & 0x02: ShiftState.add("Alt")
    if KeyData & 0x04: ShiftState.add("Ctrl")
    if KeyData & 0x01: ShiftState.add("Shift")
    return ExecuteEvent(17, (GetKeyName(Key), ShiftState))
    