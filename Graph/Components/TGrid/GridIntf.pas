unit GridIntf;

interface
uses
  Vcl.Grids;

type
  TEditorKeyPressEvent = procedure(InplaceEdit: TInplaceEdit; var Key: Char) of object;
  TGetTextEvent = procedure(Sender: TObject; ACol: Integer; ARow: Integer; var Value: string) of object;
  TSetTextEvent = procedure(Sender: TObject; ACol: Integer; ARow: Integer; Value: string) of object;

  TBaseGrid = class(TDrawGrid)
  private
    FOnEditorKeyPress: TEditorKeyPressEvent;
    FOnGetText: TGetTextEvent;
    FOnSetText: TSetTextEvent;

  protected
    procedure DoSetText(Col: Integer; Row: Integer; Value: string); virtual; abstract;
    function DoGetText(Col: Integer; Row: Integer) : string; virtual; abstract;

  public
    procedure SelectAll; virtual; abstract;
    procedure ClearAll; virtual; abstract;
    procedure ClearSelection; virtual; abstract;
    procedure CopyToClipboard(DecimalSeparator: Char); virtual; abstract;
    procedure CutToClipboard(DecimalSeparator: Char);  virtual; abstract;
    procedure PasteFromClipboard(DecimalSeparator: Char; Separator: Char=#0); virtual; abstract;
    function CanCopy: Boolean; virtual; abstract;
	  function ImportFromFile(FileName: string; DecimalSeparator: Char; Separator: Char=#0): Boolean;  virtual; abstract;
    function ExportToFile(FileName: string; Delimiter: Char; DecimalSeparator: Char; Utf8: Boolean=false): Boolean;  virtual; abstract;
    procedure AutoSizeCol(ColIndex: Integer); virtual; abstract;
    procedure LastCell; virtual; abstract;
    procedure NextCell; virtual; abstract;
    procedure InsertRows(Index: Integer; Count: Integer); virtual; abstract;
    procedure RemoveRows(Index: Integer; Count: Integer); virtual; abstract;
    procedure SetCursorPos(Pos: Integer); virtual; abstract;

    property Cells[ACol, ARow: Integer]: string read DoGetText write DoSetText;

  published
    property OnEditorKeyPress: TEditorKeyPressEvent read FOnEditorKeyPress write FOnEditorKeyPress default nil;
    property OnGetText: TGetTextEvent read FOnGetText write FOnGetText default nil;
    property OnSetText: TSetTextEvent read FOnSetText write FOnSetText default nil;
  end;

implementation

end.
