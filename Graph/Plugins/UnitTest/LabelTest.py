import Graph

def Run(Level):
    print("Running label test...")
    Label = Graph.TTextLabel()
    S = ("{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1030{\\fonttbl{\\f0\\fnil\\fcharset0 "
    "Times New Roman;}{\\f1\\fnil Times New Roman;}}\r\n{\\colortbl " 
    ";\\red0\\green0\\blue0;}\r\n\\viewkind4\\uc1\\pard\\cf1\\lang1033\\f0\\fs40 " 
    "%s\\lang1030\\f1\\par\r\n}\r\n")
    Label.Text = S % "Hello world"
    Label.Pos = (5, 4)
    Label.BackgroundColor = 0x0000FF
    Label.Rotation = 45
    Graph.FunctionList.append(Label)	
    assert Label.Caption == "Hello world"
