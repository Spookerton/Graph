object Form9: TForm9
  Left = 449
  Top = 279
  AlphaBlend = True
  AlphaBlendValue = 240
  BiDiMode = bdLeftToRight
  BorderIcons = [biSystemMenu]
  BorderStyle = bsToolWindow
  Caption = 'Evaluate'
  ClientHeight = 198
  ClientWidth = 183
  Color = clBtnFace
  DragKind = dkDock
  DragMode = dmAutomatic
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  ParentBiDiMode = False
  PopupMode = pmExplicit
  PopupParent = Form1.Owner
  Position = poMainFormCenter
  ShowHint = True
  OnEndDock = FormEndDock
  OnHide = FormHide
  OnResize = FormResize
  OnShortCut = FormShortCut
  OnStartDock = FormStartDock
  DesignSize = (
    183
    198)
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    AlignWithMargins = True
    Left = 5
    Top = 5
    Width = 173
    Height = 188
    Margins.Left = 5
    Margins.Top = 5
    Margins.Right = 5
    Margins.Bottom = 5
    Align = alClient
    AutoSize = False
    Caption = 'Select a function or tangent to evaluate.'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -16
    Font.Name = 'Microsoft Sans Serif'
    Font.Style = []
    ParentFont = False
    WordWrap = True
    ExplicitLeft = 0
    ExplicitTop = 0
    ExplicitWidth = 183
    ExplicitHeight = 198
  end
  inline PolFuncFrame1: TPolFuncFrame
    Left = 0
    Top = 0
    Width = 184
    Height = 230
    Hint = 
      'Select where the cursor will be placed when you click on the ima' +
      'ge.'
    Anchors = [akLeft, akTop, akRight]
    ParentBackground = False
    TabOrder = 3
    Visible = False
    inherited Label7: TLabel
      Width = 40
      ExplicitWidth = 40
    end
    inherited Edit4: TEdit [7]
    end
    inherited Edit5: TEdit [8]
    end
    inherited Edit6: TEdit [9]
    end
    inherited Edit1: TMyEdit [10]
      OnChange = Edit1Change
    end
    inherited Edit2: TEdit [11]
    end
    inherited Edit3: TEdit [12]
    end
    inherited UpDownEx1: TUpDownEx
      OnUpDown = UpDownExUpDown
    end
  end
  inline StdFuncFrame1: TStdFuncFrame
    Left = 0
    Top = 0
    Width = 184
    Height = 167
    HelpContext = 100
    Anchors = [akLeft, akTop, akRight]
    ParentBackground = False
    TabOrder = 0
    Visible = False
    inherited Label4: TLabel
      Left = 9
      Width = 24
      ExplicitLeft = 9
      ExplicitWidth = 24
    end
    inherited Label5: TLabel
      Width = 40
      ExplicitWidth = 40
    end
    inherited ComboBox1: TComboBox [5]
    end
    inherited Edit1: TMyEdit [6]
      OnChange = Edit1Change
    end
    inherited UpDown1: TUpDownEx
      OnUpDown = UpDownExUpDown
      Anchors = [akTop, akRight]
    end
  end
  inline AreaFrame1: TAreaFrame
    Left = 0
    Top = 0
    Width = 184
    Height = 102
    Anchors = [akLeft, akTop, akRight]
    ParentBackground = False
    TabOrder = 2
    Visible = False
    ExplicitWidth = 184
    ExplicitHeight = 102
    inherited GridPanel1: TGridPanelEx
      Width = 184
      Height = 102
      ControlCollection = <
        item
          Column = 0
          Control = AreaFrame1.Label1
          Row = 0
        end
        item
          Column = 0
          Control = AreaFrame1.Label2
          Row = 1
        end
        item
          Column = 0
          Control = AreaFrame1.Label3
          Row = 2
        end
        item
          Column = 1
          Control = AreaFrame1.Edit3
          Row = 2
        end
        item
          Column = 1
          Control = AreaFrame1.Panel1
          Row = 0
        end
        item
          Column = 1
          Control = AreaFrame1.Panel2
          Row = 1
        end>
      ExplicitWidth = 184
      ExplicitHeight = 102
      inherited Label1: TLabel
        Width = 26
        Height = 22
        ExplicitWidth = 26
      end
      inherited Label2: TLabel
        Top = 39
        Height = 22
        ExplicitTop = 39
      end
      inherited Label3: TLabel
        Top = 70
        Width = 22
        Height = 22
        ExplicitTop = 70
        ExplicitWidth = 22
      end
      inherited Edit3: TEdit
        Left = 34
        Top = 70
        Width = 146
        Height = 22
        ExplicitLeft = 34
        ExplicitTop = 70
        ExplicitWidth = 146
      end
      inherited Panel1: TPanel
        Left = 34
        Width = 146
        Height = 22
        ExplicitLeft = 34
        ExplicitWidth = 146
        ExplicitHeight = 22
        inherited Edit1: TMyEdit
          Width = 131
          OnChange = Edit1Change
          ExplicitWidth = 131
        end
        inherited UpDown1: TUpDownEx
          Left = 131
          OnUpDown = UpDownExUpDown
          ExplicitLeft = 131
        end
      end
      inherited Panel2: TPanel
        Left = 34
        Top = 39
        Width = 146
        Height = 22
        ExplicitLeft = 34
        ExplicitTop = 39
        ExplicitWidth = 146
        ExplicitHeight = 22
        inherited Edit2: TMyEdit
          Width = 131
          OnChange = Edit1Change
          ExplicitWidth = 131
        end
        inherited UpDown2: TUpDownEx
          Left = 131
          OnUpDown = UpDownExUpDown
          ExplicitLeft = 131
        end
      end
    end
  end
  inline ParFuncFrame1: TParFuncFrame
    Left = 0
    Top = 0
    Width = 184
    Height = 230
    HelpContext = 101
    Anchors = [akLeft, akTop, akRight]
    ParentBackground = False
    TabOrder = 1
    Visible = False
    inherited Label7: TLabel
      Width = 40
      ExplicitWidth = 40
    end
    inherited Edit4: TEdit [7]
    end
    inherited Edit5: TEdit [8]
    end
    inherited Edit6: TEdit [9]
    end
    inherited Edit1: TMyEdit [10]
      OnChange = Edit1Change
    end
    inherited Edit2: TEdit [11]
    end
    inherited Edit3: TEdit [12]
    end
    inherited UpDownEx1: TUpDownEx
      OnUpDown = UpDownExUpDown
    end
  end
  inline TanFrame1: TTanFrame
    Left = 0
    Top = 0
    Width = 184
    Height = 69
    HelpContext = 100
    Anchors = [akLeft, akTop, akRight]
    ParentBackground = False
    TabOrder = 4
    Visible = False
    inherited Edit1: TMyEdit
      OnChange = Edit1Change
    end
  end
  inline RelationFrame1: TRelationFrame
    Left = 0
    Top = 0
    Width = 184
    Height = 76
    Anchors = [akLeft, akTop, akRight]
    ParentBackground = False
    TabOrder = 5
    Visible = False
    ExplicitWidth = 184
    ExplicitHeight = 76
    inherited Label1: TLabel
      Width = 11
      ExplicitWidth = 11
    end
    inherited Label2: TLabel
      Width = 11
      ExplicitWidth = 11
    end
    inherited Edit1: TMyEdit
      Width = 149
      OnChange = Edit1Change
      ExplicitWidth = 149
    end
    inherited Edit2: TMyEdit
      Width = 149
      OnChange = Edit1Change
      ExplicitWidth = 149
    end
  end
end
