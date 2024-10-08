import Graph

PowerPoints = [(32.0, 1.93), (40.0, 1.67), (50.0, 1.4), (60.0, 1.22), (70.0, 1.06), (80.0, 0.93), (90.0, 0.825), (100.0, 0.74), (150.0, 0.477), (200.0, 0.341), (250.0, 0.269), (300.0, 0.22), (350.0, 0.189), (400.0, 0.17), (450.0, 0.155), (500.0, 0.145), (550.0, 0.139), (600.0, 0.137)]
LinearPoints = [(1.9, 2.9), (3.0, 3.3), (1.8, 2.5), (2.8, 2.5), (2.8, 3.0), (3.4, 3.0), (3.3, 3.0), (1.9, 3.1), (1.6, 2.0), (2.5, 2.8), (2.4, 2.2), (3.2, 2.9), (3.6, 3.5), (3.2, 3.6), (2.9, 3.5), (2.6, 3.1), (2.8, 3.1), (3.1, 3.6), (2.8, 2.4), (2.9, 2.6), (3.8, 3.4), (2.9, 3.0), (2.3, 2.3), (1.6, 1.9), (3.4, 3.6)]
LogarithmicPoints = [(1.0, 8.392361968058083), (5.0, 57.9681491752977), (9.0, 68.66433139653672), (13.0, 60.65731490562683), (17.0, 90.51017053540025), (21.0, 105.50067345931728), (25.0, 78.45340591668861), (29.0, 105.47631294590695), (33.0, 97.5580404869968), (37.0, 113.96116804104867), (41.0, 119.42684250215763), (45.0, 94.49836793175133), (49.0, 92.72937200511855), (53.0, 96.56113981430062), (57.0, 129.54849535894533), (61.0, 109.2604586303937), (65.0, 129.7344480815405), (69.0, 110.24837627871997), (73.0, 128.1286137881117), (77.0, 130.18069145011734), (81.0, 123.03953167758878), (85.0, 130.68539011362006), (89.0, 119.03101370591688), (93.0, 120.22636156033789), (97.0, 125.48273729092493)]
ExponentialPoints = [(0.0, 0.08004149126510186), (1.0, 0.13208391390260138), (2.0, 0.28713168068637135), (3.0, 0.27915606568615514), (4.0, 0.3912734573168102), (5.0, 0.5327238682319495), (6.0, 1.3117016268760155), (7.0, 1.454248228285591), (8.0, 1.9933038423940324), (9.0, 3.3329469632913713), (10.0, 6.126749191471188), (11.0, 5.877627729572124), (12.0, 16.11678458542155), (13.0, 26.243070970366624), (14.0, 25.735174060932987)]
SinusoidalPoints = [(0.0, 3.952412496879536), (1.0, 3.562793246608307), (2.0, 6.820671183638135), (3.0, 4.584799915341085), (4.0, 1.3616613107112157), (5.0, 0.42994262330724164), (6.0, -3.4503810142858873), (7.0, -2.789920296538391), (8.0, -2.904522588613234), (9.0, -5.455722762068252), (10.0, -3.16745087177242), (11.0, 0.44929092354414957), (12.0, 1.3037361914918133), (13.0, 4.464489199621244), (14.0, 3.8548406201993317)]
Polynomial3Points = [(-1.0, -6.922257123767682), (-0.8, 0.37568918204375246), (-0.6, 1.9115223779102295), (-0.4, 5.259883649092487), (-0.2, 5.232419343053032), (0.0, 2.5027682904024324), (0.2, 1.6856253956292893), (0.4, 2.0549025918230797), (0.6, 0.8494593309534921), (0.8, -2.0193476797714407), (1.0, -3.968432181527124), (1.2, -2.5106899590000533), (1.4, -2.427798555933361), (1.6, -2.423798838407041), (1.8, -2.749131349909745), (2.0, -0.5589056243827857), (2.2, 1.7592222118535026), (2.4, 7.305748688899685), (2.6, 14.885618848867363), (2.8, 23.291054738997474)]
def Run(Level=0):
  print("Running trendline test...")
  Graph.LoadDefault()
  Graph.Property.RoundTo = 4

  PointSeries = Graph.TPointSeries()

  # Test power trendline
  PointSeries.Points = PowerPoints
  Func = PointSeries.CreateTrendline(Graph.ttPower)
  assert Func.Text == "64.5171*x^-0.9818"

  # Test moving average with Period=1
  Func = PointSeries.CreateMovingAverage(1)
  for I,(x,y) in enumerate(PowerPoints):
    (x1, y1) = Func.Eval(I)
    assert abs(x1 - x) < 1E-10 and abs(y1 - y) < 1E-10 

  # Test linear trendline
  PointSeries.Points = LinearPoints
  Func = PointSeries.CreateTrendline(Graph.ttLinear)
  assert Func.Text == "0.5761*x+1.3335"

  # Test logarithmic trendline
  PointSeries.Points = LogarithmicPoints
  Func = PointSeries.CreateTrendline(Graph.ttLogarithmic)
  assert Func.Text == "25.3831*ln(x)+11.5186"

  # Test exponential trendline
  PointSeries.Points = ExponentialPoints
  Func = PointSeries.CreateTrendline(Graph.ttExponential)
  assert Func.Text == "0.0847*1.5178^x"

  # Test 3rd order polynomial trendline
  PointSeries.Points = Polynomial3Points
  Func = PointSeries.CreateTrendline(Graph.ttPolynomial, 3)
  assert Func.Text == "3.9913*x^3-7.8328*x^2-2.5595*x+3.9999"

  # Test user model trendline
  PointSeries.Points = SinusoidalPoints
  Func = PointSeries.CreateModelTrendline("$a*sin($b*x+$c)")
  assert Func.Text == "1.7952*sin(1.0995*x-0.162)" # Not a very good fit
  Func = PointSeries.CreateModelTrendline("$a*sin($b*x+$c)", {"$a":5, "$b":0.5, "$c":0.5})
  assert Func.Text == "4.8202*sin(0.4971*x+0.6936)"

