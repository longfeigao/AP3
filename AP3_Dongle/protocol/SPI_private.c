/*
 * SPI_private.c
 *
 *  Created on: 2018年8月13日
 *      Author: ggg
 *      private protocol
 *      SN(1Byte) | LEN(2Byte) | Data(0-512) | CRC(2Byte)
 *      SN：子板成功接收数据后，SN号加1。返回给母板，母板用SN号组下一包数据。
 *      LEN:length。最后一包数据由bit12位置1（0x1000）表示。
 *      Data:data
 *      CRC: sn+len+data的CRC。
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Board.h"
#include <ti/drivers/GPIO.h>
#include "event.h"
#include "protocol.h"
#include "appSPI.h"
#include "core.h"
#include "crc16.h"
#include "debug.h"
#include "bsp_uart.h"

#define SPI_CRC_ERR     (uint16_t)(0X2000)
#define SPI_QUERY       (uint16_t)(0X4000)
#define SPI_LAST_PCK    (uint16_t)(0X1000)
#define SPI_LEN_MASK    (uint16_t)(0X0FFF)

void SPIPrivate_dataInit(uint8_t* tmp_buf, uint16_t tmp_len);
int32_t SPIPrivate_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout);
int32_t SPIPrivate_recv(uint8_t* read_buf, uint16_t tmp_len);
uint8_t *SPIPrivate_getData(uint32_t *len);
int32_t SPIPrivate_recvToFlash(sn_t *x, uint32_t addr, int32_t dst_len, int32_t timeout);
int32_t SPIPrivate_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout);

st_protocolFnxTable SPIPrivateFnx={
.dataInitFnx    =   SPIPrivate_dataInit,
.sendFnx        =   SPIPrivate_send,
.recvFnx        =   SPIPrivate_recv,
.getDataFnx     =   SPIPrivate_getData,
.sendFromFlashFnx = SPIPrivate_sendFromFlash,
.recvToFlashFnx =   SPIPrivate_recvToFlash
};


typedef enum{
    ST_SPI_INIT = (uint8_t)0,
    ST_SPI_PACKET_SEND_DATA,
    ST_SPI_WRITE_SEND_BUF,
    ST_SPI_SEND_DATA,
    ST_SPI_SEND_LAST_DATA,
    ST_SPI_PACKET_CHECK_DATA,
    ST_SPI_RECV_DATA,
    ST_SPI_RECV_LAST_DATA,
    ST_SPI_ERR,
    ST_SPI_END,

}emPrivateState;

static sn_t spi_sn;
static volatile Bool SPI_writeFlashFlg   = false;
static volatile Bool SPI_recCmdAckFlg   = false;
static volatile  INT16  spi_recv_len_once = 0;
static emPrivateState privateState = ST_SPI_INIT;

void SPIPrivate_dataInit(uint8_t* tmp_buf, uint16_t len)
{
    privateState = ST_SPI_INIT;
    memset((uint8_t*)&spi_sn, 0, sizeof(sn_t));
    SPIP_DEBUG(("SPIPrivate_dataInit\r\n"));
}

void packetData(uint8_t* dst, uint8_t* src, st_SPI_private* pckst)
{

}

int32_t SPIPrivate_send(sn_t *x, uint8_t *src, int32_t len, int32_t timeout)
{
    st_SPI_private tmp;
    st_SPI_privateHead send_check;
    uint8_t *tx_ptr = transaction.txBuf;
    uint8_t *rx_ptr = transaction.rxBuf;
    uint8_t *tmp_ptr = src;
    uint16_t calu_crc;
//    uint8_t *send_ptr = src;
    volatile uint8_t exit_flg = false;
    int32_t ret_len = 0;

    SPIP_DEBUG(("---SPIPrivate_send---\r\n"));
    while(false == exit_flg){
        switch (privateState){
            case ST_SPI_INIT:
                privateState = ST_SPI_PACKET_SEND_DATA;
                memset((uint8_t*)&send_check, 0, sizeof(send_check));
                SPIP_DEBUG(("initExit"));
                break;
            case ST_SPI_PACKET_SEND_DATA:
                GPIO_write(Board_SPI_SLAVE_READY, 1);
                tmp.head.sn = x->send_sn;
                if (len > SPIPRIVATE_LEN_DAT){
                    len -= SPIPRIVATE_LEN_DAT;
                    tmp.head.len = SPIPRIVATE_LEN_DAT;
                    ret_len += tmp.head.len;
                }else{
                    tmp.head.len = len > 0 ? len : 0;
                    ret_len += tmp.head.len;
                    len -= tmp.head.len;
                    tmp.head.len |= SPI_LAST_PCK;

                }
                tmp_ptr = src;
                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&tmp , sizeof(st_SPI_privateHead));
                tmp.crc = CRC16_CaculateStepByStep(calu_crc,  tmp_ptr, tmp.head.len);

                src += tmp.head.len;
                privateState = ST_SPI_WRITE_SEND_BUF;
                SPIP_DEBUG(("packetDataExit"));
                break;
            case ST_SPI_WRITE_SEND_BUF:
                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
                memcpy(tx_ptr+sizeof(st_SPI_privateHead), tmp_ptr, tmp.head.len);
                memcpy(tx_ptr+sizeof(st_SPI_privateHead)+tmp.head.len, (uint8_t*)&tmp.crc, sizeof(tmp.crc));
                privateState = ST_SPI_SEND_DATA;
                SPIP_DEBUG(("writeBufExit"));
                break;
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG(("->ST_SPI_SEND_DATA\r\n"));
                SPI_recCmdAckFlg = true;
                GPIO_write(Board_SPI_SLAVE_READY, 0);

                if (true==Device_Recv_pend(EVENT_WAIT_US(1000000))){
                    spi_sn.send_retry_times = 0;
                    SPI_recCmdAckFlg = false;
                    privateState = ST_SPI_RECV_DATA;
                    SPIP_DEBUG(("sendOk"));
                }else{
                    GPIO_write(Board_SPI_SLAVE_READY, 1);
                    privateState = ST_SPI_SEND_DATA;
                    SPIP_DEBUG(("sendTimeout"));
                    if (++spi_sn.send_retry_times > 3){
                        spi_sn.send_retry_times = 0;
                        privateState = ST_SPI_ERR;
                        SPI_recCmdAckFlg = false;
                        SPIP_DEBUG(("sendMax3"));
                    }
                }
                SPIP_DEBUG(("sendDataExit"));
                break;
            case ST_SPI_RECV_DATA:
                memcpy((uint8_t*)&tmp, rx_ptr, sizeof(st_SPI_privateHead));
                tmp.buf = (rx_ptr + sizeof(st_SPI_privateHead));
                memcpy((uint8_t*)&tmp.crc, (uint8_t*)tmp.buf + tmp.head.len, sizeof(tmp.crc));
                calu_crc = CRC16_CaculateStepByStep(0, rx_ptr, sizeof(st_SPI_privateHead)+(tmp.head.len&SPI_LEN_MASK));
                if (send_check.len == SPI_QUERY){
                    memset((uint8_t*)&send_check, 0, sizeof(send_check));
                    if (calu_crc==tmp.crc && tmp.head.sn==x->send_sn){
                        spi_sn.nak_times = 0;
                        if (len > 0){
                            privateState = ST_SPI_PACKET_SEND_DATA;
                            x->send_sn++;
                        }else{
                            privateState = ST_SPI_END;
                        }
                    }else{
                        if (++spi_sn.nak_times > 3){
                            spi_sn.nak_times = 0;
                            privateState = ST_SPI_ERR;
                            break;
                        }
                    }
                }else{
                    privateState = ST_SPI_PACKET_CHECK_DATA;
                }
                SPIP_DEBUG(("recvDataExit"));
                break;
            case ST_SPI_PACKET_CHECK_DATA:
                send_check.sn = x->send_sn;
                send_check.len = SPI_QUERY;
                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&send_check , sizeof(st_SPI_privateHead));
                memcpy(tx_ptr, (uint8_t*)&send_check, sizeof(st_SPI_privateHead));
                memcpy(tx_ptr+sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));

                privateState = ST_SPI_SEND_DATA;
                SPIP_DEBUG(("sendCheckExit"));
                break;
            case ST_SPI_ERR:
                ret_len = 0;
            case ST_SPI_END:
                tmp.head.sn = 0;
                tmp.head.len = SPI_LAST_PCK;
                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&tmp , sizeof(st_SPI_privateHead));
                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
                memcpy(tx_ptr+sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));
                privateState = ST_SPI_INIT;
                exit_flg = true;
                SPIP_DEBUG(("endExit"));
                break;
            default:
                privateState = ST_SPI_INIT;
                exit_flg = true;
                memset((uint8_t*)&x, 0, sizeof(sn_t));
                SPIP_DEBUG(("case default"));
                break;
        }
    }
    SPIP_DEBUG(("---SPIPrivate_send exit---\r\n"));
    return ret_len;
}

int32_t SPIPrivate_recv(uint8_t* read_buf, uint16_t len)
{
    st_SPI_private tmp;
    uint8_t *tx_ptr = transaction.txBuf;
    uint8_t *rx_ptr = transaction.rxBuf;
    uint16_t calu_crc;
    volatile uint8_t exit_flg = false;
    int32_t ret_len = 0;

    SPIP_DEBUG(("---SPIPrivate_recv---\r\n"));
    while(false == exit_flg){

        switch (privateState){
            case ST_SPI_INIT:
                memset((uint8_t*)&spi_sn, 0, sizeof(spi_sn));
                privateState = ST_SPI_RECV_DATA;
                SPIP_DEBUG(("InitExit%d\r\n", exit_flg));
                break;
            case ST_SPI_RECV_DATA:
            {
                int32_t tmp_len = 0;

                SPIP_DEBUG(("->ST_SPI_RECV_DATA\r\n"));
                GPIO_write(Board_SPI_SLAVE_READY, 1);
                memcpy((uint8_t*)&tmp, rx_ptr, sizeof(st_SPI_privateHead));
                tmp_len = tmp.head.len&SPI_LEN_MASK;
                tmp.buf = (rx_ptr + sizeof(st_SPI_privateHead));
                memcpy((uint8_t*)&tmp.crc, (uint8_t*)tmp.buf + (tmp_len), sizeof(tmp.crc));
                calu_crc = CRC16_CaculateStepByStep(0, rx_ptr, sizeof(st_SPI_privateHead)+tmp_len);
                pdebughex(rx_ptr, tmp_len+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                SPIP_DEBUG(("calu=%x, buf=%x", calu_crc, tmp.crc));

                if (tmp.crc == calu_crc && spi_sn.send_sn==tmp.head.sn){
                    if (ret_len+tmp_len > len){
                        privateState = ST_SPI_ERR;
                        break;
                    }
                    if(tmp.head.len == SPI_QUERY){
                        spi_sn.send_sn = tmp.head.sn+1;
                        privateState = spi_sn.last_recv_cmd==true ? ST_SPI_END : privateState;
                        SPIP_DEBUG(("query"));
                        SPIP_DEBUG(("recvDataExit\r\n"));
                        break;
                    }else{
                        memcpy(read_buf, tmp.buf, tmp_len);
                        spi_sn.last_recv_cmd = tmp.head.len&SPI_LAST_PCK ? true : false;
                        SPIP_DEBUG(("recvData"));
                        ret_len += tmp_len;
                    }
                    tmp.head.len = SPI_LAST_PCK;
                }else{
                    if (++spi_sn.nak_times > 3){
                        spi_sn.nak_times = 0;
                        privateState = ST_SPI_ERR;
                        break;
                    }

                    tmp.head.len = SPI_LAST_PCK;
                    if (tmp.crc != calu_crc){
                        tmp.head.len |= SPI_CRC_ERR;
                    }
                    SPIP_DEBUG(("recvErr"));
                    pdebughex(tx_ptr, sizeof(st_SPI_privateHead)+sizeof(calu_crc));
                }
                tx_ptr[0] = tmp.head.sn;
                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
                calu_crc = CRC16_CaculateStepByStep(0, tx_ptr, sizeof(st_SPI_privateHead)+(tmp.head.len&SPI_LEN_MASK));
                memcpy(tx_ptr + sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));

                pdebughex(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                privateState = ST_SPI_SEND_DATA;
                SPIP_DEBUG(("recvDataExit\r\n"));
                break;
            }
            case ST_SPI_SEND_DATA:
                SPIP_DEBUG(("->ST_SPI_SEND_DATA\r\n"));
                SPI_recCmdAckFlg = true;
                pdebughex(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
                GPIO_write(Board_SPI_SLAVE_READY, 0);

                if (true==Device_Recv_pend(EVENT_WAIT_US(1000000))){
                    spi_sn.send_retry_times = 0;
                    SPI_recCmdAckFlg = false;
                    privateState = ST_SPI_RECV_DATA;
                    SPIP_DEBUG(("sendOk"));
                }else{
                    GPIO_write(Board_SPI_SLAVE_READY, 1);
                    privateState = ST_SPI_SEND_DATA;
                    SPIP_DEBUG(("sendTimeout"));
                    if (++spi_sn.send_retry_times > 3){
                        spi_sn.send_retry_times = 0;
                        privateState = ST_SPI_ERR;
                        SPI_recCmdAckFlg = false;
                        SPIP_DEBUG(("sendMax3"));
                    }
                }

                SPIP_DEBUG(("sendDataExit\r\n"));
                break;
            case ST_SPI_ERR:
                ret_len = 0;
                //no break;
            case ST_SPI_END:
                SPIP_DEBUG(("->ST_SPI_END\r\n"));
                tmp.head.sn = 0;
                tmp.head.len = SPI_LAST_PCK;
                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&tmp , sizeof(st_SPI_privateHead));
                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
                memcpy(tx_ptr+sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));
                privateState = ST_SPI_INIT;
                memset((uint8_t*)&spi_sn, 0, sizeof(spi_sn));
                exit_flg = true;
                SPIP_DEBUG(("endExit\r\n"));
                break;
            default:
                privateState = ST_SPI_INIT;
                exit_flg = true;
                SPIP_DEBUG(("default\r\n"));
                len = 0;
                break;
        }

    }
    SPIP_DEBUG(("---SPIPrivate_recv exit---%d\r\n", exit_flg));
    return ret_len;
}

uint8_t *SPIPrivate_getData(uint32_t *len)
{
    SPIP_DEBUG(("SPIPrivate_getData\r\n"));
    return 0;
}

int32_t SPIPrivate_recvToFlash(sn_t *x, uint32_t addr, int32_t dst_len, int32_t timeout)
{
    st_SPI_private tmp;
    uint8_t *tx_ptr = transaction.txBuf;
    uint8_t *rx_ptr = transaction.rxBuf;
    uint16_t calu_crc;
    volatile uint8_t exit_flg = false;

    SPIP_DEBUG(("---SPIPrivate_recvToFlash---\r\n"));
//    while(false == exit_flg){
//
//        switch (privateState){
//            case ST_SPI_INIT:
//                memset((uint8_t*)&spi_sn, 0, sizeof(spi_sn));
//                privateState = ST_SPI_RECV_DATA;
//                SPIP_DEBUG(("InitExit%d\r\n", exit_flg));
//                break;
//            case ST_SPI_RECV_DATA:
//                SPIP_DEBUG(("->ST_SPI_RECV_DATA\r\n"));
//                GPIO_write(Board_SPI_SLAVE_READY, 1);
//                memcpy((uint8_t*)&tmp, rx_ptr, sizeof(st_SPI_privateHead));
//                tmp.buf = (rx_ptr + sizeof(st_SPI_privateHead));
//                memcpy((uint8_t*)&tmp.crc, (uint8_t*)tmp.buf + (tmp.head.len&SPI_LEN_MASK), sizeof(tmp.crc));
//                calu_crc = CRC16_CaculateStepByStep(0, rx_ptr, sizeof(st_SPI_privateHead)+(tmp.head.len&SPI_LEN_MASK));
//                pdebughex(rx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
//                SPIP_DEBUG(("calu=%x, buf=%x", calu_crc, tmp.crc));
//                if (tmp.crc == calu_crc && spi_sn.send_sn==tmp.head.sn){
//                    if(tmp.head.len == SPI_QUERY){
//                        spi_sn.send_sn = tmp.head.sn+1;
//                        privateState = spi_sn.last_recv_cmd==true ? privateState : ST_SPI_END;
//                        SPIP_DEBUG(("query"));
//                        SPIP_DEBUG(("recvDataExit\r\n"));
//                        break;
//                    }else{
//                        memcpy(read_buf, tmp.buf, (tmp.head.len&SPI_LEN_MASK));
//                        spi_sn.last_recv_cmd = true;
//                        SPIP_DEBUG(("recvData"));
//                    }
//                    tmp.head.len = SPI_LAST_PCK;
//                    SPIP_DEBUG(("len=%d", tmp.head.len));
//                }else{
//                    tmp.head.len = SPI_LAST_PCK;
//                    if (tmp.crc != calu_crc){
//                        tmp.head.len |= SPI_CRC_ERR;
//                    }
//                    SPIP_DEBUG(("recvErr"));
//                    pdebughex(tx_ptr, sizeof(st_SPI_privateHead)+sizeof(calu_crc));
//                }
//                tx_ptr[0] = tmp.head.sn;
//                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
//                calu_crc = CRC16_CaculateStepByStep(0, tx_ptr, sizeof(st_SPI_privateHead)+(tmp.head.len&SPI_LEN_MASK));
//                memcpy(tx_ptr + sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));
//
//                pdebughex(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
//                privateState = ST_SPI_SEND_DATA;
//                SPIP_DEBUG(("recvDataExit\r\n"));
//                break;
//            case ST_SPI_SEND_DATA:
//                SPIP_DEBUG(("->ST_SPI_SEND_DATA\r\n"));
//                SPI_recCmdAckFlg = true;
//                pdebughex(tx_ptr, (tmp.head.len&SPI_LEN_MASK)+sizeof(calu_crc)+sizeof(st_SPI_privateHead));
//                GPIO_write(Board_SPI_SLAVE_READY, 0);
//
//                if (true==Device_Recv_pend(EVENT_WAIT_US(1000000))){
//                    privateState = ST_SPI_RECV_DATA;
//                    SPIP_DEBUG(("sendOk"));
//                    SPI_recCmdAckFlg = false;
//                }else{
//                    GPIO_write(Board_SPI_SLAVE_READY, 1);
//                    privateState = ST_SPI_SEND_DATA;
//                    SPIP_DEBUG(("sendTimeout"));
//                    if (++spi_sn.send_retry_times > 3){
//                        spi_sn.send_retry_times = 0;
//                        privateState = ST_SPI_END;
//                        SPI_recCmdAckFlg = false;
//                        SPIP_DEBUG(("sendMax3"));
//                    }
//                }
//
//                SPIP_DEBUG(("sendDataExit\r\n"));
//                break;
//            case ST_SPI_END:
//                SPIP_DEBUG(("->ST_SPI_END\r\n"));
//                tmp.head.sn = 0;
//                tmp.head.len = SPI_LAST_PCK;
//                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&tmp , sizeof(st_SPI_privateHead));
//                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
//                memcpy(tx_ptr+sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));
//                privateState = ST_SPI_INIT;
//                memset((uint8_t*)&spi_sn, 0, sizeof(spi_sn));
//                exit_flg = true;
//                SPIP_DEBUG(("endExit\r\n"));
//                break;
//            default:
//                privateState = ST_SPI_INIT;
//                exit_flg = true;
//                SPIP_DEBUG(("default\r\n"));
//                break;
//        }
//
//    }
    SPIP_DEBUG(("---SPIPrivate_recvToFlash exit---%d\r\n", exit_flg));
    return 0;
}

int32_t SPIPrivate_sendFromFlash(sn_t *x, uint32_t addr, int32_t len, int32_t timeout)
{
    SPIP_DEBUG(("---SPIPrivate_sendFromFlash---\r\n"));
//    while(false == exit_flg){
//        switch (privateState){
//            case ST_SPI_INIT:
//                privateState = ST_SPI_PACKET_SEND_DATA;
//                memset((uint8_t*)&send_check, 0, sizeof(send_check));
//                SPIP_DEBUG(("initExit"));
//                break;
//            case ST_SPI_PACKET_SEND_DATA:
//                GPIO_write(Board_SPI_SLAVE_READY, 1);
//                tmp.head.sn = x->send_sn;
//                if (len > SPIPRIVATE_LEN_DAT){
//                    len -= SPIPRIVATE_LEN_DAT;
//                    tmp.head.len = SPIPRIVATE_LEN_DAT;
//                }else{
//                    tmp.head.len = len > 0 ? len : 0;
//                    len -= tmp.head.len;
//                    tmp.head.len |= SPI_LAST_PCK;
//                }
//                tmp_ptr = src;
//                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&tmp , sizeof(st_SPI_privateHead));
//                tmp.crc = CRC16_CaculateStepByStep(calu_crc,  tmp_ptr, tmp.head.len);
//
//                src += tmp.head.len;
//                privateState = ST_SPI_WRITE_SEND_BUF;
//                SPIP_DEBUG(("packetDataExit"));
//                break;
//            case ST_SPI_WRITE_SEND_BUF:
//                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
//                memcpy(tx_ptr+sizeof(st_SPI_privateHead), tmp_ptr, tmp.head.len);
//                memcpy(tx_ptr+sizeof(st_SPI_privateHead)+tmp.head.len, (uint8_t*)&tmp.crc, sizeof(tmp.crc));
//                privateState = ST_SPI_SEND_DATA;
//                SPIP_DEBUG(("writeBufExit"));
//                break;
//            case ST_SPI_SEND_DATA:
//                SPIP_DEBUG(("->ST_SPI_SEND_DATA\r\n"));
//                SPI_recCmdAckFlg = true;
//                GPIO_write(Board_SPI_SLAVE_READY, 0);
//
//                if (true==Device_Recv_pend(EVENT_WAIT_US(1000000))){
//                    privateState = ST_SPI_RECV_DATA;
//                    SPIP_DEBUG(("sendOk"));
//                    SPI_recCmdAckFlg = false;
//                }else{
//                    GPIO_write(Board_SPI_SLAVE_READY, 1);
//                    privateState = ST_SPI_SEND_DATA;
//                    SPIP_DEBUG(("sendTimeout"));
//                    if (++spi_sn.send_retry_times > 3){
//                        spi_sn.send_retry_times = 0;
//                        privateState = ST_SPI_END;
//                        SPI_recCmdAckFlg = false;
//                        SPIP_DEBUG(("sendMax3"));
//                    }
//                }
//                SPIP_DEBUG(("sendDataExit"));
//                break;
//            case ST_SPI_RECV_DATA:
//                memcpy((uint8_t*)&tmp, rx_ptr, sizeof(st_SPI_privateHead));
//                tmp.buf = (rx_ptr + sizeof(st_SPI_privateHead));
//                memcpy((uint8_t*)&tmp.crc, (uint8_t*)tmp.buf + tmp.head.len, sizeof(tmp.crc));
//                calu_crc = CRC16_CaculateStepByStep(0, rx_ptr, sizeof(st_SPI_privateHead)+(tmp.head.len&SPI_LEN_MASK));
//                if (send_check.head.len == SPI_QUERY){
//                    memset((uint8_t*)&send_check, 0, sizeof(send_check));
//                    if (calu_crc==tmp.crc && tmp.head.sn==x->send_sn){
//                        if (len > 0){
//                            privateState = ST_SPI_PACKET_SEND_DATA;
//                            x->send_sn++;
//                        }else{
//                            privateState = ST_SPI_END;
//                        }
//                    }
//                }else{
//                    privateState = ST_SPI_PACKET_CHECK_DATA;
//                }
//                SPIP_DEBUG(("recvDataExit"));
//                break;
//            case ST_SPI_PACKET_CHECK_DATA:
//                send_check.head.sn = x->send_sn;
//                send_check.head.len = SPI_QUERY;
//                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&send_check , sizeof(st_SPI_privateHead));
//                memcpy(tx_ptr, (uint8_t*)&send_check, sizeof(st_SPI_privateHead));
//                memcpy(tx_ptr+sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));
//
//                privateState = ST_SPI_SEND_DATA;
//                SPIP_DEBUG(("sendCheckExit"));
//                break;
//            case ST_SPI_END:
//                tmp.head.sn = 0;
//                tmp.head.len = SPI_LAST_PCK;
//                calu_crc = CRC16_CaculateStepByStep(0, (uint8_t*)&tmp , sizeof(st_SPI_privateHead));
//                memcpy(tx_ptr, (uint8_t*)&tmp, sizeof(st_SPI_privateHead));
//                memcpy(tx_ptr+sizeof(st_SPI_privateHead), (uint8_t*)&calu_crc, sizeof(calu_crc));
//                privateState = ST_SPI_INIT;
//                exit_flg = true;
//                SPIP_DEBUG(("endExit"));
//                break;
//            default:
//                privateState = ST_SPI_INIT;
//                exit_flg = true;
//                memset((uint8_t*)&x, 0, sizeof(sn_t));
//                SPIP_DEBUG(("case default"));
//                break;
//        }
//    }
    SPIP_DEBUG(("---SPIPrivate_sendFromFlash exit---\r\n"));
    return 0;
}


extern volatile uint8_t core_idel_flag;
void transferCallback(SPI_Handle handle, SPI_Transaction *trans)
{
    uint8_t *ptr=trans->rxBuf;

    if ((ptr[2] | (uint16_t)ptr[3]<<8)==CORE_CMD_BACK_TO_IDLE && SPIPRIVATE_LEN_ALL==trans->count){
        core_idel_flag = 1;
        SPIP_DEBUG(("a\r\n"));
    }else if (SPI_recCmdAckFlg == true && SPIPRIVATE_LEN_ALL==trans->count){
        Device_Recv_post();
        SPIP_DEBUG(("b\r\n"));
    }else if(SPIPRIVATE_LEN_ALL==trans->count && SPI_writeFlashFlg == true){
        Device_Recv_post();
        SPIP_DEBUG(("c\r\n"));
    }else if (SPIPRIVATE_LEN_ALL==trans->count){
        Event_communicateSet(EVENT_COMMUNICATE_RX_HANDLE);
        SPIP_DEBUG(("d\r\n"));
    }else{
        memset(trans->rxBuf, 0, trans->count);
        SPIP_DEBUG(("e\r\n"));
    }
    spi_recv_len_once = trans->count;
    SPI_transfer(handle, trans);
}


