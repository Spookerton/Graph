import Graph
import os.path
import vcl

PluginName = "Unit test"
PluginVersion = "1.0"
PluginDescription = "Run unit tests from the Test directory."

FilesDir = os.path.dirname(__file__) + "/Files/"

class _AssertRaisesBaseContext(object):
    def __init__(self, expected, callable_obj=None):
        self.expected = expected
        if callable_obj is not None:
            try:
                self.obj_name = callable_obj.__name__
            except AttributeError:
                self.obj_name = str(callable_obj)
        else:
            self.obj_name = None


class _AssertRaisesContext(_AssertRaisesBaseContext):
    """A context manager used to implement TestCase.assertRaises* methods."""
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, tb):
        if exc_type is None:
            try:
                exc_name = self.expected.__name__
            except AttributeError:
                exc_name = str(self.expected)
            if self.obj_name:
                raise AssertionError("{0} not raised by {1}".format(exc_name, self.obj_name))
            else:
                raise AssertionError("{0} not raised".format(exc_name))
        if not issubclass(exc_type, self.expected):
            # let unexpected exceptions pass through
            return False
        # store exception, without traceback, for later retrieval
        self.exception = exc_value.with_traceback(None)
        return True

def assertRaises(excClass, callableObj=None, *args, **kwargs):
    """Same as unittest.assertRaises()"""
    context = _AssertRaisesContext(excClass, callableObj)
    if callableObj is None:
        return context
    with context:
        callableObj(*args, **kwargs)

def assertEqual(Left, Right):
    if Left != Right:
        raise AssertionError("Left: %s; Right: %s" % (repr(Left), repr(Right)))

def assertRange(Value, Min, Max):
    if Value < Min or Value > Max:
        raise AssertionError("Value %s; Min: %s; Max: %s" % (repr(Value), repr(Min), repr(Max)))
        
def DoNextTest():
    global NextModuleIndex
    while NextModuleIndex < len(Modules):
        Module = Modules[NextModuleIndex]
        NextModuleIndex += 1
        if Module.Run(ModuleLevel):
            break
        vcl.Application.ProcessMessages()
    else:
        print("Unit tests finished!")

def Run(Level = 0):
    global Modules
    global NextModuleIndex
    global ModuleLevel
    print("Starting unit tests...")
    # Find all modules in the same dir as this, but filter out this module
    Modules = [Module for Module in Graph.LoadPlugins(os.path.dirname(__file__)) if Module.__name__ != "__init__"]
    NextModuleIndex = 0
    ModuleLevel = Level
    DoNextTest()

def RunUnitTests(Sender):
    Graph.Form22.Show()
    Run()
    
Action = Graph.CreateAction(Caption="Run unit tests", OnExecute=RunUnitTests)  
Graph.AddActionToMainMenu(Action)  
    