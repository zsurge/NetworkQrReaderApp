/******************************************************************************

                  ��Ȩ���� (C), 2013-2023, ���ڲ�˼�߿Ƽ����޹�˾

 ******************************************************************************
  �� �� ��   : ini.c
  �� �� ��   : ����
  ��    ��   : �Ŷ�
  ��������   : 2019��5��28��
  ����޸�   :
  ��������   : ��������
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2019��5��28��
    ��    ��   : �Ŷ�
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include "ini.h"
#include "easyflash.h"
#include "version.h"
#include "string.h"
#include "templateprocess.h"
#include "tool.h"
#include "LocalData.h"


#define LOG_TAG    "ini"
#include "elog.h"



/*****************************************************************************
 �� �� ��  : RestoreDefaultSetting
 ��������  : �ָ�Ĭ������,ʵ����д��ϵͳ��������Ҫ�Ĳ���
           ��ʹ��ef_env_set_default ����Ҫ��ef_port.c�ж�Ĭ��ֵ�����޸�
 �������  : 
             
 �������  : ��
 �� �� ֵ  : void
 
 �޸���ʷ      :
  1.��    ��   : 2019��5��28��
    ��    ��   : �Ŷ�

    �޸�����   : �����ɺ���

*****************************************************************************/
void RestoreDefaultSetting(void)
{
    if(ef_env_set_default()== EF_NO_ERR)
    {
        DBG("RestoreDefaultSetting success\r\n");
    }
    
}

void SystemUpdate(void)
{
    //д������־λ
    if(ef_set_env("WSPIFLASH","5050") == EF_NO_ERR)
    {
        ef_set_env("WMCUFLASH","6060");
        //jump iap
        DBG("jump iap\r\n");
        NVIC_SystemReset();
    }
}

void readTemplateData(void)
{
    char tempBuff[8] = {0};
    int valueLen = 0;
    int index = 0;
    char tmpIndex[2] = {0};
    char tmpKey[32] = {0};
    char *callingWay[4] = {0}; //��ŷָ������ַ��� 
    int callingWayNum = 0;

    memset(&gTemplateParam,0x00,sizeof(gTemplateParam));
    TEMPLATE_PARAM_STRU *templateParam = &gTemplateParam; 


    //��ȡģ��״̬
    memset(tempBuff,0x00,sizeof(tempBuff));
    valueLen = ef_get_env_blob("templateStatus", tempBuff, sizeof(tempBuff) , NULL);
    templateParam->templateStatus = atoi(tempBuff);
    
    //��ȡ����ģʽ���Ƿ񿪽ڼ���ģʽ
    valueLen = ef_get_env_blob("modeType", templateParam->modeType, sizeof(templateParam->modeType) , NULL);
    if(valueLen == 0)
    {
        log_d("get templateParam error!\r\n");
    }   

    //��ȡģ���ʶ��ģʽ
    valueLen = 0;
    valueLen = ef_get_env_blob("T_callingWay", templateParam->callingWay, sizeof(templateParam->callingWay) , NULL);

    //����ģ��ʶ��ʽ    
    split((char *)templateParam->callingWay,",",callingWay,&callingWayNum); //���ú������зָ� 
    while(callingWayNum)
    {
        switch(atoi(callingWay[callingWayNum-1]))
        {
            case 1:
                templateParam->templateCallingWay.isFace = 1;
                break;
            case 2:
                templateParam->templateCallingWay.isQrCode = 1;
                break;
            case 3:
                templateParam->templateCallingWay.isIcCard = 1;
                break; 
        }
        callingWayNum-- ;  
    }    
    

    //��ȡ�ڼ��յ�ʶ��ģʽ
    valueLen = 0;
    valueLen = ef_get_env_blob("peakCallingWay", templateParam->peakInfo[0].callingWay, sizeof(templateParam->peakInfo[0].callingWay) , NULL);

    //����ʶ��ʽ    
    memset(callingWay,0x00,sizeof(callingWay));
    callingWayNum = 0;
    split((char *)templateParam->peakInfo[0].callingWay,",",callingWay,&callingWayNum); //���ú������зָ� 
    while(callingWayNum)
    {
        switch(atoi(callingWay[callingWayNum-1]))
        {
            case 1:
                templateParam->peakCallingWay.isFace = 1;
                break;
            case 2:
                templateParam->peakCallingWay.isQrCode = 1;
                break;
            case 3:
                templateParam->peakCallingWay.isIcCard = 1;
                break; 
        }
        callingWayNum-- ;  
    }  


    //�����Ƿ����ڼ���ģʽ
    memset(callingWay,0x00,sizeof(callingWay));
    callingWayNum = 0;
    split((char *)templateParam->modeType,",",callingWay,&callingWayNum); //���ú������зָ� 
    while(callingWayNum)
    {
        switch(atoi(callingWay[callingWayNum-1]))
        {
            case 1:
                templateParam->workMode.isPeakMode = 1;
                break;
            case 2:
                templateParam->workMode.isHolidayMode = 1;
                break;
            case 5:
                templateParam->workMode.isNormalMode = 1;
                break; 
        }
        callingWayNum-- ;  
    }  
    

    //��ȡ�ڼ����ų����ڵ�����,1������2���գ�3�ڼ��գ�Ŀǰ�ڼ����޷����Σ�
    valueLen = 0;
    valueLen = ef_get_env_blob("peakHolidaysType", templateParam->peakInfo[0].outsideTimeMode, sizeof(templateParam->peakInfo[0].outsideTimeMode) , NULL);

    //��ȡģ����Ч��ʼ����
    valueLen = 0;
    valueLen = ef_get_env_blob("peakStartDate", templateParam->peakInfo[0].beginTime, sizeof(templateParam->peakInfo[0].beginTime) , NULL);

    //��ȡģ����Ч��������
    valueLen = 0;
    valueLen = ef_get_env_blob("peakEndDate", templateParam->peakInfo[0].endTime, sizeof(templateParam->peakInfo[0].endTime) , NULL);

    //���ж��м���ʱ���
    memset(tempBuff,0x00,sizeof(tempBuff));
    valueLen = ef_get_env_blob("holidayTimeMapCnt", tempBuff, sizeof(tempBuff) , NULL);
    templateParam->peakModeCnt = atoi(tempBuff);
    
    
    memset(tmpKey,0x00,sizeof(tmpKey));
    memset(tmpIndex,0x00,sizeof(tmpIndex));  
    for(index= 0;index<templateParam->peakModeCnt;index++)
    {
        //��ȡ�ڼ���ģ��Ĳ��ܿ�ʱ���--��ʼʱ��
        sprintf(tmpIndex,"%d",index);
        strcpy(tmpKey,"hoildayModeStartTime");
        strcat(tmpKey,tmpIndex); 
 

        valueLen = ef_get_env_blob(tmpKey, templateParam->holidayMode[index].startTime, sizeof(templateParam->holidayMode[index].startTime) , NULL);
        log_d("gTemplateParam->%s = %s\r\n",tmpKey,gTemplateParam.holidayMode[index].startTime);
        

         //��ȡ�ڼ���ģ��Ĳ��ܿ�ʱ���--����ʱ��
        memset(tmpKey,0x00,sizeof(tmpKey));
        strcpy(tmpKey,"hoildayModeEndTime");
        strcat(tmpKey,tmpIndex);  
        valueLen = 0;
        valueLen = ef_get_env_blob(tmpKey, templateParam->holidayMode[index].endTime, sizeof(templateParam->holidayMode[index].endTime) , NULL);
        log_d("gTemplateParam->%s= %s\r\n",tmpKey,gTemplateParam.holidayMode[index].endTime);        
        
     }


    log_d("gTemplateParam->peakInfo[0].beginTime = %s\r\n",gTemplateParam.peakInfo[0].beginTime);      
    log_d("gTemplateParam->peakInfo[0].endTime = %s\r\n",gTemplateParam.peakInfo[0].endTime);      
    

    log_d("gTemplateParam->templateCallingWay = %d\r\n",gTemplateParam.templateCallingWay.isFace);      
    log_d("gTemplateParam->templateCallingWay = %d\r\n",gTemplateParam.templateCallingWay.isQrCode);      
    log_d("gTemplateParam->templateCallingWay = %d\r\n",gTemplateParam.templateCallingWay.isIcCard);
    
    log_d("gTemplateParam->peakCallingWay = %d\r\n",gTemplateParam.peakCallingWay.isFace);   
    log_d("gTemplateParam->peakCallingWay = %d\r\n",gTemplateParam.peakCallingWay.isQrCode);
    log_d("gTemplateParam->peakCallingWay = %d\r\n",gTemplateParam.peakCallingWay.isIcCard);      
    

    log_d("gTemplateParam->templateStatus = %d\r\n",gTemplateParam.templateStatus);      
    log_d("gTemplateParam->modeType = %s\r\n",gTemplateParam.modeType);
    log_d("gTemplateParam->callingWay = %s\r\n",gTemplateParam.callingWay);    
    log_d("gTemplateParam->peakInfo[0].callingWay = %s\r\n",gTemplateParam.peakInfo[0].callingWay);
    log_d("gTemplateParam->peakInfo[0].beginTime = %s\r\n",gTemplateParam.peakInfo[0].beginTime);
    log_d("gTemplateParam->peakInfo[0].endTime = %s\r\n",gTemplateParam.peakInfo[0].endTime);
    log_d("gTemplateParam->peakInfo[0].outsideTimeMode = %s\r\n",gTemplateParam.peakInfo[0].outsideTimeMode);
    
}



void readCardAndUserIdIndex(void)
{
    uint8_t tempBuff[6] = {0};
    
    ef_get_env_blob("CardHeaderIndex", tempBuff, sizeof(tempBuff) , NULL);
    gCurCardHeaderIndex = atoi((const char *)tempBuff);
    log_d("readCardAndUserIdIndex = %d\r\n",gCurCardHeaderIndex);

//    memset(tempBuff,0x00,sizeof(tempBuff));
//    ef_get_env_blob("UserHeaderIndex", tempBuff, sizeof(tempBuff) , NULL);
//    gCurUserHeaderIndex = atoi(tempBuff);  
//    log_d("gCurUserHeaderIndex = %d\r\n",gCurUserHeaderIndex);
}



