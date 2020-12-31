/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BSP_SET_H
#define __BSP_SET_H

/* Includes ------------------------------------------------------------------*/
#include "bsp_mcu.h"
#include "bsp_usart.h"

/* Private define ------------------------------------------------------------*/
#define SETTING_ADDRESS_BASE    0X081E0000      //���ò��� FLASH��ַ
#define PDA_MSG_LENGTH          128             //�޸�AT ����򴮿�ͨѶЭ��ʱ����Ҫ���Ǵ˳����Ƿ��㹻
typedef uint8_t (*AT_Cmd_Callback)(uint8_t* data, uint16_t len);

typedef enum
{
    ST_RS232 = 0,    
    ST_RS485,
    ST_NONE
}sendType;

typedef enum
{
    PAE_READ = 0x00,    //ֻ��
    PAE_WRITE,          //ֻд
    PAE_BOTH,           //��д
    PAE_INVALID         //��Ч
}PARA_ATTR_E;

typedef enum
{
    PTE_U8 = 0x00,        
    PTE_U16,
    PTE_I16,
    PTE_U32,
    PTE_I32,
    PTE_FLOAT,
    PTE_DOUBLE,
    PTE_CH,                //�ַ�
    PTE_STRING,            //�ִ�
    PTE_EXCUTE,            //ִ�г���-����ִ�н��
    PTE_DATA,            //ִ�г���-�ں����л�ȡָ��������
    PTE_INVALID
}PARA_TYPE_E;

typedef enum
{
    ARCE_NONE = 0,
    ARCE_OPER_INVALID = 1,            //�����Ƿ�(���Ƕ�������д����)
    ARCE_DEVADDR_NOT_MATCH = 2,        //�豸��ַ��ƥ��
    ARCE_PARA_ID_INVALID = 3,        //����ID �����޶���Χ
    ARCE_PARA_ID_NOT_MATCH = 4,        //����ID ƥ��ʧ��
    ARCE_AUTORIZE_REFUSE = 5,        //Ȩ�޲���
    ARCE_DATA_EMPTY = 6,            //д������ָ�������Ϊ��
    ARCE_KEY_PARA_INVALID = 8,        //���������޶���Χ
    ARCE_SYS_OP_DATA_EMPTY = 10,    //ϵͳ����������Ϊ��
    ARCE_SYS_OP_NOT_MATCH = 11,        //ϵͳ����������ƥ��ʧ��
}AT_RES_CODE_E;

typedef struct
{
    uint8_t            paraId;            //����ID
    PARA_ATTR_E        attr;            //��д��־
    PARA_TYPE_E        type;           //��������
    void*            paraAddr;        //�������ڴ��еĵ�ַ����ص�������ַ
}AT_Cmd_Tab_T;


//���ò�������
typedef struct _settingParamDef
{
    uint16_t    u16Inited;                //flash��ʼ����־
    uint16_t    u16BuildNumber;         //����汾
    
    uint32_t    u32BaudRate;            //������
    uint16_t    u16_485_232;            //232-485
    uint16_t    u16DevAddr;             //�豸��ַ     
    uint16_t    u16HardwareVersion;     //Ӳ���汾��Ϣ 

    uint8_t     u8WorkMode;             //--0����״̬ --1����״̬   

    

}settingParamDef_t;

extern settingParamDef_t globalSettingParam;
extern uint8_t     PdaRecvBuffer1[PDA_MSG_LENGTH];
extern uint16_t PdaRecvCount1;
void AT_Cmd_Process(uint8_t u8CmdUartX);

void Write_All_Param(settingParamDef_t *setting);
void Read_All_Param(settingParamDef_t *setting);
void Init_All_Param(void);

uint8_t SystemOperate(uint8_t* data, uint16_t len);
uint8_t WatchData(uint8_t* data, uint16_t len);
#endif /* __BSP_SET_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

