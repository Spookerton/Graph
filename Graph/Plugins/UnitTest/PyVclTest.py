import vcl
from UnitTest import assertEqual, assertRange

def TestRef(Func, Value, ExpectedMin, ExpectedMax=None):
    L = [Value]
    Func(L)
    if ExpectedMax is None:
        assertEqual(L[0], ExpectedMin)
    else:
        assertRange(L[0], ExpectedMin, ExpectedMax)

def Run(Level):
    print("Running PyVcl test...")

    Test = vcl.TPyVclUnitTest()
    TestRef(Test.TestRefString, "Test", "Hello world")
    TestRef(Test.TestRefInteger, 5, 10)
    TestRef(Test.TestRefShortInt, 5, 10)
    TestRef(Test.TestRefSmallInt, 5, 10)
    TestRef(Test.TestRefInt64, 5, 10)
    TestRef(Test.TestRefByte, 5, 10)
    TestRef(Test.TestRefWord, 5, 10)
    TestRef(Test.TestRefCardinal, 5, 10)
    TestRef(Test.TestRefUInt64, 5, 10)
    TestRef(Test.TestRefChar, "A", "C")
    TestRef(Test.TestRefAnsiChar, "A", "C")
    TestRef(Test.TestRefWideChar, "A", "C")
    TestRef(Test.TestRefPenStyle, "psDash", "psDashDot")
    TestRef(Test.TestRefEnum, "Test1", "Test3")
    TestRef(Test.TestRefSingle, 15.789, 10.122, 10.124)
    TestRef(Test.TestRefDouble, 15.789, 10.122, 10.124)
    TestRef(Test.TestRefExtended, 15.789, 10.122, 10.124)
    
   