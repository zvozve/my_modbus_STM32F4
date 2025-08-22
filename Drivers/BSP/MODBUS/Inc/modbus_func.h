#ifndef __MODBUS_FUNC_H__
#define __MODBUS_FUNC_H__

#include "modbus_app.h"
#include "modbus_uart.h"

/**
 * @brief   常量
 */
#define MB_SLAVE_ID_MIN    0
#define MB_SLAVE_ID_MAX    247

#define MB_REG_ADDR_MIN    0
#define MB_REG_ADDR_MAX    MB_BUF_LEN - 1

/**
 * @brief   枚举
 */
/**
 * -------- MODBUS功能码 --------
 * 读取线圈寄存器（Read Coils）：用于读取输出线圈状态，通常用于控制外部设备的开关。
 * 读取离散输入寄存器（Read Discrete Inputs）：用于读取离散输入状态，通常用于获取外部设备的状态信息。
 * 读取保持寄存器（Read Holding Registers）：用于读取保存数据的寄存器，通常用于获取设备参数或传感器数据。
 * 读取输入寄存器（Read Input Registers）：用于读取输入寄存器中的数据，通常用于获取传感器数据。
 *
 * 写单个线圈（Write Single Coil）：用于写入单个输出线圈的状态，通常用于控制外部设备。
 * 写单个寄存器（Write Single Register）：用于写入单个寄存器的值，通常用于配置设备参数。
 * 写多个线圈（Write Multiple Coils）：用于写入多个输出线圈的状态，通常用于控制多个外部设备。
 * 写多个寄存器（Write Multiple Registers）：用于写入多个寄存器的值，通常用于配置多个设备参数。
 *
 * 屏蔽写寄存器（Mask Write Register）：用于根据掩码位来写入寄存器的部分数据。
 * 读取文件记录（Read File Record）：用于读取文件记录区域的数据。
 * 写文件记录（Write File Record）：用于写入文件记录区域的数据。
 * 读取设备识别码（Read Device Identification）：用于获取设备的标识信息，如制造商、型号等。
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
 * -------- MODBUS错误码 --------
*/
typedef enum {
    MB_NO_ERR               = 0x00,     
    MB_CMD_FXCODE_ERR       = 0x01, // 功能码错误
    MB_CMD_ADDR_ERR         = 0x02, // 域数据首地址错误
    MB_DATA_ERR             = 0x03, // 数据内容错误
    MB_CMD_ID_ERR           = 0x04, 
    MB_CMD_LEN_ERR          = 0x05, 
    MB_CMD_CRC_ERR          = 0x06, 
    MB_DATA_TYPE_ERR        = 0x10, // 数据类型错误
    MB_DATA_LEN_ERR         = 0x11, // 域数据个数错误
    MB_DATA_LEN_NO_MATCH    = 0x12, // 数据个数与内部区域数据个数不符
    MB_DATA_CANT_WRITE      = 0x13, // 不能进行写操作    
} mbFxStatus_t;

/**
 * @brief   结构体
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

#if ENABLE_MOUBUS_DMACPY
/**
 * @brief   DMA搬运
 */
#define hdma_mb_cpy         hdma_memtomem_dma2_channel4
extern DMA_HandleTypeDef    hdma_mb_cpy;

typedef struct {
    uint16_t index;
    uint16_t cnt;
    uint16_t move_len;
    uint16_t *p_data;
    uint8_t step;
} FIFO_DMA_Context_t;
#endif // ENABLE_MOUBUS_DMACPY

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

void mbSlave_Read_Coils             (uint16_t addr, bool *p_data,       uint16_t cnt);
void mbSlave_Read_Discrete_Inputs   (uint16_t addr, bool *p_data,       uint16_t cnt);
void mbSlave_Read_Holdings          (uint16_t addr, uint16_t *p_data,   uint16_t cnt);
void mbSlave_Read_Input             (uint16_t addr, uint16_t *p_data,   uint16_t cnt);
void mbSlave_Write_Single_Coil      (uint16_t addr, bool data);
void mbSlave_Write_Single_Reg       (uint16_t addr, uint16_t data);
void mbSlave_Write_Multiple_Coils   (uint16_t addr, bool *p_data,       uint16_t cnt);
void mbSlave_Write_Multiple_Regs    (uint16_t addr, uint16_t *p_data,   uint16_t cnt);
void mbSlave_FIFO_Write_Regs        (uint16_t addr, uint16_t *p_data,   uint16_t cnt, uint16_t size);
#if ENABLE_MOUBUS_DMACPY
void mbSlave_FIFO_Write_Regs_DMA    (uint16_t addr, uint16_t *p_data, uint16_t cnt, uint16_t size);
void mbSlave_FIFO_Write_Regs_DMA_Callback(DMA_HandleTypeDef *hdma);
#endif // ENABLE_MOUBUS_DMACPY
mbFxStatus_t mbSlave_Processing_Request(void);
mbFxStatus_t mbSlave_Structuring_Response(void);
#endif // ENABLE_MOUBUS_SLAVE

/**
 * @brief       数据结构
 */
void mbMemClr_Cmd(mbRTU_t *mb);
void mbMemCpy_Buf(uint8_t *target, uint8_t *source, uint16_t len);
void mbMemCpy_Coils(bool *target, bool *source, uint16_t len);
void mbMemCpy_Regs(uint16_t *target, uint16_t *source, uint16_t len);

/**
 * @brief       计算
 */
uint16_t mbCal_CRC16(const uint8_t *data, uint16_t len);

/**
 * @brief       转换
 */
uint8_t mbConv_Coils_2_Buf(const bool *coils, uint8_t *buf, uint16_t cnt);
uint8_t mbConv_Regs_2_Buf (const uint16_t *regs, uint8_t *buf, uint16_t cnt);
uint8_t mbConv_Buf_2_Coils(const uint8_t *buf, bool *coils, uint16_t cnt);
uint8_t mbConv_Buf_2_Regs (const uint8_t *buf, uint16_t *regs, uint16_t cnt);

/**
 * @brief       检验
 * @param       返回0代表异常，返回1代表正常
 */
bool mbVerify_Is_Same_Crc16(const uint8_t *data, uint16_t len);
bool mbVerify_Is_Valid_Id(uint8_t id);
bool mbVerify_Is_Valid_FxCode(uint8_t fxcode);
bool mbVerify_Is_Valid_Addr(uint16_t addr);
bool mbVerify_Is_Valid_DataLen(uint16_t addr, uint16_t cnt);

#endif // __MODBUS_FUNC_H__
