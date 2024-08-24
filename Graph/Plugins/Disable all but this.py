import Graph

PluginName = "Disable all but this"
PluginVersion = "1.0"
PluginDescription = "Creates a context menu for disabling all other items than the selected one."

def Execute(Sender):
    for Item in Graph.FunctionList:
        if Item != Graph.Selected and type(Item) != Graph.Data.TAxesView:
            Item.Visible = False
        for Item2 in Item.ChildList:
            if Item2 != Graph.Selected:
                Item2.Visible = False
			
Action = Graph.CreateAction(Caption="Disable all but this", OnExecute=Execute, Hint="Disable all but this item.")
ContextMenuItem = Graph.AddActionToContextMenu(Action)
