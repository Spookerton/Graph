import Graph
import vcl
import math

PluginName = "Sort pointseries"
PluginVersion = "0.2"
PluginDescription = "This adds a context menu to the table in the Insert|Point series dialog, which can be used to sort the points."

# We filter using the evaluated value of the text string.
# If the text cannot be evaluated, treat it as -inf.
def Eval(x):
    try:
        return Graph.Eval(x)
    except Graph.EFuncError:
        return -math.inf   

def SortValues(Sender):
    Grid = Sender.Owner.Grid
    Cells = Grid.Cells
    ColCount = Grid.ColCount
    RowCount = Grid.RowCount
    EmptyRow = [""] * ColCount  

    # filter is used to remove empty rows
    Data = list(filter(lambda L: L != EmptyRow, [[Cells[Col,Row] for Col in range(ColCount)] for Row in range(1, RowCount)]))
    if Sender.Name == "IncX": 
        Data.sort(key=lambda L: Eval(L[0]))
    elif Sender.Name == "DecX": 
        Data.sort(key=lambda L: Eval(L[0]), reverse=True)
    elif Sender.Name == "IncY": 
        Data.sort(key=lambda L: Eval(L[1]))
    elif Sender.Name == "DecY": 
        Data.sort(key=lambda L: Eval(L[1]), reverse=True)
    for Row in range(1, RowCount):
        for Col in range(ColCount):
            Cells[Col,Row] = Data[Row-1][Col] if Row-1 < len(Data) else ""
    

def OnShowForm(Form):
    if Form.Name == "Form14":
        SortItem = vcl.TMenuItem(Form.PopupMenu1, Caption="Sort", _owned=False)
        Form.PopupMenu1.Items.Insert(7, SortItem)
        SortItem.Add(vcl.TMenuItem(Form, Caption="Increasing x", Name="IncX", OnClick=SortValues, _owned=False))
        SortItem.Add(vcl.TMenuItem(Form, Caption="Decreasing x", Name="DecX", OnClick=SortValues, _owned=False))
        SortItem.Add(vcl.TMenuItem(Form, Caption="Increasing y", Name="IncY", OnClick=SortValues, _owned=False))
        SortItem.Add(vcl.TMenuItem(Form, Caption="Decreasing y", Name="DecY", OnClick=SortValues, _owned=False))

Graph.OnShowForm.append(OnShowForm)
