import Graph
from UnitTest import assertRaises

def TestPointsGet(Points, P, Q):
  assert Points == P
  assert Points[4:10] == P[4:10]
  assert Points[4] == P[4]
  assert Points[:] == P
  assert Points[::-1] == P[::-1]
  assert Points[-10:-5] == P[-10:-5]
  assert Points[10:] == P[10:]
  assert Points[:10] == P[:10]
  assert Points[5:15:3] == P[5:15:3]
  assert Points[::2] == P[::2]
  assert Points[-15:-5:2] == P[-15:-5:2]
  assert Points[-15:-5:-2] == P[-15:-5:-2]
  assert Points[10:200] == P[10:200]
  assert Points[-100:10] == P[-100:10]
  assert Points[-1] == P[-1]
  assert len(Points) == len(P)

  Points[5:10] = Q[0:5]
  P[5:10] = Q[0:5]
  assert Points == P

  Points[7] = Q[5]
  P[7] = Q[5]
  assert Points == P

  Points[-2] = Q[6]
  P[-2] = Q[6]
  assert Points == P

  Points[5:10] = Q[7:10]
  P[5:10] = Q[7:10]
  assert Points == P

  Points[5:8] = Q[10:16]
  P[5:8] = Q[10:16]
  assert Points == P

  del Points[5]
  del P[5]
  assert Points == P

  del Points[5:10]
  del P[5:10]
  assert Points == P

  with assertRaises(IndexError):
    Points[200]
  with assertRaises(IndexError):
    Points[-200]
  with assertRaises(IndexError):
    Points[200] = Q[5]
  with assertRaises((SystemError, TypeError)):
    Points[10] = 5
  with assertRaises(ValueError):
    Points[2:10:2] = Q[0:3]

  Points.append(Q[6])
  P.append(Q[6])
  assert Points == P

  Points.insert(7, Q[10])
  P.insert(7,Q[10])
  assert Points == P

  Points[2:10:2] = Q[0:4]
  P[2:10:2] = Q[0:4]
  assert Points == P

  Points[10:2:-2] = Q[0:4]
  P[10:2:-2] = Q[0:4]
  assert Points == P

  del Points[2:10:2]
  del P[2:10:2]
  assert Points == P

  del Points[::-2]
  del P[::-2]
  assert Points == P

def Run(Level = 0):
  print("Running point series test...")
  Graph.LoadDefault()
  PointSeries = Graph.TPointSeries()
  PointSeries.FillColor = 0x0000FF
  PointSeries.FrameColor = 0x000000
  PointSeries.LineColor = 0xFF0000
  PointSeries.Size = 5
  PointSeries.LineSize = 2
  PointSeries.LineStyle = 1
  Graph.FunctionList.append(PointSeries)
  assert Graph.FunctionList[-1] == PointSeries

  print("Testing TPointSeries.Points...")
  P1 = [(32.0, 1.93), (40.0, 1.67), (50.0, 1.4), (60.0, 1.22), (70.0, 1.06), (80.0, 0.93), (90.0, 0.825), (100.0, 0.74), (150.0, 0.477), (200.0, 0.341), (250.0, 0.269), (300.0, 0.22), (350.0, 0.189), (400.0, 0.17), (450.0, 0.155), (500.0, 0.145), (550.0, 0.139), (600.0, 0.137)]
  Q1 = [(1,10), (2,20), (3,30), (4,40), (5,50), (15,80), (17,85), (20,15), (21,16), (22,17), (30,45), (31,46), (32,47), (33,48), (34,49), (35,50)]
  PointSeries.Points = P1
  TestPointsGet(PointSeries.Points, P1[:], Q1)

  print("Testing TPointSeries.PointData...")
  P2 = [(str(x[0]), str(x[1]), "", "") for x in P1]
  Q2 = [(str(x[0]), str(x[1]), "", "") for x in Q1]
  PointSeries.PointData = P2
  TestPointsGet(PointSeries.PointData, P2[:], Q2)


  PointSeries.PointData = P2
  assert PointSeries.Points == P1

  PointSeries.PointData.clear()
  assert not PointSeries.Points

  PointSeries.Points = P1
  PointSeries.PointData == P2
