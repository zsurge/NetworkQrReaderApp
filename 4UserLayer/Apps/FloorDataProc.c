/******************************************************************************

                  版权所有 (C), 2013-2023, 深圳博思高科技有限公司

 ******************************************************************************
  文 件 名   : FloorDataProc.c
  版 本 号   : 初稿
  作    者   : 张舵
  生成日期   : 2019年12月23日
  最近修改   :
  功能描述   : 电梯控制器的指令处理文件
  函数列表   :
  修改历史   :
  1.日    期   : 2019年12月23日
    作    者   : 张舵
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include "FloorDataProc.h"
#include "jsonUtils.h"
#include "LocalData.h"
#include "bsp_ds1302.h"



#define LOG_TAG    "FloorData"
#include "elog.h"


/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define AUTO_REG            1
#define MANUAL_REG          2

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/


/*----------------------------------------------*
 * 内部函数原型说明                             *
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
                return ;  //无权限
            }

            //1.发给电梯的数据
            packetToElevator(&localUserData,buf);

            //2.发给服务器
            packetPayload(&localUserData,jsonBuf); 

            len = strlen((const char*)jsonBuf);

            len = mqttSendData(jsonBuf,len);
            log_d("send = %d\r\n",len);            
            break;
        case AUTH_MODE_REMOTE:
            //直接发送目标楼层
            log_d("send desc floor\r\n");
            break;
        case AUTH_MODE_UNBIND:
            //直接发送停用设备指令
            log_d("send AUTH_MODE_UNBIND floor\r\n");
            break;
        case AUTH_MODE_BIND:
            //直接发送启动设置指令
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
    char *buf[6] = {0}; //存放分割后的子字符串 
    int num = 0;
    uint8_t key[8+1] = {0};    
    uint8_t timeStamp[16] = {0};

    memset(key,0x00,sizeof(key));

    if(pQueue->authMode == AUTH_MODE_QR) 
    {
        //二维码
        log_d("pQueue->data = %s\r\n",pQueue->data);
        strcpy((char *)key,(const char *)GetJsonItem((const uint8_t *)pQueue->data,(const uint8_t *)"ownerId",0));
        strcpy((char *)timeStamp,(const char *)GetJsonItem((const uint8_t *)pQueue->data,(const uint8_t *)"datetime",0));
    }
    else
    {
        //读卡，-2 是减掉0D 0A
        memcpy(key,pQueue->data+pQueue->dataLen-2-CARD_NO_LEN,CARD_NO_LEN);
    }

   
    log_d("key = %s\r\n",key);
    
    memset(value,0x00,sizeof(value));

    val_len = ef_get_env_blob(key, value, sizeof(value) , NULL);
   

    log_d("get env = %s,val_len = %d\r\n",value,val_len);

    if(val_len <= 0)
    {
        //未找到记录，无权限
        log_d("not find record\r\n");
        return NO_AUTHARITY_ERR;
    }

    split(value,";",buf,&num); //调用函数进行分割 
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
        //二维码
        log_d("pQueue->data = %s\r\n",pQueue->data);
        
        isFind = parseQrCode(pQueue->data,&qrCodeInfo);

        log_d("qrCodeInfo->startTime= %s\r\n",qrCodeInfo.startTime); 
        log_d("qrCodeInfo->endTime= %s\r\n",qrCodeInfo.endTime);         
        log_d("qrCodeInfo->qrStarttimeStamp= %s\r\n",qrCodeInfo.qrStarttimeStamp); 
        log_d("qrCodeInfo->qrEndtimeStamp= %s\r\n",qrCodeInfo.qrEndtimeStamp); 

        log_d("isfind = %d\r\n",isFind);      

        if(isFind == 0)
        {
            //未找到记录，无权限
            log_d("not find record\r\n");
            return NO_AUTHARITY_ERR;
        }        

        timestamp_to_time(atoi(qrCodeInfo.qrStarttimeStamp));        
        timestamp_to_time(atoi(qrCodeInfo.qrEndtimeStamp)); 
        
        localUserData->authMode = pQueue->authMode; 
        //localUserData->defaultFloor = qrCodeInfo.tagFloor;   
        localUserData->qrType = qrCodeInfo.type;   
        memcpy(localUserData->qrID,qrCodeInfo.qrID,QRID_LEN); 
        memcpy(localUserData->startTime,qrCodeInfo.startTime,TIME_LEN);
        memcpy(localUserData->endTime,qrCodeInfo.endTime,TIME_LEN); 
        memcpy(localUserData->timeStamp,time_to_timestamp(),TIMESTAMP_LEN); 
        memcpy(localUserData->accessFloor,qrCodeInfo.accessFloor,FLOOR_ARRAY_LEN); 
    }
    else
    {
        //读卡 CARD 230000000089E1E35D,23       
    
        memcpy(key,pQueue->data+pQueue->dataLen-CARD_NO_LEN,CARD_NO_LEN);
        log_d("key = %s\r\n",key);
        isFind = readUserData(key,CARD_MODE,&rUserData);   

        log_d("isFind = %d,rUserData.cardState = %d\r\n",isFind,rUserData.cardState);

        if(rUserData.cardState != CARD_VALID || isFind != 0)
        {
            //未找到记录，无权限
            log_d("not find record\r\n");
            return NO_AUTHARITY_ERR;
        } 
        
        localUserData->qrType = 2;
        localUserData->authMode = pQueue->authMode; 
        localUserData->defaultFloor = rUserData.defaultFloor;
        memcpy(localUserData->userId,rUserData.userId,CARD_USER_LEN);        
        memcpy(localUserData->cardNo,rUserData.cardNo,CARD_USER_LEN); 
        memcpy(localUserData->accessFloor,rUserData.accessFloor,FLOOR_ARRAY_LENGTH);    
        memcpy(localUserData->startTime,rUserData.startTime,TIME_LENGTH);
        memcpy(localUserData->endTime,rUserData.endTime,TIME_LENGTH);  
        memcpy(localUserData->timeStamp,time_to_timestamp(),TIMESTAMP_LEN);
        log_d("localUserData->timeStamp = %s\r\n",localUserData->timeStamp);         
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
    char *buf[6] = {0}; //存放分割后的子字符串 
    int num = 0;
    uint8_t key[8+1] = {0};    

    memset(key,0x00,sizeof(key));   
    
    memset(value,0x00,sizeof(value));

    val_len = ef_get_env_blob((const char*)key, value, sizeof(value) , NULL);
   

    log_d("get env = %s,val_len = %d\r\n",value,val_len);

    if(val_len <= 0)
    {
        //未找到记录，无权限
        log_d("not find record\r\n");
        return NO_AUTHARITY_ERR;
    }

    split(value,";",buf,&num); //调用函数进行分割 
    log_d("num = %d\r\n",num);

    if(num != 5)
    {
        log_d("read record error\r\n");
        return READ_RECORD_ERR;       
    }

    localUserData->authMode = pQueue->authMode;    
    
    if(AUTH_MODE_QR == pQueue->authMode)
    {
        strcpy((char*)localUserData->userId,(const char*)key);
        
        strcpy((char*)localUserData->cardNo,buf[0]);        
    }
    else
    {
        memcpy(localUserData->cardNo,key,CARD_NO_LEN);

        log_d("buf[0] = %s\r\n",buf[0]);
        strcpy((char*)localUserData->userId,buf[0]);        
    }   

    //3867;0;0;2019-12-29;2029-12-31
    
    
    strcpy((char*)localUserData->accessFloor,buf[1]);
    localUserData->defaultFloor = atoi(buf[2]);
    strcpy((char*)localUserData->startTime,buf[3]);
    strcpy((char*)localUserData->endTime,buf[4]);    



    log_d("localUserData->cardNo = %s\r\n",localUserData->cardNo);
    log_d("localUserData->userId = %s\r\n",localUserData->userId);
    log_d("localUserData->accessLayer = %s\r\n",localUserData->accessFloor);
    log_d("localUserData->defaultLayer = %d\r\n",localUserData->defaultFloor);    
    log_d("localUserData->startTime = %s\r\n",localUserData->startTime);        
    log_d("localUserData->endTime = %s\r\n",localUserData->endTime);        
    log_d("localUserData->authMode = %d\r\n",localUserData->authMode);

    return result;

}

static SYSERRORCODE_E packetToElevator(LOCAL_USER_STRU *localUserData,uint8_t *buff)
{
    SYSERRORCODE_E result = NO_ERR;
    uint8_t tmpBuf[MAX_SEND_LEN+1] = {0};
    char authLayer[64] = {0}; //权限楼层，最多64层
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

//    split(localUserData->accessFloor,",",authLayer,&num); //调用函数进行分割 

    memcpy(authLayer,localUserData->accessFloor,FLOOR_ARRAY_LEN);
    num = strlen((const char*)authLayer);

    dbh("accessfloor",authLayer, 64);

    memset(sendBuf,0x00,sizeof(sendBuf));
    
    if(num > 1)//多层权限
    {
        for(i=0;i<num;i++)
        {
            calcFloor(authLayer[i],MANUAL_REG,sendBuf,tmpBuf);
            memcpy(sendBuf,tmpBuf,MAX_SEND_LEN);
        }        
    }
    else    //单层权限，直接呼默认权限楼层
    {
        floor = authLayer[0];      

        calcFloor(floor,AUTO_REG,sendBuf,tmpBuf);
        dbh("tmpBuf", tmpBuf, MAX_SEND_LEN);        
    }   

    memset(sendBuf,0x00,sizeof(sendBuf));
    
    sendBuf[0] = CMD_STX;
    sendBuf[1] = bsp_dipswitch_read();
    memcpy(sendBuf+2,tmpBuf,MAX_SEND_LEN-5);        
    sendBuf[MAX_SEND_LEN-1] = xorCRC(sendBuf,MAX_SEND_LEN-2);        
    memcpy(buff,sendBuf,MAX_SEND_LEN);  

    dbh("sendBuf", (char *)sendBuf, MAX_SEND_LEN);
    
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
    else //1~7层和非8层的倍数
    {
        sendBuf[index] = setbit(sendBuf[index],remainder-1);
    }

    memcpy(outFloor,sendBuf,MAX_SEND_LEN);

//    dbh("after", sendBuf, MAX_SEND_LEN);
}





