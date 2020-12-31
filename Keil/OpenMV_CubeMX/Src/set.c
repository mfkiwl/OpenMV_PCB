/* Includes ------------------------------------------------------------------*/
#include "set.h"

/* Private variables ---------------------------------------------------------*/
uint8_t  *PdaRecvBuffer = NULL;
uint8_t  PdaRecvBuffer1[PDA_MSG_LENGTH];
uint16_t PdaRecvCount1 = 0;
uint8_t  PdaRecvOverflowFlag = 0;

uint8_t  PdaMsgBuffer[PDA_MSG_LENGTH];
uint16_t PdaMsgCount = 0;

const uint32_t Bo_Te_Lv[] = {300,600,1200,2400,4800,9600,19200,38400,56000,57600,115200,230400};

const settingParamDef_t    globalSettingParamDefault =
{
    0xA0A0, //uint16_t    u16Inited;              //flash��ʼ����־
    0121,   //uint16_t    u16BuildNumber;         //����汾

    115200,   //uint32_t    u32BaudRate;          //������
    0,      //uint16_t    u16_485_232;            //0:232 , 1:485
    1,      //uint16_t    u16DevAddr;             //�豸��ַ
    0x01,   //uint16_t    u16HardwareVersion;     //Ӳ���汾��Ϣ
    
    0,      //uint8_t     u8WorkMode;             //--0����״̬ --1����״̬
};

settingParamDef_t globalSettingParam = {0};


#define AT_CMD_TAB_SIZE     sizeof(atCmdTab)/sizeof(AT_Cmd_Tab_T)
static AT_Cmd_Tab_T atCmdTab[] =
{
    //FLASH �����б�0-100
    {0,         PAE_BOTH,        PTE_U32,            (void*)&(globalSettingParam.u32BaudRate)}
    ,{1,        PAE_BOTH,        PTE_U16,            (void*)&(globalSettingParam.u16_485_232)}
    ,{2,        PAE_BOTH,        PTE_U16,            (void*)&(globalSettingParam.u16DevAddr)}
    ,{5,        PAE_BOTH,        PTE_U16,            (void*)&(globalSettingParam.u16HardwareVersion)}

    //FLASH �����б�101-150 �ɿ��Ÿ��û�д��Ĳ���
    ,{106,        PAE_BOTH,        PTE_U16,          (void*)&(globalSettingParam.u16BuildNumber)}


    //�ڴ�����б�151-200
    ,{151,        PAE_READ,        PTE_U16,          (void*)&(globalSettingParam.u32BaudRate)}

    
    //ִ������201-255
    ,{201,        PAE_WRITE,        PTE_EXCUTE,      (void*)SystemOperate}
    ,{202,        PAE_READ,        PTE_DATA,         (void*)WatchData}
};


void Write_All_Param(settingParamDef_t *setting)
{
    uint32_t Address = 0 ,PageError = 0;
    uint32_t *pBuffer;
    FLASH_EraseInitTypeDef EraseInitStruct;
    
    //delay_ms(10);
    __disable_irq();
    HAL_FLASH_Unlock();
    
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Sector = FLASH_SECTOR_7;
    EraseInitStruct.Banks = FLASH_BANK_2;
    EraseInitStruct.NbSectors = 1;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; 

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }
    FLASH_WaitForLastOperation(5000,FLASH_BANK_2);
    
    Address = SETTING_ADDRESS_BASE;
    pBuffer = (uint32_t*)setting;
    for(uint16_t i = 0 ; i < sizeof(settingParamDef_t)/4 ; i ++ )
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, Address, *pBuffer) == HAL_OK)
        {
            Address = Address + 4;
            pBuffer ++ ;
        }
        else
        {
            // Error occurred while writing data in Flash memory.
            _Error_Handler(__FILE__, __LINE__);
        }
    }
    HAL_FLASH_Lock();
    __enable_irq();
}

void Read_All_Param(settingParamDef_t *setting)
{
    uint32_t Address = SETTING_ADDRESS_BASE;
    uint32_t *pBuffer = (uint32_t*)setting;

    for(uint16_t i=0 ; i < sizeof(settingParamDef_t)/4 ; i++)
    {
        pBuffer[i] = *(__IO uint32_t *)Address;
        Address += 4;   
    }
}
    
void Init_All_Param(void)
{
    Read_All_Param(&globalSettingParam);
    
    if(globalSettingParam.u16Inited != 0xA0A0 )//����Ĭ�ϲ���δ����
    {
        Write_All_Param((settingParamDef_t*)&globalSettingParamDefault);
        Read_All_Param(&globalSettingParam);
    }
}

uint8_t SystemOperate(uint8_t* data, uint16_t len)
{
    uint8_t res = 0;
    if(data == NULL) return ARCE_SYS_OP_DATA_EMPTY;

    switch(data[0])
    {
        //�˳���װ״̬
        case '0':
            globalSettingParam.u8WorkMode = 0;
            break;
        //���밲װ״̬
        case '1':
            globalSettingParam.u8WorkMode = 1;
            break;
        //�ָ���������
        case '2':
            Write_All_Param((settingParamDef_t*)&globalSettingParamDefault);
            break;
        //�����豸
        case '3':
            NVIC_SystemReset();
            //delay_ms(100);
            break;
        //�������(д���������)
        case '4':
            ;
            break;
        //�������(��д���������)
        case '5':
            Write_All_Param((settingParamDef_t*)&globalSettingParam);
            break;
        default:
            res = ARCE_SYS_OP_NOT_MATCH;
            break;
    }
    return res;
}
uint8_t WatchData(uint8_t* data, uint16_t len)
{                                       
    return 0;
}


/************AT �����ʽ**************
1����ȡ����AT+READ=XX,YYY    XX-�豸��ַ(�̶�Ϊ��λ��ʮ�����ַ���),YYY-����ID(�̶�Ϊ��λ��ʮ�����ַ���)
   ��������+READ:XX,YYY=ZZZ    ZZZ (��������)
                                            
2����������AT+WRITE=XX,YYY,ZZZ    XX-�豸��ַ(�̶�Ϊ��λ��ʮ�����ַ���),YYY-����ID(�̶�Ϊ��λ��ʮ�����ַ���)        
                                ZZZ-��������(���Ȳ������ַ�������ʽȡ���ڲ�������)
   ��������+WRITE:XX,YYY=ZZZ    ZZZ (���ú�Ľ��)

3��ִ������AT+WRITE:XX,YYY,ZZZ    XX-�豸��ַ(�̶�Ϊ��λ��ʮ�����ַ���),YYY-����ID        (�̶�Ϊ��λ��ʮ�����ַ���)        
                                ZZZ-��������(����Ϊ�գ������Ϊִ�ж������������)                
        ��������+WRITE:XX,YYY    =Z  Z(ִ�д�����)
                                            
*****************************************/
void AT_Cmd_Process(uint8_t u8CmdUartX)
{
    static uint8_t cmdCnt = 0;
    uint8_t i = 0 ,clearFlag = 0;
    uint8_t offset = 0 , devId = 255 ,paraId = 0;
    uint8_t errCode = 0, excCode = 0;
    PARA_ATTR_E attrReq = PAE_INVALID;
    uint8_t* pos = NULL;
    uint16_t* recvCntAddr = NULL;
    
    //ָ�����ݻ�������ַ
    offset = 0;
    if(u8CmdUartX == 1)
    {
        PdaRecvBuffer = PdaRecvBuffer1;
        recvCntAddr = &PdaRecvCount1;
    }
    else    
    {
        return;
    }
    
    pos = (uint8_t*)memistr((char*)PdaRecvBuffer, PDA_MSG_LENGTH, "AT+");
    
    while(pos != NULL)
    {
        offset += 3;
        clearFlag = 1;
        attrReq = PAE_INVALID;
        memset(PdaMsgBuffer, 0x00, sizeof(PdaMsgBuffer));   //��ʼ������
        
        StrToUpper_n((char*)pos + offset, 5);               //�ж�д����
        if(strncmp((char*)pos + offset, "READ=", 5) == 0)
        {
            offset += 5;
            attrReq = PAE_READ;
            strcpy((char*)PdaMsgBuffer, "+READ:");
            PdaMsgCount = strlen((char*)PdaMsgBuffer);
        }
        else if(strncmp((char*)pos + offset, "WRITE=", 6) == 0)
        {
            offset += 6;
            attrReq = PAE_WRITE;
            strcpy((char*)PdaMsgBuffer, "+WRITE:");
            PdaMsgCount = strlen((char*)PdaMsgBuffer);
        }
        
        
        devId = str2Digit(pos + offset);                    //�豸��ַ
        offset += 3;                                        //��λ�豸ID + 1������
        
        if(attrReq == PAE_INVALID)
        {
            errCode = ARCE_OPER_INVALID;                    //�����Ƿ�(���Ƕ�������д����)
        }
        else if((devId != globalSettingParam.u16DevAddr) && (devId != 99))
        {
            errCode = ARCE_DEVADDR_NOT_MATCH;               //�豸��ַ��ƥ��
        }
        else
        {
            paraId = str2Digit(pos + offset);               //��ȡ����ID
            offset += 3;
            if(paraId > 255)
            {   
                errCode = ARCE_PARA_ID_INVALID;             //����ID �����޶���Χ
            }
            
            else
            {
                
                xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%02d,%03d=", globalSettingParam.u16DevAddr, paraId);
                PdaMsgCount = strlen((char*)PdaMsgBuffer);
                for(i = 0; i < AT_CMD_TAB_SIZE; i++)
                {
                    if(atCmdTab[i].paraId == paraId)
                    {
                        if(atCmdTab[i].paraAddr == NULL)
                        {
                            xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "NULL\r\n");
                        }
                        else if((atCmdTab[i].attr == attrReq) || (atCmdTab[i].attr == PAE_BOTH))
                        {
                            if(attrReq == PAE_READ)     //������
                            {
                                switch(atCmdTab[i].type)
                                {
                                    case PTE_U8:
                                        xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%u\r\n", *((uint8_t*)atCmdTab[i].paraAddr));
                                        break;
                                    case PTE_U16:
                                        xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%u\r\n", *((uint16_t*)atCmdTab[i].paraAddr));
                                        break;
                                    case PTE_I16:
                                        xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%d\r\n", *((int16_t*)atCmdTab[i].paraAddr));
                                        break;
                                    case PTE_U32:
                                        xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%u\r\n", *((uint32_t*)atCmdTab[i].paraAddr));
                                        break;
                                    case PTE_I32:
                                        xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%d\r\n", *((int32_t*)atCmdTab[i].paraAddr));
                                        break;
                                    case PTE_STRING:
                                        xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%s\r\n", ((char*)atCmdTab[i].paraAddr));
                                        break;
                                    case PTE_DATA:
                                        if(pos[offset++] != ',')
                                        {
                                            errCode = ARCE_DATA_EMPTY;  //����ָ�������Ϊ��
                                        }
                                        else
                                        {
                                            errCode = ((AT_Cmd_Callback)((uint32_t)(atCmdTab[i].paraAddr)))(pos + offset, *recvCntAddr - offset);
                                        }
                                        break;
                                    default:
                                        break;
                                }
                            }
                            else if(attrReq == PAE_WRITE)   //д����
                            {
                                if(pos[offset++] != ',')
                                {
                                    errCode = ARCE_DATA_EMPTY;  //д��ָ�������Ϊ��
                                }
                                else
                                {
                                    switch(atCmdTab[i].type)
                                    {
                                        case PTE_U8:
                                            *((uint8_t*)atCmdTab[i].paraAddr) = str2Digit(pos + offset);
                                            xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%u\r\n", *((uint8_t*)atCmdTab[i].paraAddr));
                                            break;
                                        case PTE_U16:
                                            *((uint16_t*)atCmdTab[i].paraAddr) = str2Digit(pos + offset);
                                            xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%u\r\n", *((uint16_t*)atCmdTab[i].paraAddr));
                                            break;
                                        case PTE_I16:
                                            *((int16_t*)atCmdTab[i].paraAddr) = str2Digit(pos + offset);
                                            xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%d\r\n", *((int16_t*)atCmdTab[i].paraAddr));
                                            break;
                                        case PTE_U32:
                                            *((uint32_t*)atCmdTab[i].paraAddr) = str2Digit(pos + offset);
                                            xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%u\r\n", *((uint32_t*)atCmdTab[i].paraAddr));
                                            break;
                                        case PTE_I32:
                                            *((int32_t*)atCmdTab[i].paraAddr) = str2Digit(pos + offset);
                                            xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%d\r\n", *((int32_t*)atCmdTab[i].paraAddr));
                                            break;
                                        case PTE_EXCUTE:
                                            excCode = ((AT_Cmd_Callback)((uint32_t)(atCmdTab[i].paraAddr)))(pos + offset, *recvCntAddr - offset);
                                            xsprintf((char*)(PdaMsgBuffer + PdaMsgCount), "%d\r\n", excCode);
                                            break;
                                        default:
                                            break; 
                                    }
                                    offset += strlen((char*)(pos + offset));
                                }
                            }
                        }
                        else
                        {
                            errCode = ARCE_AUTORIZE_REFUSE;     //Ȩ�޲���
                        }
                        break;
                    }
                }
                if(i >= AT_CMD_TAB_SIZE)
                {
                    errCode = ARCE_PARA_ID_NOT_MATCH;           //����ID ƥ��ʧ��
                }
            }
        }            

        if(errCode != 0)    //���� , ���ҽ�����485�豸,���ǵ�ַ����Ų��ᱨ��
        {
            memset(PdaMsgBuffer, 0x00, sizeof(PdaMsgBuffer));
            if((globalSettingParam.u16_485_232 != ST_RS485) || (errCode != ARCE_DEVADDR_NOT_MATCH)) 
            {
                xsprintf((char*)PdaMsgBuffer, "+ERROR:%02d,%03d=%d\r\n", devId, paraId, errCode);
            }
        }
        PdaMsgCount = strlen((char*)PdaMsgBuffer);
        if(PdaMsgCount > 0)
        {
            if(u8CmdUartX == 1)
            {
                //UsartPutString(&huart3,PdaMsgBuffer, PdaMsgCount); //���ڵ������
                HAL_UART_Transmit(&huart3,PdaMsgBuffer, PdaMsgCount, 10000);
            }
        }

        cmdCnt++;

        if(cmdCnt >= 25)    break;
        pos = (uint8_t*)memistr((char*)pos + offset, PDA_MSG_LENGTH - offset - (pos - PdaRecvBuffer), "AT+");
        
    }

    if(*recvCntAddr >= PDA_MSG_LENGTH)
    {
        clearFlag = 1;
    }
    
    if(clearFlag == 1)
    {
        memset(PdaRecvBuffer, 0, PDA_MSG_LENGTH);
        *recvCntAddr = 0;
        PdaRecvOverflowFlag = 0;
    }
    
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

