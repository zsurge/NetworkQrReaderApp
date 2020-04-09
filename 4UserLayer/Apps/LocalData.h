/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : LocalData.h
  �� �� ��   : ����
  ��    ��   :  
  ��������   : 2020��3��21��
  ����޸�   :
  ��������   : LocalData.c ��ͷ�ļ�
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2020��3��21��
    ��    ��   :  
    �޸�����   : �����ļ�

******************************************************************************/
#ifndef __LOCALDATA_H__
#define __LOCALDATA_H__

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include "stm32f4xx.h" 


#define HEAD_lEN 4               //ÿ����¼ռ4���ֽ�
#define MAX_HEAD_RECORD     30000 //���3������¼
#define SECTOR_SIZE         4096    //ÿ��������С
#define CARD_NO_HEAD_ADDR   0x600000
#define CARD_NO_HEAD_SIZE   (HEAD_lEN*MAX_HEAD_RECORD)
//#define USER_ID_HEAD_ADDR   (CARD_NO_HEAD_ADDR+CARD_NO_HEAD_SIZE)
//#define USER_ID_HEAD_SIZE   (CARD_NO_HEAD_SIZE)

#define CARD_SECTOR_NUM     (CARD_NO_HEAD_SIZE/SECTOR_SIZE)
//#define USER_SECTOR_NUM     CARD_SECTOR_NUM

#define CARD_NO_DATA_ADDR   0X700000
//#define USER_ID_DATA_ADDR   0X800000

#define DATA_SECTOR_NUM     ((CARD_NO_DATA_ADDR-CARD_NO_HEAD_ADDR)/SECTOR_SIZE)


#define CARD_USER_LEN              (8)
#define OWNERID_LEN                (10)
#define TIME_LENGTH                (10)
#define RESERVE_LENGTH             (16) //Ԥ���ռ�,Ҳ����λ�ã�ÿ���û�����64���ֽ�

#define CARD_MODE                   0 //��ģʽ

////���ÿ�״̬Ϊ0��ɾ����
#define CARD_DEL                    0
#define CARD_VALID                  1
#define TABLE_HEAD                 0xAA



/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
extern uint16_t gCurCardHeaderIndex;    //��������
//extern uint16_t gCurUserHeaderIndex;    //�û�ID����
extern uint16_t gCurRecordIndex;


/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/
#pragma pack(1)
typedef struct USERDATA
{
    uint8_t head;                                   //����ͷ
    uint8_t cardState;                              //��״̬ ��Ч/��ɾ��/������/��ʱ��    
    uint8_t ownerId[OWNERID_LEN+1];                 //�û�ID
    uint8_t cardNo[CARD_USER_LEN+1];                //����
    uint8_t startTime[TIME_LENGTH+1];               //�˻���Чʱ��
    uint8_t endTime[TIME_LENGTH+1];                 //�˻�����ʱ��    
    uint8_t reserve[RESERVE_LENGTH+1];              //Ԥ���ռ�
    uint8_t crc;                                    //У��ֵ head~reseve
}USERDATA_STRU;
#pragma pack()



/*
typedef struct USERSTATE
{
    uint8_t isInvalid;
    uint8_t isValid;
    uint8_t isTemporary;
    uint8_t isBlackList;
}USERSTATE_STRU;
*/

void eraseHeadSector(void);
void eraseDataSector(void);
void eraseUserDataAll(void);
uint8_t writeHeader(uint8_t* header,uint8_t mode);
uint8_t searchHeaderIndex(uint8_t* header,uint8_t mode,uint16_t *index);
uint8_t writeUserData(USERDATA_STRU userData,uint8_t mode);
uint8_t readUserData(uint8_t* header,uint8_t mode,USERDATA_STRU *userData);
uint8_t modifyUserData(USERDATA_STRU userData,uint8_t mode);

void TestFlash(uint8_t mode);






#endif /* __LOCALDATA_H__ */
