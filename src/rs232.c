/*
 * rs232.c
 *
 * modify from qiyongzhong rs485 package
 * Change Logs:
 * Date           Author            Notes
 * 2022-01-06     diskwu       		first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <rs232.h>
#include <string.h>

#define DBG_TAG "rs232"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* 定时器  超时函数 */
static void rs232_recv_timeout(void *parameter)
{
    rs232_inst_t *hinst = (rs232_inst_t *)(parameter);

    if (hinst !=RT_NULL)
    {
        rt_sem_release(&hinst->rx_sem);
    }
}

static rt_err_t rs232_recv_ind_hook(rt_device_t dev, rt_size_t size)
{
	/*当前每次接收1byte*/
    rs232_inst_t *hinst = (rs232_inst_t *)(dev->user_data);

    if (size > 0)
    {
		// if (rt_mutex_take(hinst->lock, RT_WAITING_FOREVER) != RT_EOK)
		// {
		// 	LOG_E("rs232 send fail. it is destoried.");
		// 	return(-RT_ERROR);
		// }
        int len = rt_device_read(hinst->serial, 0, hinst->received_buf + hinst->received_len, size);
        if (len)
        {
            if (hinst->received_len < hinst->received_max_len-1)
            {
                hinst->received_len++;
            }
        }		
        rt_timer_start(hinst->received_over_timer);
		//rt_mutex_release(hinst->lock);
    }
    return(RT_EOK);
}

// static rt_err_t rs232_sendover_hook(rt_device_t dev, void *buffer)
// {
//     /*DMA发送完毕后执行，可以添加RS485控制脚*/
//     return(RT_EOK);
// }


static int rs232_cal_byte_tmo(int baudrate)
{
    int tmo = (40 * 1000) / baudrate;
    if (tmo < RS232_BYTE_TMO_MIN)
    {
        tmo = RS232_BYTE_TMO_MIN;
    }
    else if (tmo > RS232_BYTE_TMO_MAX)
    {
        tmo = RS232_BYTE_TMO_MAX;
    }
    return (tmo);
}

/* 
 * @brief   create rs232 instance dynamically
 * @param   serial      - serial device name
 * @param   baudrate    - serial baud rate
 * @param   parity      - serial parity mode
 * @retval  instance handle
 */
rs232_inst_t * rs232_create(const char *name, int baudrate, int parity)
{
    rs232_inst_t *hinst;
    rt_device_t dev;
    
    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        LOG_E("rs232 instance initiliaze error, the serial device(%s) no found.", name);
        return(RT_NULL);
    }
    
    if (dev->type != RT_Device_Class_Char)
    {
        LOG_E("rs232 instance initiliaze error, the serial device(%s) type is not char.", name);
        return(RT_NULL);
    }
    
    hinst = rt_malloc(sizeof(struct rs232_inst));
    if (hinst == RT_NULL)
    {
        LOG_E("rs232 create fail. no memory for rs232 create instance.");
        return(RT_NULL);
    }

    hinst->lock = rt_mutex_create(name, RT_IPC_FLAG_FIFO);
    if (hinst->lock == RT_NULL)
    {
        rt_free(hinst);
        LOG_E("rs232 create fail. no memory for rs232 create mutex.");
        return(RT_NULL);
    }

	/* 初始化信号量 */
    rt_sem_init(&hinst->rx_sem, name, 0, RT_IPC_FLAG_FIFO);
    hinst->received_len = 0;
    hinst->serial = dev;
    hinst->status = 0;
    hinst->byte_tmo = rs232_cal_byte_tmo(baudrate);
    hinst->received_over_timer = rt_timer_create(name, rs232_recv_timeout,
                                 hinst,  hinst->byte_tmo,
                                 RT_TIMER_FLAG_ONE_SHOT);
	if (hinst->received_over_timer == RT_NULL)
	{
		rt_mutex_delete(hinst->lock);
        rt_free(hinst);
        LOG_E("rs232 create fail. no memory for rs232 create timer.");
        return(RT_NULL);
	}							 
    rs232_config(hinst, baudrate, (parity?9:8), parity, 0);

    LOG_D("rs232 create success.");

    return(hinst);
}

/* 
 * @brief   destory rs232 instance created dynamically
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs232_destory(rs232_inst_t * hinst)
{
    if (hinst == RT_NULL)
    {
        LOG_E("rs232 destory fail. hinst is NULL.");
        return(-RT_ERROR);
    }
    
    rs232_disconn(hinst);

    if (hinst->lock)
    {
        rt_mutex_delete(hinst->lock);
        hinst->lock = RT_NULL;
    }
   
    rt_free(hinst);
    
    LOG_D("rs232 destory success.");
    
    return(RT_EOK);
}

/* 
 * @brief   config rs232 params 
 * @param   hinst       - instance handle
 * @param   baudrate    - baudrate of communication
 * @param   databits    - data bits, 5~8
 * @param   parity      - parity bit, 0~2, 0 - none, 1 - odd, 2 - even
 * @param   stopbits    - stop bits, 0~1, 0 - 1 stop bit, 1 - 2 stop bits
 * @retval  0 - success, other - error
 */
int rs232_config(rs232_inst_t * hinst, int baudrate, int databits, int parity, int stopbits)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (hinst == RT_NULL)
    {
        LOG_E("rs232 config fail. hinst is NULL.");
        return(-RT_ERROR);
    }

    hinst->byte_tmo = rs232_cal_byte_tmo(baudrate);
	if (hinst->received_over_timer != RT_NULL)
	{
	    /* 重置断帧时间 */
        rt_timer_control(hinst->received_over_timer,
                         RT_TIMER_CTRL_SET_TIME,
                         &hinst->byte_tmo);
	}

    config.baud_rate = baudrate;
    config.data_bits = databits;
    config.parity = parity;
    config.stop_bits = stopbits;
    rt_device_control(hinst->serial, RT_DEVICE_CTRL_CONFIG, &config);

    return(RT_EOK);
}

/* 
 * @brief   set byte interval timeout for receiving
 * @param   hinst       - instance handle
 * @param   tmo_ms      - byte interval timeout, default is calculated from baudrate
 * @retval  0 - success, other - error
 */
int rs232_set_byte_tmo(rs232_inst_t * hinst, int tmo_ms)
{
    if (hinst == RT_NULL)
    {
        LOG_E("rs232 set byte timeout fail. hinst is NULL.");
        return(-RT_ERROR);
    }
    
    if (tmo_ms < RS232_BYTE_TMO_MIN)
    {
        tmo_ms = RS232_BYTE_TMO_MIN;
    }
    else if (tmo_ms > RS232_BYTE_TMO_MAX)
    {
        tmo_ms = RS232_BYTE_TMO_MAX;
    }
    
    hinst->byte_tmo = tmo_ms;
    if (hinst->received_over_timer != RT_NULL)
    {
        /* 重置断帧时间 */
        rt_timer_control(hinst->received_over_timer,
                         RT_TIMER_CTRL_SET_TIME,
                         &hinst->byte_tmo);
    }

    LOG_D("rs232 set byte timeout success. the value is %d.", tmo_ms);

    return(RT_EOK);
}

/* 
 * @brief   open rs232 connect
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs232_connect(rs232_inst_t * hinst)
{
    if (hinst == RT_NULL)
    {
        LOG_E("rs232 connect fail. hinst is NULL.");
        return(-RT_ERROR);
    }

    if (hinst->status == 1)//is connected
    {
        LOG_D("rs232 is connected.");
        return(RT_EOK);
    }
    
	//必须单个中断接收，DMA发送
    if ( rt_device_open(hinst->serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX |RT_DEVICE_FLAG_DMA_TX) != RT_EOK)
    {
        LOG_E("rs232 instance connect error. serial open fail.");
        return(-RT_ERROR);
    }
    
    hinst->serial->user_data = hinst;
    hinst->serial->rx_indicate = rs232_recv_ind_hook;
    //hinst->serial->tx_complete = rs232_sendover_hook;	//如果希望知道发送完成，使用本语句
    hinst->status = 1;

    LOG_D("rs232 connect success.");

    return(RT_EOK);
}

/* 
 * @brief   close rs232 connect
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs232_disconn(rs232_inst_t * hinst)
{
    if (hinst == RT_NULL)
    {
        LOG_E("rs232 disconnect fail. hinst is NULL.");
        return(-RT_ERROR);
    }
    
    if (hinst->status == 0)//is not connected
    {
        LOG_D("rs232 is not connected.");
        return(RT_EOK);
    }

    rt_mutex_take(hinst->lock, RT_WAITING_FOREVER);

    if (hinst->received_over_timer)
    {
        rt_timer_delete(hinst->received_over_timer);
    }

    if (hinst->serial)
    {
        hinst->serial->rx_indicate = RT_NULL;
        rt_device_close(hinst->serial);
    }    
   
    hinst->status = 0;
    
    rt_mutex_release(hinst->lock);
    
    LOG_D("rs232 disconnect success.");
    
    return(RT_EOK);
}

/* 
 * @brief   receive datas from rs232
 * @param   hinst       - instance handle
 * @param   buf         - buffer addr
 * @param   size        - maximum length of received datas
 * @retval  >=0 - length of received datas, <0 - error
 */
int rs232_recv(rs232_inst_t * hinst, void *buf, int size)
{
    int recv_len = 0;
    
    if (hinst == RT_NULL || buf == RT_NULL || size == 0)
    {
        LOG_E("rs232 receive fail. param error.");
        return(-RT_ERROR);
    }
    
    if (hinst->status == 0)
    {
        LOG_E("rs232 receive fail. it is not connected.");
        return(-RT_ERROR);
    }
    
	while(size)
	{
		while (rt_sem_take(&(hinst->rx_sem), RT_WAITING_FOREVER) == RT_EOK)
		{
			// if (rt_mutex_take(hinst->lock, RT_WAITING_FOREVER) != RT_EOK)
			// {
			//     LOG_E("rs232 receive fail. it is destoried.");
			//     return(-RT_ERROR);
			// }
			memcpy(buf ,hinst->received_buf , hinst->received_len);
			recv_len = hinst->received_len;
			hinst->received_len = 0;
			size = 0;
			//rt_mutex_release(hinst->lock);
			break;
		}
	}
    
    return(recv_len);
}

/* 
 * @brief   send datas to rs232
 * @param   hinst       - instance handle
 * @param   buf         - buffer addr
 * @param   size        - length of send datas
 * @retval  >=0 - length of sent datas, <0 - error
 */
int rs232_send(rs232_inst_t * hinst, void *buf, int size)
{
    int send_len = 0;
    
    if (hinst == RT_NULL || buf == RT_NULL || size == 0)
    {
        LOG_E("rs232 send fail. param is error.");
        return(-RT_ERROR);
    }

    if (hinst->status == 0)
    {
        LOG_E("rs232 send fail. it is not connected.");
        return(-RT_ERROR);
    }
    
    send_len = rt_device_write(hinst->serial, 0, buf, size);

    return(send_len);
}
