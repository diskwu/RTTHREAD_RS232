/*
 * rs232_sample.c
 *
 * Change Logs:
 * Date           Author            Notes
 * 2022-01-06     diskwu            first version
 */
    
#include <rtthread.h>
#include <rtdevice.h>
#include <rs232.h>

#define DBG_TAG "rs232.sample"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#ifdef RS232_USING_SAMPLE

#ifndef RS232_SAMPLE_SERIAL
#define RS232_SAMPLE_SERIAL       "uart2"
#endif

#ifndef RS232_SAMPLE_BAUDRATE
#define RS232_SAMPLE_BAUDRATE     9600
#endif

#ifndef RS232_SAMPLE_MASTER_PARITY
#define RS232_SAMPLE_MASTER_PARITY      0 //0 -- none parity
#endif

#define RS232_SEND_MAX_SIZE 100 //发送数据缓存大小
#define RS232_RECEIVED_MAX_SIZE 256 //接收数据缓存大小

rs232_inst_t *sample_hinst;
uint8_t mRs232ReceivedMessage[RS232_RECEIVED_MAX_SIZE]; //接收到的数据
uint8_t mRs232SendBuf[RS232_SEND_MAX_SIZE]; //接收中的数据，缓存在这里
uint8_t mRs232ReceivedBuf[RS232_RECEIVED_MAX_SIZE]; //发送中的数据，保存在这里

//接收数据线程函数
void rs232_receivedFrame(void *parameter)
{
	int len;

	while(1)
	{
	    len = rs232_recv(sample_hinst, mRs232ReceivedMessage, RS232_RECEIVED_MAX_SIZE);
        if (len > 0)
        {
			//mRs232ReceivedMessage为接收到的数据
            LOG_I("received data, data[0] is %d\n",mRs232ReceivedMessage[0]);
        }
	}
}

static void rs232_sample_loopback_test(void *args)
{
    static rt_uint8_t buf[256];
	int len;

    sample_hinst = rs232_create(RS232_SAMPLE_SERIAL, RS232_SAMPLE_BAUDRATE, RS232_SAMPLE_MASTER_PARITY);
    sample_hinst->received_buf = mRs232ReceivedBuf;
    sample_hinst->received_max_len = RS232_RECEIVED_MAX_SIZE;

    if (sample_hinst == RT_NULL)
    {
        LOG_E("create rs232 instance fail.");
        return;
    }

	rt_thread_t tid = rt_thread_create("rs232_rec", rs232_receivedFrame, RT_NULL, 1024, 16, 20);
    RT_ASSERT(tid != RT_NULL);
    rt_thread_startup(tid);

    if (rs232_connect(sample_hinst) != RT_EOK)
    {
        rs232_destory(sample_hinst);
        LOG_E("rs232 connect fail.");
        return;
    }

    while(1)
    {
		len = 16;
		rs232_send(sample_hinst, buf, len);
		rt_thread_mdelay(1000);
    }
}

static int rs232_sample_init(void)
{
    rt_thread_t tid = rt_thread_create("rs232", rs232_sample_loopback_test, RT_NULL, 1024, 16, 20);
    RT_ASSERT(tid != RT_NULL);
    rt_thread_startup(tid);
    LOG_I("rs232 sample thread startup...");
    return(RT_EOK);
}
INIT_APP_EXPORT(rs232_sample_init);

#endif

