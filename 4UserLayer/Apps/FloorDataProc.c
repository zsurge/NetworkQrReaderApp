/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : FloorDataProc.c
  �� �� ��   : ����
  ��    ��   : �Ŷ�
  ��������   : 2019��12��23��
  ����޸�   :
  ��������   : ���ݿ�������ָ����ļ�
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2019��12��23��
    ��    ��   : �Ŷ�
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include "FloorDataProc.h"
#include "jsonUtils.h"
#include "LocalData.h"
#include "bsp_ds1302.h"


#define LOG_TAG    "FloorData"
#include "elog.h"


/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#define AUTO_REG            1
#define MANUAL_REG          2

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/


/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/
static SYSERRORCODE_E packetToElevator(LOCAL_USER_STRU *localUserData,uint8_t *buff);
static void calcFloor(uint8_t layer,uint8_t regMode,uint8_t *src,uint8_t *outFloor);
static SYSERRORCODE_E authReader(READER_BUFF_STRU *pQueue,LOCAL_USER_STRU *localUserData);


void packetDefaultSendBuf(uint8_t *buf)
{
    uint8_t sendBuf[64] = {0};

    sendBuf[0] = CMD_STX;
    sendBuf[1] = 0;//bsp_dipswitch_read();
    sendBuf[MAX_SEND_LEN-1] = xorCRC(sendBuf,MAX_SEND_LEN-2);

    memcpy(buf,sendBuf,MAX_SEND_LEN);
}


void packetSendBuf(READER_BUFF_STRU *pQueue,uint8_t *buf)
{
    uint8_t jsonBuf[JSON_ITEM_MAX_LEN] = {0};
    uint8_t sendBuf[64] = {0};
    uint16_t len = 0;
    uint16_t ret = 0;
    LOCAL_USER_STRU localUserData= {0};
    memset(&localUserData,0x00,sizeof(LOCAL_USER_STRU));
    
    sendBuf[0] = CMD_STX;
    sendBuf[1] = 0;//bsp_dipswitch_read();
    sendBuf[MAX_SEND_LEN-1] = xorCRC(sendBuf,MAX_SEND_LEN-2);
    log_d("card or QR data = %s\r\n",pQueue->data);

    switch(pQueue->authMode)
    {
        case AUTH_MODE_CARD:
        case AUTH_MODE_QR:
            log_d("card or QR auth,pQueue->authMode = %d\r\n",pQueue->authMode);
            ret = authReader(pQueue,&localUserData);    

            if(ret != NO_ERR)
            {
                log_d("reject access\r\n");
                return ;  //��Ȩ��
            }

            //1.�������ݵ�����
            packetToElevator(&localUserData,buf);

            //2.����������
            packetPayload(&localUserData,jsonBuf); 

            len = strlen(jsonBuf);

            len = mqttSendData(jsonBuf,len);
            log_d("send = %d\r\n",len);            
            break;
        case AUTH_MODE_REMOTE:
            //ֱ�ӷ���Ŀ��¥��
            log_d("send desc floor\r\n");
            break;
        case AUTH_MODE_UNBIND:
            //ֱ�ӷ���ͣ���豸ָ��
            log_d("send AUTH_MODE_UNBIND floor\r\n");
            break;
        case AUTH_MODE_BIND:
            //ֱ�ӷ�����������ָ��
            log_d("send AUTH_MODE_BIND floor\r\n");
            break;

        default:
            log_d("invalid authMode\r\n");
            break;    
   }

}
#if 0
SYSERRORCODE_E authReader(READER_BUFF_STRU *pQueue,LOCAL_USER_STRU *localUserData)
{
    SYSERRORCODE_E result = NO_ERR;
    char value[128] = {0};
    int val_len = 0;
    char *buf[6] = {0}; //��ŷָ������ַ��� 
    int num = 0;
    uint8_t key[8+1] = {0};    
    uint8_t timeStamp[16] = {0};

    memset(key,0x00,sizeof(key));

    if(pQueue->authMode == AUTH_MODE_QR) 
    {
        //��ά��
        log_d("pQueue->data = %s\r\n",pQueue->data);
        strcpy((char *)key,(const char *)GetJsonItem((const uint8_t *)pQueue->data,(const uint8_t *)"ownerId",0));
        strcpy((char *)timeStamp,(const char *)GetJsonItem((const uint8_t *)pQueue->data,(const uint8_t *)"datetime",0));
    }
    else
    {
        //������-2 �Ǽ���0D 0A
        memcpy(key,pQueue->data+pQueue->dataLen-2-CARD_NO_LEN,CARD_NO_LEN);
    }

   
    log_d("key = %s\r\n",key);
    
    memset(value,0x00,sizeof(value));

    val_len = ef_get_env_blob(key, value, sizeof(value) , NULL);
   

    log_d("get env = %s,val_len = %d\r\n",value,val_len);

    if(val_len <= 0)
    {
        //δ�ҵ���¼����Ȩ��
        log_d("not find record\r\n");
        return NO_AUTHARITY_ERR;
    }

    split(value,";",buf,&num); //���ú������зָ� 
    log_d("num = %d\r\n",num);

    if(num != 5)
    {
        log_d("read record error\r\n");
        return READ_RECORD_ERR;       
    }

    localUserData->authMode = pQueue->authMode;    
    
    if(AUTH_MODE_QR == pQueue->authMode)
    {
        strcpy(localUserData->userId,key);
        
        strcpy(localUserData->cardNo,buf[0]);      
        strcpy(localUserData->timeStamp,timeStamp);   
    }
    else
    {
        memcpy(localUserData->cardNo,key,CARD_NO_LEN);

        log_d("buf[0] = %s\r\n",buf[0]);
        strcpy(localUserData->userId,buf[0]);        
    }   

    //3867;0;0;2019-12-29;2029-12-31
    
    
    strcpy(localUserData->accessFloor,buf[1]);
    localUserData->defaultFloor = atoi(buf[2]);
    strcpy(localUserData->startTime,buf[3]);
    strcpy(localUserData->endTime,buf[4]);    


    log_d("localUserData->cardNo = %s\r\n",localUserData->cardNo);
    log_d("localUserData->userId = %s\r\n",localUserData->userId);
    log_d("localUserData->accessLayer = %s\r\n",localUserData->accessFloor);
    log_d("localUserData->defaultLayer = %d\r\n",localUserData->defaultFloor);    
    log_d("localUserData->startTime = %s\r\n",localUserData->startTime);        
    log_d("localUserData->endTime = %s\r\n",localUserData->endTime);        
    log_d("localUserData->authMode = %d\r\n",localUserData->authMode);
    log_d("localUserData->timeStamp = %s\r\n",localUserData->timeStamp);

    return result;
}
#endif

SYSERRORCODE_E authReader(READER_BUFF_STRU *pQueue,LOCAL_USER_STRU *localUserData)
{
    SYSERRORCODE_E result = NO_ERR;
    uint8_t key[9] = {0};  
    uint8_t isFind = 0;
    
    USERDATA_STRU rUserData = {0};
    QRCODE_INFO_STRU qrCodeInfo = {0};

    memset(key,0x00,sizeof(key));
    memset(&rUserData,0x00,sizeof(USERDATA_STRU));        
    memset(&qrCodeInfo,0x00,sizeof(QRCODE_INFO_STRU));

    
    log_d("card or QR data = %s\r\n",pQueue->data);
    
    if(pQueue->authMode == AUTH_MODE_QR) 
    {
        //��ά��
        log_d("pQueue->data = %s\r\n",pQueue->data);
        
        isFind = parseQrCode(pQueue->data,&qrCodeInfo);

        log_d("qrCodeInfo->startTime= %s\r\n",qrCodeInfo.startTime); 
        log_d("qrCodeInfo->endTime= %s\r\n",qrCodeInfo.endTime);         
        log_d("qrCodeInfo->qrStarttimeStamp= %s\r\n",qrCodeInfo.qrStarttimeStamp); 
        log_d("qrCodeInfo->qrEndtimeStamp= %s\r\n",qrCodeInfo.qrEndtimeStamp); 

        log_d("isfind = %d\r\n",isFind);      

        if(isFind == 0)
        {
            //δ�ҵ���¼����Ȩ��
            log_d("not find record\r\n");
            return NO_AUTHARITY_ERR;
        }        

        timestamp_to_time(atoi(qrCodeInfo.qrStarttimeStamp));        
        timestamp_to_time(atoi(qrCodeInfo.qrEndtimeStamp)); 
        
        localUserData->authMode = pQueue->authMode; 
        localUserData->defaultFloor = qrCodeInfo.tagFloor;   
        localUserData->qrType = qrCodeInfo.type;   
        memcpy(localUserData->qrID,qrCodeInfo.qrID,QRID_LEN); 
        memcpy(localUserData->startTime,qrCodeInfo.startTime,TIME_LEN);
        memcpy(localUserData->endTime,qrCodeInfo.endTime,TIME_LEN); 
        memcpy(localUserData->timeStamp,time_to_timestamp(),TIMESTAMP_LEN); 
    }
    else
    {
        //���� CARD 230000000089E1E35D,23
    
        memcpy(key,pQueue->data+pQueue->dataLen-CARD_NO_LEN,CARD_NO_LEN);
        log_d("key = %s\r\n",key);
        isFind = readUserData(key,CARD_MODE,&rUserData);   

        log_d("isFind = %d,rUserData.cardState = %d\r\n",isFind,rUserData.cardState);

        if(rUserData.cardState != CARD_VALID || isFind != 0)
        {
            //δ�ҵ���¼����Ȩ��
            log_d("not find record\r\n");
            return NO_AUTHARITY_ERR;
        } 
        
        localUserData->qrType = 4;
        localUserData->authMode = pQueue->authMode; 
        localUserData->defaultFloor = rUserData.defaultFloor;
        memcpy(localUserData->userId,rUserData.userId,CARD_USER_LEN);        
        memcpy(localUserData->cardNo,rUserData.cardNo,CARD_USER_LEN);      
        memcpy(localUserData->timeStamp,time_to_timestamp(),TIMESTAMP_LEN);
    log_d("localUserData->timeStamp = %s\r\n",localUserData->timeStamp);        
        memcpy(localUserData->accessFloor,rUserData.accessFloor,FLOOR_ARRAY_LENGTH);    
        memcpy(localUserData->startTime,rUserData.startTime,TIME_LENGTH);
        memcpy(localUserData->endTime,rUserData.endTime,TIME_LENGTH);            
    }

    log_d("localUserData->cardNo = %s\r\n",localUserData->cardNo);
    log_d("localUserData->userId = %s\r\n",localUserData->userId);
    log_d("localUserData->accessLayer = %s\r\n",localUserData->accessFloor);
    log_d("localUserData->defaultLayer = %d\r\n",localUserData->defaultFloor);    
    log_d("localUserData->startTime = %s\r\n",localUserData->startTime);        
    log_d("localUserData->endTime = %s\r\n",localUserData->endTime);        
    log_d("localUserData->authMode = %d\r\n",localUserData->authMode);
    log_d("localUserData->timeStamp = %s\r\n",localUserData->timeStamp);
    log_d("localUserData->qrID = %s\r\n",localUserData->qrID);

    return result;
}




SYSERRORCODE_E authRemote(READER_BUFF_STRU *pQueue,LOCAL_USER_STRU *localUserData)
{
    SYSERRORCODE_E result = NO_ERR;
    char value[128] = {0};
    int val_len = 0;
    char *buf[6] = {0}; //��ŷָ������ַ��� 
    int num = 0;
    uint8_t key[8+1] = {0};    

    memset(key,0x00,sizeof(key));   
    
    memset(value,0x00,sizeof(value));

    val_len = ef_get_env_blob(key, value, sizeof(value) , NULL);
   

    log_d("get env = %s,val_len = %d\r\n",value,val_len);

    if(val_len <= 0)
    {
        //δ�ҵ���¼����Ȩ��
        log_d("not find record\r\n");
        return NO_AUTHARITY_ERR;
    }

    split(value,";",buf,&num); //���ú������зָ� 
    log_d("num = %d\r\n",num);

    if(num != 5)
    {
        log_d("read record error\r\n");
        return READ_RECORD_ERR;       
    }

    localUserData->authMode = pQueue->authMode;    
    
    if(AUTH_MODE_QR == pQueue->authMode)
    {
        strcpy(localUserData->userId,key);
        
        strcpy(localUserData->cardNo,buf[0]);        
    }
    else
    {
        memcpy(localUserData->cardNo,key,CARD_NO_LEN);

        log_d("buf[0] = %s\r\n",buf[0]);
        strcpy(localUserData->userId,buf[0]);        
    }   

    //3867;0;0;2019-12-29;2029-12-31
    
    
    strcpy(localUserData->accessFloor,buf[1]);
    localUserData->defaultFloor = atoi(buf[2]);
    strcpy(localUserData->startTime,buf[3]);
    strcpy(localUserData->endTime,buf[4]);    



    log_d("localUserData->cardNo = %s\r\n",localUserData->cardNo);
    log_d("localUserData->userId = %s\r\n",localUserData->userId);
    log_d("localUserData->accessLayer = %s\r\n",localUserData->accessFloor);
    log_d("localUserData->defaultLayer = %d\r\n",localUserData->defaultFloor);    
    log_d("localUserData->startTime = %s\r\n",localUserData->startTime);        
    log_d("localUserData->endTime = %s\r\n",localUserData->endTime);        
    log_d("localUserData->authMode = %d\r\n",localUserData->authMode);

    return NO_ERR;

}

static SYSERRORCODE_E packetToElevator(LOCAL_USER_STRU *localUserData,uint8_t *buff)
{
    SYSERRORCODE_E result = NO_ERR;
    uint8_t oneLayer[8] = {0};
    uint8_t tmpBuf[MAX_SEND_LEN+1] = {0};
//    char *authLayer[64] = {0}; //Ȩ��¥�㣬���64��
    int num = 0;    
    uint8_t sendBuf[MAX_SEND_LEN+1] = {0};

    uint8_t floor = 0;

    uint8_t div = 0;
    uint8_t remainder = 0;
    uint8_t i = 0;

    
    log_d("localUserData->cardNo = %s\r\n",localUserData->cardNo);
    log_d("localUserData->userId = %s\r\n",localUserData->userId);
    log_d("localUserData->accessLayer = %s\r\n",localUserData->accessFloor);
    log_d("localUserData->defaultLayer = %d\r\n",localUserData->defaultFloor);    
    log_d("localUserData->startTime = %s\r\n",localUserData->startTime);        
    log_d("localUserData->endTime = %s\r\n",localUserData->endTime);        
    log_d("localUserData->authMode = %d\r\n",localUserData->authMode);    

//    split(localUserData->accessFloor,",",authLayer,&num); //���ú������зָ� 

//    log_d("num = %d\r\n",num);

    memset(sendBuf,0x00,sizeof(sendBuf));
    
//    if(num > 1)//���Ȩ��
//    {
//        for(i=0;i<num;i++)
//        {
//            calcFloor(atoi(authLayer[i]),MANUAL_REG,sendBuf,tmpBuf);
//            memcpy(sendBuf,tmpBuf,MAX_SEND_LEN);
//        }        
//    }
//    else    //����Ȩ�ޣ�ֱ�Ӻ�Ĭ��Ȩ��¥��
//    {
//        floor = atoi(authLayer[0]);      

//        calcFloor(floor,AUTO_REG,sendBuf,tmpBuf);
//        dbh("tmpBuf", tmpBuf, MAX_SEND_LEN);        
//    }
   
    calcFloor(localUserData->defaultFloor,AUTO_REG,sendBuf,tmpBuf);

    memset(sendBuf,0x00,sizeof(sendBuf));
    sendBuf[0] = CMD_STX;
    sendBuf[1] = 0;//bsp_dipswitch_read();
    memcpy(sendBuf+2,tmpBuf,MAX_SEND_LEN-5);        
    sendBuf[MAX_SEND_LEN-1] = xorCRC(sendBuf,MAX_SEND_LEN-2);        
    memcpy(buff,sendBuf,MAX_SEND_LEN);  

    dbh("sendBuf", sendBuf, MAX_SEND_LEN);
    
    return result;
}



static void calcFloor(uint8_t layer,uint8_t regMode,uint8_t *src,uint8_t *outFloor)
{
    uint8_t div = 0;
    uint8_t remainder = 0;
    uint8_t floor = layer;
    uint8_t sendBuf[MAX_SEND_LEN+1] = {0};
    uint8_t tmpFloor = 0;
    uint8_t index = 0;
    
    memcpy(sendBuf,src,MAX_SEND_LEN);

//    dbh("before", sendBuf, MAX_SEND_LEN);
        
    div = floor / 8;
    remainder = floor % 8;

    if(regMode == AUTO_REG)
    {
        index = div + 8;
    }
    else
    {
        index = div;
    }

    log_d("div = %d,remain = %d\r\n",div,remainder);
    

    if(div != 0 && remainder == 0)// 8,16,24
    {       
        sendBuf[index-1] = setbit(sendBuf[index-1],8-1);
    } 
    else //1~7��ͷ�8��ı���
    {
        sendBuf[index] = setbit(sendBuf[index],remainder-1);
    }

    memcpy(outFloor,sendBuf,MAX_SEND_LEN);

//    dbh("after", sendBuf, MAX_SEND_LEN);
}




