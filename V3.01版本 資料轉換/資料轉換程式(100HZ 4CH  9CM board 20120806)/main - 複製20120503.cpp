#include <vcl.h>
#pragma hdrstop

#include "main.h"
#include "math.h"
#include "libmseed.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

#define Pack_N 1800


TForm1 *Form1;
HANDLE hDevice;
DWORD dwBlockRead ;

typedef struct Obs_Sd_Data_s{
    int32_t data;    //--資料
    hptime_t time;   //--資料時間
}Obs_Sd_Data;
//---------------------------------------------------------------------------
bool SD_Card_file_load = false;           // 讀取SD檔案之旗標,以防讀取錯誤

//--------function-----------------------------------------------------------
hptime_t Line_Interpolation_Compress(HANDLE hD,unsigned __int64 *sector_n,Obs_Sd_Data *Sd_Data,hptime_t find_t,int32_t *RawData,int ch,int *DataInSector);

int Line_Interpolation_Find_X_Compress(Obs_Sd_Data *Sd_Data,hptime_t find_t);

int Sd_Data_InputTime_Compress(HANDLE hD,Obs_Sd_Data *sd,unsigned __int64 s1,int ch);
int Find_SD_Sector_Data_Compress(unsigned char *SdBuff,int32_t *Raw,int ch);
hptime_t Read_OBS_SD_Sector_Time(IN HANDLE hFile,unsigned char *lpBuffer,unsigned __int64 SectorNum);

unsigned __int64 FindTimeSector_Compress(HANDLE hD, String Tstr);


int Out_Miniseed(String filename,hptime_t m_start,hptime_t m_end);



//----2010 / 10 01 修改為轉壓縮版本使用
static FILE *outfile       = 0;
static void record_handler (char *record, int reclen, void *ptr);
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
  if ( fwrite(record, reclen, 1, outfile) != 1 )
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
//---------------------------------------------------------------------------





int Sd_Data_InputTime_Compress(HANDLE hD,Obs_Sd_Data *sd,unsigned __int64 s1,int ch){
            //---每次讀取三個sector的時間間距  s1, s1+1, s1+2
            //---設置兩資料暫存區

            //---  資料就算壓縮  一個sector內最多不會超過164筆資料 因為~ 512 - 7   - 1    - 3*4 -1   = 491, 491/3 = 163.33
            //---                   因為個人不喜歡164 所以+1 = 165
            //---------------------------------------------------------- all -time -count -raw  -cks = 491
unsigned char buf1[512];
unsigned char buf2[512];
unsigned char buf3[512];

hptime_t t1, t2, t3 ;
//int32_t RawData[42];
  int32_t RawData[165];


double timediff;
double errPoint;
       t1 =    Read_OBS_SD_Sector_Time(hD,buf1,s1+0);
       t2 =    Read_OBS_SD_Sector_Time(hD,buf2,s1+1);
       t3 =    Read_OBS_SD_Sector_Time(hD,buf3,s1+2);

                                 //--每個sector(512Byte)間格時間3Ch/120Hz 不可能超過 0.6S = (hptime)600000~
                                 //--意謂如果超過1S代表該sector時間紀錄上有問題(硬體BUG)
                                 //--目前猜測BUG來自電子干擾-------------------
                                 //-------- TB31.70 有問題 需要檢查看看----

     if(  (abs(t2-t1)>600000) || (abs(t3-t2)>600000) ){
             // Form1->Memo2->Lines->Add(s1);
     }

     if(((t2-t1)<1000000) && ((t3-t2)>1000000)){

          Form1->Memo2->Lines->Add(String(s1)+"    "+IntToStr(t3-t2));
         t3 = t2 + (t2-t1);
     }
     if(((t2-t1)>1000000) && ((t3-t2)<0)      ){
         t2 = (t1+t3)/2;
     }
     if(((t2-t1)<0      ) && ((t3-t2)<1000000)){
         t1 = t2 + t2-t3;
     }



 int data_no1,data_no2;

 data_no1  = Find_SD_Sector_Data_Compress(buf1, RawData  ,ch);


// Form1->Memo2->Lines->Add(data_no1);

 timediff = double(t2 - t1)/double(data_no1);


 for(int i=0;i<data_no1;i++){
        sd[i].time = t1 + timediff*(double)i;
        sd[i].data = RawData[i];
 }

 data_no2 = Find_SD_Sector_Data_Compress(buf2, RawData ,ch);

// Form1->Memo2->Lines->Add(data_no2);

 timediff = double(t3 - t2)/double(data_no2);

  for(int i=0;i<(data_no2);i++){
        sd[i+data_no1].time = t2 + timediff*(double)(i);
        sd[i+data_no1].data = RawData[i];
 }





    return(data_no1);

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
int Find_SD_Sector_Data_Compress(unsigned char *SdBuff,int32_t *Raw,int ch){
      //回傳值為Sector內第幾CH的資料數目
      //此處寫法原理請參考OBS_SD卡資料格式~ 如資料格式為壓縮版~ 這裡需要大修= ="

        int DataCount = 0;
   long int  DataBuf[3] = {0};


  //int data_in_buff = SdBuff[7]-1;   //--此處修正一下 因為MSP那端沒寫好 所以改這裡，先不理會SdBuff[7]內的值
                                      //--- 此處修正 因為MSP那端已寫好 SdBuff[7]內的值 為資料數量


  int data_in_buff = SdBuff[7];

  DataBuf[0] = SdBuff[9]<<16  | SdBuff[10]<<8 | SdBuff[11];
  DataBuf[1] = SdBuff[13]<<16 | SdBuff[14]<<8 | SdBuff[15];
  DataBuf[2] = SdBuff[17]<<16 | SdBuff[18]<<8 | SdBuff[19];



   if(ch==1){ Raw[DataCount] = DataBuf[0]; DataCount++;}
   if(ch==2){ Raw[DataCount] = DataBuf[1]; DataCount++;}
   if(ch==3){ Raw[DataCount] = DataBuf[2]; DataCount++;}

             int indexcount = 40;             //此為固定值 壓縮資料起始位子 7+1+3*4 = 20 = 40/2
             char diff_index =0;

             int ch_no;

         long int data =0;

//              for(int i=0;i<(data_in_buff-1)*3;i++){
              for(int i=0;i<512;i++){
                     data = 0;                     ch_no = 0;

                   if(indexcount%2){
                     diff_index = ((SdBuff[indexcount/2] & 0x03)<<4 )|(SdBuff[indexcount/2+1] & 0xf0)>>4 ;
                     ch_no =  (SdBuff[indexcount/2] & 0x0c);
                   }
                   else{
                     diff_index =  SdBuff[indexcount/2] & 0x3f ;
                     ch_no =  (SdBuff[indexcount/2] & 0xc0)>>4 ;
                   }

                  if(ch_no==0)break;

                    indexcount+=2;

                    indexcount = ReCompress(diff_index,SdBuff, &data,indexcount);
                   

                    if((ch_no==4) && (ch==1)){
                      DataBuf[0] = data^DataBuf[0];
                      Raw[DataCount] = DataBuf[0];
                      DataCount++;

                      if(DataBuf[0]>1089016250){
                        int gg=0;

                        gg = gg+1;
                        Form1->Memo1->Lines->Add("gg");

                      }
                    }
                    if((ch_no==8) && (ch==2)){
                      DataBuf[1] = data^DataBuf[1];
                      Raw[DataCount] = DataBuf[1];
                      DataCount++;
                    }
                    if((ch_no==12)&& (ch==3)){
                      DataBuf[2] = data^DataBuf[2];
                      Raw[DataCount] = DataBuf[2];
                      DataCount++;
                    }

               }


//    return(data_in_buff);
                return(DataCount);

}

//-----------------------------------------------------------------------------
hptime_t Read_OBS_SD_Sector_Time(IN HANDLE hFile,unsigned char *lpBuffer,unsigned __int64 SectorNum){
   //    回傳值為該Sector所記錄之時間(格式為hptime_t)

    SectorNum = SectorNum*512;   //每個BLCOK單位為512byte
    LARGE_INTEGER  lPpoint={0};      //讀取超過2G以上檔案需要

    lPpoint.LowPart  = (SectorNum & 0x00000000FFFFFFFF);
    lPpoint.HighPart =((SectorNum & 0xFFFFFFFF00000000)>>32);
    if(!SetFilePointerEx(hFile,lPpoint ,0 ,FILE_BEGIN))return(0);
    ReadFile(hFile, lpBuffer,512, &dwBlockRead, 0); //讀取1個Sector

    hptime_t Data_time = 0;
    String Time;
    Time = String(2000+lpBuffer[0])+"/"+ String(lpBuffer[1])+"/"+ String(lpBuffer[2])
                +" "+String(lpBuffer[3])+":"+String(lpBuffer[4])+":"+
                String( lpBuffer[5] +double(lpBuffer[6])/125) ;

    Data_time = ms_timestr2hptime(Time.c_str());


    //--此處加入時間補償 _2011/04/20 [V3.2]
    Data_time = Data_time +
    (((double)(Data_time-Start_sn_hptime)/(double)(End_sn_hptime - Start_sn_hptime))*SubTime);

    return(Data_time);         
}
//----------------------------------------------------------------------------
unsigned __int64 FindTimeSector_Compress(HANDLE hD, String Tstr){

      //  找出對應時間的Sector address 如果沒發現則回傳0
  hptime_t t1,t2,t3;
  hptime_t t2_b;// = 1000000; 1S
  int      T1_Sn,T2_Sn,T3_Sn;

  hptime_t T_err ;// = 1000000; 1S

  T1_Sn =  StarToT;
  T3_Sn =  EndToT;

  unsigned char Buf[512];
  unsigned __int64 Search_Sector;
  if( hD == INVALID_HANDLE_VALUE ){ ShowMessage("Device Handle Error!!");return(0);}

    t1 = Read_OBS_SD_Sector_Time(hD,Buf,T1_Sn);
    t2 =  ms_timestr2hptime(Tstr.c_str());
    t3 = Read_OBS_SD_Sector_Time(hD,Buf,T3_Sn);
  //  t3 = t3 + ((double)(T3_Sn - T1_Sn))*SubTime;     //--此處加入時間補償


//     Form1->Memo1->Lines->Add(t3-t1);

  if( (t2 > t3) || (t2 < t1) )       { ShowMessage("不在範圍內");  return(0); }
  else{


   Search_Sector = ceil(((double(t2-t1)) / (double(t3-t1)) )* (double(T3_Sn - T1_Sn))) +T1_Sn;

   t2_b = Read_OBS_SD_Sector_Time(hD,Buf,Search_Sector);

   T_err = abs( t2 -  t2_b);


         int errcounter = 0;

       while(T_err > 500000)
       {
             errcounter++;
             if(errcounter>1000){
                Form1->Memo1->Lines->Add("err over 1000");
                break;
             }
             
          if(t2 > t2_b){

            t1 = t2_b;            T1_Sn = Search_Sector;
            t3 = t3;              T3_Sn = T3_Sn;
            Search_Sector = ceil(((double(t2-t1)) / (double(t3-t1)) )* (double(T3_Sn - T1_Sn))) +T1_Sn;
            t2_b = Read_OBS_SD_Sector_Time(hD,Buf,Search_Sector);
            T_err = abs( t2 -  t2_b);
          }
          else{
            t1 = t1;              T1_Sn = T1_Sn;
            t3 = t2_b;              T3_Sn = Search_Sector;
            Search_Sector = ceil(((double(t2-t1)) / (double(t3-t1)) )* (double(T3_Sn - T1_Sn))) +T1_Sn;
            t2_b = Read_OBS_SD_Sector_Time(hD,Buf,Search_Sector);
            T_err = abs( t2 -  t2_b);
          }
       }

       if(t2_b < t2){return(Search_Sector+1);}
       else{  return(Search_Sector);}
  }




}
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

     /*

double TimeDiff = 0;   //總偏移時間

 TimeDiff = 0;   //總偏移時間
 SubTime = 0;    //每個Sector所需回填時間
             TimeDiff = Edit4->Text.ToDouble();
             SubTime =  TimeDiff /( double(EndToT-StarToT)) *(-100.0);
             Memo1->Lines->Add(SubTime);
       */

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

void __fastcall TForm1::Button5Click(TObject *Sender)
{
   Obs_Sd_Data Sd_Data[165*2];                   //-- SD內sector資料 每次讀取兩個Sector
	if( hDevice == INVALID_HANDLE_VALUE ){ ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);		return;
	}
int32_t RawData[Pack_N];
        unsigned char buff[512];
        double SMrate =    Edit7->Text.ToDouble();//  120;
        double SMrate_diff =    (1/SMrate * 1000000);
      

        hptime_t New_find_t,end_t,start_t;
        int32_t New_Data;
    //    int packedrecords;
        int packedsamples;

        String ch;

   unsigned __int64 s1,s2,s_buff,cut_time;
           s1 = FindTimeSector_Compress( hDevice,Edit1->Text);
           s2 = FindTimeSector_Compress( hDevice,Edit2->Text);

           end_t = Read_OBS_SD_Sector_Time(hDevice,buff,s2);

           Sd_Data_InputTime_Compress(hDevice,&(*Sd_Data),s1,1);   //--輸出CH1測試
           New_find_t = Sd_Data[0].time;

           s_buff = s1;
           start_t = New_find_t;

           cut_time = end_t-start_t+1000000;
        int cut_hours = double(cut_time) / ( 3600000000);

        int cut_mins = double(cut_time-(double)cut_hours*3600000000) / ( 60000000);

     String SN,Station,strT_string;
     SN = "TB";
     Station = "Y";
     char strT[30];
    if( ms_hptime2seedtimestr (start_t, strT, 1) == NULL )
       Memo1->Lines->Add("Cannot convert trace start time for %s\n");
    strT_string = strT;

    if (!InputQuery("輸入佈放站位", "EX: Y2", Station))
           {
              ShowMessage("有問題嗎?");   return;
           }
    if (!InputQuery("輸入序號", "EX: TB31", SN))
           {
              ShowMessage("有問題嗎?");   return;
           }


   for(int ch_n = 1;ch_n<=3;ch_n++){

            New_find_t =  start_t;
            s1 = s_buff;
            Form1->ProgressBar1->Max = ( (end_t - New_find_t)/1000000);
            Form1->ProgressBar1->Min = 0;
            Form1->ProgressBar1->Position = 0;

            ch = "";

      switch(ch_n){
             case 1:
                outfile = fopen((Station + "_" + SN +".Z."+String(cut_hours)+"."+String(cut_mins)
                                   + "." +strT_string.SubString(0,4) +"."+strT_string.SubString(6,3)
                                   + "." +strT_string.SubString(10,2)+"."+strT_string.SubString(13,2)+"."
                                   +strT_string.SubString(16,2)+".mseed").c_str(),"wb");
                 ch = "Z";
             break;
             case 2:
                outfile = fopen((Station + "_" + SN +".N."+String(cut_hours)+"."+String(cut_mins)
                                   + "." +strT_string.SubString(0,4) +"."+strT_string.SubString(6,3)
                                   + "." +strT_string.SubString(10,2)+"."+strT_string.SubString(13,2)+"."
                                   +strT_string.SubString(16,2)+".mseed").c_str(),"wb");

                 ch = "1";
             break;
             case 3:
                outfile = fopen((Station + "_" + SN +".E."+String(cut_hours)+"."+String(cut_mins)
                                   + "." +strT_string.SubString(0,4) +"."+strT_string.SubString(6,3)
                                   + "." +strT_string.SubString(10,2)+"."+strT_string.SubString(13,2)+"."
                                   +strT_string.SubString(16,2)+".mseed").c_str(),"wb");
                 ch = "2";
             break;
      }
              
              MSRecord *msr = 0;
              msr =  msr_init(NULL);
              msr->sequence_number = 0;
              msr->reclen = 4096;
              msr->encoding = 10;              //--steim1
              msr->numsamples = Pack_N;        //--每XXX寫入1次
              msr->sampletype = 'i';           //-  a ascii, i int ,f float ,d double
              msr->dataquality = 'D';
              msr->byteorder = 0;

              msr->starttime = New_find_t;
              msr->samprate = SMrate;


               ch = Form1->Edit14->Text+ch;

               strcpy (msr->channel, ch.c_str());
               strcpy (msr->station, Form1->Edit13->Text.c_str() );
               strcpy (msr->network, Form1->Edit12->Text.c_str() );



          int Data_NO;

          Data_NO = Sd_Data_InputTime_Compress(hDevice,&(*Sd_Data),s1,ch_n);   //--輸出CH1測試


          int pk_N = 0;
          double Data_count = 1;
    while( New_find_t <= end_t){

            if(Line_Interpolation_Compress(hDevice,&s1,Sd_Data,New_find_t,&New_Data,ch_n,&Data_NO)==-1){
                ShowMessage("samplerate error 1");
                return;
            }

            New_find_t = start_t +  (SMrate_diff * Data_count);
                                     Data_count++;



            RawData[pk_N] = New_Data;
           pk_N++;
         if(pk_N >=Pack_N)
            {
               pk_N = 0;
               msr->datasamples = RawData;
           //    packedrecords =
           msr_pack (msr, &record_handler, NULL, &packedsamples, ch_n, 0);


               msr->starttime = New_find_t;       //-*-------------------*--------

               Memo2->Lines->Add(New_find_t);

               Form1->ProgressBar1->Position =  (New_find_t - start_t)/1000000;
               Application->ProcessMessages();

            }

    }
                    if ( outfile )
                    fclose (outfile);
                 //   msr_free(&msr);
  }
                ShowMessage("end");
}
//---------------------------------------------------------------------------

//--------------------------------------
hptime_t Line_Interpolation_Compress(HANDLE hD,unsigned __int64 *sector_n,
Obs_Sd_Data *Sd_Data,hptime_t find_t,int32_t *RawData,int ch,int *DataInSector){
        double newdata;
        int find_count = -1;

     find_count =  Line_Interpolation_Find_X_Compress(Sd_Data,find_t);
     if(find_count == -1){ShowMessage("time rate error");return(-1);}

          newdata = (double)Sd_Data[find_count].data + (
                   (double)(Sd_Data[find_count+1].data - Sd_Data[find_count].data)/
                   (double)(Sd_Data[find_count+1].time - Sd_Data[find_count].time)*
                   (double)(find_t - Sd_Data[find_count].time)
                    );

            *RawData = (int32_t)newdata;


     if(find_count>(*DataInSector)*2){
       Form1->Memo1->Lines->Add(*sector_n);
       ShowMessage("error 102");
       return(-1);

     }

     if(find_count>=(*DataInSector+20))               //--- 20 代表資料暫存的OVERLAP
     {
        *sector_n +=1;
        *DataInSector = Sd_Data_InputTime_Compress(hD,&(*Sd_Data), *sector_n, ch);   //--往下繼續讀取新的sector
     }

     if(find_t == -1){
      ShowMessage("error 101");
     }
           return(find_t );
}
//------------------------------------------------------------------------------
int Line_Interpolation_Find_X_Compress(Obs_Sd_Data *Sd_Data,hptime_t find_t){
         int find_count = -1;

           for(int i=0;i<(165*2)-1;i++){
                 if(  (find_t >= Sd_Data[i].time) && ( find_t <= Sd_Data[i+1].time) )
                  {
                        find_count = i;
                        return(i);
                  }
           }

          if(find_count>150){
           ShowMessage("error 105");
          }


           return(find_count);
}
//--------------------------------------
void __fastcall TForm1::Button6Click(TObject *Sender)
{
   CloseHandle(hDevice);
   Close();
}
//---------------------------------------------------------------------------
int Out_Miniseed(String filename,unsigned __int64 m_start_sn,unsigned __int64 m_end_sn){

	if( hDevice == INVALID_HANDLE_VALUE ){ ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);		return(-1);
	}

   Obs_Sd_Data Sd_Data[165*2];                   //-- SD內sector資料 每次讀取兩個Sector
   int32_t RawData[Pack_N];

        unsigned char buff[512];
        double SMrate =    Form1->Edit7->Text.ToDouble();//  120;
        double SMrate_diff =    (1/SMrate * 1000000);

        hptime_t New_find_t,end_t,start_t;
        int32_t New_Data;

        int packedsamples;

        String ch;

   unsigned __int64 s_buff,cut_time;


           end_t   = Read_OBS_SD_Sector_Time(hDevice,buff,m_end_sn);
           start_t = Read_OBS_SD_Sector_Time(hDevice,buff,m_start_sn);
//           Sd_Data_InputTime_Compress(hDevice,&(*Sd_Data),m_start_sn,1);   //--找出第一筆資料時間~
//           New_find_t = Sd_Data[0].time;

           s_buff = m_start_sn;       //-- SN

   for(int ch_n = 1;ch_n<=3;ch_n++){

            New_find_t =  start_t;       // hptime
            s_buff     = m_start_sn;     // SN  //            s1 = s_buff;

            Form1->ProgressBar1->Max = ( (end_t - New_find_t)/1000000);
            Form1->ProgressBar1->Min = 0;
            Form1->ProgressBar1->Position = 0;

            ch = "";

      switch(ch_n){
             case 1:
                 outfile = fopen((filename.SubString(0,8)+"Z"+filename.SubString(10,filename.Length())).c_str(),"wb");
                 ch = "1";
             break;
             case 2:
                 outfile = fopen((filename.SubString(0,8)+"N"+filename.SubString(10,filename.Length())).c_str(),"wb");
                 ch = "2";
             break;
             case 3:
                 outfile = fopen((filename.SubString(0,8)+"E"+filename.SubString(10,filename.Length())).c_str(),"wb");
                 ch = "3";
             break;
      }
              
              MSRecord *msr = 0;
              msr =  msr_init(NULL);
              msr->sequence_number = 0;               msr->reclen = 4096;
              msr->encoding = 10;              //--steim1
              msr->numsamples = Pack_N;        //--每XXX寫入1次
              msr->sampletype = 'i';           //-  a ascii, i int ,f float ,d double
              msr->dataquality = 'D';                 msr->byteorder = 0;

              msr->starttime = New_find_t;
              msr->samprate = SMrate;

               ch = Form1->Edit14->Text+ch;

               strcpy (msr->channel, ch.c_str());
               strcpy (msr->station, Form1->Edit13->Text.c_str() );
               strcpy (msr->network, Form1->Edit12->Text.c_str() );


          int Data_NO;
          Data_NO = Sd_Data_InputTime_Compress(hDevice,&(*Sd_Data),s_buff,ch_n);   //--輸出CH1測試   找出SECTOR內有多少筆資料


          int pk_N = 0;
          double Data_count = 1;
    while( New_find_t <= end_t){

            if(Line_Interpolation_Compress(hDevice,&s_buff,Sd_Data,New_find_t,&New_Data,ch_n,&Data_NO)==-1){
                ShowMessage("samplerate error ");
                return(-1);
            }

            New_find_t = start_t +  (SMrate_diff * Data_count);
                                     Data_count++;

            RawData[pk_N] = New_Data;
           pk_N++;
         if(pk_N >=Pack_N)
            {
               pk_N = 0;
               msr->datasamples = RawData;

               msr_pack(msr, &record_handler, NULL, &packedsamples, ch_n, 0);

               msr->starttime = New_find_t;       //-*-------------------*--------

               Form1->ProgressBar1->Position =  (New_find_t - start_t)/1000000;
               Application->ProcessMessages();
            }
    }
                    if ( outfile )
                    fclose (outfile);
                    msr_free(&msr);
  }
return(1);
}
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


void __fastcall TForm1::Button9Click(TObject *Sender)
{
//--------------------------------------------------------------
// 檔名格式範例:   " Y3_TB70.E.24.0.2010.237.00.00.00.mseed"
// Y3      : 站名
// TB70 : yardbird序號
// E        : 代表Z,N,E其中一軸
// 24      : 資料長度  24小時
// 0        : 資料長度     0分
// 2010.237.00.00.00 : 資料起始時間
//--------------------------------------------------------------

String SeN,Station,strT_string,endT_string;                               //   檔名設定
String FileName = "";
hptime_t New_find_t,end_t,start_t,Add_time,Add_overlay_time;
unsigned __int64 Start_n,End_n;//,cut_time;
unsigned char buff[512];
         char strT[30];
         char str_buf[30];

unsigned __int64 A_n,B_n;
hptime_t         A_t=0,B_t=0;


int filecount = 0;         

  SeN = "TB";     Station = "Y";

    if (!InputQuery("輸入佈放站位", "EX: Y2", Station))
           {
              ShowMessage("有問題嗎?");   return;
           }
    if (!InputQuery("輸入序號", "EX: TB31", SeN))
           {
              ShowMessage("有問題嗎?");   return;
           }

 Start_n = FindTimeSector_Compress( hDevice,Edit8->Text);    //    找出對應時間的sector序號
 End_n = FindTimeSector_Compress( hDevice,Edit9->Text);      //    Memo1->Lines->Add(Out_Miniseed("t67_311.mseed",Sn,En));


    start_t = Read_OBS_SD_Sector_Time(hDevice,buff,Start_n);      //    須轉換為整數型態方便運算
    end_t   = Read_OBS_SD_Sector_Time(hDevice,buff,End_n);        //    回傳值為該Sector所記錄之時間(格式為hptime_t)



   Add_time = (hptime_t)(Edit10->Text.ToDouble()*60*1000000);
   Add_overlay_time = (hptime_t)(Edit11->Text.ToDouble()*60*1000000);


 filecount =  (end_t - start_t+1000000) / Add_time;

 //cut_time = end_t-start_t+1000000+Add_overlay_time;



        int cut_hours = (Edit10->Text.ToInt()/60);
        int cut_mins = (Edit10->Text.ToInt()%60);


         A_t = start_t;
         B_t = A_t+Add_time+Add_overlay_time;




         for(int i=0;i<filecount;i++){


         strT_string = "";


           if( ms_hptime2mdtimestr (A_t, str_buf, 1) == NULL )Memo1->Lines->Add("Cannot convert trace start time for 1\n");
           A_n = FindTimeSector_Compress( hDevice,str_buf);

           if( ms_hptime2seedtimestr (A_t,strT, 1) == NULL )Memo1->Lines->Add("Cannot convert trace start time for 2\n");
            strT_string =  strT; // 拆解字串用  兩者用途不一

           if( ms_hptime2mdtimestr (B_t, str_buf, 1) == NULL )Memo1->Lines->Add("Cannot convert trace start time for 3\n");
           B_n = FindTimeSector_Compress( hDevice,str_buf);


           FileName =  (Station + "_" + SeN +".*."+String(cut_hours)+"."+String(cut_mins)
                                   + "." +strT_string.SubString(0,4) +"."+strT_string.SubString(6,3)
                                   + "." +strT_string.SubString(10,2)+"."+strT_string.SubString(13,2)+"."
                                   +strT_string.SubString(16,2)+".mseed");

           
        //   Memo1->Lines->Add(FileName);
           Out_Miniseed(FileName,A_n,B_n);


           A_t = A_t + Add_time;
           B_t = A_t+Add_time+Add_overlay_time;


         }

        Memo2->Lines->Add("end");

                                   
}
//---------------------------------------------------------------------------
int Out_Miniseed_v2(String filename,unsigned __int64 m_start_sn,unsigned __int64 m_end_sn)
{             // filename 輸出檔案名稱
              // m_start_sn 輸出檔案起始時間  _sn 代表第幾個Sector, Sector SIZE IS 512Bytes

	if( hDevice == INVALID_HANDLE_VALUE ){ ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);		return(-1);
	}

   Obs_Sd_Data Sd_Data[165*2];                   //-- SD內sector資料 每次讀取兩個Sector
   int32_t RawData[Pack_N];

        unsigned char buff[512];
        double SMrate =    Form1->Edit7->Text.ToDouble();//  default is 120;
        double SMrate_diff =    (1/SMrate * 1000000);

        hptime_t New_find_t,end_t,start_t;
        int32_t New_Data;

        int packedsamples;

        String ch;

   unsigned __int64 s_buff,cut_time;


           end_t   = Read_OBS_SD_Sector_Time(hDevice,buff,m_end_sn);
           start_t = Read_OBS_SD_Sector_Time(hDevice,buff,m_start_sn);

           s_buff = m_start_sn;       //-- SN

   for(int ch_n = 1;ch_n<=3;ch_n++){

    //        New_find_t =  (start_t / 1000000 )*1000000;       // hptime      2012_0501 /
              New_find_t =  (start_t );       // hptime      2012_0501 /

            s_buff     = m_start_sn;     // SN  //            s1 = s_buff;

            Form1->ProgressBar1->Max = ( (end_t - New_find_t)/1000000);
            Form1->ProgressBar1->Min = 0;
            Form1->ProgressBar1->Position = 0;

            ch = "";

      switch(ch_n){
             case 1:
                 outfile = fopen(StringReplace(filename,"**","HHZ",TReplaceFlags()).c_str(),"wb");
                 ch = "1";
             break;
             case 2:
                 outfile = fopen(StringReplace(filename,"**","HH1",TReplaceFlags()).c_str(),"wb");
                 ch = "2";
             break;
             case 3:
                 outfile = fopen(StringReplace(filename,"**","HH2",TReplaceFlags()).c_str(),"wb");
                 ch = "3";
             break;
      }

              MSRecord *msr = 0;
              msr =  msr_init(NULL);
              msr->sequence_number = 0;               msr->reclen = 4096;
              msr->encoding = 10;              //--steim1
              msr->numsamples = Pack_N;        //--每XXX寫入1次
              msr->sampletype = 'i';           //-  a ascii, i int ,f float ,d double
              msr->dataquality = 'D';                 msr->byteorder = 0;

              msr->starttime = New_find_t;
              msr->samprate = SMrate;

               ch = Form1->Edit14->Text+ch;

               strcpy (msr->channel, ch.c_str());
               strcpy (msr->station, Form1->Edit13->Text.c_str() );
               strcpy (msr->network, Form1->Edit12->Text.c_str() );


          int Data_NO;
          Data_NO = Sd_Data_InputTime_Compress(hDevice,&(*Sd_Data),s_buff,ch_n);   //--輸出CH1測試


          int pk_N = 0;
          double Data_count = 1;
    while( New_find_t <= end_t){

            if(Line_Interpolation_Compress(hDevice,&s_buff,Sd_Data,New_find_t,&New_Data,ch_n,&Data_NO)==-1){
                ShowMessage("samplerate error ");
                return(-1);
            }

            New_find_t = start_t +  (SMrate_diff * Data_count);
                                     Data_count++;

            RawData[pk_N] = New_Data;
           pk_N++;
         if(pk_N >=Pack_N)
            {
               pk_N = 0;
               msr->datasamples = RawData;

               msr_pack(msr, &record_handler, NULL, &packedsamples, ch_n, 0);

               msr->starttime = New_find_t;       //-*-------------------*--------

               Form1->ProgressBar1->Position =  (New_find_t - start_t)/1000000;
               Application->ProcessMessages();
            }
    }
                    if ( outfile )
                    fclose (outfile);
                    msr_free(&msr);
  }
return(1);
}





void __fastcall TForm1::Button2Click(TObject *Sender)
{



 Out_Miniseed_v2("hha.**.mseed",23256125,23267549);



}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button3Click(TObject *Sender)
{
           Memo1->Lines->Add(FindTimeSector_Compress( hDevice,Edit1->Text));
           Memo2->Lines->Add(FindTimeSector_Compress( hDevice,Edit2->Text));        
}
//---------------------------------------------------------------------------

