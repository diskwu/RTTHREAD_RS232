/*
 * rs232.h
 *
 * modify from qiyongzhong rs485 package
 * Change Logs:
 * Date           Author            Notes
 * 2022-01-06     diskwu       		first version
 */

#ifndef __DRV_RS232_H__
#define __DRV_RS232_H__

#include <rtconfig.h>

#define RS232_BYTE_TMO_MIN      2
#define RS232_BYTE_TMO_MAX      15

typedef struct rs232_inst
{
    rt_device_t serial;     //serial device handle
    rt_timer_t received_over_timer;
    rt_mutex_t lock;        //mutex handle
    struct rt_semaphore rx_sem;         //event handle
    rt_uint8_t status;      //connect status
    rt_int32_t byte_tmo;    //receive byte interval timeout, ms
    rt_uint8_t *received_buf;	//receive data buffer point
    rt_uint32_t received_len;	//received data length
    rt_uint32_t received_max_len;	//received data max length
}rs232_inst_t;

/* 
 * @brief   create rs232 instance dynamically
 * @param   serial      - serial device name
 * @param   baudrate    - serial baud rate
 * @param   parity      - serial parity mode
 * @param   level       - send mode level
 * @retval  instance handle
 */
rs232_inst_t * rs232_create(const char *serial, int baudrate, int parity);

/* 
 * @brief   destory rs232 instance created dynamically
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs232_destory(rs232_inst_t * hinst);

/* 
 * @brief   config rs232 params 
 * @param   hinst       - instance handle
 * @param   baudrate    - baudrate of communication
 * @param   databits    - data bits, 5~8
 * @param   parity      - parity bit, 0~2, 0 - none, 1 - odd, 2 - even
 * @param   stopbits    - stop bits, 0~1, 0 - 1 stop bit, 1 - 2 stop bits
 * @retval  0 - success, other - error
 */
int rs232_config(rs232_inst_t * hinst, int baudrate, int databits, int parity, int stopbits);

/* 
 * @brief   set byte interval timeout for receiving
 * @param   hinst       - instance handle
 * @param   tmo_ms      - byte interval timeout, default is calculated from baudrate
 * @retval  0 - success, other - error
 */
int rs232_set_byte_tmo(rs232_inst_t * hinst, int tmo_ms);

/* 
 * @brief   open rs232 connect
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs232_connect(rs232_inst_t * hinst);

/* 
 * @brief   close rs232 connect
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs232_disconn(rs232_inst_t * hinst);

/* 
 * @brief   receive datas from rs232
 * @param   hinst       - instance handle
 * @param   buf         - buffer addr
 * @param   size        - maximum length of received datas
 * @retval  >=0 - length of received datas, <0 - error
 */
int rs232_recv(rs232_inst_t * hinst, void *buf, int size);

/* 
 * @brief   send datas to rs232
 * @param   hinst       - instance handle
 * @param   buf         - buffer addr
 * @param   size        - length of send datas
 * @retval  >=0 - length of sent datas, <0 - error
 */
int rs232_send(rs232_inst_t * hinst, void *buf, int size);

#endif
