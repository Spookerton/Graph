import Graph
import os

ShowEvents = False
LogFile = os.path.dirname(__file__) + "\\Events.log"

File = open(LogFile, "w", encoding="utf8") if LogFile else None
class EventLogger:
    def __init__(self, s):
        self.s = s
    def __call__(self, *args):
        Str = self.s + (str(args) if len(args) != 1 else "(%s)" % args[0])
        if ShowEvents:
            print(Str)
        if File:
            File.write(Str + "\n")
            File.flush()

for EventName in filter(lambda x: x[:2]=="On", dir(Graph)):
  Graph.__dict__[EventName].insert(0, EventLogger(EventName))
File.write("PluginTester loaded!\n")

