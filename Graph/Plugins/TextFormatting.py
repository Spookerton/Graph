import Graph

PluginName = "Text formatting"
PluginVersion = "0.2"
PluginDescription = ("Adds 4 text formatting functions that can be used in labels: format, polar, rectangular, real\n\n" 
    "format(Value, FormatStr)\n"
    "This will format Value according to the Python format specifier in FormatStr.\n"
    "For how FormatStr works, see https://docs.python.org/3/library/string.html#formatspec\n"
    "Example: %format('sin(0.5)', '09.5f')\n\n"
    "polar(S [, Decimals])\n"
    "S is an equation string to evaluate. Decimlas is an optional number of decimals. "
    "This will evaluate S using complex numbers and show the result in polar format. Make sure to use a font that supports the angle symbol, e.g. Cambria.\n"
    "Example: %polar('(-1)^(1/3)', 2)\n\n"
    "rectangular(S [, Decimals])\n"
    "S is an equation string to evaluate. Decimlas is an optional number of decimals. "
    "This will evaluate S using complex numbers and show the result in rectangular format (a+bi).\n"
    "Example: %rectangular('(-1)^(1/3)', 3)\n\n"
    "real(S [, Decimals])\n"
    "S is an equation string to evaluate. Decimlas is an optional number of decimals. "
    "This will evaluate S using real numbers and show the result.\n"
    "Example: %real('(-1)^(1/3)')"
)

def FormatPolar(S, Decimals=None):
    return Graph.FormatNumber(Graph.EvalComplex(S), Decimals, Graph.cfPolar)
  
def FormatRectangular(S, Decimals=None):
    return Graph.FormatNumber(Graph.EvalComplex(S), Decimals, Graph.cfRectangular)
  
def FormatReal(S, Decimals=None):
    return Graph.FormatNumber(Graph.Eval(S), Decimals, Graph.cfReal)

def FormatPy(S, FormatSpec):
   return format(Graph.Eval(S), FormatSpec)


Graph.AddTextFormatFunc("polar", FormatPolar)  
Graph.AddTextFormatFunc("rectangular", FormatRectangular)
Graph.AddTextFormatFunc("real", FormatReal)
Graph.AddTextFormatFunc("format", FormatPy)
