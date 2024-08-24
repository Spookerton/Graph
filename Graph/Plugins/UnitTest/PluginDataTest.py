import Graph
import os.path
import UnitTest
from UnitTest import assertRaises

def Run(Level):
  print("Running plugin data test...")
  
  # Test Graph.GlobalPluginData
  StoredPluginData = Graph.GlobalPluginData.copy()
  Graph.GlobalPluginData.clear()
  Graph.GlobalPluginData["Test"] = (5, "Hello world", [1,2,3,4])
  assert Graph.GlobalPluginData["Test"] == (5, "Hello world", [1,2,3,4])
  Graph.GlobalPluginData["Test2"] = ({"1":5, "2":7, "6":9},)
  Graph.GlobalPluginData["Test3"] = (3, 5.5, True)  
  assert Graph.GlobalPluginData["Test2"] == ({"1":5, "2":7, "6":9},)
  assert Graph.GlobalPluginData["Test3"] == (3, 5.5, True)
  assert len(Graph.GlobalPluginData) == 3
  assert [x for x in Graph.GlobalPluginData] == ["Test", "Test2", "Test3"]
  del Graph.GlobalPluginData["Test"]
  del Graph.GlobalPluginData["Test2"]
  del Graph.GlobalPluginData["Test3"]

  with assertRaises(KeyError):
    del Graph.GlobalPluginData["Test"]
  with assertRaises(KeyError):
    del Graph.GlobalPluginData[5]
  with assertRaises(KeyError):
    Graph.GlobalPluginData[5] = (7,) 
  Graph.GlobalPluginData.update(StoredPluginData)
  
  # Test Graph.PluginData and TStdFunc.PluginData
  Graph.LoadDefault()
  Graph.PluginData["Test"] = (5, "Hello world", [1,2,3,4])
  Graph.PluginData["Test2"] = ({"1":5, "2":7, "6":9},)
  Graph.PluginData["Test3"] = (3, 5.5, True)  
  
  Func = Graph.TStdFunc("sin x")
  Func.PluginData["Test4"] = ("Testing", (3, 5))
  Graph.FunctionList.append(Func)
  Graph.SaveToFile(UnitTest.FilesDir + "/PluginData.grf")
  Graph.LoadDefault()
  assert Graph.PluginData == {}
  assert len(Graph.FunctionList) == 1
  Graph.LoadFromFile(UnitTest.FilesDir + "/PluginData.grf")
  assert list(Graph.PluginData.keys()) == ["Test", "Test2", "Test3"]
  assert Graph.PluginData["Test"] == (5, "Hello world", [1,2,3,4])
  assert Graph.PluginData["Test2"] == ({"1":5, "2":7, "6":9},)
  assert Graph.PluginData["Test3"] == (3, 5.5, True)  
  Func = Graph.FunctionList[1]
  assert len(Graph.PluginData) == 3
  assert Func.PluginData["Test4"] == ("Testing", [3, 5]) #Notice conversion from tuple to list

  
   
    