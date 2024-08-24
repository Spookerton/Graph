import Graph
import Gui
from Gui import ScaledVcl

PluginName = "Transform"
PluginVersion = "0.1"
PluginDescription = """Creates a new point series that is a transformation of the selected point series. 
You use equations to describe the new (X,Y) coordinate at index n in the point series. " +
x[a] refers to the x-coordinate at index a in the existing point series.
y[a] refers to the y-coordinate at index a in the existing point series.
n refers to the index of the coordinate set in the new point series.
Example: X[n] = x[n-1], Y[n] = x[n] + y[n]
This creates a new point series where the x-coordinate is the previous x-coordinate from the old series, and
the y-coordinate is the existing x- and y-coordinates added."""

class TransformDialog(Gui.SimpleDialog):
    def __init__(self):
        Gui.SimpleDialog.__init__(self)
        self.Caption = "Transform point series"
        self.Height = 146
        self.Width = 346

        self.Label1 = ScaledVcl.TLabel(None, Parent = self.Panel, Caption = "X[n] = ", Top = 12, Left = 8)
        self.Label2 = ScaledVcl.TLabel(None, Parent = self.Panel, Caption = "Y[n] = ", Top = 44, Left = 8)
        self.Edit1 = ScaledVcl.TEdit(None, Parent = self.Panel, Top = 8, Left = 44, Width = 270)
        self.Edit2 = ScaledVcl.TEdit(None, Parent = self.Panel, Top = 40, Left = 44, Width = 270)

    def OnOk(self, sender):
        P = Graph.TPointSeries()
        Locals = {"x": lambda n: Graph.Selected.Points[int(n)][0], "x": lambda n: Graph.Selected.Points[int(n)][1]}
        for n in range(len(Graph.Selected.Points)):
            Locals["n"] = n
            x = Graph.Eval(self.Edit1.Text, Locals=Locals) if self.Edit1.Text != "" else Graph.Selected.Points[n][0]
            y = Graph.Eval(self.Edit2.Text, Locals=Locals) if self.Edit2.Text != "" else Graph.Selected.Points[n][1]
            P.Points.append((x, y))
        Graph.FunctionList.append(P)
        Graph.Redraw()
        self.Close()

def Execute(action):
    d = TransformDialog()
    d.ShowModal()

def OnSelect(Item):
    Action.Enabled = isinstance(Item, Graph.TPointSeries)
    ContextMenuItem.Visible = Action.Enabled

Action = Graph.CreateAction(Caption="Transform", OnExecute=Execute, Hint="Transform point series.")
Graph.AddActionToMainMenu(Action)
ContextMenuItem = Graph.AddActionToContextMenu(Action)
Graph.OnSelect.append(OnSelect)
