import Graph
import vcl
import math
import UnitTest

Points = [(X, 5*math.sin(X)) for X in range(-10, 11)]

def Timeout(Sender):
    global Index
    global Timer
    PointSeries.Points.append(Points[Index])
    Index += 1
    if Index == len(Points):
        Timer.Enabled = False
        assert PointSeries.Points == Points
        UnitTest.DoNextTest()
    
def Run(Level):
    global PointSeries
    global Timer
    global Index
    print("Running timer test...")
    Index = 0
    PointSeries = Graph.TPointSeries()
    PointSeries.Size = 5
    PointSeries.LineSize = 0
    Graph.FunctionList.append(PointSeries)
    Timer = vcl.TTimer(None, OnTimer=Timeout, Interval=100)
    return True

