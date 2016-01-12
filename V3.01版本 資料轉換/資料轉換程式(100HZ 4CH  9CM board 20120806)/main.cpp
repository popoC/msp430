////   2013 修改處
//     Read_OBS_SD_Sector_Time()      -- 時間單位 回傳修正
//    Get_SD_Sector_Data_DeCompress() -- 引入時間單位 所有位子後移1byte
//
//
//


#include <vcl.h>
#pragma hdrstop

#include "main.h"
#include "math.h"
#include "libmseed.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

#define Pack_N 1800
#define Yb_Ch  4

TForm1 *Form1;
HANDLE hDevice;
DWORD dwBlockRead ;


typedef struct Obs_Sd_Data_s{
    int32_t data;    //--資料
    hptime_t time;   //--資料時間
    int data_no;
}Obs_Sd_Data;
//---------------------------------------------------------------------------
bool SD_Card_file_load = false;           // 讀取SD檔案之旗標,以防讀取錯誤

//--------function-----------------------------------------------------------

hptime_t Read_OBS_SD_Sector_Time(unsigned __int64 SectorNum);
unsigned __int64 Find_TimeSN_Compress(String Tstr);
int  Line_Interpolation( hptime_t find_t,unsigned __int64 *s1,Obs_Sd_Data **D_Data,int32_t *New_Data);
unsigned __int64 Out_Miniseed(String filename,unsigned __int64 search_sn,hptime_t start_t,hptime_t end_t);
int Get_SD_Sector_Data_DeCompress(Obs_Sd_Data **D_Data,unsigned __int64 SectorNum);
//----2010 / 10 01 修改為轉壓縮版本使用
static FILE *outfile[4]       = {0,0,0,0};


//static void record_handler (char *record, int reclen, void *ptr);
//--------variable------------------------

int StarToT,EndToT;       //
hptime_t Start_sn_hptime,End_sn_hptime;
double SubTime = 0;       //每個Sector所需回填時間

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}

/***************************************************************************
 * record_handler:
 * Saves passed records to the output file.
 ***************************************************************************/
static void
record_handler (char *record, int reclen, void *ptr)
{
  if ( fwrite(record, reclen, 1,(FILE *)ptr) != 1 )
    {
      ms_log (2, "Cannot write to output file\n");
    }
}

//---------------------------------------------------------------------------
bool SetFilePointerEx_32G(unsigned int Sector_num,HANDLE hD){
  LARGE_INTEGER  offset ;
  unsigned __int64 BigNumber;
  BigNumber = (unsigned __int64)Sector_num*512;
    offset.LowPart  = (BigNumber & 0x00000000FFFFFFFF );
    offset.HighPart = ((BigNumber & 0xFFFFFFFF00000000)>>32);
      if(!SetFilePointerEx(hD,offset ,0 ,FILE_BEGIN)){
       CloseHandle(hD);
       return 0;
      }
       else{
       return 1;
      }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Button1Click(TObject *Sender)
{
 if(OpenDialog1->Execute()){

   unsigned char DataList[512];           // Format Table
   unsigned char Temporary[512]={0};

   unsigned int TotalDataList = 0;

     SD_Card_file_load = true;

	hDevice = CreateFile(OpenDialog1->FileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if( hDevice != INVALID_HANDLE_VALUE )
	{
          if(!SetFilePointerEx_32G(0,hDevice)){ShowMessage("無法開啟");return;}
       	  ReadFile(hDevice, DataList, 512,&dwBlockRead, 0);
	}
        else {
	  ShowMessage("Device Handle Error!!");	  CloseHandle(hDevice);		return;
	}
        
	String str;
        TotalDataList = (DataList[0]<<8)| DataList[1];
	str = "Data: " + IntToStr(TotalDataList);
        Form1->Caption = str;
      	ListBox1->Clear();

	unsigned int StartSector = 0, EndSector = 0;

	for( unsigned int i=1; i<TotalDataList+1; i++ )
	{
             StartSector = (DataList[16*i+0]<<24)|(DataList[16*i+1]<<16)|(DataList[16*i+2]<<8)|(DataList[16*i+3]);
             EndSector   = (DataList[16*i+4]<<24)|(DataList[16*i+5]<<16)|(DataList[16*i+6]<<8)|(DataList[16*i+7]);

          str = "Data" + IntToStr(i) + " : Sector ." + IntToStr(StartSector) + ". - ." + IntToStr(EndSector) + ".   Date: ";

          if(!SetFilePointerEx_32G(StartSector,hDevice)){ShowMessage("無法開啟");return;}
	  ReadFile(hDevice, Temporary, 512, &dwBlockRead, 0);
                           		// 月日時分秒
 	  str += IntToStr(Temporary[1]) + "/" + IntToStr(Temporary[2]) + " " + IntToStr(Temporary[3]) + ":" + IntToStr(Temporary[4]) + " -> " ;

          if(!SetFilePointerEx_32G(EndSector,hDevice)){ShowMessage("無法開啟");return;}
          ReadFile(hDevice, Temporary, 512, &dwBlockRead, 0);
 		                        // 月日時分秒
 	  str += IntToStr(Temporary[1]) + "/" + IntToStr(Temporary[2]) + " " + IntToStr(Temporary[3]) + ":" + IntToStr(Temporary[4]) + " ." ;

          ListBox1->Items->Add(str);
	}
 }
}
//-----------------------------------------------------------------------------
int ReCompress(char diff_index,unsigned char *databuf,long int* ans,int addresscount){
   char bit[6]={32,16,8,4,2,1};

      for(int i=0;i<6;i++){
        if((diff_index & bit[i])==bit[i]){

          (addresscount%2) ?  *ans |=  (databuf[addresscount/2]&0x0f):
                              *ans |=  (databuf[addresscount/2]&0xf0)>>4;
           addresscount++;
        }
        if(i!=5)*ans<<=4;
      }
      return(addresscount);
}
//-----------------------------------------------------------------------------
hptime_t Read_OBS_SD_Sector_Time(unsigned __int64 SectorNum){
   //    回傳值為該Sector所記錄之時間(格式為hptime_t)

  if( hDevice == INVALID_HANDLE_VALUE ){ ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);		return(-1);
            }
    unsigned char lpBuffer[512];
    SectorNum = SectorNum*512;   //每個BLCOK單位為512byte
    LARGE_INTEGER  lPpoint={0};      //讀取超過2G以上檔案需要

    lPpoint.LowPart  = (SectorNum & 0x00000000FFFFFFFF);
    lPpoint.HighPart =((SectorNum & 0xFFFFFFFF00000000)>>32);
    if(!SetFilePointerEx(hDevice,lPpoint ,0 ,FILE_BEGIN))return(0);
    ReadFile(hDevice, lpBuffer,512, &dwBlockRead, 0); //讀取1個Sector

    hptime_t Data_time = 0;
    String Time;
//    Time = String(2000+lpBuffer[0])+"/"+ String(lpBuffer[1])+"/"+ String(lpBuffer[2])
//                +" "+String(lpBuffer[3])+":"+String(lpBuffer[4])+":"+
//                String( lpBuffer[5] +double(lpBuffer[6])/125) ;
    Time = String(2000+lpBuffer[0])+"/"+ String(lpBuffer[1])+"/"+ String(lpBuffer[2])
                +" "+String(lpBuffer[3])+":"+String(lpBuffer[4])+":"+
                String( lpBuffer[5] + double(lpBuffer[6])/125+ double(lpBuffer[7])/10000) ;

   // Form1->Memo1->Lines->Add(Time);            
    Data_time = ms_timestr2hptime(Time.c_str());


    //--此處加入時間補償 _2011/04/20 [V3.2]
    Data_time = Data_time +
    (((double)(Data_time-Start_sn_hptime)/(double)(End_sn_hptime - Start_sn_hptime))*SubTime);

    return(Data_time);         
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

void __fastcall TForm1::ListBox1DblClick(TObject *Sender)
{
 if(SD_Card_file_load){

     int starA , endA ;                 //找出Sector的起始終點
      starA =StrToInt(_StringSegment(ListBox1->Items->Strings[ListBox1->ItemIndex],".",2));
      endA =StrToInt(_StringSegment(ListBox1->Items->Strings[ListBox1->ItemIndex],".",4));
      StarToT = starA;
      EndToT  = endA;                           


        Edit5->Text = StarToT;
        Edit6->Text = EndToT;

     Memo1->Lines->Add(EndToT-StarToT);

	    if( hDevice == INVALID_HANDLE_VALUE ){ ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);		return;
            }



    unsigned __int64 Sn=0 ;
    unsigned char bf[512];
    LARGE_INTEGER  lPpoint;      //讀取超過2G以上檔案需要
    String Time;


    Sn = (unsigned __int64)StarToT*512;   //每個BLCOK單位為512byte
    lPpoint.LowPart  = (Sn & 0x00000000FFFFFFFF);    lPpoint.HighPart =((Sn & 0xFFFFFFFF00000000)>>32);
    if(!SetFilePointerEx(hDevice,lPpoint ,0 ,FILE_BEGIN))ShowMessage("ListBox1DblClick error");
    ReadFile(hDevice, bf,512, &dwBlockRead, 0); //讀取1個Sector

    Time = String(2000+bf[0])+"/"+ String(bf[1])+"/"+ String(bf[2])+" "+String(bf[3])+":"
               +String(bf[4])+":"+ String( bf[5] +double(bf[6])/125) ;
      Memo2->Lines->Add(Time);
    Start_sn_hptime = ms_timestr2hptime(Time.c_str());
    Sn = 0;
    Sn = (unsigned __int64)EndToT*512;   //每個BLCOK單位為512byte
    lPpoint.HighPart=0;lPpoint.LowPart=0;
    lPpoint.LowPart  = (Sn & 0x00000000FFFFFFFF);    lPpoint.HighPart =((Sn & 0xFFFFFFFF00000000)>>32);
    if(!SetFilePointerEx(hDevice,lPpoint ,0 ,FILE_BEGIN))ShowMessage("ListBox1DblClick error");
    ReadFile(hDevice, bf,512, &dwBlockRead, 0); //讀取1個Sector

    Time = String(2000+bf[0])+"/"+ String(bf[1])+"/"+ String(bf[2])+" "+String(bf[3])+":"
               +String(bf[4])+":"+ String( bf[5] +double(bf[6])/125) ;
     Memo2->Lines->Add(Time);
    End_sn_hptime = ms_timestr2hptime(Time.c_str());

     SubTime = Edit4->Text.ToDouble()*(-100);
     Memo1->Lines->Add(Start_sn_hptime);
     Memo1->Lines->Add(End_sn_hptime);

 }
}
//---------------------------------------------------------------------------
//拆解字串段的自寫函式 _StringSegment()
//String A="ABCD,EFG,H,IJK,LM";    //String B=_StringSegment(A , "," , 3); // 以逗號來做分隔 , 求第 3 段字串
//所以 B 就等於 "H"    //也可不用逗號做分隔 , 用您指定的其他符號做分隔
String __fastcall TForm1::_StringSegment(AnsiString Str , AnsiString Comma , int Seg)
{
  if ((Str=="") || (Seg<1)) return "";
  String C=Comma; if (C=="") C=",";
  String s=Str;           String sTmp, r;
  int iPosComma;
  TStringList *TempList = new TStringList; // declare the list
  TempList->Clear();
    while (s.Pos(C)>0){
       iPosComma = s.Pos(C); // locate commas
       sTmp = s.SubString(1,iPosComma - 1); // copy item to tmp string
       TempList->Add(sTmp); // add to list
       s = s.SubString(iPosComma + 1,s.Length()); // delete item from string
    }
            // trap for trailing filename
    if (s.Length()!=0) TempList->Add(s);
    if (Seg > TempList->Count)r="";
    else r= TempList->Strings[Seg-1];
    delete TempList; // destroy the list object
    return r;
}
//--------------------------------------
void __fastcall TForm1::Button6Click(TObject *Sender)
{
   CloseHandle(hDevice);
   Close();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------------
void __fastcall TForm1::Button7Click(TObject *Sender)
{
  StarToT = Edit5->Text.ToInt();
  EndToT  = Edit6->Text.ToInt();

    unsigned __int64 Sn=0 ;
    unsigned char bf[512];
    LARGE_INTEGER  lPpoint={0};      //讀取超過2G以上檔案需要
    String Time;

    Sn = (unsigned __int64)StarToT*512;   //每個BLCOK單位為512byte
    lPpoint.LowPart  = (Sn & 0x00000000FFFFFFFF);    lPpoint.HighPart =((Sn & 0xFFFFFFFF00000000)>>32);
    if(!SetFilePointerEx(hDevice,lPpoint ,0 ,FILE_BEGIN))ShowMessage("ListBox1DblClick error");
    ReadFile(hDevice, bf,512, &dwBlockRead, 0); //讀取1個Sector

    Time = String(2000+bf[0])+"/"+ String(bf[1])+"/"+ String(bf[2])+" "+String(bf[3])+":"
               +String(bf[4])+":"+ String( bf[5] +double(bf[6])/125) ;
      Memo2->Lines->Add(Time);
    Start_sn_hptime = ms_timestr2hptime(Time.c_str());
    Sn = 0;
    Sn = (unsigned __int64)EndToT*512;   //每個BLCOK單位為512byte
    lPpoint.HighPart=0;lPpoint.LowPart=0;
    lPpoint.LowPart  = (Sn & 0x00000000FFFFFFFF);    lPpoint.HighPart =((Sn & 0xFFFFFFFF00000000)>>32);
    if(!SetFilePointerEx(hDevice,lPpoint ,0 ,FILE_BEGIN))ShowMessage("ListBox1DblClick error");
    ReadFile(hDevice, bf,512, &dwBlockRead, 0); //讀取1個Sector

    Time = String(2000+bf[0])+"/"+ String(bf[1])+"/"+ String(bf[2])+" "+String(bf[3])+":"
               +String(bf[4])+":"+ String( bf[5] +double(bf[6])/125) ;
     Memo2->Lines->Add(Time);
    End_sn_hptime = ms_timestr2hptime(Time.c_str());


     SubTime = Edit4->Text.ToDouble()*(-100);
     Memo1->Lines->Add(Start_sn_hptime);
     Memo1->Lines->Add(End_sn_hptime);        
}
//---------------------------------------------------------------------------


int Get_SD_Sector_Data_DeCompress(Obs_Sd_Data **D_Data,unsigned __int64 SectorNum){
// -- 2013 0528 修改   資料排序 往後推1Byte    // 2013 0528 -- 修改時間單位為小數以下
      //回傳值為Sector內第幾CH的資料數目     //此處寫法原理請參考OBS_SD卡資料格式~

  if( hDevice == INVALID_HANDLE_VALUE ){ ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);		return(-1);
            }
   unsigned char SdBuff[512];               //每個BLCOK單位為512byte
    LARGE_INTEGER  lPpoint={0};             //讀取超過2G以上檔案需要

    lPpoint.LowPart  = ((SectorNum*512) & 0x00000000FFFFFFFF);  lPpoint.HighPart =(((SectorNum*512) & 0xFFFFFFFF00000000)>>32);
    if(!SetFilePointerEx(hDevice,lPpoint ,0 ,FILE_BEGIN))return(0);
    ReadFile(hDevice, SdBuff,512, &dwBlockRead, 0); //讀取1個Sector


   int DataCount = 0;
   long int  DataBuf[4] = {0};

  int data_in_buff = SdBuff[7+1];    //--- 此處修正 因為MSP那端已寫好 SdBuff[7]內的值 為資料數量

  DataBuf[0] = SdBuff[9+1]<<16  | SdBuff[10+1]<<8 | SdBuff[11+1];
  DataBuf[1] = SdBuff[13+1]<<16 | SdBuff[14+1]<<8 | SdBuff[15+1];
  DataBuf[2] = SdBuff[17+1]<<16 | SdBuff[18+1]<<8 | SdBuff[19+1];
  DataBuf[3] = SdBuff[21+1]<<16 | SdBuff[22+1]<<8 | SdBuff[23+1];



  D_Data[DataCount][0].data =  DataBuf[0];
  D_Data[DataCount][1].data =  DataBuf[1];
  D_Data[DataCount][2].data =  DataBuf[2];
  D_Data[DataCount][3].data =  DataBuf[3];

  D_Data[0][0].data_no = data_in_buff;

          //   int indexcount = 48;             //此為固定值 壓縮資料起始位子 7+1+4*4 = 24 = 48/2
             int indexcount = 50;               // 8+1+4*4 =25 = 50/2 // 2013 0528  fix
             char diff_index =0;

             int ch_no;


         long int check_diff =0;


hptime_t t1, t2, t3;    // -----2012 0806 
double timediff;


       t1 =    Read_OBS_SD_Sector_Time(SectorNum);
       t2 =    Read_OBS_SD_Sector_Time(SectorNum+1);


    if((t2-t1)>1000000){

        t2 -= 1000000;   //  此為硬體設計上缺失  秒數可能會多一秒存在Sector中    fix bug
        t3 =    Read_OBS_SD_Sector_Time(SectorNum+2);
         Form1->Memo1->Lines->Add("Sector Time error 1");
         Form1->Memo1->Lines->Add(SectorNum);
        if(t3<t2){
          Form1->Memo1->Lines->Add("Sector Time error 1.1");
          Form1->Memo1->Lines->Add(SectorNum);
          return(-1);
        }
    }
    if(t1 > t2){
       t1 -= 1000000;
          Form1->Memo1->Lines->Add("Sector Time error 2");
          Form1->Memo1->Lines->Add(SectorNum);
       if(t1 > t2){
          Form1->Memo1->Lines->Add("Sector Time error 2.2");
          Form1->Memo1->Lines->Add(SectorNum);
          return(-1);
       }
    }

   timediff = double(t2 - t1)/double(data_in_buff);

     D_Data[DataCount][0].time = t1;
     D_Data[DataCount][1].time = t1;
     D_Data[DataCount][2].time = t1;
     D_Data[DataCount][3].time = t1;

     DataCount++;

         for(int i=0;i<(data_in_buff-1)*4;i++){
           check_diff = 0;         ch_no = 0;

                   if(indexcount%2){
                     diff_index = ((SdBuff[indexcount/2] & 0x03)<<4 )|(SdBuff[indexcount/2+1] & 0xf0)>>4 ;
                     ch_no =  (SdBuff[indexcount/2] & 0x0c);
                   }
                   else{
                     diff_index =  SdBuff[indexcount/2] & 0x3f ;
                     ch_no =  (SdBuff[indexcount/2] & 0xc0)>>4 ;
                   }

                   indexcount+=2;
                   indexcount = ReCompress(diff_index,SdBuff, &check_diff,indexcount);

                   if(ch_no==4){
                      DataBuf[0] = check_diff^DataBuf[0];
                      D_Data[DataCount][0].data = DataBuf[0];
                      D_Data[DataCount][0].time =  t1 + timediff*(double)DataCount;
                   }
                   if(ch_no==8){
                      DataBuf[1] = check_diff^DataBuf[1];
                      D_Data[DataCount][1].data = DataBuf[1];
                      D_Data[DataCount][1].time =  t1 + timediff*(double)DataCount;
                   }
                   if(ch_no==12){
                      DataBuf[2] = check_diff^DataBuf[2];
                      D_Data[DataCount][2].data = DataBuf[2];
                      D_Data[DataCount][2].time =  t1 + timediff*(double)DataCount;
                     // DataCount++;
                   }
                   if(ch_no==0){
                      DataBuf[3] = check_diff^DataBuf[3];
                      D_Data[DataCount][3].data = DataBuf[3];
                      D_Data[DataCount][3].time =  t1 + timediff*(double)DataCount;
                      DataCount++;
                   }


         }

         
    return(data_in_buff);
 //               return(DataCount);

}


int  Line_Interpolation( hptime_t find_t,unsigned __int64 *s1,Obs_Sd_Data **D_Data,int32_t *New_Data){

    // int32_t New_Data;

     hptime_t T1 = 0,T2 = 0;
     double Scale = 0; 

     if(D_Data[0][0].data_no == 0)Get_SD_Sector_Data_DeCompress(D_Data,*s1);


     for(int i=0;i<D_Data[0][0].data_no-2;i++){


       if(( find_t >= D_Data[i][0].time ) &&  (find_t < D_Data[i+1][0].time )   )
       {
        T1 = D_Data[i][0].time;
        T2 = D_Data[i+1][0].time;

        Scale = (double)(find_t - T1) / (double)(T2-T1);


        New_Data[0] = (double) D_Data[i][0].data +((double)( D_Data[i+1][0].data - D_Data[i][0].data ) *Scale);
        New_Data[1] = (double) D_Data[i][1].data +((double)( D_Data[i+1][1].data - D_Data[i][1].data ) *Scale);
        New_Data[2] = (double) D_Data[i][2].data +((double)( D_Data[i+1][2].data - D_Data[i][2].data ) *Scale);
        New_Data[3] = (double) D_Data[i][3].data +((double)( D_Data[i+1][3].data - D_Data[i][3].data ) *Scale);

        return(1);
       }

     }

     int32_t D1,D2,D3,D4;


      if(find_t > D_Data[D_Data[0][0].data_no-1][0].time)   // 找到檔案最後一個了
      {


         T1 =  D_Data[D_Data[0][0].data_no-1][0].time;
         D1 =  D_Data[D_Data[0][0].data_no-1][0].data;
         D2 =  D_Data[D_Data[0][0].data_no-1][1].data;
         D3 =  D_Data[D_Data[0][0].data_no-1][2].data;
         D4 =  D_Data[D_Data[0][0].data_no-1][3].data;


         *s1 += 1;
         Get_SD_Sector_Data_DeCompress(D_Data,*s1);
         T2 =  D_Data[0][0].time;

         if(T2==T1){

          ShowMessage("!!!");
         }
         Scale = (double)(find_t - T1) / (double)(T2-T1);

        New_Data[0] = (double) D1 +((double)( D_Data[0][0].data - D1 ) *Scale);
        New_Data[1] = (double) D2 +((double)( D_Data[0][1].data - D2 ) *Scale);
        New_Data[2] = (double) D3 +((double)( D_Data[0][2].data - D3 ) *Scale);
        New_Data[3] = (double) D4 +((double)( D_Data[0][3].data - D4 ) *Scale);

        return(1);


      }

      for(int i=0;i<4;i++){   // 正常情況不會往後讀取 只有一開始會 為了將起始點內差為整數
                              // 往前面的SECTOR尋找資料 最多只找3個
          if(find_t < D_Data[0][0].time){
            T2 =  D_Data[0][0].time;D1 =  D_Data[0][0].data;
            D2 =  D_Data[0][1].data;D3 =  D_Data[0][2].data;
            D4 =  D_Data[0][3].data;
            *s1 -= 1;
            Get_SD_Sector_Data_DeCompress(D_Data,*s1);
          }
          else{
             for(int i=0;i<D_Data[0][0].data_no-1;i++){
                   if(( find_t > D_Data[i][0].time ) &&  (find_t < D_Data[i+1][0].time )   )
                      {
                         T1 = D_Data[i][0].time;
                         T2 = D_Data[i+1][0].time;

                         Scale = (double)(find_t - T1) / (double)(T2-T1);
                           New_Data[0] = (double) D_Data[i][0].data +((double)( D_Data[i+1][0].data - D_Data[i][0].data ) *Scale);
                           New_Data[1] = (double) D_Data[i][1].data +((double)( D_Data[i+1][1].data - D_Data[i][1].data ) *Scale);
                           New_Data[2] = (double) D_Data[i][2].data +((double)( D_Data[i+1][2].data - D_Data[i][2].data ) *Scale);
                           New_Data[3] = (double) D_Data[i][3].data +((double)( D_Data[i+1][3].data - D_Data[i][3].data ) *Scale);

                             return(1);
                      }

             }
         
             T1 =  D_Data[D_Data[0][0].data_no-1][0].time;
             D1 =  D_Data[D_Data[0][0].data_no-1][0].data;
             D2 =  D_Data[D_Data[0][0].data_no-1][1].data;
             D3 =  D_Data[D_Data[0][0].data_no-1][2].data;
             D4 =  D_Data[D_Data[0][0].data_no-1][3].data;

                  Scale = (double)(find_t - T1) / (double)(T2-T1);

                   New_Data[0] = (double) D_Data[D_Data[0][0].data_no-1][0].data +((double)( D1-  D_Data[D_Data[0][0].data_no-1][0].data ) *Scale);
                   New_Data[1] = (double) D_Data[D_Data[0][0].data_no-1][1].data +((double)( D2 - D_Data[D_Data[0][0].data_no-1][1].data ) *Scale);
                   New_Data[2] = (double) D_Data[D_Data[0][0].data_no-1][2].data +((double)( D3 - D_Data[D_Data[0][0].data_no-1][2].data ) *Scale);
                   New_Data[3] = (double) D_Data[D_Data[0][0].data_no-1][3].data +((double)( D4 - D_Data[D_Data[0][0].data_no-1][3].data ) *Scale);

               return(1);

          }


      }

    return(-1);

 }

//-----------------------------------------------------------------------------
unsigned __int64 Find_TimeSN_Compress(String Tstr){

  if( hDevice == INVALID_HANDLE_VALUE ){ ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);		return(-1);
            }

        //  找出對應時間的Sector address 如果沒發現則回傳0
  hptime_t t1,t2,t3;
  hptime_t t2_b;// = 1000000; 1S
  int      T1_Sn,T2_Sn,T3_Sn;

  hptime_t T_err ;// = 1000000; 1S

  T1_Sn =  StarToT;
  T3_Sn =  EndToT;

  unsigned __int64 Search_Sector;

    t1 = Read_OBS_SD_Sector_Time(T1_Sn);
    t2_b =  ms_timestr2hptime(Tstr.c_str());
    t3 = Read_OBS_SD_Sector_Time(T3_Sn);


  if( (t2_b > t3) || (t2_b < t1) )       { ShowMessage("不在範圍內");  return(0); }



  int find_count = 0;

  // Search_Sector = ceil(((double(t2-t1)) / (double(t3-t1)) )* (double(T3_Sn - T1_Sn))) +T1_Sn;

  /*
    while(find_count < 10000){
            find_count++;

      T2_Sn =  ceil(((double(t2_b-t1)) / (double(t3-t1)) )* (double(T3_Sn - T1_Sn))) +T1_Sn;

        t2 = Read_OBS_SD_Sector_Time(T2_Sn);

     if(t2 > t2_b){

       T3_Sn = T2_Sn ;
       t3 = Read_OBS_SD_Sector_Time(T3_Sn);
     }
     else{
       T1_Sn = T2_Sn;
       t1 = Read_OBS_SD_Sector_Time(T1_Sn);
     }

      if(abs(T1_Sn-T3_Sn)<=1)
      {  Form1->Memo1->Lines->Add(find_count);
        return((unsigned __int64)T2_Sn);
      }


   }  */
  while(find_count < 10000){

     find_count++;
     T2_Sn = (T1_Sn+T3_Sn )/2;

     if(abs(T1_Sn-T3_Sn)==1)
     { // Form1->Memo1->Lines->Add(find_count);
        return((unsigned __int64)T2_Sn);
     }


     t1 = Read_OBS_SD_Sector_Time(T1_Sn);
     t2 = Read_OBS_SD_Sector_Time(T2_Sn);
     t3 = Read_OBS_SD_Sector_Time(T3_Sn);

     if(t2==t2_b)return((unsigned __int64)T2_Sn);

     if( (t2_b > t1) && (t2_b < t2) ){
       T3_Sn = T2_Sn;
     }
     if( (t2_b > t2) && (t2_b < t3) ){
       T1_Sn = T2_Sn;
     }
  }

  return(0);
}
//-----------------------------------------------------------------------------
unsigned __int64 Out_Miniseed(String filename,unsigned __int64 search_sn,hptime_t start_t,hptime_t end_t)
{             // filename 輸出檔案名稱
              // start_t 輸出檔案起始時間  _sn 代表第幾個Sector, Sector SIZE IS 512Bytes
Obs_Sd_Data **Raw_Data = new Obs_Sd_Data*[512];
   for(int i=0;i<512;i++) Raw_Data[i] = new Obs_Sd_Data[4];

     double SMrate =    Form1->Edit7->Text.ToDouble();//  default is 120;
     double SMrate_diff =    (1/SMrate * 1000000);

      hptime_t Now_t = start_t;


   int32_t RawData[4][Pack_N];
   int32_t New_Data;
   int packedsamples;

            Form1->ProgressBar1->Max = ( (end_t - start_t)/1000000);
            Form1->ProgressBar1->Min = 0;
            Form1->ProgressBar1->Position = 0;
            
         outfile[0] = fopen(StringReplace(filename,"**","HHZ",TReplaceFlags()).c_str(),"wb");
         outfile[1] = fopen(StringReplace(filename,"**","HH1",TReplaceFlags()).c_str(),"wb");
         outfile[2] = fopen(StringReplace(filename,"**","HH2",TReplaceFlags()).c_str(),"wb");
         outfile[3] = fopen(StringReplace(filename,"**","HH3",TReplaceFlags()).c_str(),"wb");

              MSRecord *msr[4] = {0,0,0,0};
              String ch ="1";
            for(int i=0;i<Yb_Ch;i++){
              msr[i] =  msr_init(NULL);
              msr[i]->sequence_number = 0;               msr[i]->reclen = 4096;
              msr[i]->encoding = 10;              //--steim1
              msr[i]->numsamples = Pack_N;        //--每XXX寫入1次
              msr[i]->sampletype = 'i';           //-  a ascii, i int ,f float ,d double
              msr[i]->dataquality = 'D';                 msr[i]->byteorder = 0;
              msr[i]->starttime = Now_t;
              msr[i]->samprate = SMrate;
           if(i==0) {  ch = Form1->Edit14->Text+"Z"; }
           else{
            ch = Form1->Edit14->Text+IntToStr(i);
           }

               strcpy (msr[i]->channel, ch.c_str());
               strcpy (msr[i]->station, Form1->Edit13->Text.c_str() );
               strcpy (msr[i]->network, Form1->Edit12->Text.c_str() );
            }


            if(Get_SD_Sector_Data_DeCompress(Raw_Data,search_sn)==0){
               ShowMessage("no data");
            }

     int32_t Find_Data[4];
     int pk_N = 0;
     double Data_count = 1;
    while( Now_t < end_t){

            if(  Line_Interpolation(Now_t,&search_sn,Raw_Data,Find_Data) == -1){
                ShowMessage("samplerate error ");
                return(-1);
            }

            Now_t = start_t +  (SMrate_diff * Data_count);
                                     Data_count++;
           for(int i=0;i<Yb_Ch;i++)RawData[i][pk_N] = Find_Data[i];

           pk_N++;
         if(pk_N >=Pack_N)
            {
               pk_N = 0;
               for(int i=0;i<Yb_Ch;i++){

                 msr[i]->datasamples = RawData[i];
                 msr_pack(msr[i], &record_handler, outfile[i], &packedsamples, 1, 0);
                 msr[i]->starttime = Now_t;       //-*-------------------*--------

               }
               Form1->ProgressBar1->Position =  (Now_t - start_t)/1000000;
               Application->ProcessMessages();
            }
    }

     if(pk_N != 0){
               for(int i=0;i<Yb_Ch;i++){
                  msr[i]->numsamples = pk_N;        //--每XXX寫入1次
                 msr[i]->datasamples = RawData[i];
                 msr_pack(msr[i], &record_handler, outfile[i], &packedsamples, 1, 0);
                 msr[i]->starttime = Now_t;       //-*-------------------*--------
               }
                pk_N = 0;
     }
                    if ( outfile[3] )fclose (outfile[3]);
                    msr_free(&msr[3]);
                    if ( outfile[2] )fclose (outfile[2]);
                    msr_free(&msr[2]);
                    if ( outfile[1] )fclose (outfile[1]);
                    msr_free(&msr[1]);
                    if ( outfile[0] )fclose (outfile[0]);
                    msr_free(&msr[0]);



      for(int i=0;i<512;i++)delete [] Raw_Data[i];
          delete [] Raw_Data;     
                    
return(search_sn);
}
//-------------------------------------------------------------------------------
void __fastcall TForm1::Button14Click(TObject *Sender)
{
// 取得目前 CPU frequency
LARGE_INTEGER m_liPerfFreq={0};	QueryPerformanceFrequency(&m_liPerfFreq);
// 取得執行前時間
LARGE_INTEGER m_liPerfStart={0}; QueryPerformanceCounter(&m_liPerfStart);


unsigned __int64 s1,s2;
  s1 =  Find_TimeSN_Compress( Edit1->Text);
  s2 =  Find_TimeSN_Compress( Edit2->Text);

  hptime_t   start_t = Read_OBS_SD_Sector_Time(s1);

 // hptime_t   end_t = Read_OBS_SD_Sector_Time(s2);


  hptime_t   end_t =  ms_timestr2hptime(Edit2->Text.c_str());

  hptime_t New_find_t = ms_timestr2hptime(Edit1->Text.c_str());     // hptime      2012_0501 /


  String _filename = "";
  _filename = OpenDialog1->FileName+".**.mseed";

   //   Out_Miniseed("IIa.**.mseed",s1,New_find_t,end_t);
    Out_Miniseed(_filename,s1,New_find_t,end_t);



// 取得執行後的時間
LARGE_INTEGER liPerfNow={0};	QueryPerformanceCounter(&liPerfNow);
// 計算出 Total 所需要的時間 (millisecond)
long decodeDulation=( ((liPerfNow.QuadPart - m_liPerfStart.QuadPart) * 1000)/m_liPerfFreq.QuadPart);
// print
TCHAR buffer[100];	wsprintf(buffer,"執行時間 %d millisecond ",decodeDulation);
MessageBox(NULL,buffer,"計算時間",MB_OK);
//      ShowMessage("end");
}
//---------------------------------------------------------------------------





void __fastcall TForm1::Button9Click(TObject *Sender)
{
//--------------------------------------------------------------
// 檔名格式範例:   " YB01.TW..2010.237.00.00.00.mseed"
// YB01      : 序號
// TW        : 站名
// HH1       : 軸
// 2010.237.00.00.00 : 資料起始時間
//--------------------------------------------------------------
String SeN,Station,strT_string;                               //   檔名設定
String FileName = "";
hptime_t    Add_time,Add_overlay_time;
hptime_t    A_t=0,B_t=0;
unsigned __int64 A_n;

         char strT[30];
         char str_buf[30];
int filecount = 0;


        

  SeN = "TB";     Station = "Y";

    if (!InputQuery("輸入序號", "EX: YB01", SeN))
           {  ShowMessage("有問題嗎?");   return;  }

    if (!InputQuery("輸入佈放站位", "EX: TW", Station))
           {  ShowMessage("有問題嗎?");   return;  }



 A_t =  ms_timestr2hptime(Edit8->Text.c_str());
 B_t =  ms_timestr2hptime(Edit9->Text.c_str());
        if(Find_TimeSN_Compress(Edit9->Text.c_str())==0)return;


   Add_time = (hptime_t)(Edit10->Text.ToDouble()*60*1000000);
   Add_overlay_time = (hptime_t)(Edit11->Text.ToDouble()*60*1000000);


 filecount =  (B_t - A_t+1000000) / Add_time;


         A_t =  ms_timestr2hptime(Edit8->Text.c_str());
         B_t = A_t+Add_time+Add_overlay_time;

         for(int i=0;i<filecount;i++){

         strT_string = "";

           if( ms_hptime2mdtimestr (A_t, str_buf, 1) == NULL )Memo1->Lines->Add("Cannot convert trace start time for 1\n");
           A_n = Find_TimeSN_Compress( str_buf);

           if( ms_hptime2seedtimestr (A_t,strT, 1) == NULL )Memo1->Lines->Add("Cannot convert trace start time for 2\n");
            strT_string =  strT; // 拆解字串用  兩者用途不一


           FileName =  (SeN + "." +  Station+".."
                                   +"**."+strT_string.SubString(0,4) +"."+strT_string.SubString(6,3)
                                   + "." +strT_string.SubString(10,2)+"."+strT_string.SubString(13,2)+"."
                                   +strT_string.SubString(16,2)+".mseed");

           Out_Miniseed(FileName,A_n,A_t,B_t);

           A_t = A_t + Add_time;
           B_t = A_t+Add_time+Add_overlay_time;
           Label12->Caption = IntToStr(filecount-i);
         }

        Memo2->Lines->Add("end");        
}
//---------------------------------------------------------------------------






