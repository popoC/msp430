//********************************************************
// **** ROUTINES FOR FAT32 IMPLEMATATION OF SD CARD *****
//********************************************************
//Controller: ATmega32 (Clock: 8 Mhz-internal)
//Compiler: AVR-GCC
//Version : 2.0
//Author: CC Dharmani, Chennai (India)
//www.dharmanitech.com
//Date: 18 April 2009
//********************************************************

//Link to the Post: http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html

//**************************************************
// ***** HEADER FILE : FAT32.h ******
//**************************************************
#ifndef _fat32_H_
#define _fat32_H_


#pragma pack(1)   //--對齊

struct MSP_FILE{
  
// fat32 BootSector partition  ** 引導扇區資料
//       unsigned long totalBlocks;
       unsigned int bytesPerSector;      //-- 預設通常為                          512  /16G SDcard
       unsigned int sectorPerCluster;    //-- 每個簇大小由16個BLOCK(SECTOR)組成   16   /16G SDcard
       unsigned int reservedSectorCount; //-- 保留區大小 (內有bootsector,Fsinfo)  2420 /16G SDcard
//       unsigned long rootCluster;        //-- 根目錄起始簇號通常為2 // 根目錄內為檔案名稱與內容-- 以32BYTE為一單位
                                         //((2 - 2)*16 + 8192+2420+(15174*2) = 40960. 0根1號簇通常不使用
       unsigned int FSinfo_sector;       //-- FSINFO起始扇區通常為1 ((代表MBR下面的扇區 8192+1 = 8193 
       unsigned long FAT_table_sector;
       
       unsigned long totalClusters;
       unsigned long unusedSectors;      //-- 保留區大小 (保留區後面通常接FAT表--簇分配表 0,1號簇預設不用 
    
        //(FAT表 通常起始位子為 MBR大小+保留區大小
//  dir position      
        unsigned char   F_Name[20];  //--檔名不要太長
        unsigned long   dir_cluster;
        unsigned long   dir_sector;
        unsigned int    dir_start_byte;
        
        unsigned long   firstSector;
//  data position      
        unsigned long  data_startBlock;
        unsigned long   data_cluster;
        unsigned long   data_cluster_Buff;
        
        unsigned int    data_count;  // 512byte = 1 block
        unsigned int    sector_count;// 16sector = 1 cluster? in the bootsector
        unsigned char  Data_Buff[512];  //-- 可以加快檔案寫入速度 缺點...很占空間....同時開5個MSPFILE可能就會爆了..
        unsigned long   filesize; 
};                                  // This is the FILE object    */




//Structure to access Master Boot Record for getting info about partioions
struct MBRinfo_Structure{
unsigned char	nothing[446];		//ignore, placed here to fill the gap in the structure
unsigned char	partitionData[64];	//partition records (16x4)
unsigned int	signature;		//0xaa55
};

//Structure to access info of the first partioion of the disk 
struct partitionInfo_Structure{ 				
unsigned char	status;				//0x80 - active partition
unsigned char 	headStart;			//starting head
unsigned int	cylSectStart;		//starting cylinder and sector
unsigned char	type;				//partition type 
unsigned char	headEnd;			//ending head of the partition
unsigned int	cylSectEnd;			//ending cylinder and sector
unsigned long	firstSector;		//total sectors between MBR & the first sector of the partition
unsigned long	sectorsTotal;		//size of this partition in sectors
};

//Structure to access boot sector data
struct BS_Structure{
unsigned char jumpBoot[3]; //default: 0x009000EB
unsigned char OEMName[8];
unsigned int bytesPerSector; //deafault: 512
unsigned char sectorPerCluster;
unsigned int reservedSectorCount;
unsigned char numberofFATs;
unsigned int rootEntryCount;
unsigned int totalSectors_F16; //must be 0 for FAT32
unsigned char mediaType;
unsigned int FATsize_F16; //must be 0 for FAT32
unsigned int sectorsPerTrack;
unsigned int numberofHeads;
unsigned long hiddenSectors;
unsigned long totalSectors_F32;
unsigned long FATsize_F32; //count of sectors occupied by one FAT
unsigned int extFlags;
unsigned int FSversion; //0x0000 (defines version 0.0)
unsigned long rootCluster; //first cluster of root directory (=2)
unsigned int FSinfo; //sector number of FSinfo structure (=1)
unsigned int BackupBootSector;
unsigned char reserved[12];
unsigned char driveNumber;
unsigned char reserved1;
unsigned char bootSignature;
unsigned long volumeID;
unsigned char volumeLabel[11]; //"NO NAME "
unsigned char fileSystemType[8]; //"FAT32"
unsigned char bootData[420];
unsigned int bootEndSignature; //0xaa55
};


//Structure to access FSinfo sector data
struct FSInfo_Structure
{
unsigned long leadSignature; //0x41615252
unsigned char reserved1[480];
unsigned long structureSignature; //0x61417272
unsigned long freeClusterCount; //initial: 0xffffffff
unsigned long nextFreeCluster; //initial: 0xffffffff
unsigned char reserved2[12];
unsigned long trailSignature; //0xaa550000
};

//Structure to access Directory Entry in the FAT
struct dir_Structure{
unsigned char name[11];
unsigned char attrib; //file attributes
unsigned char NTreserved; //always 0
unsigned char timeTenth; //tenths of seconds, set to 0 here
unsigned int createTime; //time file was created
unsigned int createDate; //date file was created
unsigned int lastAccessDate;
unsigned int firstClusterHI; //higher word of the first cluster number
unsigned int writeTime; //time of last write
unsigned int writeDate; //date of last write
unsigned int firstClusterLO; //lower word of the first cluster number
unsigned long fileSize; //size of file in bytes
};

//Attribute definitions for file/directory
#define ATTR_READ_ONLY     0x01
#define ATTR_HIDDEN        0x02
#define ATTR_SYSTEM        0x04
#define ATTR_VOLUME_ID     0x08
#define ATTR_DIRECTORY     0x10
#define ATTR_ARCHIVE       0x20
#define ATTR_LONG_NAME     0x0f


#define DIR_ENTRY_SIZE     0x32
#define EMPTY              0x00
#define DELETED            0xe5
#define GET     0
#define SET     1
#define READ	0
#define VERIFY  1
#define ADD		0
#define REMOVE	1
#define TOTAL_FREE   1
#define NEXT_FREE    2
#define GET_LIST     0
#define GET_FILE     1
#define DELETE		 2
#define F32_EOF		0x0fffffff


//************* external variables *************
//extern volatile unsigned char buffer[512];
/*extern volatile unsigned long startBlock0;
extern volatile unsigned long totalBlocks;
extern volatile unsigned long firstDataSector, rootCluster, totalClusters;
extern volatile unsigned int  bytesPerSector, sectorPerCluster, reservedSectorCount;
//global flag to keep track of free cluster count updating in FSinfo sector
extern volatile unsigned char freeClusterCountUpdated;
extern volatile unsigned long unusedSectors;
*/
//************* functions *************


unsigned char SD_readSingleBlock(unsigned long startBlock);
unsigned char SD_writeSingleBlock(unsigned long startBlock);

unsigned char getBootSectorData (void);
unsigned char readBootSector(void);
unsigned char readDirInfo(void);
/**
unsigned long getSetFreeCluster(unsigned char totOrNext, unsigned char get_set, unsigned long FSEntry);
unsigned long getSetNextCluster (unsigned long clusterNumber,unsigned char get_set,unsigned long clusterEntry);
unsigned long searchNextFreeCluster (unsigned long startCluster);
*/


struct dir_Structure* findFiles (unsigned char flag, unsigned char *fileName);
unsigned char readFile (unsigned char flag, unsigned char *fileName);
unsigned char convertFileName (unsigned char *fileName);
void createFile (unsigned char *fileName);

void memoryStatistics (void);
void displayMemory (unsigned long memory);
void deleteFile (unsigned char *fileName);
void freeMemoryUpdate (unsigned char flag, unsigned long size);

//*************new functions *************
 //  MSP_FILE *MSP_fopen(unsigned char *filename);
int  MSP_fopen(const char *filename,MSP_FILE *);
   bool MSP_close(MSP_FILE *fp);
   bool MSP_fprintf(const char *wstring,MSP_FILE *fp,int mode);
   int FAT32_getBootSectorData();
   int FAT32_INIT(MSP_FILE *f32_fp);
#endif
