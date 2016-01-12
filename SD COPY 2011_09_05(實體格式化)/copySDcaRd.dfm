object Form1: TForm1
  Left = 882
  Top = 276
  Width = 523
  Height = 210
  BorderIcons = [biSystemMenu, biMinimize]
  Caption = 'SD'#21345#25335#35997#31243#24335' V3.01_'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Button1: TButton
    Left = 8
    Top = 40
    Width = 75
    Height = 33
    Caption = 'view sd card'
    TabOrder = 0
    OnClick = Button1Click
  end
  object ProgressBar1: TProgressBar
    Left = 96
    Top = 8
    Width = 377
    Height = 16
    Min = 0
    Max = 100
    TabOrder = 1
  end
  object ComboBox1: TComboBox
    Left = 8
    Top = 8
    Width = 81
    Height = 21
    ItemHeight = 13
    TabOrder = 2
    Text = 'ComboBox1'
  end
  object Button2: TButton
    Left = 8
    Top = 80
    Width = 75
    Height = 33
    Caption = 'start copy'
    TabOrder = 3
    OnClick = Button2Click
  end
  object ListBox1: TListBox
    Left = 96
    Top = 32
    Width = 385
    Height = 121
    ItemHeight = 13
    TabOrder = 4
  end
  object Button4: TButton
    Left = 8
    Top = 120
    Width = 81
    Height = 33
    Caption = 'SD'#21345' '#26684#24335#21270' '
    TabOrder = 5
    OnClick = Button4Click
  end
  object SaveDialog1: TSaveDialog
    Left = 104
    Top = 112
  end
end
