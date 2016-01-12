//---------------------------------------------------------------------------

#ifndef mainH
#define mainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Dialogs.hpp>
#include <ComCtrls.hpp>
#include <Chart.hpp>
#include <ExtCtrls.hpp>
#include <TeEngine.hpp>
#include <TeeProcs.hpp>
#include <Series.hpp>
#include <Graphics.hpp>
#include <jpeg.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
        TButton *Button1;
        TListBox *ListBox1;
        TOpenDialog *OpenDialog1;
        TMemo *Memo1;
        TMemo *Memo2;
        TGroupBox *GroupBox1;
        TLabel *Label1;
        TLabel *Label2;
        TEdit *Edit5;
        TEdit *Edit6;
        TEdit *Edit4;
        TLabel *Label3;
        TProgressBar *ProgressBar1;
        TEdit *Edit7;
        TButton *Button6;
        TButton *Button7;
        TGroupBox *GroupBox2;
        TLabel *Label4;
        TLabel *Label5;
        TEdit *Edit8;
        TEdit *Edit9;
        TLabel *Label6;
        TEdit *Edit10;
        TLabel *Label7;
        TEdit *Edit11;
        TLabel *Label8;
        TButton *Button9;
        TEdit *Edit12;
        TEdit *Edit13;
        TEdit *Edit14;
        TLabel *Label9;
        TLabel *Label10;
        TLabel *Label11;
        TLabel *Label12;
        TGroupBox *GroupBox3;
        TEdit *Edit1;
        TEdit *Edit2;
        TButton *Button14;
        TLabel *Label13;
        TLabel *Label14;
        TImage *Image1;
        void __fastcall Button1Click(TObject *Sender);
        void __fastcall ListBox1DblClick(TObject *Sender);
        String __fastcall _StringSegment(AnsiString Str , AnsiString Comma , int Seg);
        void __fastcall Button6Click(TObject *Sender);
        void __fastcall Button7Click(TObject *Sender);
        void __fastcall Button14Click(TObject *Sender);
        void __fastcall Button9Click(TObject *Sender);

private:	// User declarations
public:		// User declarations
        __fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
