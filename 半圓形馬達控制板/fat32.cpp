//*******************************************************
// **** ROUTINES FOR FAT32 IMPLEMATATION OF SD CARD ****
//*******************************************************
//Controller: 	msp430f169 (8 Mhz internal)
//Compiler: 	IAR
//Version: 		
//Author: 		poki

//Date: 		2010 / 2 / 23
//*******************************************************
//**************************************************
// ***** SOURCE FILE : FAT32.c ******
//**************************************************
//#include <msp430x16x.h>
#include "main.h"
#include "sdhc.h"
#include "sdcomm_spi.h"
#include "spi.h"
#include "FAT32.h"
#include "stdio.h"

// -- 定義FAT32 全域變數 --  每片SD卡的FAT32格式都不一定相同 -- 每換一張卡片必須重新讀取一次
//  sdc, sd_buffer1  -- 定義在 SDHC.H裡

//global flag to keep track of free cluster count updating in FSinfo sector

unsigned char SD_writeData_Buff(unsigned long startBlock,MSP_FILE *msp_fp){
     sd_write_block(&sdc, startBlock, msp_fp->Data_Buff);
     sd_wait_notbusy (&sdc);
     return 0;
}
unsigned char SD_readBlock(unsigned long startBlock,MSP_FILE *msp_fp){
     sd_read_block(&sdc, startBlock, msp_fp->Data_Buff);
     sd_wait_notbusy (&sdc);
     return 0;
}

unsigned char SD_readSingleBlock(unsigned long startBlock){
     sd_read_block(&sdc, startBlock, sd_buffer1);
     sd_wait_notbusy (&sdc);
     return 0;
}
unsigned char SD_writeSingleBlock(unsigned long startBlock){
     sd_write_block(&sdc, startBlock, sd_buffer1);
     sd_wait_notbusy (&sdc);
     return 0;
}
//***************************************************************************
unsigned long F32_FatTable_searchNextFreeCluster (unsigned long startCluster,MSP_FILE *msp_fp)
{
  unsigned long cluster, *value, sector;
  unsigned char i;
    
	startCluster -=  (startCluster % 128);   //to start with the first file in a FAT sector
    for(cluster =startCluster; cluster <msp_fp->totalClusters; cluster+=128) 
    {
      sector = msp_fp->FAT_table_sector+((cluster * 4) / msp_fp->bytesPerSector);
      SD_readSingleBlock(sector);
      for(i=0; i<128; i++)
      {
       	 value = (unsigned long *) &sd_buffer1[i*4];
         if(((*value) & 0x0fffffff) == 0)
            return(cluster+i);
      }  
    } 

 return 0;
}
//***************************************************************************
unsigned long F32_FatTable_getSetNextCluster (unsigned long clusterNumber,unsigned char get_set,
                                 unsigned long clusterEntry,MSP_FILE *msp_fp)
{
unsigned int FATEntryOffset;
unsigned long *FATEntryValue;
unsigned long FATEntrySector;

//get sector number of the cluster entry in the FAT
FATEntrySector =  msp_fp->FAT_table_sector + ((clusterNumber * 4) / msp_fp->bytesPerSector) ;
//get the offset address in that sector number
FATEntryOffset = (unsigned int) ((clusterNumber * 4) % msp_fp->bytesPerSector);
//read the sector into a buffer
SD_readSingleBlock(FATEntrySector);
//get the cluster address from the buffer
FATEntryValue = (unsigned long *) &sd_buffer1[FATEntryOffset];

if(get_set == GET) return ((*FATEntryValue) & 0x0fffffff);
if(get_set == SET) *FATEntryValue = clusterEntry;   //for setting new value in cluster entry in FAT

SD_writeSingleBlock(FATEntrySector);
return (0);

}
//***************************************************************************
unsigned long F32_FSINFO_FreeCluster(unsigned char totOrNext, unsigned char get_set, unsigned long FSEntry,MSP_FILE *msp_fp)
{
struct FSInfo_Structure *FS = (struct FSInfo_Structure *) &sd_buffer1;
  SD_readSingleBlock(msp_fp->FSinfo_sector);

if((FS->leadSignature != 0x41615252) || (FS->structureSignature != 0x61417272) || (FS->trailSignature !=0xaa550000)) return 0xffffffff;

 if(get_set == GET)
 {
   if(totOrNext == TOTAL_FREE)  return(FS->freeClusterCount);
   if(totOrNext == NEXT_FREE)    return(FS->nextFreeCluster);
 }
 if(get_set == SET)
 {
   if(totOrNext == TOTAL_FREE) FS->freeClusterCount = FSEntry;
   if(totOrNext == NEXT_FREE)   FS->nextFreeCluster = FSEntry;
 
   SD_writeSingleBlock(msp_fp->FSinfo_sector);	//update FSinfo
 }
 return 0xffffffff;
}
//***************************************************************************
void F32_freeMemoryUpdate (unsigned char flag, unsigned long size,MSP_FILE *msp_fp)
{
  unsigned long freeClusters;
  //convert file size into number of clusters occupied
  if((size % msp_fp->bytesPerSector) == 0) size = size / msp_fp->bytesPerSector;
  else size = (size / msp_fp->bytesPerSector) +1;
  if((size % msp_fp->sectorPerCluster) == 0) size = size / msp_fp->sectorPerCluster;
  else size = (size / msp_fp->sectorPerCluster) +1;

   freeClusters = F32_FSINFO_FreeCluster (TOTAL_FREE, GET, 0,msp_fp);
	if(flag == ADD)
  	   freeClusters = freeClusters + size;
	else  //when flag = REMOVE
	   freeClusters = freeClusters - size;
	F32_FSINFO_FreeCluster (TOTAL_FREE, SET, freeClusters,msp_fp);
}
//***************************************************************************
int Find_dir_EMPTY(MSP_FILE *msp_fp){

int i,j;
struct dir_Structure *dir;
unsigned long dirFirstCluster_buff,dirFreeCluster_buff;

//getSetNextCluster(fp->cluster,SET,F32_EOF);
 while(1){
         dirFirstCluster_buff = (F32_FatTable_getSetNextCluster (msp_fp->dir_cluster, GET, 0,msp_fp));
          if(dirFirstCluster_buff > 0x0ffffff6)
         	break;
         msp_fp->dir_cluster = dirFirstCluster_buff; 
         }
 msp_fp->dir_sector =   (msp_fp->dir_cluster-2)*msp_fp->sectorPerCluster + msp_fp->firstSector;

 for( j=0; j<msp_fp->sectorPerCluster ; j++){
   
       SD_readSingleBlock(msp_fp->dir_sector);
 
       for(i=0;i<msp_fp->bytesPerSector;i+=32){
       
         dir = (struct dir_Structure *)&sd_buffer1[i];
         msp_fp->dir_start_byte = i;
           
         if(dir->name[0] == EMPTY){
           
           dirFreeCluster_buff = F32_FatTable_searchNextFreeCluster (msp_fp->dir_cluster,msp_fp);
           if(dirFreeCluster_buff == 0){ return(99); } //(" No free cluster!"));
           F32_FSINFO_FreeCluster (NEXT_FREE, SET,dirFreeCluster_buff,msp_fp);
           return 0;
         }
       }
       msp_fp->dir_sector++;
 }


 msp_fp->dir_start_byte = 0;
 
// msp_fp->dir_cluster 
      dirFreeCluster_buff = F32_FatTable_searchNextFreeCluster (msp_fp->dir_cluster,msp_fp);
      if(dirFreeCluster_buff == 0){ return(99); } //(" No free cluster!"));
 
   F32_FatTable_getSetNextCluster(msp_fp->dir_cluster,SET,dirFreeCluster_buff,msp_fp);
   F32_FatTable_getSetNextCluster(dirFreeCluster_buff,SET,F32_EOF,msp_fp);
 
 msp_fp->dir_sector =   (dirFreeCluster_buff-2)*msp_fp->sectorPerCluster + msp_fp->firstSector;
 msp_fp->dir_cluster = dirFreeCluster_buff;
 
      dirFreeCluster_buff = F32_FatTable_searchNextFreeCluster (msp_fp->dir_cluster,msp_fp);
      F32_FSINFO_FreeCluster (NEXT_FREE, SET, dirFreeCluster_buff,msp_fp); //update next free cluster entry in FSinfo sector


 
 
 return 0;
}


int MSP_fopen(const char *fileName,MSP_FILE *msp_fp){

  struct dir_Structure *dir;
  int i,j;
 for(j=0;j<11;j++){
    msp_fp->F_Name[j] = fileName[j];
 }
 
  i = Find_dir_EMPTY(msp_fp);
  if(i==99)return 2;//----空間已滿
 

  msp_fp->data_cluster = F32_FSINFO_FreeCluster (NEXT_FREE, GET, 0,msp_fp);
//  msp_fp->data_cluster = F32_FatTable_searchNextFreeCluster(msp_fp->data_cluster,msp_fp);
  
  
   SD_readSingleBlock(msp_fp->dir_sector);
   dir = (struct dir_Structure *) &sd_buffer1[msp_fp->dir_start_byte];
  
  if(msp_fp->data_cluster == 0){ return 2; } //(" No free cluster!"));
  for(int j=0; j<11; j++) dir->name[j] = msp_fp->F_Name[j];
		  dir->attrib = ATTR_ARCHIVE;	//settting file attribute as 'archive'
		  dir->NTreserved = 0;			//always set to 0
		  dir->timeTenth = 0;			//always set to 0
		  dir->createTime = 0x9684;		//fixed time of creation
		  dir->createDate = 0x3a37;		//fixed date of creation
		  dir->lastAccessDate = 0x3a37;	//fixed date of last access
		  dir->writeTime = 0x9684;		//fixed time of last write
		  dir->writeDate = 0x3a37;		//fixed date of last write
		  dir->firstClusterHI =(unsigned int) ((msp_fp->data_cluster & 0xffff0000) >> 16 );
		  dir->firstClusterLO =(unsigned int) ( msp_fp->data_cluster & 0x0000ffff);
		  dir->fileSize = 0;
   SD_writeSingleBlock (msp_fp->dir_sector);
   
   
    F32_FatTable_getSetNextCluster(msp_fp->data_cluster,SET,F32_EOF,msp_fp);
    
     msp_fp->sector_count = 0;
     msp_fp->filesize = 0;
     msp_fp->data_count = 0;
     
     
   return 0;
}
bool MSP_close(MSP_FILE *fp){

  struct dir_Structure *dir;
  F32_FatTable_getSetNextCluster(fp->data_cluster,SET,F32_EOF,fp);  //標記結束簇號
  
   SD_readSingleBlock(fp->dir_sector);
   dir = (struct dir_Structure *) &sd_buffer1[fp->dir_start_byte];
   dir->fileSize = fp->filesize;
   SD_writeSingleBlock (fp->dir_sector);
   
   F32_freeMemoryUpdate (REMOVE, fp->filesize,fp);

   return 1;
}

bool MSP_fprintf(const char *wstring,MSP_FILE *fp,int mode){

  fp->data_startBlock = ((fp->data_cluster-2) * fp->sectorPerCluster) + fp->firstSector + fp->sector_count;
    
    while(*wstring != '\0'){
      
      fp->Data_Buff[fp->data_count] = *wstring;
      *wstring++;      fp->data_count++;      fp->filesize++;
     
         if(fp->data_count==512){
            fp->data_count = 0;
              SD_writeData_Buff(fp->data_startBlock,fp);
              fp->sector_count++; 
              fp->data_startBlock++;           
           
           if(fp->sector_count == fp->sectorPerCluster){
              fp->sector_count = 0; 
         
             fp->data_cluster_Buff = F32_FatTable_searchNextFreeCluster (fp->data_cluster,fp);
             if(fp->data_cluster_Buff == 0){ return(99); } //(" No free cluster!"));
             F32_FatTable_getSetNextCluster(fp->data_cluster,SET,fp->data_cluster_Buff,fp); 
             F32_FatTable_getSetNextCluster(fp->data_cluster_Buff,SET,F32_EOF,fp);  // close那邊會做
             fp->data_startBlock = ((fp->data_cluster_Buff-2) * fp->sectorPerCluster) + fp->firstSector;   
          
             fp->data_cluster = fp->data_cluster_Buff;
           }
         }
     }

    if(*wstring == '\0'){
         int dj = fp->data_count;
      for(;dj<512;dj++){         fp->Data_Buff[dj] = 0x00;      }
      SD_writeData_Buff(fp->data_startBlock,fp);
    }
  //  getSetFreeCluster(NEXT_FREE,SET,fp->cluster);
    return(1);
};




//----------------------------------------------------------------------------
int FAT32_INIT(MSP_FILE *f32_fp){
  struct BS_Structure *bpb; //mapping the buffer onto the structure
  struct MBRinfo_Structure *mbr;
  struct partitionInfo_Structure *partition;
  unsigned long dataSectors;

f32_fp->unusedSectors = 0;

SD_readSingleBlock(0);
bpb = (struct BS_Structure *)sd_buffer1;

if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB)   //check if it is boot sector
{
  mbr = (struct MBRinfo_Structure *) sd_buffer1;       //if it is not boot sector, it must be MBR
  
  if(mbr->signature != 0xaa55) return 1;               //if it is not even MBR then it's not FAT32
  	
  partition = (struct partitionInfo_Structure *)(mbr->partitionData);//first partition
  f32_fp->unusedSectors = partition->firstSector; //the unused sectors, hidden to the FAT
  
  SD_readSingleBlock(partition->firstSector);//read the bpb sector
  bpb = (struct BS_Structure *)sd_buffer1;
  if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB) return 1; 
}

f32_fp->bytesPerSector = bpb->bytesPerSector;
f32_fp->sectorPerCluster = bpb->sectorPerCluster;
f32_fp->reservedSectorCount = bpb->reservedSectorCount;
//f32_fp->rootCluster = bpb->rootCluster;// + (sector / sectorPerCluster) +1;

f32_fp->dir_cluster = bpb->rootCluster;
f32_fp->firstSector = bpb->hiddenSectors + bpb->reservedSectorCount + (bpb->numberofFATs * bpb->FATsize_F32); //--第一個資料SECTOR 根目錄通常都以這裡為期始   40960...

f32_fp->FSinfo_sector = f32_fp->unusedSectors+1;
f32_fp->FAT_table_sector = f32_fp->unusedSectors+ f32_fp->reservedSectorCount;

//f32_fp->firstDataSector = bpb->hiddenSectors + reservedSectorCount + (bpb->numberofFATs * bpb->FATsize_F32);

dataSectors = bpb->totalSectors_F32 - bpb->reservedSectorCount - ( bpb->numberofFATs * bpb->FATsize_F32);
f32_fp->totalClusters = dataSectors / f32_fp->sectorPerCluster;


/*
if((getSetFreeCluster (TOTAL_FREE, GET, 0)) > f32_fp->totalClusters)  //check if FSinfo free clusters count is valid
     f32_fp->freeClusterCountUpdated = 0;
else
	 f32_fp->freeClusterCountUpdated = 1;

*/

return 0;

}





