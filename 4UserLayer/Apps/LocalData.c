/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : LocalData.c
  �� �� ��   : ����
  ��    ��   :  
  ��������   : 2020��3��21��
  ����޸�   :
  ��������   : �����������ݣ��Կ��ţ��û�IDΪ��������-
                   �ݽ�����ɾ�Ĳ�
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2020��3��21��
    ��    ��   :  
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include "LocalData.h"
#include "bsp_spi_flash.h"
#include "easyflash.h"
#include "stdio.h"
#include "tool.h"
#include "malloc.h"

#define LOG_TAG    "localData"
#include "elog.h"


/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ģ�鼶����                                   *
// *----------------------------------------------*/
uint16_t gCurCardHeaderIndex = 0;
//uint16_t gCurUserHeaderIndex = 0;
uint16_t gCurRecordIndex = 0;


/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/
static uint8_t checkFlashSpace(uint8_t mode);
static void eraseUserDataIndex(void);
 
/*****************************************************************************
 �� �� ��  : writeHeader
 ��������  : д��ͷ�ļ�
 �������  : uint8_t  * header  ���Ż������û�ID
             uint8_t mode   0��д����       1:д�û�ID
 �������  : ��
 �� �� ֵ  : 0:�ɹ���������ʧ��
 
 �޸���ʷ      :
  1.��    ��   : 2020��3��24��
    ��    ��   :  
uint8_t writeHeader(uint8_t  * header,uint8_t mode)
    �޸�����   : �����ɺ���

*****************************************************************************/
uint8_t writeHeader(uint8_t  * header,uint8_t mode)
{
    uint8_t times = 3;
    uint8_t readBuff[HEAD_lEN+1] = {0};
	uint8_t ret = 0;
    uint8_t headCnt[6] = {0};
    uint8_t temp[HEAD_lEN+1] = {0};
    uint32_t addr = 0;
         

//    if(mode == CARD_MODE)
//    {
        addr = CARD_NO_HEAD_ADDR + gCurCardHeaderIndex * HEAD_lEN;
//    }
//    else 
//    {
//        addr = USER_ID_HEAD_ADDR + gCurUserHeaderIndex * HEAD_lEN;
//    }


    if(header == NULL || strlen((const char*)header) == 0)
    {
        log_d("<<<<<<<<<<<<<<cardNoHeader is empty>>>>>>>>>>>>>>\r\n");
        return 1;
    }

    log_e("writeHeader = %s\r\n",header);

    memset(temp,0x00,sizeof(temp));
    asc2bcd(temp, header,HEAD_lEN*2, 0);
    
   	while(times)
	{		
		ret = bsp_sf_WriteBuffer (temp, addr, HEAD_lEN);
        
		//�ٶ��������Ա��Ƿ�һ��
		memset(readBuff,0x00,sizeof(readBuff));
		bsp_sf_ReadBuffer (readBuff, addr, HEAD_lEN);
		
		ret = compareArray(temp,readBuff,HEAD_lEN);
		
		if(ret == 0)
		{
			break;
		}

		if(ret != 0 && times == 1)
		{
            log_d("write header error\r\n");
			return 3;
		}

		times--;
	} 

    //������Ҫ����  
//    if(mode == CARD_MODE)
//    {
        gCurCardHeaderIndex++;
        memset(headCnt,0x00,sizeof(headCnt));
        sprintf((char *)headCnt,"%06d",gCurCardHeaderIndex);
        ef_set_env_blob("CardHeaderIndex",headCnt,6);          
//    }
//    else
//    {
//        gCurUserHeaderIndex++;
//        memset(headCnt,0x00,sizeof(headCnt));
//        sprintf(headCnt,"%06d",gCurUserHeaderIndex);
//        ef_set_env_blob("UserHeaderIndex",headCnt,6);           
//    }

	log_d("gCurCardHeaderIndex = %d\r\n",gCurCardHeaderIndex);

    
    return 0;
    
}



//isFind = 0 δ�ҵ���isFind = 1 ���ҵ�
/*****************************************************************************
 �� �� ��  : searchHeaderIndex
 ��������  : ����ָ���Ŀ���/�û�ID������ֵ
 �������  : uint8_t* header 
           uint8_t mode     
           
 �������  : uint16_t *index 
 �� �� ֵ  : 1:�ҵ�����ֵ 0:δ�ҵ�����ֵ
 
 �޸���ʷ      :
  1.��    ��   : 2020��3��24��
    ��    ��   :  
uint8_t searchHeaderIndex(uint8_t* header,uint8_t mode,uint16_t *index)
    �޸�����   : �����ɺ���

*****************************************************************************/
uint8_t searchHeaderIndex(uint8_t* header,uint8_t mode,uint16_t *index)
{
    char *buff = my_malloc(CARD_NO_HEAD_SIZE);
	uint8_t temp[HEAD_lEN] = {0};
	uint32_t addr = 0;
	uint16_t i = 0;
    uint32_t num = 0;
    uint8_t isFind = 0;   

     if (buff == NULL || header == NULL)
     {
        my_free(buff);
        log_d("my_malloc error\r\n");
        return isFind;
     }

	memset(temp,0x00,sizeof(temp));
    asc2bcd(temp, header,HEAD_lEN*2, 0);     
//    if(mode == CARD_MODE)
//    {
        addr = CARD_NO_HEAD_ADDR;
        bsp_sf_ReadBuffer ((uint8_t *)buff, addr, gCurCardHeaderIndex*4);        
        num = gCurCardHeaderIndex;

//    }
//    else
//    {
//        addr = USER_ID_HEAD_ADDR;
//        bsp_sf_ReadBuffer (buff, addr, gCurUserHeaderIndex*4);
//        num = gCurUserHeaderIndex;        
//    } 


	for(i=0; i<num; i++)
	{
        if(memcmp(temp,buff+i*HEAD_lEN,HEAD_lEN) == 0)
        {
            isFind = 1;
            *index = i;

            my_free(buff);
            return isFind;
        }        
    }

    *index = 0;
    my_free(buff);
    return isFind;	     
}

void eraseUserDataAll(void)
{
    int32_t iTime1, iTime2;
    iTime1 = xTaskGetTickCount();	/* ���¿�ʼʱ�� */
    eraseHeadSector();
    eraseDataSector();    
    iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_d("eraseUserDataAll�ɹ�����ʱ: %dms\r\n",iTime2 - iTime1);
}

static void eraseUserDataIndex(void)
{
    uint8_t headCnt[6] = {0};
    gCurCardHeaderIndex = 0;
    sprintf((char *)headCnt,"%06d",gCurCardHeaderIndex);
    ef_set_env_blob("CardHeaderIndex",headCnt,6);    

//    memset(headCnt,0x00,sizeof(headCnt));
//    gCurUserHeaderIndex = 0;
//    sprintf(headCnt,"%06d",gCurUserHeaderIndex);
//    ef_set_env_blob("UserHeaderIndex",headCnt,6);      

}


void eraseHeadSector(void)
{  
    uint16_t i = 0;

    for(i=0;i<CARD_SECTOR_NUM;i++)
    {
        bsp_sf_EraseSector(CARD_NO_HEAD_ADDR+i*SECTOR_SIZE);
    }

    eraseUserDataIndex();
}
void eraseDataSector(void)
{  
    uint16_t i = 0;

    for(i=0;i<DATA_SECTOR_NUM;i++)
    {
        bsp_sf_EraseSector(CARD_NO_DATA_ADDR+i*SECTOR_SIZE);
//        bsp_sf_EraseSector(USER_ID_DATA_ADDR+i*SECTOR_SIZE);
    }
}


uint8_t writeUserData(USERDATA_STRU userData,uint8_t mode)
{
    uint8_t wBuff[255] = {0};
    uint8_t rBuff[255] = {0}; 
    uint8_t isFull = 0;
    uint8_t header[CARD_USER_LEN] = {0};
    uint8_t crc=0;
    uint8_t ret = 0;
    uint8_t times = 3;
    uint32_t addr = 0;
    
    int32_t iTime1, iTime2;
//	log_d("sizeof(USERDATA_STRU) = %d\r\n",sizeof(USERDATA_STRU));
    
    iTime1 = xTaskGetTickCount();	/* ���¿�ʼʱ�� */
 
	
    //���洢�ռ��Ƿ�������
    isFull = checkFlashSpace(mode);

    if(isFull == 1)
    {
        return 1; //��ʾ�Ѿ�����
    }

    
//    if(mode == CARD_MODE)
//    {
        addr = CARD_NO_DATA_ADDR + gCurCardHeaderIndex * (sizeof(USERDATA_STRU));
        memcpy(header,userData.cardNo,CARD_USER_LEN);
//    }
//    else
//    {
//        addr = USER_ID_DATA_ADDR + gCurUserHeaderIndex * (sizeof(USERDATA_STRU));
//        memcpy(header,userData.userId,CARD_USER_LEN);
//    }

    if(header == NULL)
    {
        return 1;//��ʾ��ͷ��Ч
    }
    
   //д��ͷ
   ret = writeHeader(header,mode);

   if(ret != 0)
   {
       return 1;//��ʾд��ͷʧ��
   }

    //packet write buff
    memset(wBuff,0x00,sizeof(wBuff));  
	
    //copy to buff
    memcpy(wBuff, &userData, sizeof(USERDATA_STRU)-1);
	
    //calc crc
	crc = xorCRC(wBuff, sizeof(USERDATA_STRU)-1);

	//copy crc
	wBuff[sizeof(USERDATA_STRU)-1] = crc;	


    //write flash
	while(times)
	{
	
		bsp_sf_WriteBuffer (wBuff, addr, sizeof(USERDATA_STRU));

		//�ٶ��������Ա��Ƿ�һ��
		memset(rBuff,0x00,sizeof(rBuff));
		bsp_sf_ReadBuffer (rBuff, addr, sizeof(USERDATA_STRU));

		ret = compareArray(wBuff,rBuff,sizeof(USERDATA_STRU));
		
		if(ret == 0)
		{
			break;
		}
				

		if(ret != 0 && times == 1)
		{
			log_d("д���׼�¼��ʧ��!\r\n");
			return 3;
		}

		times--;
	}

    iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_d("writeUserData�ɹ�����ʱ: %dms\r\n",iTime2 - iTime1);	

//    dbh("writeUserData", wBuff, sizeof(USERDATA_STRU));

    return 0;
}

uint8_t readUserData(uint8_t* header,uint8_t mode,USERDATA_STRU *userData)
{
	uint8_t rBuff[255] = {0};
	uint8_t crc = 0;	
	uint8_t ret = 0;
	uint16_t index = 0;
    uint32_t addr = 0;
    
    int32_t iTime1, iTime2;
    iTime1 = xTaskGetTickCount();	/* ���¿�ʼʱ�� */
    

	if(header == NULL)
	{
        log_d("invalid head\r\n"); 
        return 1; //��ʾ����Ĳ���
	}

		
	ret = searchHeaderIndex(header,mode,&index);

	log_d("searchHeaderIndex ret = %d",ret);
	
	if(ret != 1)
	{
        log_d("can't find the head index\r\n");    
	
		return 3;//��ʾδ�ҵ�����
	}
	

//    if(mode == CARD_MODE)
//    {
        addr = CARD_NO_DATA_ADDR + index * (sizeof(USERDATA_STRU));
//    }
//    else
//    {
//        addr = USER_ID_DATA_ADDR + index * (sizeof(USERDATA_STRU));
//    }	

    bsp_sf_ReadBuffer (rBuff, addr, sizeof(USERDATA_STRU));

    //judge head
	if(rBuff[0] != TABLE_HEAD)
	{
        log_d("invalid start char\r\n");    	
		return 4;
	}

    //calc crc
	crc = xorCRC(rBuff, sizeof(USERDATA_STRU)-1);

	//judge crc
	if(crc != rBuff[sizeof(USERDATA_STRU) - 1])
	{
        log_d("crc check error\r\n");   
		return 5;
	}

	memcpy(userData, rBuff, sizeof(USERDATA_STRU));



    iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_d("readUserData�ɹ�����ʱ: %dms\r\n",iTime2 - iTime1);		

    dbh("readUserData", (char *)rBuff, sizeof(USERDATA_STRU));

	return 0;

}


static uint8_t checkFlashSpace(uint8_t mode)
{
//    if(mode == CARD_MODE)
//    {
        if(gCurCardHeaderIndex > MAX_HEAD_RECORD-5)
        {
            log_d("card flash is full\r\n");

            return 1;
        }
//    }
//    else
//    {
//        if(gCurUserHeaderIndex > MAX_HEAD_RECORD-5)
//        {
//            log_d("card flash is full\r\n");
//            return 1;
//        }        
//    }


    return 0;
}



uint8_t modifyUserData(USERDATA_STRU userData,uint8_t mode)
{
    uint8_t wBuff[255] = {0};
    uint8_t rBuff[255] = {0}; 
    uint8_t isFull = 0;
    uint8_t header[CARD_USER_LEN] = {0};
    uint8_t crc=0;
    uint8_t ret = 0;
    uint8_t times = 3;
    uint32_t addr = 0;
    uint16_t index = 0;
    
    int32_t iTime1, iTime2;
	log_d("sizeof(USERDATA_STRU) = %d\r\n",sizeof(USERDATA_STRU));
    
    iTime1 = xTaskGetTickCount();	/* ���¿�ʼʱ�� */
 
	
    //���洢�ռ��Ƿ�������
    isFull = checkFlashSpace(mode);

    if(isFull == 1)
    {
        log_d("not enough speac storage the data\r\n");    
        return 1; //��ʾ�Ѿ�����
    } 

//    if(mode == CARD_MODE)
//    {
        memcpy(header,userData.cardNo,CARD_USER_LEN);
//    }
//    else
//    {
//        memcpy(header,userData.userId,CARD_USER_LEN);
//    }   

	ret = searchHeaderIndex(header,mode,&index);

	log_d("searchHeaderIndex ret = %d",ret);
	
	if(ret != 1)
	{
	    log_d("can't find the head index\r\n");
		return 3;//��ʾδ�ҵ�����
	}

//	if(mode == CARD_MODE)
//	{
        addr = CARD_NO_DATA_ADDR + index * (sizeof(USERDATA_STRU));

//   }
//    else
//    {
//        addr = USER_ID_DATA_ADDR + index * (sizeof(USERDATA_STRU));
//    }

    //packet write buff
    memset(wBuff,0x00,sizeof(wBuff)); 
	
    //copy to buff
    memcpy(wBuff, &userData, sizeof(USERDATA_STRU));
	
    //calc crc
	crc = xorCRC(wBuff, sizeof(USERDATA_STRU)-1);

	//copy crc
	wBuff[sizeof(USERDATA_STRU) - 1] = crc;	


    //write flash
	while(times)
	{
	
		bsp_sf_WriteBuffer (wBuff, addr, sizeof(USERDATA_STRU));

		//�ٶ��������Ա��Ƿ�һ��
		memset(rBuff,0x00,sizeof(rBuff));
		bsp_sf_ReadBuffer (rBuff, addr, sizeof(USERDATA_STRU));

		ret = compareArray(wBuff,rBuff,sizeof(USERDATA_STRU));
		
		if(ret == 0)
		{
			break;
		}
				

		if(ret != 0 && times == 1)
		{
			log_d("modify record is error\r\n");
			return 3;
		}

		times--;
	}

    iTime2 = xTaskGetTickCount();	/* ���½���ʱ�� */
	log_d("�޸ļ�¼�ɹ�����ʱ: %dms\r\n",iTime2 - iTime1);	

    return 0;
}


void TestFlash(uint8_t mode)
{
    char buff[156] = {0};
    uint8_t temp[4] = {0};
    uint32_t addr = 0;
    uint32_t data_addr = 0;
    uint16_t i = 0;
    uint32_t num = 0;

     if (buff == NULL )
     {
        my_free(buff);
        log_d("my_malloc error\r\n");
        return ;
     }

    memset(temp,0x00,sizeof(temp));
     
//    if(mode == CARD_MODE)
//    {
        addr = CARD_NO_HEAD_ADDR;
        data_addr = CARD_NO_DATA_ADDR;
        num = gCurCardHeaderIndex;
//    }
//    else
//    {
//        addr = USER_ID_HEAD_ADDR;   
//        data_addr = USER_ID_DATA_ADDR;
//        num = gCurUserHeaderIndex;        
//    } 

    for(i=0;i<num;i++)
    {
        memset(temp,0x00,sizeof(temp));
        memset(buff,0x00,sizeof(buff));
        bsp_sf_ReadBuffer (temp, addr+i*4, 4);
        bcd2asc((uint8_t *)buff, temp, 8, 0);
        printf("the %d header ====================== %s\r\n",i,buff);    
 
    }

    for(i=0;i<num;i++)
    {
        memset(buff,0x00,sizeof(buff));
        bsp_sf_ReadBuffer (temp, data_addr+i * (sizeof(USERDATA_STRU)), sizeof(USERDATA_STRU));        
        printf("the %d data ====================== \r\n",i); 
        dbh("data", (char *)temp, (sizeof(USERDATA_STRU)));

    }    

}

