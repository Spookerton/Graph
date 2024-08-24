# Module with utility functions and classes for helping with GUI dialogs.
import vcl
import Utility
import Graph

PixelsPerInch = (Graph.Options.FontScale * vcl.Screen.PixelsPerInch) // 100

def OptionsChanged():
  global PixelsPerInch
  PixelsPerInch = (Graph.Options.FontScale * vcl.Screen.PixelsPerInch) // 100

Graph.OnOptionsChanged.append(OptionsChanged)

def Scale(x):
  return x if PixelsPerInch == 96 else (x * PixelsPerInch) // 96

def UnScale(x):
  return x if PixelsPerInch == 96 else (x * 96) // PixelsPerInch 

ScaledProperties = ["Top", "Left", "Width", "Height", "ClientWidth", "ClientHeight", "ItemHeight"]

class ScaledObjectFactory:
    def __init__(self, Type):
        self.Type = Type
  
    def __call__(self, *arg, **kwds):
        return ScaledObject(self.Type(*arg), kwds)

class ScaledObject:
    def __init__(self, Object, kwds={}, Scaled=False):
        object.__setattr__(self, "Object", Object)
        object.__setattr__(self, "_Wrapper", Object)
        if not Scaled:
            if PixelsPerInch != 96:
                if hasattr(Object, "ItemHeight"):
                    Object.ItemHeight = Scale(Object.ItemHeight)
                if hasattr(Object, "ParentFont") and not Object.ParentFont:
                    Object.Font.Size = (Object.Font.Size * Graph.Options.FontScale) // 100
            
            Object.Top = Scale(Object.Top)
            Object.Left = Scale(Object.Left)
            Object.Width = Scale(Object.Width)
            Object.Height = Scale(Object.Height)
            for Name, Value in kwds.items():
                setattr(Object, Name, Scale(Value) if Name in ScaledProperties else Value)
  
    def __getattr__(self, Name):
        Value = getattr(self.Object, Name)
        return UnScale(Value) if Name in ScaledProperties else Value
    
    def __setattr__(self, Name, Value):
        setattr(self.Object, Name, Scale(Value) if Name in ScaledProperties else Value)

class ScaledVclType:
    def __getattr__(self, Name):
        return ScaledObjectFactory(getattr(vcl, Name))
    
ScaledVcl = ScaledVclType()    

"""Helper class for creating dialogs. When you derive from this class, your dialog will get an OK button and a Cancel button if ShowCancel is True,
   both at the bottom right of the dialog. A panel will fill the rest of the dialog where you can fill in your content.
"""
class SimpleDialog:
    def __init__(self, ShowCancel=True, **keywords):
        self.Form = ScaledVcl.TForm(None, **keywords)
        self.Name = self.__class__.__name__
        self.Position = "poMainFormCenter"
        self.BorderStyle = "bsDialog"
        self.BorderIcons = "biSystemMenu"
        self.Panel = ScaledVcl.TPanel(None, Parent=self.Form, BevelInner="bvRaised", BevelOuter="bvLowered", Left=8, Top=8, Width=self.ClientWidth - 16, Height=self.ClientHeight - 50, Caption="", Anchors="akLeft,akTop,akRight,akBottom")
        self.Button1 = ScaledVcl.TButton(None, Parent=self.Form, Caption=Utility.GetText("OK"), Anchors="akRight,akBottom", Default=True, OnClick=self.OnOk, Top=self.ClientHeight-32, Left=self.ClientWidth-176)
        if ShowCancel:
            self.Button2 = ScaledVcl.TButton(None, Parent=self.Form, Caption=Utility.GetText("Cancel"), Anchors="akRight,akBottom", ModalResult=1, Cancel=True, Top=self.ClientHeight-32, Left=self.ClientWidth-88)
        else:
            self.Button2 = None
            self.Button1.Cancel = True
            self.Button1.Anchors = "akLeft,akRight,akBottom"
            self.Button1.Left = (self.ClientWidth - self.Button1.Width) / 2
        self.OnShow = self.FormOnShow
        self.panel = self.Panel # For backward compatibility

    def FormOnShow(self, sender):
        self.Button1.TabOrder = 100
        if self.Button2:
            self.Button2.TabOrder = 101
        pass

    # Method to be overwitten in sub class. It is called when the OK button is pressed.
    def OnOk(self, Sender):
        self.Close()

    def __setattr__(self, Name, Value):
        try:
            setattr(self.Form, Name, Value)
        except: #PropertyError:
            object.__setattr__(self, Name, Value)

    def __getattr__(self, Name):
        try:
            return getattr(self.Form, Name)
        except:
            return object.__getattribute__(self, Name)

TimerRefList = []

class DelayCallHelper:
    def __init__(self, Func, Timeout, Args):
        self.Timer = vcl.TTimer(None, Interval=Timeout, OnTimer=self.OnTimer)
        self.Func = Func
        self.Args = Args
    def OnTimer(self, Sender):
        self.Timer.Enabled = False
        try:
            self.Func(*self.Args)
        finally:
            TimerRefList.remove(self)    

"""Starts a timer that will call Func after Timeout in milliseconds with Args as arguments.
   Func: Function to call
   Timeout: Time in milliseconds
   Args: Optional arguments to Func
"""
def DelayCall(Func, Timeout, Args=()):
    TimerRefList.append(DelayCallHelper(Func, Timeout, Args))


"""Delay execution for Timeout in milliseconds while still handling Windows messages.
"""
Delay = Graph.GraphImpl.Delay            

class DfmForm:
  def __init__(self, Form):
    object.__setattr__(self, "_Form", Form)
    object.__setattr__(self, "_Object", ScaledObject(Form, Scaled=True))
    
  def __getattr__(self, Key):
    try:
      return getattr(self._Object, Key)
    except:
      Component = self._Form.FindComponent(Key)
      if Component:
        Component = ScaledObject(Component, Scaled=True)
        object.__setattr__(self, Key, Component)
        return Component
      raise  
    
  def __setattr__(self, Key, Value):
    setattr(self._Object, Key, Value)

""" Translate a component using GetText as translations method.
    WARNING: Needs work; Crashes!
"""    
def TranslateComponent(Comp, GetText):
    ObjectList = [Comp]
    TranslatedComponents = []
    while ObjectList:
        Comp = ObjectList[0]
        del ObjectList[0]
        if Comp not in TranslatedComponents:
            TranslatedComponents.append(Comp)
            for Name in dir(Comp):
                if Name not in ["Name", "Parent", "Owner", "StandardGestures"]: # Reading StandardGestures gits bug in RTTI
                    try:
                        Value = getattr(Comp, Name)
                        if type(Value) is str:
                            setattr(Comp, Name, GetText(Value))
                        elif type(Value) is vcl.VclObject:
                            ObjectList.append(Value)
                    except:
                        pass
            if hasattr(Comp, "Components"):
                for C in Comp:
                    ObjectList.append(C)

def LoadDfmFile(FileName):
    Stream = vcl.TFileStream(FileName, 0x20)
    Stream2 = vcl.TMemoryStream()
    vcl.ObjectTextToBinary(Stream, Stream2)
    Stream2.Position = 0
    Form = vcl.TForm(None)
    Stream2.ReadComponent(Form)
    return DfmForm(Form)
    