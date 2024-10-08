object Form3: TForm3
  Left = 414
  Top = 211
  HelpContext = 30
  BiDiMode = bdLeftToRight
  BorderIcons = [biSystemMenu]
  BorderStyle = bsDialog
  Caption = 'Edit axes'
  ClientHeight = 270
  ClientWidth = 422
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  ParentBiDiMode = False
  Position = poMainFormCenter
  ShowHint = True
  OnShow = FormShow
  DesignSize = (
    422
    270)
  PixelsPerInch = 96
  TextHeight = 13
  object PageControl1: TPageControl
    Left = 10
    Top = 8
    Width = 404
    Height = 213
    ActivePage = TabSheet3
    Anchors = [akLeft, akTop, akRight, akBottom]
    TabOrder = 0
    object TabSheet1: TTabSheet
      Caption = 'x-axis'
      object GridPanel1: TGridPanelEx
        Left = 0
        Top = 0
        Width = 396
        Height = 185
        Align = alClient
        BevelOuter = bvNone
        ColumnCollection = <
          item
            SizeStyle = ssAuto
            Value = 50.000000000000000000
          end
          item
            Value = 100.000000000000000000
          end
          item
            SizeStyle = ssAuto
          end
          item
            SizeStyle = ssAuto
          end>
        ControlCollection = <
          item
            Column = 0
            Control = Label1
            Row = 0
          end
          item
            Column = 1
            Control = Edit1
            Row = 0
          end
          item
            Column = 0
            Control = Label2
            Row = 1
          end
          item
            Column = 1
            Control = Edit2
            Row = 1
          end
          item
            Column = 0
            Control = Label3
            Row = 2
          end
          item
            Column = 1
            Control = Edit3
            Row = 2
          end
          item
            Column = 0
            Control = Label14
            Row = 3
          end
          item
            Column = 1
            Control = Edit4
            Row = 3
          end
          item
            Column = 2
            Control = CheckBox4
            Row = 2
          end
          item
            Column = 3
            Control = CheckBox6
            Row = 2
          end
          item
            Column = 2
            Control = CheckBox5
            Row = 3
          end
          item
            Column = 3
            Control = CheckBox7
            Row = 3
          end
          item
            Column = 0
            ColumnSpan = 4
            Control = CheckBox1
            Row = 4
          end
          item
            Column = 0
            ColumnSpan = 2
            Control = CheckBox2
            Row = 5
          end
          item
            Column = 2
            ColumnSpan = 2
            Control = CheckBox17
            Row = 5
          end
          item
            Column = 2
            ColumnSpan = 2
            Control = GridPanelEx3
            Row = 0
          end
          item
            Column = 2
            ColumnSpan = 2
            Control = GridPanelEx4
            Row = 1
          end>
        Padding.Left = 3
        Padding.Right = 2
        RowCollection = <
          item
            SizeStyle = ssAbsolute
            Value = 32.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 32.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 32.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 32.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 28.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 28.000000000000000000
          end>
        TabOrder = 0
        DesignSize = (
          396
          185)
        object Label1: TLabel
          Left = 6
          Top = 0
          Width = 44
          Height = 32
          Align = alLeft
          Caption = 'Minimum:'
          FocusControl = Edit1
          Layout = tlCenter
          ExplicitHeight = 13
        end
        object Edit1: TMyEdit
          Left = 58
          Top = 5
          Width = 110
          Height = 21
          Hint = 'Minimum x-value.'
          Anchors = [akLeft, akTop, akRight, akBottom]
          ParentShowHint = False
          ShowHint = True
          TabOrder = 0
        end
        object Label2: TLabel
          Left = 6
          Top = 32
          Width = 47
          Height = 32
          Align = alLeft
          Caption = 'Maximum:'
          FocusControl = Edit2
          Layout = tlCenter
          ExplicitHeight = 13
        end
        object Edit2: TMyEdit
          Left = 58
          Top = 37
          Width = 110
          Height = 21
          Hint = 'Maximum x-value.'
          Anchors = [akLeft, akTop, akRight, akBottom]
          ParentShowHint = False
          ShowHint = True
          TabOrder = 1
        end
        object Label3: TLabel
          Left = 6
          Top = 64
          Width = 44
          Height = 32
          Align = alLeft
          Caption = 'Tick unit:'
          FocusControl = Edit3
          Layout = tlCenter
          ExplicitHeight = 13
        end
        object Edit3: TMyEdit
          Left = 58
          Top = 69
          Width = 110
          Height = 21
          Hint = 'Units between ticks on the x-axis.'
          Anchors = [akLeft, akTop, akRight, akBottom]
          ParentShowHint = False
          ShowHint = True
          TabOrder = 2
        end
        object Label14: TLabel
          Left = 6
          Top = 96
          Width = 42
          Height = 32
          Align = alLeft
          Caption = 'Grid unit:'
          FocusControl = Edit4
          Layout = tlCenter
          ExplicitHeight = 13
        end
        object Edit4: TMyEdit
          Left = 58
          Top = 101
          Width = 110
          Height = 21
          Hint = 'Units between the grid lines perpendicular to the x-axis.'
          Anchors = [akLeft, akTop, akRight, akBottom]
          TabOrder = 3
        end
        object CheckBox4: TCheckBoxEx
          Left = 173
          Top = 64
          Width = 63
          Height = 32
          Hint = 'Determine the tick unit automatically.'
          Align = alLeft
          Caption = 'Auto tick'
          TabOrder = 8
          OnClick = CheckBoxClick
        end
        object CheckBox6: TCheckBoxEx
          Left = 241
          Top = 64
          Width = 73
          Height = 32
          Hint = 'Show tick marks on the x-axis.'
          Align = alLeft
          Caption = 'Show ticks'
          TabOrder = 10
        end
        object CheckBox5: TCheckBoxEx
          Left = 173
          Top = 96
          Width = 63
          Height = 32
          Hint = 'Set the grid unit to the same as the tick unit.'
          Align = alLeft
          Caption = 'Auto grid'
          TabOrder = 9
          OnClick = CheckBoxClick
        end
        object CheckBox7: TCheckBoxEx
          Left = 241
          Top = 96
          Width = 68
          Height = 32
          Hint = 'Show vertical grid lines.'
          Align = alLeft
          Caption = 'Show grid'
          TabOrder = 11
        end
        object CheckBox1: TCheckBoxEx
          Left = 6
          Top = 128
          Width = 103
          Height = 28
          Hint = 'Use logarithmic scale on the x-axis.'
          Align = alLeft
          Caption = 'Logarithmic scale'
          ParentShowHint = False
          ShowHint = True
          TabOrder = 4
          OnClick = CheckBox1Click
        end
        object CheckBox2: TCheckBoxEx
          Left = 6
          Top = 156
          Width = 91
          Height = 28
          Hint = 'Show units on the x-axis.'
          Align = alLeft
          Caption = 'Show numbers'
          ParentShowHint = False
          ShowHint = True
          TabOrder = 5
          OnClick = CheckBoxClick
        end
        object CheckBox17: TCheckBoxEx
          Left = 173
          Top = 156
          Width = 137
          Height = 28
          Hint = 'Show units on the selected axis as a fraction multiplied by pi.'
          Align = alLeft
          Caption = 'Show as a multiple of '#960
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Arial'
          Font.Style = []
          ParentFont = False
          TabOrder = 12
        end
        object GridPanelEx3: TGridPanelEx
          Left = 173
          Top = 0
          Width = 219
          Height = 32
          Anchors = []
          BevelOuter = bvNone
          ColumnCollection = <
            item
              SizeStyle = ssAuto
              Value = 50.000000000000000000
            end
            item
              Value = 100.000000000000000000
            end>
          ControlCollection = <
            item
              Column = 1
              Control = Edit5
              Row = 0
            end
            item
              Column = 0
              Control = CheckBox3
              Row = 0
            end>
          ParentColor = True
          RowCollection = <
            item
              Value = 100.000000000000000000
            end
            item
              SizeStyle = ssAuto
            end>
          TabOrder = 6
          DesignSize = (
            219
            32)
          object Edit5: TMyEdit
            Left = 50
            Top = 5
            Width = 169
            Height = 21
            Hint = 'Edit the label for the axis here.'
            Margins.Left = 0
            Margins.Top = 5
            Margins.Right = 0
            Margins.Bottom = 6
            Anchors = [akTop, akBottom]
            ParentShowHint = False
            ShowHint = True
            TabOrder = 1
          end
          object CheckBox3: TCheckBoxEx
            Left = 0
            Top = 0
            Width = 50
            Height = 32
            Hint = 
              'Show the label at the right side of the graphing area, above the' +
              ' x-axis.'
            Align = alLeft
            Caption = 'Label:'
            ParentShowHint = False
            ShowHint = True
            TabOrder = 0
            OnClick = CheckBoxClick
          end
        end
        object GridPanelEx4: TGridPanelEx
          Left = 173
          Top = 32
          Width = 219
          Height = 32
          Anchors = []
          BevelOuter = bvNone
          ColumnCollection = <
            item
              SizeStyle = ssAuto
              Value = 50.000000000000000000
            end
            item
              Value = 100.000000000000000000
            end>
          ControlCollection = <
            item
              Column = 1
              Control = Edit6
              Row = 0
            end
            item
              Column = 0
              Control = Label4
              Row = 0
            end>
          ParentColor = True
          RowCollection = <
            item
              Value = 100.000000000000000000
            end>
          TabOrder = 7
          DesignSize = (
            219
            32)
          object Edit6: TMyEdit
            Left = 102
            Top = 5
            Width = 117
            Height = 21
            Hint = 'The x-axis will intersect with the y-axis at this value.'
            Margins.Top = 5
            Margins.Bottom = 6
            Anchors = [akTop, akBottom]
            TabOrder = 0
          end
          object Label4: TLabel
            Left = 0
            Top = 0
            Width = 102
            Height = 32
            Align = alLeft
            Caption = 'The x-axis cross at y='
            FocusControl = Edit6
            Layout = tlCenter
            ExplicitHeight = 13
          end
        end
      end
    end
    object TabSheet2: TTabSheet
      Caption = 'y-axis'
      object GridPanel2: TGridPanelEx
        Left = 0
        Top = 0
        Width = 396
        Height = 185
        Align = alClient
        BevelEdges = []
        BevelOuter = bvNone
        ColumnCollection = <
          item
            SizeStyle = ssAuto
            Value = 25.000000000000000000
          end
          item
            Value = 100.000000000000000000
          end
          item
            SizeStyle = ssAuto
          end
          item
            SizeStyle = ssAuto
          end>
        ControlCollection = <
          item
            Column = 0
            Control = Label5
            Row = 0
          end
          item
            Column = 1
            Control = Edit7
            Row = 0
          end
          item
            Column = 0
            Control = Label6
            Row = 1
          end
          item
            Column = 1
            Control = Edit8
            Row = 1
          end
          item
            Column = 0
            Control = Label7
            Row = 2
          end
          item
            Column = 1
            Control = Edit9
            Row = 2
          end
          item
            Column = 0
            Control = Label9
            Row = 3
          end
          item
            Column = 1
            Control = Edit10
            Row = 3
          end
          item
            Column = 2
            Control = CheckBox11
            Row = 2
          end
          item
            Column = 3
            Control = CheckBox13
            Row = 2
          end
          item
            Column = 2
            Control = CheckBox12
            Row = 3
          end
          item
            Column = 3
            Control = CheckBox14
            Row = 3
          end
          item
            Column = 0
            ColumnSpan = 4
            Control = CheckBox8
            Row = 4
          end
          item
            Column = 0
            ColumnSpan = 2
            Control = CheckBox9
            Row = 5
          end
          item
            Column = 2
            ColumnSpan = 2
            Control = CheckBox18
            Row = 5
          end
          item
            Column = 2
            ColumnSpan = 2
            Control = GridPanelEx1
            Row = 1
          end
          item
            Column = 2
            ColumnSpan = 2
            Control = GridPanelEx2
            Row = 0
          end>
        Padding.Left = 3
        Padding.Right = 2
        RowCollection = <
          item
            SizeStyle = ssAbsolute
            Value = 32.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 32.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 32.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 32.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 28.000000000000000000
          end
          item
            SizeStyle = ssAbsolute
            Value = 28.000000000000000000
          end>
        TabOrder = 0
        DesignSize = (
          396
          185)
        object Label5: TLabel
          Left = 6
          Top = 0
          Width = 44
          Height = 32
          Align = alLeft
          Caption = 'Minimum:'
          FocusControl = Edit7
          Layout = tlCenter
          ExplicitHeight = 13
        end
        object Edit7: TMyEdit
          Left = 58
          Top = 5
          Width = 110
          Height = 21
          Hint = 'Minimum y-value.'
          Anchors = [akLeft, akTop, akRight, akBottom]
          ParentShowHint = False
          ShowHint = True
          TabOrder = 0
        end
        object Label6: TLabel
          Left = 6
          Top = 32
          Width = 47
          Height = 32
          Align = alLeft
          Caption = 'Maximum:'
          FocusControl = Edit8
          Layout = tlCenter
          ExplicitHeight = 13
        end
        object Edit8: TMyEdit
          Left = 58
          Top = 37
          Width = 110
          Height = 21
          Hint = 'Maximum y-value.'
          Anchors = [akLeft, akTop, akRight, akBottom]
          ParentShowHint = False
          ShowHint = True
          TabOrder = 1
        end
        object Label7: TLabel
          Left = 6
          Top = 64
          Width = 44
          Height = 32
          Align = alLeft
          Caption = 'Tick unit:'
          FocusControl = Edit9
          Layout = tlCenter
          ExplicitHeight = 13
        end
        object Edit9: TMyEdit
          Left = 58
          Top = 69
          Width = 110
          Height = 21
          Hint = 'Units between ticks on the y-axis.'
          Anchors = [akLeft, akTop, akRight, akBottom]
          ParentShowHint = False
          ShowHint = True
          TabOrder = 2
        end
        object Label9: TLabel
          Left = 6
          Top = 96
          Width = 42
          Height = 32
          Align = alLeft
          Caption = 'Grid unit:'
          FocusControl = Edit10
          Layout = tlCenter
          ExplicitHeight = 13
        end
        object Edit10: TMyEdit
          Left = 58
          Top = 101
          Width = 110
          Height = 21
          Hint = 'Units between the grid lines perpendicular to the y-axis.'
          Anchors = [akLeft, akTop, akRight, akBottom]
          TabOrder = 3
        end
        object CheckBox11: TCheckBoxEx
          Left = 173
          Top = 64
          Width = 63
          Height = 32
          Hint = 'Determine the tick unit automatically.'
          Align = alLeft
          Caption = 'Auto tick'
          TabOrder = 8
          OnClick = CheckBoxClick
        end
        object CheckBox13: TCheckBoxEx
          Left = 241
          Top = 64
          Width = 73
          Height = 32
          Hint = 'Show tick marks on the y-axis.'
          Align = alLeft
          Caption = 'Show ticks'
          TabOrder = 10
        end
        object CheckBox12: TCheckBoxEx
          Left = 173
          Top = 96
          Width = 63
          Height = 32
          Hint = 'Set the grid unit to the same as the tick unit.'
          Align = alLeft
          Caption = 'Auto grid'
          TabOrder = 9
          OnClick = CheckBoxClick
        end
        object CheckBox14: TCheckBoxEx
          Left = 241
          Top = 96
          Width = 68
          Height = 32
          Hint = 'Show horizontal grid lines.'
          Align = alLeft
          Caption = 'Show grid'
          ParentShowHint = False
          ShowHint = True
          TabOrder = 11
        end
        object CheckBox8: TCheckBoxEx
          Left = 6
          Top = 128
          Width = 103
          Height = 28
          Hint = 'Use logarithmic scale for the y-axis.'
          Align = alLeft
          Caption = 'Logarithmic scale'
          ParentShowHint = False
          ShowHint = True
          TabOrder = 4
          OnClick = CheckBox8Click
        end
        object CheckBox9: TCheckBoxEx
          Left = 6
          Top = 156
          Width = 91
          Height = 28
          Hint = 'Show units on the y-axis.'
          Align = alLeft
          Caption = 'Show numbers'
          ParentShowHint = False
          ShowHint = True
          TabOrder = 5
          OnClick = CheckBoxClick
        end
        object CheckBox18: TCheckBoxEx
          Left = 173
          Top = 156
          Width = 137
          Height = 28
          Hint = 'Show units on the selected axis as a fraction multiplied by pi.'
          Align = alLeft
          Caption = 'Show as a multiple of '#960
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Arial'
          Font.Style = []
          ParentFont = False
          TabOrder = 12
        end
        object GridPanelEx1: TGridPanelEx
          Left = 173
          Top = 32
          Width = 219
          Height = 32
          BevelOuter = bvNone
          ColumnCollection = <
            item
              SizeStyle = ssAuto
              Value = 100.000000000000000000
            end
            item
              Value = 100.000000000000000000
            end>
          ControlCollection = <
            item
              Column = 0
              Control = Label8
              Row = 0
            end
            item
              Column = 1
              Control = Edit12
              Row = 0
            end>
          ParentColor = True
          RowCollection = <
            item
              Value = 100.000000000000000000
            end>
          ShowCaption = False
          TabOrder = 7
          DesignSize = (
            219
            32)
          object Label8: TLabel
            Left = 0
            Top = 0
            Width = 102
            Height = 32
            Align = alLeft
            Caption = 'The y-axis cross at x='
            FocusControl = Edit12
            Layout = tlCenter
            ExplicitHeight = 13
          end
          object Edit12: TMyEdit
            Left = 102
            Top = 5
            Width = 117
            Height = 21
            Hint = 'The y-axis will intersect with the x-axis at this value.'
            Margins.Top = 5
            Margins.Bottom = 6
            Anchors = [akTop, akBottom]
            ParentShowHint = False
            ShowHint = True
            TabOrder = 0
          end
        end
        object GridPanelEx2: TGridPanelEx
          Left = 173
          Top = 0
          Width = 219
          Height = 32
          Anchors = []
          BevelOuter = bvNone
          ColumnCollection = <
            item
              SizeStyle = ssAuto
              Value = 100.000000000000000000
            end
            item
              Value = 100.000000000000000000
            end>
          ControlCollection = <
            item
              Column = 1
              Control = Edit11
              Row = 0
            end
            item
              Column = 0
              Control = CheckBox10
              Row = 0
            end>
          ParentColor = True
          RowCollection = <
            item
              Value = 100.000000000000000000
            end>
          ShowCaption = False
          TabOrder = 6
          DesignSize = (
            219
            32)
          object Edit11: TMyEdit
            Left = 50
            Top = 5
            Width = 169
            Height = 21
            Hint = 'Edit the label for the axis here.'
            Margins.Top = 5
            Margins.Bottom = 6
            Anchors = [akTop, akBottom]
            ParentShowHint = False
            ShowHint = True
            TabOrder = 1
          end
          object CheckBox10: TCheckBoxEx
            Left = 0
            Top = 0
            Width = 50
            Height = 32
            Hint = 
              'Show the label at the top of the graphing area, to the right of ' +
              'the y-axis.'
            Align = alClient
            Caption = 'Label:'
            TabOrder = 0
            OnClick = CheckBoxClick
          end
        end
      end
    end
    object TabSheet3: TTabSheet
      Caption = 'Settings'
      ImageIndex = 2
      DesignSize = (
        396
        185)
      object Label16: TLabel
        Left = 8
        Top = 12
        Width = 23
        Height = 13
        Caption = 'Title:'
        FocusControl = Edit13
      end
      object CheckBox15: TCheckBoxEx
        Left = 8
        Top = 40
        Width = 83
        Height = 17
        Hint = 'Select to show a legend in the coordinate system.'
        Caption = 'Show legend'
        TabOrder = 2
        OnClick = CheckBox15Click
      end
      object RadioGroup1: TRadioGroup
        Left = 152
        Top = 64
        Width = 116
        Height = 73
        Hint = 
          'Choose if you want to show the axes, and if you want to show the' +
          'm crossed or at the left and bottom of the image.'
        Anchors = [akTop, akRight]
        Caption = 'Axes style'
        ItemIndex = 1
        Items.Strings = (
          'None'
          'Crossed'
          'Boxed')
        TabOrder = 5
      end
      object RadioGroup2: TRadioGroup
        Left = 283
        Top = 64
        Width = 97
        Height = 57
        Hint = 
          'Choose if trigonometric functions should calculate in radians or' +
          ' degrees.'
        Anchors = [akTop, akRight]
        Caption = 'Trigonometry'
        ItemIndex = 0
        Items.Strings = (
          'Radian'
          'Degree')
        ParentShowHint = False
        ShowHint = True
        TabOrder = 6
      end
      object Edit13: TMyEdit
        Left = 40
        Top = 10
        Width = 332
        Height = 21
        Hint = 'Enter a title to show above the coordinate system.'
        Anchors = [akLeft, akTop, akRight]
        TabOrder = 0
      end
      object RadioGroup3: TRadioGroup
        Left = 8
        Top = 64
        Width = 129
        Height = 89
        Hint = 'Choose where the legend should be placed.'
        Caption = 'Legend placement'
        Items.Strings = (
          'Top Right'
          'Bottom Right'
          'Top Left'
          'Bottom Left')
        TabOrder = 4
      end
      object CheckBox19: TCheckBoxEx
        Left = 152
        Top = 40
        Width = 172
        Height = 17
        Hint = 
          'Check this to use complex numbers for calculations. Notice that ' +
          'this will slow graphing down.'
        Anchors = [akLeft, akTop, akRight]
        Caption = 'Calculate with complex numbers'
        TabOrder = 3
      end
      object Button7: TButton
        Left = 371
        Top = 9
        Width = 21
        Height = 21
        Hint = 'Select the font for the title.'
        ImageAlignment = iaCenter
        ImageIndex = 5
        Images = Form1.ScaledImageList3
        TabOrder = 1
        OnClick = Button7Click
      end
    end
    object TabSheet4: TTabSheet
      Caption = 'Font and color'
      ImageIndex = 3
      DesignSize = (
        396
        185)
      object GroupBox3: TGroupBox
        Left = 8
        Top = 8
        Width = 193
        Height = 113
        Caption = 'Colors'
        TabOrder = 0
        object Label17: TLabel
          Left = 8
          Top = 82
          Width = 48
          Height = 13
          Caption = 'Grid color:'
          FocusControl = ExtColorBox3
        end
        object Label18: TLabel
          Left = 8
          Top = 50
          Width = 52
          Height = 13
          Caption = 'Axes color:'
          FocusControl = ExtColorBox2
        end
        object Label19: TLabel
          Left = 8
          Top = 18
          Width = 87
          Height = 13
          Caption = 'Background color:'
          FocusControl = ExtColorBox1
        end
        object ExtColorBox1: TExtColorBox
          Left = 104
          Top = 16
          Width = 81
          Height = 21
          Hint = 'Select the color of the image background.'
          ItemHeight = 15
          TabOrder = 0
          AutoDroppedWidth = True
          Selected = clScrollBar
          DefaultName = 'Default'
          CustomName = 'Custom...'
        end
        object ExtColorBox2: TExtColorBox
          Left = 104
          Top = 48
          Width = 81
          Height = 21
          Hint = 'Select the color of the axes.'
          ItemHeight = 15
          TabOrder = 1
          AutoDroppedWidth = True
          Selected = clScrollBar
          DefaultName = 'Default'
          CustomName = 'Custom...'
        end
        object ExtColorBox3: TExtColorBox
          Left = 104
          Top = 80
          Width = 81
          Height = 21
          Hint = 'Select the color of the grid lines.'
          ItemHeight = 15
          TabOrder = 2
          AutoDroppedWidth = True
          Selected = clScrollBar
          DefaultName = 'Default'
          CustomName = 'Custom...'
        end
      end
      object GroupBox4: TGroupBox
        Left = 235
        Top = 8
        Width = 145
        Height = 113
        Anchors = [akTop, akRight]
        Caption = 'Fonts'
        TabOrder = 1
        object Label20: TLabel
          Left = 8
          Top = 50
          Width = 61
          Height = 13
          Caption = 'Number font:'
          FocusControl = Button5
        end
        object Label21: TLabel
          Left = 8
          Top = 18
          Width = 50
          Height = 13
          Caption = 'Label font:'
          FocusControl = Button4
        end
        object Label10: TLabel
          Left = 8
          Top = 80
          Width = 60
          Height = 13
          Caption = 'Legend font:'
          FocusControl = Button6
        end
        object Button4: TButton
          Left = 115
          Top = 14
          Width = 21
          Height = 21
          Hint = 'Select the font used to show the labels on the axes.'
          ImageAlignment = iaCenter
          ImageIndex = 5
          Images = Form1.ScaledImageList3
          TabOrder = 0
          OnClick = Button4Click
        end
        object Button5: TButton
          Left = 115
          Top = 46
          Width = 21
          Height = 21
          Hint = 'Select the font used to show the numbers on the axes.'
          ImageAlignment = iaCenter
          ImageIndex = 5
          Images = Form1.ScaledImageList3
          TabOrder = 1
          OnClick = Button5Click
        end
        object Button6: TButton
          Left = 115
          Top = 78
          Width = 21
          Height = 21
          Hint = 'Select the font used to show the legend text.'
          ImageAlignment = iaCenter
          ImageIndex = 5
          Images = Form1.ScaledImageList3
          TabOrder = 2
          OnClick = Button6Click
        end
      end
    end
  end
  object Button2: TButton
    Left = 251
    Top = 235
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 3
  end
  object Button3: TButton
    Left = 339
    Top = 235
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Caption = 'Help'
    TabOrder = 4
    OnClick = Button3Click
  end
  object CheckBox16: TCheckBoxEx
    Left = 8
    Top = 239
    Width = 95
    Height = 17
    Hint = 'Select this to use these settings as defaults in the future.'
    Anchors = [akLeft, akBottom]
    Caption = 'Save as default'
    TabOrder = 1
  end
  object Button1: TButton
    Left = 163
    Top = 235
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Caption = 'OK'
    Default = True
    TabOrder = 2
    OnClick = Button1Click
  end
  object FontDialog1: TFontDialog
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    Options = [fdEffects, fdForceFontExist]
    Left = 368
    Top = 192
  end
end
