import os
import sys
import glob

PythonFiles = "../Plugins/ShowConsole.py ../Plugins/PluginManager/__init__.py"

wine = "wine " if sys.platform.startswith('linux') else ""
python = "python3 " if sys.platform.startswith('linux') else "py "
os.system(wine + "dxgettext ../Source/Forms/*.dfm ../Source/Frames/*.dfm ../Source/Source/Resource.rc /../Components/TIPrintDialog/*.dfm ../Plugins/PluginManager/PluginForm.dfm --nonascii")
os.system(wine + "msgremove default.po -i Ignore.po -o Temp.po")
os.system(python + "-X utf8 pygettext.py -v -o Plugins.po " + PythonFiles) 

InFile = open("Temp.po", "r")
Lines = InFile.readlines()
List = {}
for I in range(len(Lines) - 1, 0, -1):
    Line = Lines[I][:-1]
    if Line[:5] == "msgid":
        MsgId = Line[7:-1]
    if Line[:2] == "#.":
        List[Line[3:]] = MsgId

OutFile = open("Graph.pot", "w")
RcFile = open("../Source/Source/Resource.rc")
RcLines = RcFile.readlines()

for Line in Lines:
    if Line == 'msgstr "Project-Id-Version: PACKAGE VERSION\\n"\n':
        OutFile.write('msgstr "Project-Id-Version: Graph\\n"\n')
    elif Line[:21] == "#. Programmer's name:":
        ID = Line[22:-1]
        for RcLine in RcLines:
            if RcLine.find(ID) != -1:
                Pos = RcLine.find("//")
                if Pos != -1:
                    OutFile.write("#. " + RcLine[Pos+2:])
    elif Line[:2] == "#.":
        Words = Line[3:].split("..")
        NewLine = "#. "
        Key = ""
        for Word in Words:
            Key += Word + ".."
            if (Key + "Caption") in List:
                NewLine += Word + '("' + List[Key + "Caption"] + '")..'
            else:
                NewLine += Word + ".."
        OutFile.write(NewLine[:-2])
    elif Line[:2] != "#:":
        OutFile.write(Line)

InFile.close()
OutFile.close()

os.system("msgcat --use-first -o Graph.pot Graph.pot Plugins.po")

os.remove("default.po")
os.remove("Temp.po")
os.remove("Plugins.po")

# Update po files unless -no-merge is specified as argument
if not "-nomerge" in sys.argv:
    for FileName in glob.glob("*.po"):
        if FileName != "Ignore.po":
            print(FileName + ":")
            os.system('msgmerge -U -v --backup=off "' + FileName + '" Graph.pot --no-wrap')
