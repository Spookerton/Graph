unit PyVclUnitTest;

interface
uses Vcl.Graphics, Vcl.Controls;

{$TYPEINFO ON}
{$M+}

type
  TUnitTestEnum = (Test1, Test2, Test3, Test4);
  TUnitTestSet = set of TUnitTestEnum;

  TPyVclUnitTest = class(TObject)
  public
    var
      Test : TUnitTestEnum;
    procedure TestRefString(var Value: string);
    procedure TestRefInteger(var Value: Integer);
    procedure TestRefShortInt(var Value: ShortInt);
    procedure TestRefSmallInt(var Value: SmallInt);
    procedure TestRefInt64(var Value: Int64);
    procedure TestRefByte(var Value: Byte);
    procedure TestRefWord(var Value: Word);
    procedure TestRefCardinal(var Value: Cardinal);
    procedure TestRefUInt64(var Value: UInt64);
    procedure TestRefChar(var Value: Char);
    procedure TestRefAnsiChar(var Value: AnsiChar);
    procedure TestRefWideChar(var Value: WideChar);
    procedure TestRefPenStyle(var Value: TPenStyle);
    procedure TestRefEnum(var Value: TUnitTestEnum);
    procedure TestRefSingle(var Value: Single);
    procedure TestRefDouble(var Value: Double);
    procedure TestRefExtended(var Value: Extended);
    function TestSet : TStandardGestures;
  end;


implementation
uses
  FindClass;

procedure TPyVclUnitTest.TestRefString(var Value: string);
begin
  Assert(Value = 'Test');
  Value := 'Hello world';
end;

procedure TPyVclUnitTest.TestRefInteger(var Value: Integer);
begin
  Assert(Value = 5);
  Value := 10;
end;

procedure TPyVclUnitTest.TestRefShortInt(var Value: ShortInt);
begin
  Assert(Value = 5);
  Value := 10;
end;

procedure TPyVclUnitTest.TestRefSmallInt(var Value: SmallInt);
begin
  Assert(Value = 5);
  Value := 10;
end;

procedure TPyVclUnitTest.TestRefInt64(var Value: Int64);
begin
  Assert(Value = 5);
  Value := 10;
end;

procedure TPyVclUnitTest.TestRefByte(var Value: Byte);
begin
  Assert(Value = 5);
  Value := 10;
end;

procedure TPyVclUnitTest.TestRefWord(var Value: Word);
begin
  Assert(Value = 5);
  Value := 10;
end;

procedure TPyVclUnitTest.TestRefCardinal(var Value: Cardinal);
begin
  Assert(Value = 5);
  Value := 10;
end;

procedure TPyVclUnitTest.TestRefUInt64(var Value: UInt64);
begin
  Assert(Value = 5);
  Value := 10;
end;

procedure TPyVclUnitTest.TestRefChar(var Value: Char);
begin
  Assert(Value = 'A');
  Value := 'C';
end;

procedure TPyVclUnitTest.TestRefAnsiChar(var Value: AnsiChar);
begin
  Assert(Value = 'A');
  Value := 'C';
end;

procedure TPyVclUnitTest.TestRefWideChar(var Value: WideChar);
begin
  Assert(Value = 'A');
  Value := 'C';
end;

procedure TPyVclUnitTest.TestRefEnum(var Value: TUnitTestEnum);
begin
  Assert(Value = Test1);
  Value := Test3;
end;

procedure TPyVclUnitTest.TestRefPenStyle(var Value: TPenStyle);
begin
  Assert(Value = psDash);
  Value := psDashDot;
end;

procedure TPyVclUnitTest.TestRefSingle(var Value: Single);
begin
  Assert((Value > 15.788) and (Value < 15.79));
  Value := 10.123;
end;

procedure TPyVclUnitTest.TestRefDouble(var Value: Double);
begin
  Assert((Value > 15.788) and (Value < 15.79));
  Value := 10.123;
end;

procedure TPyVclUnitTest.TestRefExtended(var Value: Extended);
begin
  Assert((Value > 15.788) and (Value < 15.79));
  Value := 10.123;
end;

function TPyVclUnitTest.TestSet : TStandardGestures;
begin
  Result := [sgDown];
end;

begin
  RegisterType(TypeInfo(TPyVclUnitTest));
end.
