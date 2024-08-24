import Graph

PluginName = "Store Zoom"
PluginVersion = "0.1"
PluginDescription = ("Allows you to use Ctrl+Shift+1 to Ctrl+Shift+9 to store axes settings. "
    "You can afterwards use Ctrl+1 to Ctrl+9 to apply a stored setting. "
    "The settings are stored globally and may be used between different .grf files.\n\n"
    "The settings stored are Minimum, Maximum and Logarithmic scale for both axes.")

def StoreAxes(Index):
    Setting = {
        "xMin" : Graph.Axes.xAxis.Min,
        "xMax" : Graph.Axes.xAxis.Max,
        "xLogScl" : Graph.Axes.xAxis.LogScl,
        "yMin" : Graph.Axes.yAxis.Min,
        "yMax" : Graph.Axes.yAxis.Max,
        "yLogScl" : Graph.Axes.yAxis.LogScl,
    }
    Graph.GlobalPluginData["StoredZoom" + str(Index)] = (Setting,)
        
def RestoreAxes(Index):
    if "StoredZoom" + str(Index) in Graph.GlobalPluginData:
        Setting = Graph.GlobalPluginData["StoredZoom" + str(Index)][0]
        Graph.Axes.xAxis.Min = Setting["xMin"]
        Graph.Axes.xAxis.Max = Setting["xMax"]
        Graph.Axes.xAxis.LogScl = Setting["xLogScl"]
        Graph.Axes.yAxis.Min = Setting["yMin"]
        Graph.Axes.yAxis.Max = Setting["yMax"]
        Graph.Axes.yAxis.LogScl = Setting["yLogScl"]

def HandleShortCut(Key, ShiftState):
    if len(Key) == 1 and "1" <= Key <= "9":
        if ShiftState == {"Shift", "Ctrl"}:
            StoreAxes(ord(Key) - 0x30)
        elif ShiftState == {"Ctrl"}:
            RestoreAxes(ord(Key) - 0x30)


Graph.OnShortCut.append(HandleShortCut)
