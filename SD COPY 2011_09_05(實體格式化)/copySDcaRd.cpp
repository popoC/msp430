//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <stdio.h>
#include <string>
#include "copySDcaRd.h"
#include <WinIoCtl.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;


unsigned char DataList[512]; // Format Table
unsigned int TotalDataList = 0;
unsigned char Buffer[512*100*20]; // 10 seconds quota

unsigned char AD_Time[512*100*10][7];
double AD_Buf1[512*100*10];
double AD_Buf2[512*100*10];
double AD_Buf3[512*100*10];

#define read_BLOCK_tot 2000


String DataTime_block[read_BLOCK_tot];
   double seismometer_x[42*read_BLOCK_tot];
   double seismometer_y[42*read_BLOCK_tot];
   double seismometer_z[42*read_BLOCK_tot];



HANDLE GetPhysicalDiskHandle(String DriveLetter);
bool GetPhysicalDiskNum(String DriveLetter, DWORD &DiskNum);
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------


void __fastcall TForm1::FormCreate(TObject *Sender)
{
	char pBuf[255];
	DWORD dwDiskBufLen = GetLogicalDriveStrings(255, pBuf);
	std::string strAllDiskLetter(pBuf, dwDiskBufLen);
	std::string::size_type headIdx = 0, tailIdx = 0;
	while((tailIdx = strAllDiskLetter.find_first_of((const char)0, headIdx)) != std::string::npos)
	{
		if( GetDriveType(strAllDiskLetter.substr(headIdx, tailIdx).c_str()) == DRIVE_REMOVABLE )
                {   
                      AnsiString Msg = strAllDiskLetter.substr(headIdx, tailIdx).c_str();
                      if(Msg!="A:\\")ComboBox1->Items->Add(Msg);
		}
        headIdx = tailIdx + 1;
        tailIdx = headIdx;
	}
	ComboBox1->ItemIndex = 0;
     Form1->Caption ="Please select your removable devices  "+Form1->Caption;
//        ShowMessage("Please select your removable devices");

}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button1Click(TObject *Sender)
{

TotalDataList = 0;
	HANDLE hDevice;
      	DWORD dwBlockRead ;
	unsigned char tmp[512]={0};
	String DevicePath = "\\\\.\\" + ComboBox1->Items->Strings[ComboBox1->ItemIndex].SubString(1,2);
	//hDevice = CreateFile(DevicePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

        hDevice  =  GetPhysicalDiskHandle(DevicePath);

         bool   errMess;
         LARGE_INTEGER  offset ;
         unsigned __int64 BigNumber;


	if( hDevice != INVALID_HANDLE_VALUE )
	{
     	        //SetFilePointer(hDevice, 0, 0, FILE_CURRENT);
        BigNumber = 0*512;
        offset.LowPart  = 0;//(BigNumber & 0x00000000FFFFFFFF );
        offset.HighPart = 0;// ((BigNumber & 0xFFFFFFFF00000000)>>32);
        errMess = SetFilePointerEx(hDevice,offset ,0 ,FILE_BEGIN);
        if(!errMess){ShowMessage("無法開啟");CloseHandle(hDevice);return;}

       	ReadFile(hDevice, DataList, 512,&dwBlockRead, 0);
	} else {
		ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);
		return;
	}
	String str;
	TotalDataList = DataList[0];
	TotalDataList = (TotalDataList << 8) + DataList[1];
	str = "Data: " + IntToStr(TotalDataList);

	Form1->Caption = str;

	ListBox1->Clear();

	unsigned int StartBlock = 0, EndBlock = 0;

	for( unsigned int i=1; i<TotalDataList+1; i++ )
	{
		StartBlock = DataList[16*i+0];
		StartBlock = (StartBlock << 8)+DataList[16*i+1];
		StartBlock = (StartBlock << 8)+DataList[16*i+2];
		StartBlock = (StartBlock << 8)+DataList[16*i+3];

		EndBlock = DataList[16*i+4];
		EndBlock = (EndBlock << 8)+DataList[16*i+5];
		EndBlock = (EndBlock << 8)+DataList[16*i+6];
		EndBlock = (EndBlock << 8)+DataList[16*i+7];

str = "Data" + IntToStr(i) + " : Block ." + IntToStr(StartBlock) + ". - ." + IntToStr(EndBlock) + ".   Date: ";

//		SetFilePointer(hDevice, 512*StartBlock, 0, FILE_BEGIN);
        BigNumber = (unsigned __int64)StartBlock*512;
        offset.LowPart  = (BigNumber & 0x00000000FFFFFFFF );
        offset.HighPart = ((BigNumber & 0xFFFFFFFF00000000)>>32);
        errMess = SetFilePointerEx(hDevice,offset ,0 ,FILE_BEGIN);
        if(!errMess){ShowMessage("無法開啟");CloseHandle(hDevice);return;}

		ReadFile(hDevice, tmp, 512, &dwBlockRead, 0);

		// 月日時分秒
		str += IntToStr(tmp[1]) + "/";
		str += IntToStr(tmp[2]) + " ";
		str += IntToStr(tmp[3]) + ":";
		str += IntToStr(tmp[4]) + " -> ";

//		SetFilePointer(hDevice, 512*EndBlock, 0, FILE_BEGIN);
        BigNumber = (unsigned __int64)EndBlock*512;
        offset.LowPart  = (BigNumber & 0x00000000FFFFFFFF );
        offset.HighPart = ((BigNumber & 0xFFFFFFFF00000000)>>32);
        errMess = SetFilePointerEx(hDevice,offset ,0 ,FILE_BEGIN);
        if(!errMess){ShowMessage("無法開啟");CloseHandle(hDevice);return;}

		ReadFile(hDevice, tmp, 512, &dwBlockRead, 0);

		// 月日時分秒
		str += IntToStr(tmp[1]) + "/";
		str += IntToStr(tmp[2]) + " ";
		str += IntToStr(tmp[3]) + ":";
		str += IntToStr(tmp[4]) ;

		ListBox1->Items->Add(str);
	}

	CloseHandle(hDevice);              
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Button2Click(TObject *Sender)
{
         bool   errMess;
         LARGE_INTEGER  offset ;
         unsigned __int64 BigNumber;

if(SaveDialog1->Execute()){
     	HANDLE hDevice; 	DWORD dwBlockRead;
    	String DevicePath = "\\\\.\\" + ComboBox1->Items->Strings[ComboBox1->ItemIndex].SubString(1,2);
	hDevice = CreateFile(DevicePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);


	if( hDevice != INVALID_HANDLE_VALUE )
	{
//		SetFilePointer(hDevice, 0, 0, FILE_BEGIN);
        BigNumber = (unsigned __int64)0*512;
        offset.LowPart  = (BigNumber & 0x00000000FFFFFFFF );
        offset.HighPart = ((BigNumber & 0xFFFFFFFF00000000)>>32);
        errMess = SetFilePointerEx(hDevice,offset ,0 ,FILE_BEGIN);
        if(!errMess){ShowMessage("無法開啟");CloseHandle(hDevice);return;}

		ReadFile(hDevice, DataList, 512, &dwBlockRead, 0);
	}
         else {
		ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);
		return;
	}

	TotalDataList = DataList[0];
	TotalDataList = (TotalDataList << 8) + DataList[1];
        if(TotalDataList <= 0){
         ShowMessage("SD Card is no Data");
              return;
        }
	unsigned int StartBlock = 0, EndBlock = 0;
      		StartBlock = DataList[16*1+0];
		StartBlock = (StartBlock << 8)+DataList[16*1+1];
		StartBlock = (StartBlock << 8)+DataList[16*1+2];
		StartBlock = (StartBlock << 8)+DataList[16*1+3];
		EndBlock = DataList[16*TotalDataList+4];
		EndBlock = (EndBlock << 8)+DataList[16*TotalDataList+5];
		EndBlock = (EndBlock << 8)+DataList[16*TotalDataList+6];
		EndBlock = (EndBlock << 8)+DataList[16*TotalDataList+7];
              //----SD卡內全部資料

           ProgressBar1->Max = EndBlock;
           ProgressBar1->Min = 0;
           float sdMB = EndBlock * 0.5 /1024;
           String showMB = "檔案大小為 : "+ FloatToStr(sdMB)+" MB";

        if(MessageBox( NULL, showMB.c_str() , "SD卡複製所需空間大小" ,MB_OKCANCEL) == 2){
            CloseHandle(hDevice);
            return;
        }
      	FILE *fp;


        fp = fopen(SaveDialog1->FileName.c_str(),"w");
        fclose(fp);


int Block_No = EndBlock; //-要複製的BLOCK數目
//int Block_No = 1500; //-要複製的BLOCK數目
int Next_Block = 0;
int Bok_No = 0;
        HANDLE copySD;

      copySD = CreateFile(SaveDialog1->FileName.c_str(),
                    GENERIC_WRITE,
                    FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

	
//          SetFilePointer(hDevice, 0, 0,FILE_BEGIN);           //先複製標頭檔
        BigNumber = (unsigned __int64)0*512;
        offset.LowPart  = (BigNumber & 0x00000000FFFFFFFF );
        offset.HighPart = ((BigNumber & 0xFFFFFFFF00000000)>>32);
        errMess = SetFilePointerEx(hDevice,offset ,0 ,FILE_BEGIN);
        if(!errMess){ShowMessage("無法開啟");CloseHandle(hDevice);return;}


          ReadFile(hDevice, Buffer, 512*1, &dwBlockRead, 0);

//          SetFilePointer(copySD, 0, 0,FILE_BEGIN);
        BigNumber = (unsigned __int64)0*512;
        offset.LowPart  = (BigNumber & 0x00000000FFFFFFFF );
        offset.HighPart = ((BigNumber & 0xFFFFFFFF00000000)>>32);
        errMess = SetFilePointerEx(copySD,offset ,0 ,FILE_BEGIN);
        if(!errMess){ShowMessage("無法開啟");CloseHandle(hDevice);return;}



          WriteFile(copySD, Buffer, 512*1, &dwBlockRead, 0);
          Next_Block = 1;
          ProgressBar1->Position = 1;
          //---------------------------------------------------------------------------
           while(Block_No>0){
            Block_No >= read_BLOCK_tot ? Bok_No= read_BLOCK_tot : Bok_No= Block_No;
//             SetFilePointer(hDevice, 512*(Next_Block), 0,FILE_BEGIN);   //-BLOCK 2
        BigNumber = (unsigned __int64)Next_Block*512;
        offset.LowPart  = (BigNumber & 0x00000000FFFFFFFF );
        offset.HighPart = ((BigNumber & 0xFFFFFFFF00000000)>>32);
        errMess = SetFilePointerEx(hDevice,offset ,0 ,FILE_BEGIN);
        if(!errMess){ShowMessage("無法開啟");CloseHandle(hDevice);return;}


             ReadFile(hDevice, Buffer, 512*Bok_No, &dwBlockRead, 0); //讀取read_BLOCK_tot個BLOCK

//             SetFilePointer(copySD, 512*(Next_Block), 0,FILE_BEGIN);
        BigNumber = (unsigned __int64)Next_Block*512;
        offset.LowPart  = (BigNumber & 0x00000000FFFFFFFF );
        offset.HighPart = ((BigNumber & 0xFFFFFFFF00000000)>>32);
        errMess = SetFilePointerEx(copySD,offset ,0 ,FILE_BEGIN);
        if(!errMess){ShowMessage("無法開啟");CloseHandle(hDevice);return;}

             WriteFile(copySD, Buffer, 512*Bok_No, &dwBlockRead, 0);

             ProgressBar1->Position = Next_Block;
               Next_Block+=read_BLOCK_tot;
               Block_No-=read_BLOCK_tot;
             Application->ProcessMessages();
           }

            CloseHandle(copySD);
            CloseHandle(hDevice);
            ShowMessage("ok");

}
 Close();
}
//---------------------------------------------------------------------------

bool GetPhysicalDiskNum(String DriveLetter, DWORD &DiskNum)
{   
VOLUME_DISK_EXTENTS DiskExtent;

HANDLE hDrive;   
DWORD  BytesRet;   
BOOL   Success;   
//char   DriveStr[]="\\\\.\\X:";
// DriveStr[4] = DriveLetter;

hDrive = ::CreateFile(DriveLetter.c_str(),   
        GENERIC_READ|GENERIC_WRITE,   
        FILE_SHARE_READ|FILE_SHARE_WRITE,   
        NULL,   
        OPEN_EXISTING,   
        FILE_ATTRIBUTE_NORMAL,   
        NULL );   

Success = FALSE;   
DiskNum = -1;   
  
if (hDrive != INVALID_HANDLE_VALUE)   
{        
    Success = ::DeviceIoControl( hDrive,        // handle to device
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,    // dwIoControlCode
        NULL,        // lpInBuffer   
        0,        // nInBufferSize   
        &DiskExtent,        // output buffer   
        sizeof(DiskExtent),        // size of output buffer   
        &BytesRet,        // number of bytes returned   
        NULL);        // OVERLAPPED structure   
    if (Success)   
    DiskNum = DiskExtent.Extents[0].DiskNumber;   
  
    ::CloseHandle(hDrive);   
}   
return (bool)Success;   
}   
/**  
取得邏輯磁碟機對應的實體磁碟機的Handle  
*/
HANDLE GetPhysicalDiskHandle(String DriveLetter)
{   
HANDLE hPhysical;   
DWORD  DiskNum;
  
AnsiString PhysStr = "\\\\.\\PHYSICALDRIVE";   
  
if (!GetPhysicalDiskNum(DriveLetter, DiskNum))
    hPhysical = INVALID_HANDLE_VALUE;   
else  
{   
    PhysStr += IntToStr(DiskNum);       
  
    hPhysical = CreateFile( PhysStr.c_str(),   
        GENERIC_READ | GENERIC_WRITE,   
        FILE_SHARE_READ | FILE_SHARE_WRITE,   
        NULL,   
        OPEN_EXISTING,   
        FILE_FLAG_WRITE_THROUGH,   
        NULL);   
}   
return hPhysical;   
}
/**
找出邏輯磁碟機對應的實體磁碟機代號
傳入值:
DriveLetter: 邏輯磁碟機的符號, 如; C, D, E...
DiskNum    : 傳回的實體磁碟機代號, 如果要進一步用CreateFile()開啟實體磁碟
機, 可用"\\\\.\\PHYSICALDRIVEn", 做為第一個參數, 其中n就是
DiskNum
傳回值:
成功傳回true, 實體磁碟機的代號放在DiskNum, 否則傳回false

Note:
1. 根據MSDN的說明, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS傳回的
VOLUME_DISK_EXTENTS結構裏 , NumberOfDiskExtents表示此邏輯磁碟機"分佈"
的實體磁碟機數量, 一般來說, 此值應該為1, 表示傳入的邏輯磁碟機的磁區均
位於同一顆實體磁碟中, 例如: 一顆硬碟分割成C,D,E三槽, 則用C,D,E傳入此
函式時, 得到的 DiskExtent.Extents[0].DiskNumber應該相同

2. 如果邏輯磁碟機的磁區分佈於不同的實體磁碟機, 則NumberOfDiskExtents的
值不為1, 而DeviceIoControl()失敗後, 呼叫 GetLastError()應該會傳回
ERROR_MORE_DATA, 此時要修正傳入 DeviceIoControl 的Output Buffer大小,

3. 此函式只處理第一種狀況, 第二種狀況就當做fail
*/
bool GetPhysicalDiskNum(char DriveLetter, DWORD &DiskNum)
{
VOLUME_DISK_EXTENTS DiskExtent;

HANDLE hDrive;
DWORD  BytesRet;
BOOL   Success;
char   DriveStr[]="\\\\.\\X:";

DriveStr[4] = DriveLetter;

hDrive = ::CreateFile( DriveStr,
GENERIC_READ|GENERIC_WRITE,
FILE_SHARE_READ|FILE_SHARE_WRITE,
NULL,
OPEN_EXISTING,
FILE_ATTRIBUTE_NORMAL,
NULL );

Success = FALSE;
DiskNum = -1;

if (hDrive != INVALID_HANDLE_VALUE)
{
Success = ::DeviceIoControl( hDrive,        // handle to device
IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,    // dwIoControlCode
NULL,        // lpInBuffer
0,        // nInBufferSize
&DiskExtent,        // output buffer
sizeof(DiskExtent),        // size of output buffer
&BytesRet,        // number of bytes returned
NULL);        // OVERLAPPED structure
if (Success)
DiskNum = DiskExtent.Extents[0].DiskNumber;

::CloseHandle(hDrive);
}
return (bool)Success;
}
/**
取得邏輯磁碟機對應的實體磁碟機的Handle
*/
HANDLE GetPhysicalDiskHandle(char DriveLetter)
{
HANDLE hPhysical;
DWORD  DiskNum;

AnsiString PhysStr = "\\\\.\\PHYSICALDRIVE";

if (!GetPhysicalDiskNum(DriveLetter, DiskNum))
hPhysical = INVALID_HANDLE_VALUE;
else
{
PhysStr += IntToStr(DiskNum);

hPhysical = CreateFile( PhysStr.c_str(),
GENERIC_READ | GENERIC_WRITE,
FILE_SHARE_READ | FILE_SHARE_WRITE,
NULL,
OPEN_EXISTING,
FILE_FLAG_WRITE_THROUGH,
NULL);
}
return hPhysical;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button4Click(TObject *Sender)
{
 if(  MessageBox(0,"已經都拷貝過了???","確定要格式化 ??",MB_YESNO )== IDYES  ){

      	HANDLE hDevice;
	DWORD dwBlockWrite;
	String DevicePath = "\\\\.\\" + ComboBox1->Items->Strings[ComboBox1->ItemIndex].SubString(1,2);
	unsigned char NullData[512] = {0};


        hDevice  =  GetPhysicalDiskHandle(DevicePath);
	if( hDevice != INVALID_HANDLE_VALUE )
	{
                   for(int i=0;i<200;i++){
			SetFilePointer(hDevice, i*512, 0, FILE_BEGIN);
			WriteFile(hDevice, NullData, 512, &dwBlockWrite, 0);
                   }
	} else {
		ShowMessage("Device Handle Error!!");
		CloseHandle(hDevice);
		return;
	}

	Sleep(0.1);
	CloseHandle(hDevice);

	if( dwBlockWrite )
		ShowMessage("Format SD Card Success!!");
	else {
		ShowMessage("Format SD Card Failure!!");
		return;
	}
  }
//	But        
}
//---------------------------------------------------------------------------



