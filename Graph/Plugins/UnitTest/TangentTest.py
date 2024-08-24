import Graph

def Run(Level):
  print("Running tangent test...")
  Function = Graph.TStdFunc("x^2")
  Tan = Graph.TTangent()
  Tan.Pos = 2
  assert not Tan.Valid
  Function.ChildList.append(Tan)
  assert Tan.Valid
