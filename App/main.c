/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include "includes.h"
/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
__IO uint8_t USART1_RD_Buff[10];	//USART1 接收缓冲区

static OS_STK AppTaskSysStatusStk[APP_TASK_SYS_STK_SIZE];
static OS_STK AppTaskStartStk[APP_TASK_START_STK_SIZE];

static  OS_STK App_TaskLEDStk[APP_TASK_LED_STK_SIZE];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static  void App_TaskCreate(void);
static  void App_TaskLED(void* p_arg);

/*
*********************************************************************************************************
*                                            LOCAL MACRO'S
*********************************************************************************************************
*/
/*#define  APP_TASK_STOP();                         { while (DEF_ON) { \
															;			 \
														}				 \
													  }
	
	
#define  APP_TEST_ERR(err_var, err_code)          { if ((err_var) != (err_code)) {                                                                \
															APP_TRACE_DBG(("	%s() error #%05d @ line #%05d\n\r", __func__, (err_var), __LINE__)); \
														}																							  \
													  }
	
#define  APP_TEST_FAULT(err_var, err_code)        { APP_TEST_ERR(err_var, err_code); \
														if ((err_var) != (err_code)) {	 \
															APP_TASK_STOP();			 \
														}								 \
													  }*/

/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void AppTaskStart (void);


void UARTxDMA_TestInit(void)
{
	/*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f4xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
        system_stm32f4xx.c file
     */
	DMA_InitTypeDef     DMA_InitStructure;
	GPIO_InitTypeDef 	GPIO_InitStructure;
	NVIC_InitTypeDef 	NVIC_InitStructure; 
	USART_InitTypeDef 	USART_InitStructure;
	/* GPIOD Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	/* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	
	    
  	//分配USART1管脚
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	USART_InitStructure.USART_BaudRate = 38400;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	/* Configure USART1 */
	USART_Init(USART1, &USART_InitStructure);
	/* Enable the USART1 */
	USART_Cmd(USART1, ENABLE);//仿真看到执行这里，TC标志居然被设置为1了，不知道实际在flash中运行是否是这样
	
	USART_ClearFlag(USART1, USART_FLAG_TC);
	//USART_ClearFlag(USART1, USART_FLAG_LBD);
	//USART_ClearFlag(USART1, USART_FLAG_LBD);
	//USART_ClearFlag(USART1, USART_FLAG_RXNE);
	//USART_ITConfig(USART1, USART_IT_TC, ENABLE);
	
	//DMA 请求
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
	

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);	//记住DMA也要开时钟的
	//USART1 RX DMA Configure
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;  
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(USART1->DR));
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&USART1_RD_Buff;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = 10;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//这里是byte
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;	//这里必须设置为这个模式
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream2, &DMA_InitStructure);
	/* DMA2_Stream2(USART1_RX) enable */
	DMA_Cmd(DMA2_Stream2, ENABLE);
  
  	//USART1 TX DMA Configure
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;  
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(USART1->DR));
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&USART1_RD_Buff;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_BufferSize = 10;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//这里是byte
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream7, &DMA_InitStructure);
	DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);
	//DMA_ITConfig(DMA2_Stream7, DMA_IT_TE, ENABLE);
	//DMA_ITConfig(DMA2_Stream7, DMA_IT_FE, ENABLE);
	//DMA_ITConfig(DMA2_Stream7, DMA_IT_HT, ENABLE);
	/* DMA2_Stream7(USART1_TX) enable */
	//DMA_Cmd(DMA2_Stream7, ENABLE);	注意:这里不能使能DMA,否则会在第一帧发送的第一个字节出错的
 
 
 
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x04;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

int main (void)
{
	INT8U err;

	//BSP_IntDisAll();
	UARTxDMA_TestInit();
	OSInit();

	OSTaskCreateExt(AppTaskStart, 
					(void *)0,
					(OS_STK *)&AppTaskStartStk[APP_TASK_START_STK_SIZE - 1],
					APP_TASK_START_PRIO,
					APP_TASK_START_PRIO,
					(OS_STK *)&AppTaskStartStk[0],
					APP_TASK_START_STK_SIZE,
					(void *)0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
#if (OS_TASK_NAME_EN > 0)
	OSTaskNameSet(APP_TASK_START_PRIO, "Start Task", &err);
	//OSInit();
#endif
	OSStart(); 
}


void AppTaskStart (void)
{
	INT32U hclk_freq;
	INT32U cnts;

	OS_CPU_SysTickInit();									/* Initialize the SysTick.       */



#if OS_TASK_STAT_EN > 0
	OSStatInit();
#endif
	App_TaskCreate();
	
	while (1) 
	{ 
        GPIO_SetBits(GPIOD, GPIO_Pin_13);
		//OSTimeDlyHMSM(0, 0, 1, 0); 
		OSTimeDly(3000);
		GPIO_ResetBits(GPIOD, GPIO_Pin_13);
		//OSTimeDlyHMSM(0, 0, 1, 0);
		OSTimeDly(3000);
	}
}

static  void App_TaskCreate(void)
{
   INT8U os_err;

   os_err = OSTaskCreate((void (*) (void *)) App_TaskLED, (void *) 0,
               (OS_STK *) &App_TaskLEDStk[APP_TASK_LED_STK_SIZE - 1],
               (INT8U) APP_TASK_LCD_PRIO);
}


static  void App_TaskLED(void* p_arg)
{	
	DMA_InitTypeDef       DMA_InitStructure;
	unsigned int temp16 = 0;
	unsigned int i = 0;
	p_arg = p_arg;

	while (1)
	{
		if(DMA2->LISR & DMA_IT_TCIF2)
		{	
			/*非DMA发送
			for(i=0; i<10; i++)
			{
				while (!(USART1->SR & USART_FLAG_TXE));
				USART_SendData(USART1, USART1_RD_Buff[i]);
			}
			
			DMA2->LIFCR |= 0X00200000; //也可以这样清除
			*/
			DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_TCIF2);
			DMA_ClearFlag(DMA2_Stream2, DMA_IT_HTIF2);
			
			//2012.05.05哈哈,终于搞定在ucos ii使用DMA发送USART数据了,可以轻松一下,玩两盘麻将了
			DMA_ClearITPendingBit(DMA2_Stream7, DMA_IT_TCIF7);
			DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&USART1_RD_Buff;
			DMA_InitStructure.DMA_BufferSize = 10;
			DMA_Cmd(DMA2_Stream7, ENABLE);
			USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
		}
		
		GPIO_ResetBits(GPIOD, GPIO_Pin_15);
		GPIO_ResetBits(GPIOD, GPIO_Pin_12);
		GPIO_ResetBits(GPIOD, GPIO_Pin_14);
		//OSTimeDlyHMSM(0, 0, 0, 500);
		OSTimeDly(30);
		GPIO_SetBits(GPIOD, GPIO_Pin_12);
		GPIO_SetBits(GPIOD, GPIO_Pin_14);
		OSTimeDly(30);
		//OSTimeDlyHMSM(0, 0, 0, 500);
	}
		
}


/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */

void USART1_IRQHandler(void)
//void BSP_IntHandlerUSART1(void)
{
	if(USART_GetITStatus(USART1, USART_FLAG_TC) != RESET)
	{
		USART_ClearITPendingBit(USART1, USART_FLAG_TC);
	}
}


void DMA2_Stream7_IRQHandler(void)
{
 /*         @arg DMA_IT_TCIFx:  Streamx transfer complete interrupt
  *            @arg DMA_IT_HTIFx:  Streamx half transfer complete interrupt
  *            @arg DMA_IT_TEIFx:  Streamx transfer error interrupt
  *            @arg DMA_IT_DMEIFx: Streamx direct mode error interrupt
  *            @arg DMA_IT_FEIFx:  Streamx FIFO error interrupt
  *         Where x can be 0 to 7 to select the DMA Stream.
  * @retval The new state of DMA_IT (SET or RESET).
  */
	/* os暂时不管理这个中断
	CPU_SR   cpu_sr;
	OS_ENTER_CRITICAL();
    OSIntNesting++;
    OS_EXIT_CRITICAL();
	*/
	if(DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7) != RESET)
	{
		GPIO_SetBits(GPIOD, GPIO_Pin_15);
		DMA_ClearITPendingBit(DMA2_Stream7, DMA_IT_TCIF7);
		USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE);	
		DMA_Cmd(DMA2_Stream7, DISABLE);
		
	}
	
}
