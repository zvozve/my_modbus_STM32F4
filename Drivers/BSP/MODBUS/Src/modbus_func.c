#include "modbus_func.h"

#if ENABLE_MOUBUS_DMACPY

static FIFO_DMA_Context_t fifo_dma_ctx;

#endif // ENABLE_MOUBUS_DMACPY

/**
 * @brief       ENABLE_MB_RTU_MASTER
 * @param       无
 * @retval      无
 */	
#if ENABLE_MB_RTU_MASTER  
mbRTU_t  mbMaster = {0};
bool     mbMaster_Coils_Buf[MB_BUF_LEN];
uint16_t mbMaster_Holding_Buf[MB_BUF_LEN];

void mbMaster_cmd_clr(void) {    
    memset(&mbMaster.Tx, 0, sizeof(mbCommand_t));    
    memset(&mbMaster.Rx, 0, sizeof(mbCommand_t));    
}

void mbMaster_transfer_block(uint16_t us) {
    #if ENABLE_MASTER_TX_BLOCK
    while (mbMaster_Task.status != MBT_IDLE) {
        MB_TIM_Delay_us(us);
    }
    #endif // ENABLE_MASTER_TX_BLOCK
}

void mbMaster_Read_Coils            (uint8_t id, uint16_t addr, bool *p_data,     uint8_t cnt) {
    mbMaster_cmd_clr();
    
    mbMaster.Tx.fxcode  = READ_COILS;
    mbMaster.Tx.id      = id; // 常量定义默认ID为1
	mbMaster.Tx.addr    = addr;
	mbMaster.Rx.dataCnt = cnt;
    mbMaster.Rx.rdBit   = p_data;
    mbMaster_Task.status = MBT_REQUESTING;
    
    mbMaster_transfer_block(MB_TIM_PERIOD_US);
}

void mbMaster_Read_Discrete_Inputs  (uint8_t id, uint16_t addr, bool *p_data,     uint8_t cnt) {
    mbMaster_cmd_clr();
    
    mbMaster.Tx.fxcode  = READ_DISCRETE_INPUTS;
    mbMaster.Tx.id      = id; // 常量定义默认ID为1
	mbMaster.Tx.addr    = addr;
	mbMaster.Rx.dataCnt = cnt;
    mbMaster.Rx.rdBit =  p_data;
    mbMaster_Task.status = MBT_REQUESTING;
    
    mbMaster_transfer_block(MB_TIM_PERIOD_US);
}

void mbMaster_Read_Holdings         (uint8_t id, uint16_t addr, uint16_t *p_data, uint8_t cnt) {
    mbMaster_cmd_clr();
    
    mbMaster.Tx.fxcode  = READ_HOLDING_REGISTERS;
    mbMaster.Tx.id      = id; // 常量定义默认ID为1
	mbMaster.Tx.addr    = addr;
	mbMaster.Rx.dataCnt = cnt;
    mbMaster.Rx.rdU16   = p_data;
    mbMaster_Task.status = MBT_REQUESTING;
    
    mbMaster_transfer_block(MB_TIM_PERIOD_US);
}

void mbMaster_Read_Input            (uint8_t id, uint16_t addr, uint16_t *p_data, uint8_t cnt) {
    mbMaster_cmd_clr();
    
    mbMaster.Tx.fxcode  = READ_INPUT_REGISTERS;
    mbMaster.Tx.id      = id; // 常量定义默认ID为1
	mbMaster.Tx.addr    = addr;
	mbMaster.Rx.dataCnt = cnt;
    mbMaster.Rx.rdU16   = p_data;
    mbMaster_Task.status = MBT_REQUESTING;
    
    mbMaster_transfer_block(MB_TIM_PERIOD_US);
}

void mbMaster_Write_Single_Coil     (uint8_t id, uint16_t addr, bool data) {
    mbMaster_cmd_clr();
    
	mbMaster.Tx.fxcode  = WRITE_SINGLE_COIL;
    mbMaster.Tx.id      = id; // 常量定义默认ID为1
    mbMaster.Tx.addr    = addr;
	mbMaster.Tx.dataCnt = 1;
    mbMaster.Tx.wtBit[0] = data;
    mbMaster_Task.status = MBT_REQUESTING;
        
    mbMaster_transfer_block(MB_TIM_PERIOD_US);
}

void mbMaster_Write_Single_Reg      (uint8_t id, uint16_t addr, uint16_t data) {
    mbMaster_cmd_clr();
    
	mbMaster.Tx.fxcode  = WRITE_SINGLE_REGISTER;
    mbMaster.Tx.id      = id; // 常量定义默认ID为1
    mbMaster.Tx.addr    = addr;
	mbMaster.Tx.dataCnt = 1;
    mbMaster.Tx.wtU16[0] = data;
    mbMaster_Task.status = MBT_REQUESTING;
        
    mbMaster_transfer_block(MB_TIM_PERIOD_US);
}

void mbMaster_Write_Multiple_Coils  (uint8_t id, uint16_t addr, bool *p_data,     uint8_t cnt) {
    mbMaster_cmd_clr();
    
	mbMaster.Tx.fxcode  = WRITE_MULTIPLE_COILS;
    mbMaster.Tx.id      = id; // 常量定义默认ID为1
    mbMaster.Tx.addr    = addr;
	mbMaster.Tx.dataCnt = cnt;
    mbMemCpy_Coils(mbMaster.Tx.wtBit, p_data, cnt);
    mbMaster_Task.status = MBT_REQUESTING;
        
    mbMaster_transfer_block(MB_TIM_PERIOD_US);
}

void mbMaster_Write_Multiple_Regs   (uint8_t id, uint16_t addr, uint16_t *p_data, uint8_t cnt) {
    mbMaster_cmd_clr();
    
	mbMaster.Tx.fxcode  = WRITE_MULTIPLE_REGISTERS;
    mbMaster.Tx.id      = id; // 常量定义默认ID为1
    mbMaster.Tx.addr    = addr;
	mbMaster.Tx.dataCnt = cnt;
    mbMemCpy_Regs(mbMaster.Tx.wtU16, p_data, cnt);
    mbMaster_Task.status = MBT_REQUESTING;
        
    mbMaster_transfer_block(MB_TIM_PERIOD_US);
}

mbFxStatus_t mbMaster_Structuring_Request(void) {
    mbCommand_t *mbm_tx = &mbMaster.Tx;
//    mbCommand_t *mbm_rx = &mbMaster.Rx;

    // 设备 ID 校验
    if (!mbVerify_Is_Valid_Id(mbm_tx->id)){
        return MB_CMD_ID_ERR;
    }
    
    // 检查功能码
    if(!mbVerify_Is_Valid_FxCode(mbm_tx->fxcode)){
        return MB_CMD_FXCODE_ERR;
    }
    
    // 根据功能码配置
    switch (mbm_tx->fxcode) {
        case READ_COILS: {
            // 构建 Modbus 发送帧
            mbm_tx->cmdBuf[0] = mbm_tx->id;
            mbm_tx->cmdBuf[1] = mbm_tx->fxcode;
            mbm_tx->cmdBuf[2] = mbm_tx->addr >> 8;
            mbm_tx->cmdBuf[3] = mbm_tx->addr & 0xFF;
            // 数据个数
            mbm_tx->cmdBuf[4] = mbMaster.Rx.dataCnt >> 8;
            mbm_tx->cmdBuf[5] = mbMaster.Rx.dataCnt & 0xFF;
            // 指令长度
            mbm_tx->cmdByte = 8;
        } break;
        
        case READ_DISCRETE_INPUTS: {
            // 构建 Modbus 发送帧
            mbm_tx->cmdBuf[0] = mbm_tx->id;
            mbm_tx->cmdBuf[1] = mbm_tx->fxcode;
            mbm_tx->cmdBuf[2] = mbm_tx->addr >> 8;
            mbm_tx->cmdBuf[3] = mbm_tx->addr & 0xFF;
            // 数据个数
            mbm_tx->cmdBuf[4] = mbMaster.Rx.dataCnt >> 8;
            mbm_tx->cmdBuf[5] = mbMaster.Rx.dataCnt & 0xFF;
            // 指令长度
            mbm_tx->cmdByte = 8;
        } break;
        
        case READ_HOLDING_REGISTERS: {
            // 构建 Modbus 发送帧
            mbm_tx->cmdBuf[0] = mbm_tx->id;
            mbm_tx->cmdBuf[1] = mbm_tx->fxcode;
            mbm_tx->cmdBuf[2] = mbm_tx->addr >> 8;
            mbm_tx->cmdBuf[3] = mbm_tx->addr & 0xFF;
            // 数据个数
            mbm_tx->cmdBuf[4] = mbMaster.Rx.dataCnt >> 8;
            mbm_tx->cmdBuf[5] = mbMaster.Rx.dataCnt & 0xFF;
            // 指令长度
            mbm_tx->cmdByte = 8;
        } break;
        
        case READ_INPUT_REGISTERS: {
            // 构建 Modbus 发送帧
            mbm_tx->cmdBuf[0] = mbm_tx->id;
            mbm_tx->cmdBuf[1] = mbm_tx->fxcode;
            mbm_tx->cmdBuf[2] = mbm_tx->addr >> 8;
            mbm_tx->cmdBuf[3] = mbm_tx->addr & 0xFF;
            // 数据个数
            mbm_tx->cmdBuf[4] = mbMaster.Rx.dataCnt >> 8;
            mbm_tx->cmdBuf[5] = mbMaster.Rx.dataCnt & 0xFF;
            // 指令长度
            mbm_tx->cmdByte = 8;
        } break;
        
        case WRITE_SINGLE_COIL: {
            // 构建 Modbus 发送帧
            mbm_tx->cmdBuf[0] = mbm_tx->id;
            mbm_tx->cmdBuf[1] = mbm_tx->fxcode;
            mbm_tx->cmdBuf[2] = mbm_tx->addr >> 8;
            mbm_tx->cmdBuf[3] = mbm_tx->addr & 0xFF;
            // 数据
            mbm_tx->dataByte = mbConv_Coils_2_Buf(mbm_tx->wtBit, &mbm_tx->cmdBuf[4], mbm_tx->dataCnt);
            // 指令长度
            mbm_tx->cmdByte = 8;
        } break;
        
        case WRITE_SINGLE_REGISTER: {
            // 构建 Modbus 发送帧
            mbm_tx->cmdBuf[0] = mbm_tx->id;
            mbm_tx->cmdBuf[1] = mbm_tx->fxcode;
            mbm_tx->cmdBuf[2] = mbm_tx->addr >> 8;
            mbm_tx->cmdBuf[3] = mbm_tx->addr & 0xFF;
            // 数据
            mbm_tx->dataByte = mbConv_Regs_2_Buf(mbm_tx->wtU16, &mbm_tx->cmdBuf[4], mbm_tx->dataCnt);
            // 指令长度
            mbm_tx->cmdByte = 8;
        } break;
        
        case WRITE_MULTIPLE_COILS: {
            // 构建 Modbus 发送帧
            mbm_tx->cmdBuf[0] = mbm_tx->id;
            mbm_tx->cmdBuf[1] = mbm_tx->fxcode;
            mbm_tx->cmdBuf[2] = mbm_tx->addr >> 8;
            mbm_tx->cmdBuf[3] = mbm_tx->addr & 0xFF;
            // 数据
            mbm_tx->cmdBuf[4] = mbm_tx->dataCnt >> 8;
            mbm_tx->cmdBuf[5] = mbm_tx->dataCnt & 0xFF;
            mbm_tx->dataByte = mbConv_Coils_2_Buf(mbm_tx->wtBit, &mbm_tx->cmdBuf[7], mbm_tx->dataCnt);
            mbm_tx->cmdBuf[6] = mbm_tx->dataByte;
            // 指令长度
            mbm_tx->cmdByte = mbm_tx->dataByte + 9;
        } break;
        
        case WRITE_MULTIPLE_REGISTERS: {
            // 构建 Modbus 发送帧
            mbm_tx->cmdBuf[0] = mbm_tx->id;
            mbm_tx->cmdBuf[1] = mbm_tx->fxcode;
            mbm_tx->cmdBuf[2] = mbm_tx->addr >> 8;
            mbm_tx->cmdBuf[3] = mbm_tx->addr & 0xFF;
            // 数据
            mbm_tx->cmdBuf[4] = mbm_tx->dataCnt >> 8;
            mbm_tx->cmdBuf[5] = mbm_tx->dataCnt & 0xFF;
            mbm_tx->dataByte = mbConv_Regs_2_Buf(mbm_tx->wtU16, &mbm_tx->cmdBuf[7], mbm_tx->dataCnt);
            mbm_tx->cmdBuf[6] = mbm_tx->dataByte;
            // 指令长度
            mbm_tx->cmdByte = mbm_tx->dataByte + 9;
        } break;        
    }

    // 计算 CRC
    uint16_t tx_crc = mbCal_CRC16(mbm_tx->cmdBuf, mbm_tx->cmdByte - 2);
    mbm_tx->cmdBuf[mbm_tx->cmdByte - 2] = tx_crc & 0xFF;
    mbm_tx->cmdBuf[mbm_tx->cmdByte - 1] = (tx_crc >> 8) & 0xFF;

    // 发送数据（使用中断方式）
    return MB_NO_ERR; // 数据有效
}

mbFxStatus_t mbMaster_Processing_Response(void) { 
//    mbCommand_t *mbm_tx = &mbMaster.Tx;
    mbCommand_t *mbm_rx = &mbMaster.Rx;

    // 检查CRC
    if(!mbVerify_Is_Same_Crc16(mbm_rx->cmdBuf, mbm_rx->cmdByte)) {
        return MB_CMD_CRC_ERR;
    }

    // 从机返回异常码
    if (mbm_rx->cmdBuf[1] & 0x80){
        return (mbFxStatus_t)mbm_rx->cmdBuf[2]; 
    }

    // 提取fxcode
    mbm_rx->fxcode = mbm_rx->cmdBuf[1];
    if(!mbVerify_Is_Valid_FxCode(mbm_rx->fxcode)){
        return MB_CMD_FXCODE_ERR;
    }

    // 根据功能码配置
    switch (mbm_rx->fxcode) {
        case READ_COILS: {
            // 检查指令长度
            mbm_rx->dataByte = (mbm_rx->dataCnt + 7) / 8;
            uint16_t cmdByte = mbm_rx->dataByte + 5;
            if (mbm_rx->cmdByte != cmdByte) {
                return MB_CMD_LEN_ERR;
            }
            // 检查字节数
            uint8_t rx_dataByte = mbm_rx->cmdBuf[2];
            if (rx_dataByte != mbm_rx->dataByte){
                return MB_DATA_LEN_ERR; // 接收数据长度不符
            }
            // 数据长度，数据写入
            mbm_rx->addr = mbMaster.Tx.addr;
            bool *p_data = &mbMaster_Coils_Buf[mbm_rx->addr];
            mbConv_Buf_2_Coils(&mbm_rx->cmdBuf[3], p_data, mbm_rx->dataCnt);
            mbMemCpy_Coils(mbm_rx->rdBit, p_data, mbm_rx->dataCnt);
        } break;

        case READ_DISCRETE_INPUTS: {
            // 检查指令长度
            mbm_rx->dataByte = (mbm_rx->dataCnt + 7) / 8;
            uint16_t cmdByte = mbm_rx->dataByte + 5;
            if (mbm_rx->cmdByte != cmdByte) {
                return MB_CMD_LEN_ERR;
            }
            // 检查字节数
            uint8_t rx_dataByte = mbm_rx->cmdBuf[2];
            if (rx_dataByte != mbm_rx->dataByte){
                return MB_DATA_LEN_ERR; // 接收数据长度不符
            }
            // 数据长度，数据写入
            mbm_rx->addr = mbMaster.Tx.addr;
            bool *p_data = &mbMaster_Coils_Buf[mbm_rx->addr];
            mbConv_Buf_2_Coils(&mbm_rx->cmdBuf[3], p_data, mbm_rx->dataCnt);
            mbMemCpy_Coils(mbm_rx->rdBit, p_data, mbm_rx->dataCnt);
        } break;
        
        case READ_HOLDING_REGISTERS: {
            // 检查指令长度
            mbm_rx->dataByte = mbm_rx->dataCnt * 2;
            uint16_t cmdByte = mbm_rx->dataByte + 5;
            if (mbm_rx->cmdByte != cmdByte) {
                return MB_CMD_LEN_ERR;
            }
            // 检查字节数
            uint8_t rx_dataByte = mbm_rx->cmdBuf[2];
            if (rx_dataByte != mbm_rx->dataByte){
                return MB_DATA_LEN_ERR; // 接收数据长度不符
            }
            // 数据长度，数据写入
            mbm_rx->addr = mbMaster.Tx.addr;
            uint16_t *p_data = &mbMaster_Holding_Buf[mbm_rx->addr];
            mbConv_Buf_2_Regs(&mbm_rx->cmdBuf[3], p_data, mbm_rx->dataCnt);
            mbMemCpy_Regs(mbm_rx->rdU16, p_data, mbm_rx->dataCnt);
        } break;
        
        case READ_INPUT_REGISTERS: {
            // 检查指令长度
            mbm_rx->dataByte = mbm_rx->dataCnt * 2;
            uint16_t cmdByte = mbm_rx->dataByte + 5;
            if (mbm_rx->cmdByte != cmdByte) {
                return MB_CMD_LEN_ERR;
            }
            // 检查字节数
            uint8_t rx_dataByte = mbm_rx->cmdBuf[2];
            if (rx_dataByte != mbm_rx->dataByte){
                return MB_DATA_LEN_ERR; // 接收数据长度不符
            }
            // 数据长度，数据写入
            mbm_rx->addr = mbMaster.Tx.addr;
            uint16_t *p_data = &mbMaster_Holding_Buf[mbm_rx->addr];
            mbConv_Buf_2_Regs(&mbm_rx->cmdBuf[3], p_data, mbm_rx->dataCnt);
            mbMemCpy_Regs(mbm_rx->rdU16, p_data, mbm_rx->dataCnt);
        } break;
        
        case WRITE_SINGLE_COIL: {
            // 检查指令长度
            uint16_t cmdByte = 8;
            if (mbm_rx->cmdByte != cmdByte) {
                return MB_CMD_LEN_ERR;
            }
            // 检查地址
            mbm_rx->addr = mbMaster.Tx.addr;
            uint16_t rx_addr = mbm_rx->cmdBuf[2] << 8 | mbm_rx->cmdBuf[3];
            if (rx_addr != mbm_rx->addr){
                return MB_CMD_ADDR_ERR; // 接收数据地址不符
            }
            // 数据长度，数据写入
            mbm_rx->dataCnt = 1;
            bool rx_data;
            mbConv_Buf_2_Coils(&mbm_rx->cmdBuf[4], &rx_data, 1);
            if (rx_data != *mbMaster.Tx.wtBit){
                return MB_DATA_ERR; // 接收数据不符
            }
            bool *p_data = &mbMaster_Coils_Buf[mbm_rx->addr];
            mbMemCpy_Coils(p_data, &rx_data, mbm_rx->dataCnt);
        } break;
        
        case WRITE_SINGLE_REGISTER: {
            // 检查指令长度
            uint16_t cmdByte = 8;
            if (mbm_rx->cmdByte != cmdByte) {
                return MB_CMD_LEN_ERR;
            }
            // 检查地址
            mbm_rx->addr = mbMaster.Tx.addr;
            uint16_t rx_addr = mbm_rx->cmdBuf[2] << 8 | mbm_rx->cmdBuf[3];
            if (rx_addr != mbm_rx->addr){
                return MB_CMD_ADDR_ERR; // 接收数据地址不符
            }
            // 数据长度，数据写入
            mbm_rx->dataCnt = 1;
            uint16_t rx_data;
            mbConv_Buf_2_Regs(&mbm_rx->cmdBuf[4], &rx_data, mbm_rx->dataCnt);
            if (rx_data != *mbMaster.Tx.wtU16){
                return MB_DATA_ERR; // 接收数据不符
            }
            uint16_t *p_data = &mbMaster_Holding_Buf[mbm_rx->addr];
            mbMemCpy_Regs(p_data, &rx_data, mbm_rx->dataCnt);
        } break;
        
        case WRITE_MULTIPLE_COILS: {
            // 检查指令长度
            uint16_t cmdByte = 8;
            if (mbm_rx->cmdByte != cmdByte) {
                return MB_CMD_LEN_ERR;
            }
            // 检查地址
            mbm_rx->addr = mbMaster.Tx.addr;
            uint16_t rx_addr = mbm_rx->cmdBuf[2] << 8 | mbm_rx->cmdBuf[3];
            if (rx_addr != mbm_rx->addr){
                return MB_CMD_ADDR_ERR; // 接收数据地址不符
            }
            // 数据长度，数据写入
            mbm_rx->dataCnt = mbm_rx->cmdBuf[4] << 8 | mbm_rx->cmdBuf[5];
            if (mbm_rx->dataCnt != mbMaster.Tx.dataCnt){
                return MB_DATA_ERR; // 接收数据长度不符
            }
            bool *p_data = &mbMaster_Coils_Buf[mbm_rx->addr];
            mbMemCpy_Coils(p_data, mbMaster.Tx.wtBit, mbm_rx->dataCnt);            
        } break;
        
        case WRITE_MULTIPLE_REGISTERS: {
            // 检查指令长度
            uint16_t cmdByte = 8;
            if (mbm_rx->cmdByte != cmdByte) {
                return MB_CMD_LEN_ERR;
            }
            // 检查地址
            mbm_rx->addr = mbMaster.Tx.addr;
            uint16_t rx_addr = mbm_rx->cmdBuf[2] << 8 | mbm_rx->cmdBuf[3];
            if (rx_addr != mbm_rx->addr){
                return MB_CMD_ADDR_ERR; // 接收数据地址不符
            }
            // 数据长度，数据写入
            mbm_rx->dataCnt = mbm_rx->cmdBuf[4] << 8 | mbm_rx->cmdBuf[5];
            if (mbm_rx->dataCnt != mbMaster.Tx.dataCnt){
                return MB_DATA_ERR; // 接收数据长度不符
            }
            uint16_t *p_data = &mbMaster_Holding_Buf[mbm_rx->addr];
            mbMemCpy_Regs(p_data, mbMaster.Tx.wtU16, mbm_rx->dataCnt);
        } break;        
    }
    return MB_NO_ERR;
}

#endif // ENABLE_MB_RTU_MASTER

/**
 * @brief       ENABLE_MB_RTU_SLAVE
 * @param       无
 * @retval      无
 */	
#if  ENABLE_MB_RTU_SLAVE 
mbRTU_t  mbSlave = {0};
bool     mbSlave_Coils_Buf[MB_BUF_LEN];
uint16_t mbSlave_Holding_Buf[MB_BUF_LEN];

void mbSlave_cmd_clr(void) {    
    memset(&mbSlave.Tx, 0, sizeof(mbCommand_t));    
    memset(&mbSlave.Rx, 0, sizeof(mbCommand_t));    
}

void mbSlave_Read_Coils             (uint16_t addr, bool *p_data,     uint16_t cnt){
    for (uint16_t i = 0; i < cnt; i++) {
        p_data[i] = mbSlave_Coils_Buf[i + addr];
    }
}

void mbSlave_Read_Discrete_Inputs   (uint16_t addr, bool *p_data,     uint16_t cnt){
    for (uint16_t i = 0; i < cnt; i++) {
        p_data[i] = mbSlave_Coils_Buf[i + addr];
    }
}

void mbSlave_Read_Holdings          (uint16_t addr, uint16_t *p_data, uint16_t cnt){
    for (uint16_t i = 0; i < cnt; i++) {
        p_data[i] = mbSlave_Holding_Buf[i + addr];
    }
}

void mbSlave_Read_Input             (uint16_t addr, uint16_t *p_data, uint16_t cnt){
    for (uint16_t i = 0; i < cnt; i++) {
        p_data[i] = mbSlave_Holding_Buf[i + addr];
    }
}

void mbSlave_Write_Single_Coil      (uint16_t addr, bool data){
    mbSlave_Coils_Buf[addr] = data;
}

void mbSlave_Write_Single_Reg       (uint16_t addr, uint16_t data){
    mbSlave_Holding_Buf[addr] = data;
}

void mbSlave_Write_Multiple_Coils   (uint16_t addr, bool *p_data,     uint16_t cnt){
    for (uint16_t i = 0; i < cnt; i++) {
        mbSlave_Coils_Buf[i + addr] =  p_data[i];
    }
}

void mbSlave_Write_Multiple_Regs    (uint16_t addr, uint16_t *p_data, uint16_t cnt){
    for (uint16_t i = 0; i < cnt; i++) {
        mbSlave_Holding_Buf[i + addr] = p_data[i];
    }
}

void mbSlave_FIFO_Write_Regs        (uint16_t addr, uint16_t *p_data, uint16_t cnt, uint16_t size) {
    if (cnt > size)
        return;
    
    uint16_t index = addr;
    uint16_t move_len = size - cnt;
    
    for (uint16_t i = 0; i < move_len; i++) {
        mbSlave_Holding_Buf[index + i] = mbSlave_Holding_Buf[index + cnt + i];
    }

    for (uint16_t i = 0; i < cnt; i++) {
        mbSlave_Holding_Buf[index + move_len + i] = p_data[i];
    }
}

#if ENABLE_MOUBUS_DMACPY 
void mbSlave_FIFO_Write_Regs_DMA(uint16_t addr, uint16_t *p_data, uint16_t cnt, uint16_t size) {
    if (cnt > size) return;

    fifo_dma_ctx.index = addr;
    fifo_dma_ctx.cnt = cnt;
    fifo_dma_ctx.move_len = size - cnt;
    fifo_dma_ctx.p_data = p_data;
    fifo_dma_ctx.step = 0;

    HAL_DMA_Start_IT(&hdma_mb_cpy,
                     (uint32_t)&mbSlave_Holding_Buf[addr + cnt],
                     (uint32_t)&mbSlave_Holding_Buf[addr],
                     fifo_dma_ctx.move_len);
}

void mbSlave_FIFO_Write_Regs_DMA_Callback(DMA_HandleTypeDef *hdma) {
    if (hdma == &hdma_mb_cpy) {
        if (fifo_dma_ctx.step == 0) {
            fifo_dma_ctx.step = 1;

            HAL_DMA_Start_IT(&hdma_mb_cpy,
                             (uint32_t)fifo_dma_ctx.p_data,
                             (uint32_t)&mbSlave_Holding_Buf[fifo_dma_ctx.index + fifo_dma_ctx.move_len],
                             fifo_dma_ctx.cnt);
        } else {
            fifo_dma_ctx.step = 0;
            // 所有 DMA 任务完成，可设标志或回调通知上层
        }
    }
}

void DMA2_Channel4_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_mb_cpy);
}
#endif // ENABLE_MOUBUS_DMACPY
mbFxStatus_t mbSlave_Processing_Request(void){
    mbCommand_t *mbs_rx = &mbSlave.Rx;
    // 检查CRC
    if(!mbVerify_Is_Same_Crc16(mbs_rx->cmdBuf, mbs_rx->cmdByte)) {
        return MB_CMD_CRC_ERR; // CRC异常
    }

    // 提取ID
    mbs_rx->id = mbs_rx->cmdBuf[0];
    if(!mbVerify_Is_Valid_Id(mbs_rx->id)){
        return MB_CMD_ID_ERR;
    }

    // 提取fxcode
    mbs_rx->fxcode = mbs_rx->cmdBuf[1];
    if(!mbVerify_Is_Valid_FxCode(mbs_rx->fxcode)){
        return MB_CMD_FXCODE_ERR;
    }

    // 提取addr
    mbs_rx->addr = mbs_rx->cmdBuf[2] << 8 | mbs_rx->cmdBuf[3];
    if (!mbVerify_Is_Valid_Addr(mbs_rx->addr)){
        return MB_CMD_ADDR_ERR;
    }
        
    // 根据功能码分别解析
    switch(mbs_rx->fxcode) {    
        case READ_COILS: {
            mbs_rx->dataCnt = mbs_rx->cmdBuf[4] << 8 | mbs_rx->cmdBuf[5];
            if (!mbVerify_Is_Valid_DataLen(mbs_rx->addr, mbs_rx->dataCnt)){
                return MB_DATA_LEN_NO_MATCH;
            }
        
        } break;

        case READ_DISCRETE_INPUTS: {
            mbs_rx->dataCnt = mbs_rx->cmdBuf[4] << 8 | mbs_rx->cmdBuf[5];
            if (!mbVerify_Is_Valid_DataLen(mbs_rx->addr, mbs_rx->dataCnt)){
                return MB_DATA_LEN_NO_MATCH;
            }
        
        } break;        
   
        case READ_HOLDING_REGISTERS:  {
            mbs_rx->dataCnt = mbs_rx->cmdBuf[4] << 8 | mbs_rx->cmdBuf[5];
            if (!mbVerify_Is_Valid_DataLen(mbs_rx->addr, mbs_rx->dataCnt)){
                return MB_DATA_LEN_NO_MATCH;
            }
        
        } break;

        case READ_INPUT_REGISTERS: {
            mbs_rx->dataCnt = mbs_rx->cmdBuf[4] << 8 | mbs_rx->cmdBuf[5];
            if (!mbVerify_Is_Valid_DataLen(mbs_rx->addr, mbs_rx->dataCnt)){
                return MB_DATA_LEN_NO_MATCH;
            }
        
        } break;        

        case WRITE_SINGLE_COIL:{
            mbs_rx->dataCnt = 1;
            if (!mbVerify_Is_Valid_DataLen(mbs_rx->addr, mbs_rx->dataCnt)){
                return MB_DATA_LEN_NO_MATCH;
            }
            // 单个线圈按 0xFF00 / 0x0000 解析
            bool *data = &mbSlave_Coils_Buf[mbs_rx->addr];
            mbConv_Buf_2_Coils(&mbs_rx->cmdBuf[4], data, 1);
        
        } break;
        
        case WRITE_SINGLE_REGISTER:{
            mbs_rx->dataCnt = 1;
            if (!mbVerify_Is_Valid_DataLen(mbs_rx->addr, mbs_rx->dataCnt)){
                return MB_DATA_LEN_NO_MATCH;
            }
            // 写数据
            uint16_t *data = &mbSlave_Holding_Buf[mbs_rx->addr];
            mbConv_Buf_2_Regs(&mbs_rx->cmdBuf[4], data, mbs_rx->dataCnt);
        
        } break;

        case WRITE_MULTIPLE_COILS: {
            mbs_rx->dataCnt = mbs_rx->cmdBuf[4] << 8 | mbs_rx->cmdBuf[5];
            if (!mbVerify_Is_Valid_DataLen(mbs_rx->addr, mbs_rx->dataCnt)){
                return MB_DATA_LEN_NO_MATCH;
            }
            // 提取byte
            mbs_rx->dataByte = mbs_rx->cmdBuf[6];
            if (mbs_rx->dataByte + 9 != mbs_rx->cmdByte){
                return MB_DATA_LEN_ERR;
            }
            // 多个线圈按 bit 解析
            bool *data = &mbSlave_Coils_Buf[mbs_rx->addr];
            mbConv_Buf_2_Coils(&mbs_rx->cmdBuf[7], data, mbs_rx->dataCnt);            
        
        } break;        

        case WRITE_MULTIPLE_REGISTERS: {
            mbs_rx->dataCnt = mbs_rx->cmdBuf[4] << 8 | mbs_rx->cmdBuf[5];
            if (!mbVerify_Is_Valid_DataLen(mbs_rx->addr, mbs_rx->dataCnt)){
                return MB_DATA_LEN_NO_MATCH;
            }
            // 提取byte
            mbs_rx->dataByte = mbs_rx->cmdBuf[6];
            if (mbs_rx->dataByte + 9 != mbs_rx->cmdByte){
                return MB_DATA_LEN_ERR;
            }
            // 写数据
            uint16_t *data = &mbSlave_Holding_Buf[mbs_rx->addr];
            mbConv_Buf_2_Regs(&mbs_rx->cmdBuf[7], data, mbs_rx->dataCnt);                    
        } break;
    }    
    return MB_NO_ERR; // 数据有效    
}

mbFxStatus_t mbSlave_Structuring_Response(void){
    mbCommand_t *mbs_tx = &mbSlave.Tx;
    
    // 配置ID
    mbs_tx->id = mbSlave.ID;
    mbs_tx->cmdBuf[0] = mbs_tx->id;

    // 检查异常
    if (mbSlave.FxStatus != MB_NO_ERR){
        mbs_tx->cmdBuf[1] = mbSlave.FxStatus;    
        mbs_tx->cmdBuf[2] = 0;
        mbs_tx->cmdByte = 5;
    } else {
        // 配置功能码
        mbs_tx->fxcode    = mbSlave.Rx.fxcode;
        mbs_tx->cmdBuf[1] = mbs_tx->fxcode;

        mbs_tx->addr      = mbSlave.Rx.addr;
        mbs_tx->dataCnt   = mbSlave.Rx.dataCnt;

        // 根据功能码分别解析
        // 配置地址，数据个数，数据字节数
        switch(mbs_tx->fxcode) {
            case READ_COILS: {
                bool *data = &mbSlave_Coils_Buf[mbs_tx->addr];
                mbs_tx->dataByte = mbConv_Coils_2_Buf(data, &mbs_tx->cmdBuf[3], mbs_tx->dataCnt);
                mbs_tx->cmdBuf[2] = mbs_tx->dataByte;
                mbs_tx->cmdByte = 5 + mbs_tx->dataByte;
            } break;
            
            case READ_DISCRETE_INPUTS: {
                bool *data = &mbSlave_Coils_Buf[mbs_tx->addr];
                mbs_tx->dataByte = mbConv_Coils_2_Buf(data, &mbs_tx->cmdBuf[3], mbs_tx->dataCnt);
                mbs_tx->cmdBuf[2] = mbs_tx->dataByte;
                mbs_tx->cmdByte = 5 + mbs_tx->dataByte;
            } break;
            
            case READ_HOLDING_REGISTERS: {
                uint16_t *data = &mbSlave_Holding_Buf[mbs_tx->addr];
                mbs_tx->dataByte = mbConv_Regs_2_Buf(data, &mbs_tx->cmdBuf[3], mbs_tx->dataCnt);
                mbs_tx->cmdBuf[2] = mbs_tx->dataByte;
                mbs_tx->cmdByte = 5 + mbs_tx->dataByte;
            } break;
            
            case READ_INPUT_REGISTERS: {
                uint16_t *data = &mbSlave_Holding_Buf[mbs_tx->addr];
                mbs_tx->dataByte = mbConv_Regs_2_Buf(data, &mbs_tx->cmdBuf[3], mbs_tx->dataCnt);
                mbs_tx->cmdBuf[2] = mbs_tx->dataByte;
                mbs_tx->cmdByte = 5 + mbs_tx->dataByte;
            } break;
            
            case WRITE_SINGLE_COIL: {
                mbs_tx->cmdBuf[2] = mbs_tx->addr >> 8; 
                mbs_tx->cmdBuf[3] = mbs_tx->addr & 0xFF;
                bool *data = &mbSlave_Coils_Buf[mbs_tx->addr];
                mbConv_Coils_2_Buf(data, &mbs_tx->cmdBuf[4], 1);
                mbs_tx->cmdByte = 8;
            } break;

            case WRITE_SINGLE_REGISTER: {
                mbs_tx->cmdBuf[2] = mbs_tx->addr >> 8; 
                mbs_tx->cmdBuf[3] = mbs_tx->addr & 0xFF;
                uint16_t *data = &mbSlave_Holding_Buf[mbs_tx->addr];
                mbConv_Regs_2_Buf(data, &mbs_tx->cmdBuf[4], 1);
                mbs_tx->cmdByte = 8;        
            } break;
            
            case WRITE_MULTIPLE_COILS: {
                mbs_tx->cmdBuf[2] = mbs_tx->addr >> 8; 
                mbs_tx->cmdBuf[3] = mbs_tx->addr & 0xFF;
                mbs_tx->cmdBuf[4] = mbs_tx->dataCnt >> 8;
                mbs_tx->cmdBuf[5] = mbs_tx->dataCnt & 0xFF;
                mbs_tx->cmdByte = 8;
            } break;

            case WRITE_MULTIPLE_REGISTERS: {
                mbs_tx->cmdBuf[2] = mbs_tx->addr >> 8; 
                mbs_tx->cmdBuf[3] = mbs_tx->addr & 0xFF;
                mbs_tx->cmdBuf[4] = mbs_tx->dataCnt >> 8;
                mbs_tx->cmdBuf[5] = mbs_tx->dataCnt & 0xFF;
                mbs_tx->cmdByte = 8;
            } break;
        }    
    }

    // 计算 CRC
    uint16_t tx_crc = mbCal_CRC16(mbs_tx->cmdBuf, mbs_tx->cmdByte - 2);
    mbs_tx->cmdBuf[mbs_tx->cmdByte - 2] = tx_crc & 0xFF;
    mbs_tx->cmdBuf[mbs_tx->cmdByte - 1] = (tx_crc >> 8) & 0xFF;
    
    return MB_NO_ERR; // 数据有效  
}

#endif // ENABLE_MB_RTU_SLAVE

// ------------------ 通用功能 -------------------------
/**
 * @brief       数据结构
 */
void mbMemCpy_Buf(uint8_t *target, uint8_t *source, uint16_t len) {
    if (target == NULL || source == NULL || len < 1) return;
    for(uint16_t i = 0; i < len; i++){
        target[i] = source[i];
    }
}

void mbMemCpy_Coils(bool *target, bool *source, uint16_t len) {
    if (target == NULL || source == NULL || len < 1) return;
    for(uint16_t i = 0; i < len; i++){
        target[i] = source[i];
    }
}

void mbMemCpy_Regs(uint16_t *target, uint16_t *source, uint16_t len) {
    if (target == NULL || source == NULL || len < 1) return;
    for(uint16_t i = 0; i < len; i++){
        target[i] = source[i];
    }
}

/**
 * @brief       计算
 */
uint16_t mbCal_CRC16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;  		/* 初始化CRC寄存器为0xFFFF */ 
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];  			/* 从数据中异或当前字节到CRC寄存器 */ 
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;  /* 如果CRC的最低位为1，右移一位并与0xA001异或 */ 
            } else {
                crc >>= 1;  /* 如果CRC的最低位为0，只右移一位 */ 
            }
        }
    }    
    return crc;  // 返回计算得到的CRC16校验值
}

/**
 * @brief       转换
 */
uint8_t mbConv_Coils_2_Buf(const bool *coils, uint8_t *buf, uint16_t cnt) {
    // 安全检查
    if (!coils || !buf) return 0;
    if (cnt == 1) {
        buf[0] = (*coils) ? 0xFF : 0x00;
        buf[1] = 0x00;  // 高位始终为0x00    
    } else {
        // 逐位打包（LSB优先）
        for (uint16_t i = 0; i < cnt; i++) {
            if (coils[i]) {
                buf[i / 8] |= (1 << (i % 8));
            }
        }    
    }
    return (cnt == 1) ? 2 : ((cnt + 7) / 8);
}

uint8_t mbConv_Regs_2_Buf(const uint16_t *regs, uint8_t *buf, uint16_t cnt) {
    if (!regs || !buf) return 0;
    for (uint16_t i = 0; i < cnt; i++) {
        buf[i * 2]     = regs[i] >> 8;
        buf[i * 2 + 1] = regs[i] & 0xFF;
    }    
    return (cnt * 2);
}

uint8_t mbConv_Buf_2_Coils(const uint8_t *buf, bool *coils, uint16_t cnt) {
    if (!buf || !coils) return 0;
    if (cnt == 1) {
        *coils = ((buf[0] << 8 | buf[1]) == 0xFF00) ? 1 : 0;
    } else {
        for (uint16_t i = 0; i < cnt; i++) {
            uint16_t byte_pos = i / 8;   // 当前字节位置
            uint8_t bit_pos = i % 8;     // 当前位位置
            coils[i] = (buf[byte_pos] >> bit_pos) & 0x01;  // LSB优先
        }
    }
    return ((cnt - 1) / 8 + 1);
}

uint8_t mbConv_Buf_2_Regs(const uint8_t *buf, uint16_t *regs, uint16_t cnt) {
    if (!buf || !regs) return 0;
    for (uint16_t i = 0; i < cnt; i++) {
        regs[i] = buf[i * 2] << 8 | buf[i * 2 + 1];
    }
    return (cnt * 2);
}

/**
 * @brief       检验
 * @param       返回0代表异常，返回1代表正常
 */
bool mbVerify_Is_Same_Crc16(const uint8_t *data, uint16_t len) {
    // 添加长度检查，确保至少有2个字节用于CRC
    if (len < 2) {
        return false;
    }
    
    uint16_t received_crc = (data[len - 1] << 8) | data[len - 2];
    uint16_t calculated_crc = mbCal_CRC16(data, len - 2);
    
    return (calculated_crc == received_crc);
}

bool mbVerify_Is_Valid_Id(uint8_t id){
    return (id <= MB_SLAVE_ID_MAX);
}

bool mbVerify_Is_Valid_FxCode(uint8_t fxcode) {
    switch (fxcode) {
        case READ_COILS:
        case READ_DISCRETE_INPUTS:
        case READ_HOLDING_REGISTERS:
        case READ_INPUT_REGISTERS:
        case WRITE_SINGLE_COIL:
        case WRITE_SINGLE_REGISTER:
        case WRITE_MULTIPLE_COILS:
        case WRITE_MULTIPLE_REGISTERS:
        case MASK_WRITE_REGISTER:
        case READ_FILE_RECORD:
        case WRITE_FILE_RECORD:
        case READ_DEVICE_IDENTIFICATION:
            return true;
        default:
            return false;
    }
}

bool mbVerify_Is_Valid_Addr(uint16_t addr) {
    return (addr <= MB_REG_ADDR_MAX);
}

bool mbVerify_Is_Valid_DataLen(uint16_t addr, uint16_t cnt) {
    return (addr + cnt >= 1 && addr + cnt <= MB_REG_ADDR_MAX); // 仍然需要检查 addr + cnt 是否越界
}
