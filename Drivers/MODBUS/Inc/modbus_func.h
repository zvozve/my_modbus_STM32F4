#ifndef __MODBUS_FUNC_H__
#define __MODBUS_FUNC_H__

#include "modbus_app.h"
#include "modbus_uart.h"

/**
 * @brief   ����
 */
#define MB_SLAVE_ID_MIN    0
#define MB_SLAVE_ID_MAX    247

#define MB_REG_ADDR_MIN    0
#define MB_REG_ADDR_MAX    MB_BUF_LEN - 1

/**
 * @brief   ö��
 */
/**
 * -------- MODBUS������ --------
 * ��ȡ��Ȧ�Ĵ�����Read Coils�������ڶ�ȡ�����Ȧ״̬��ͨ�����ڿ����ⲿ�豸�Ŀ��ء�
 * ��ȡ��ɢ����Ĵ�����Read Discrete Inputs�������ڶ�ȡ��ɢ����״̬��ͨ�����ڻ�ȡ�ⲿ�豸��״̬��Ϣ��
 * ��ȡ���ּĴ�����Read Holding Registers�������ڶ�ȡ�������ݵļĴ�����ͨ�����ڻ�ȡ�豸�����򴫸������ݡ�
 * ��ȡ����Ĵ�����Read Input Registers�������ڶ�ȡ����Ĵ����е����ݣ�ͨ�����ڻ�ȡ���������ݡ�
 *
 * д������Ȧ��Write Single Coil��������д�뵥�������Ȧ��״̬��ͨ�����ڿ����ⲿ�豸��
 * д�����Ĵ�����Write Single Register��������д�뵥���Ĵ�����ֵ��ͨ�����������豸������
 * д�����Ȧ��Write Multiple Coils��������д���������Ȧ��״̬��ͨ�����ڿ��ƶ���ⲿ�豸��
 * д����Ĵ�����Write Multiple Registers��������д�����Ĵ�����ֵ��ͨ���������ö���豸������
 *
 * ����д�Ĵ�����Mask Write Register�������ڸ�������λ��д��Ĵ����Ĳ������ݡ�
 * ��ȡ�ļ���¼��Read File Record�������ڶ�ȡ�ļ���¼��������ݡ�
 * д�ļ���¼��Write File Record��������д���ļ���¼��������ݡ�
 * ��ȡ�豸ʶ���루Read Device Identification�������ڻ�ȡ�豸�ı�ʶ��Ϣ���������̡��ͺŵȡ�
*/
typedef enum {
    READ_COILS                  = 0x01,
    READ_DISCRETE_INPUTS        = 0x02,
    READ_HOLDING_REGISTERS      = 0x03,
    READ_INPUT_REGISTERS        = 0x04,
    WRITE_SINGLE_COIL           = 0x05,
    WRITE_SINGLE_REGISTER       = 0x06,
    WRITE_MULTIPLE_COILS        = 0x0F,
    WRITE_MULTIPLE_REGISTERS    = 0x10,
    MASK_WRITE_REGISTER         = 0x16,
    READ_FILE_RECORD            = 0x14,
    WRITE_FILE_RECORD           = 0x15,
    READ_DEVICE_IDENTIFICATION  = 0x2B
} mbFxCode_t;

/**
 * -------- MODBUS������ --------
*/
typedef enum {
    MB_NO_ERR               = 0x00,     
    MB_CMD_FXCODE_ERR       = 0x01, // ���������
    MB_CMD_ADDR_ERR         = 0x02, // �������׵�ַ����
    MB_DATA_ERR             = 0x03, // �������ݴ���
    MB_CMD_ID_ERR           = 0x04, 
    MB_CMD_LEN_ERR          = 0x05, 
    MB_CMD_CRC_ERR          = 0x06, 
    MB_DATA_TYPE_ERR        = 0x10, // �������ʹ���
    MB_DATA_LEN_ERR         = 0x11, // �����ݸ�������
    MB_DATA_LEN_NO_MATCH    = 0x12, // ���ݸ������ڲ��������ݸ�������
    MB_DATA_CANT_WRITE      = 0x13, // ���ܽ���д����    
} mbFxStatus_t;

/**
 * @brief   �ṹ��
 */
typedef struct {
    uint8_t  id;       
	uint8_t  fxcode;              
    uint16_t addr;
	uint16_t dataCnt;
	uint8_t  dataByte;
    bool     *rdBit;
    uint16_t *rdU16;
    bool     wtBit[MB_BUF_LEN];
    uint16_t wtU16[MB_BUF_LEN];
    uint8_t  cmdBuf[MB_BUF_LEN];
    uint16_t cmdByte; 
}mbCommand_t;

typedef struct {
    uint8_t         ID;
    mbCommand_t     Tx;
    mbCommand_t     Rx;
    mbFxStatus_t    FxStatus;
}mbRTU_t;

/**
 * @brief   ENABLE_MOUBUS_MASTER
*/
#if ENABLE_MOUBUS_MASTER
extern mbRTU_t  mbMaster;
extern bool     mbMaster_Coils_Buf[MB_BUF_LEN];
extern uint16_t mbMaster_Holding_Buf[MB_BUF_LEN];

void mbMaster_Read_Coils            (uint8_t id, uint16_t addr, bool *p_data,     uint8_t cnt);
void mbMaster_Read_Discrete_Inputs  (uint8_t id, uint16_t addr, bool *p_data,     uint8_t cnt);
void mbMaster_Read_Holdings         (uint8_t id, uint16_t addr, uint16_t *p_data, uint8_t cnt);
void mbMaster_Read_Input            (uint8_t id, uint16_t addr, uint16_t *p_data, uint8_t cnt);
void mbMaster_Write_Single_Coil     (uint8_t id, uint16_t addr, bool data);
void mbMaster_Write_Single_Reg      (uint8_t id, uint16_t addr, uint16_t data);
void mbMaster_Write_Multiple_Coils  (uint8_t id, uint16_t addr, bool *p_data,     uint8_t cnt);
void mbMaster_Write_Multiple_Regs   (uint8_t id, uint16_t addr, uint16_t *p_data, uint8_t cnt);
mbFxStatus_t mbMaster_Structuring_Request(void);
mbFxStatus_t mbMaster_Processing_Response(void);
#endif // ENABLE_MOUBUS_MASTER

/**
 * @brief   ENABLE_MOUBUS_SLAVE
*/
#if  ENABLE_MOUBUS_SLAVE
extern mbRTU_t  mbSlave;
extern bool     mbSlave_Coils_Buf[MB_BUF_LEN];
extern uint16_t mbSlave_Holding_Buf[MB_BUF_LEN];

void mbSlave_Read_Coils             (uint16_t addr, bool *p_data,       uint8_t cnt);
void mbSlave_Read_Discrete_Inputs   (uint16_t addr, bool *p_data,       uint8_t cnt);
void mbSlave_Read_Holdings          (uint16_t addr, uint16_t *p_data,   uint8_t cnt);
void mbSlave_Read_Input             (uint16_t addr, uint16_t *p_data,   uint8_t cnt);
void mbSlave_Write_Single_Coil      (uint16_t addr, bool data);
void mbSlave_Write_Single_Reg       (uint16_t addr, uint16_t data);
void mbSlave_Write_Multiple_Coils   (uint16_t addr, bool *p_data,       uint8_t cnt);
void mbSlave_Write_Multiple_Regs    (uint16_t addr, uint16_t *p_data,   uint8_t cnt);
void mbSlave_FIFO_Write_Regs        (uint16_t addr, uint16_t *p_data,   uint8_t cnt, uint16_t size);
mbFxStatus_t mbSlave_Processing_Request(void);
mbFxStatus_t mbSlave_Structuring_Response(void);
#endif // ENABLE_MOUBUS_SLAVE

/**
 * @brief       ���ݽṹ
 */
void mbMemClr_Cmd(mbRTU_t *mb);
void mbMemCpy_Buf(uint8_t *target, uint8_t *source, uint16_t len);
void mbMemCpy_Coils(bool *target, bool *source, uint16_t len);
void mbMemCpy_Regs(uint16_t *target, uint16_t *source, uint16_t len);

/**
 * @brief       ����
 */
uint16_t mbCal_CRC16(const uint8_t *data, uint16_t len);

/**
 * @brief       ת��
 */
uint8_t mbConv_Coils_2_Buf(const bool *coils, uint8_t *buf, uint16_t cnt);
uint8_t mbConv_Regs_2_Buf (const uint16_t *regs, uint8_t *buf, uint16_t cnt);
uint8_t mbConv_Buf_2_Coils(const uint8_t *buf, bool *coils, uint16_t cnt);
uint8_t mbConv_Buf_2_Regs (const uint8_t *buf, uint16_t *regs, uint16_t cnt);

/**
 * @brief       ����
 * @param       ����0�����쳣������1��������
 */
bool mbVerify_Is_Same_Crc16(const uint8_t *data, uint16_t len);
bool mbVerify_Is_Valid_Id(uint8_t id);
bool mbVerify_Is_Valid_FxCode(uint8_t fxcode);
bool mbVerify_Is_Valid_Addr(uint16_t addr);
bool mbVerify_Is_Valid_DataLen(uint16_t addr, uint16_t cnt);

#endif // __MODBUS_FUNC_H__
