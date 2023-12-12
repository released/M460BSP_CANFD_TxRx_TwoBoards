/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "misc_config.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/

struct flag_32bit flag_PROJ_CTL;
#define FLAG_PROJ_TIMER_PERIOD_1000MS                 	(flag_PROJ_CTL.bit0)
#define FLAG_PROJ_CANFD_Transmit_Trigger       			(flag_PROJ_CTL.bit1)
#define FLAG_PROJ_BUS_OFF_TRIGGER                 		(flag_PROJ_CTL.bit2)
#define FLAG_PROJ_CANFD_IRQ_RX                          (flag_PROJ_CTL.bit3)
#define FLAG_PROJ_TIMER_PERIOD_BUS_OFF                  (flag_PROJ_CTL.bit4)
#define FLAG_PROJ_REVERSE5                              (flag_PROJ_CTL.bit5)
#define FLAG_PROJ_REVERSE6                              (flag_PROJ_CTL.bit6)
#define FLAG_PROJ_REVERSE7                              (flag_PROJ_CTL.bit7)


/*_____ D E F I N I T I O N S ______________________________________________*/

volatile unsigned int counter_systick = 0;
volatile uint32_t counter_tick = 0;

CANFD_T * g_pCanfd = CANFD0;

#define CAN_sNormBitRate                                    (1000000)
#define CAN_sDataBitRate                                    (4000000)

// M467 IOT
#define DEVICE_A
#define ENABLE_EVM
#define MSG_ID_TX                                       (0)
#define MSG_ID_RX                                       (1)

// M463 EVB
// #define DEVICE_B
// #define ENABLE_EVM
// #define MSG_ID_TX                                       (2)
// #define MSG_ID_RX                                       (3)

#define DEVICE_ID_1                                     (0x44444)
#define DEVICE_ID_2                                     (0x33333)


#define ENABLE_CAN_INT
// #define ENABLE_CAN_POLLING

// #define ENABLE_LOOPBACK
// #define ENABLE_CAN_NORMAL
#define ENABLE_CAN_FD
// #define ENABLE_CAN_MONITOR

uint8_t canbus_INT_flag = 0x00;
#define CAN_BOFF_INT        5

uint8_t canbus_BO_COUNT = 0;
#define CAN_BO_FAST_RESP        (1)
#define CAN_BO_SLOW_RESP        (2)
#define CAN_BO_DEFAULT_RESP     (3)

uint8_t canbus_off_state = CAN_BO_FAST_RESP;

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

unsigned int get_systick(void)
{
	return (counter_systick);
}

void set_systick(unsigned int t)
{
	counter_systick = t;
}

void systick_counter(void)
{
	counter_systick++;
}

void SysTick_Handler(void)
{

    systick_counter();

    if (get_systick() >= 0xFFFFFFFF)
    {
        set_systick(0);      
    }

    // if ((get_systick() % 1000) == 0)
    // {
       
    // }

    #if defined (ENABLE_TICK_EVENT)
    TickCheckTickEvent();
    #endif    
}

void SysTick_delay(unsigned int delay)
{  
    
    unsigned int tickstart = get_systick(); 
    unsigned int wait = delay; 

    while((get_systick() - tickstart) < wait) 
    { 
    } 

}

void SysTick_enable(unsigned int ticks_per_second)
{
    set_systick(0);
    if (SysTick_Config(SystemCoreClock / ticks_per_second))
    {
        /* Setup SysTick Timer for 1 second interrupts  */
        printf("Set system tick error!!\n");
        while (1);
    }

    #if defined (ENABLE_TICK_EVENT)
    TickInitTickEvent();
    #endif
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void tick_counter(void)
{
	counter_tick++;
    if (get_tick() >= 60000)
    {
        set_tick(0);
    }
}

void delay_ms(uint16_t ms)
{
	#if 1
    uint32_t tickstart = get_tick();
    uint32_t wait = ms;
	uint32_t tmp = 0;
	
    while (1)
    {
		if (get_tick() > tickstart)	// tickstart = 59000 , tick_counter = 60000
		{
			tmp = get_tick() - tickstart;
		}
		else // tickstart = 59000 , tick_counter = 2048
		{
			tmp = 60000 -  tickstart + get_tick();
		}		
		
		if (tmp > wait)
			break;
    }
	
	#else
	TIMER_Delay(TIMER0, 1000*ms);
	#endif
}

void CAN_ShowMsg(CANFD_FD_MSG_T * sRxMsg)
{
    uint8_t u8Cnt;
    /* Show the message information */
    if(sRxMsg->eIdType == eCANFD_SID)
        printf("IRQ)Rx buf(Standard ID): ID = 0x%08X(11-bit),DLC = %d\n", sRxMsg->u32Id,sRxMsg->u32DLC);
    else
        printf("IRQ)Rx buf(Extended ID): ID = 0x%08X(29-bit),DLC = %d\n", sRxMsg->u32Id,sRxMsg->u32DLC);

    printf("Message Data : \r\n");
    for (u8Cnt = 0; u8Cnt < sRxMsg->u32DLC; u8Cnt++)
    {
        printf("%02X ,", sRxMsg->au8Data[u8Cnt]);
        if ((u8Cnt+1)%8 ==0)
        {
            printf("\r\n");
        }  
    }
    printf("\n\n");
}

void CANFD00_IRQHandler(void)
{

    if(g_pCanfd->IR & CANFD_IR_BO_Msk)
    {
        printf("BOFF INT\n") ;
        CANFD_ClearStatusFlag(g_pCanfd, CANFD_IR_BO_Msk);
        canbus_INT_flag = CAN_BOFF_INT;

        FLAG_PROJ_BUS_OFF_TRIGGER = 1;
    }
    

    if(g_pCanfd->IR  & CANFD_IR_EW_Msk)
    {
        printf("EWARN INT\n") ;
        CANFD_ClearStatusFlag(g_pCanfd, CANFD_IR_EW_Msk);
    }
    /* Protocol Error in Arbitration Phase */
    if(g_pCanfd->IR & CANFD_IR_PEA_Msk)
    {
        printf("Protocol error INT\r\n");
        // g_u8CanBitRateAdjust++; 
        CANFD_ClearStatusFlag(g_pCanfd, CANFD_IR_PEA_Msk);
    }

    if(g_pCanfd->IR & CANFD_IR_RF0N_Msk )   //CANFD_IR_DRX_Msk
    {

        //   g_u8RxBufRcvOk = 1;
        /*Clear the Interrupt flag */
        CANFD_ClearStatusFlag(g_pCanfd, CANFD_IR_TOO_Msk | CANFD_IR_DRX_Msk | CANFD_IR_RF0N_Msk);
        /*Receive the Rx buffer message(Standard ID) */

        FLAG_PROJ_CANFD_IRQ_RX = 1;
    }

}


void CANFD_IRQHandler_polling(void)
{
    CANFD_FD_MSG_T g_sRxMsgFrame;

    if (FLAG_PROJ_CANFD_IRQ_RX)
    {
        FLAG_PROJ_CANFD_IRQ_RX = 0;
        if(CANFD_ReadRxFifoMsg(g_pCanfd, 0, &g_sRxMsgFrame) == 1)
        {
            CAN_ShowMsg(&g_sRxMsgFrame);
        }
    }
}

void CANFD_polling(void)
{
    uint8_t u8Cnt;
    uint8_t u8ErrFlag = 0;
    uint8_t u8RxTempLen = 0;
    CANFD_FD_MSG_T sRxMsgFrame;

    CANFD_FD_MSG_T sRxFIFOMsgFrame;

    // FIFO0
    if (CANFD_ReadRxFifoMsg(g_pCanfd, 0, &sRxFIFOMsgFrame) == 1)    
    {
        if (sRxFIFOMsgFrame.eIdType == eCANFD_SID)
            printf("polling)Rx FIFO1(Standard ID),ID = 0x%08X\r\n", sRxFIFOMsgFrame.u32Id);
        else
            printf("polling)Rx FIFO1(Extended ID),ID = 0x%08X\r\n", sRxFIFOMsgFrame.u32Id);

        printf("Message Data(%02u bytes) : \r\n", sRxFIFOMsgFrame.u32DLC);

        for (u8Cnt = 0; u8Cnt <  sRxFIFOMsgFrame.u32DLC; u8Cnt++)
        {
            // printf("%02u ,", sRxFIFOMsgFrame.au8Data[u8Cnt]);            
            printf("0x%2X ,", sRxFIFOMsgFrame.au8Data[u8Cnt]);

            if ((u8Cnt+1)%8 ==0)
            {
                printf("\r\n");
            }  
        }

        printf("\r\n");
        memset(&sRxFIFOMsgFrame, 0, sizeof(sRxFIFOMsgFrame));

    }


    /* check for any received messages on CAN FD0 message buffer 1 */
    if (CANFD_ReadRxBufMsg(g_pCanfd, MSG_ID_RX, &sRxMsgFrame) == 1)
    {

        printf("Rx buf : Received message 0x%08X (29-bit)\r\n", sRxMsgFrame.u32Id);
        printf("Message Data : \r\n");

        for (u8Cnt = 0; u8Cnt < sRxMsgFrame.u32DLC; u8Cnt++)
        {
            printf("0x%2X,", sRxMsgFrame.au8Data[u8Cnt]);

            if ((u8Cnt+1)%8 ==0)
            {
                printf("\r\n");
            }  
        }

        printf(" \n\n");

        /* Check Extend ID number */
        #if 0
        if (sRxMsgFrame.u32Id != DEVICE_ID)
        {
            u8ErrFlag = 1;
            printf("CAN FD EXD ID Error \r\n");
        }
        #endif

        u8RxTempLen = 64;

        /* Check Data lenght */
        if ((u8RxTempLen != sRxMsgFrame.u32DLC) || (sRxMsgFrame.eIdType != eCANFD_XID))
        {
            u8ErrFlag = 1;
            printf("CAN FD Data length Error \r\n");
        }

        if (u8ErrFlag == 1)
        {
            u8ErrFlag = 0;
            printf("CAN FD EXD ID or Data Error \r\n");
        }
    }
}

void CANFD_transmit(void)
{
    CANFD_FD_MSG_T sTxMsgFrame;
    uint8_t u8Cnt;
    uint8_t resp = 0;
    static uint8_t count = 0;

    /* Set the ID Number */
    
    #if defined (DEVICE_A)
    sTxMsgFrame.u32Id = DEVICE_ID_2;
    #elif defined (DEVICE_B)
    sTxMsgFrame.u32Id = DEVICE_ID_1;
    #endif
    /*Set the ID type*/
    sTxMsgFrame.eIdType = eCANFD_XID;

    /*Set the frame type*/
    sTxMsgFrame.eFrmType = eCANFD_DATA_FRM;

    #if defined (ENABLE_CAN_FD)
    /*Set CAN FD frame format */
    sTxMsgFrame.bFDFormat = 1;
    /*Set the bitrate switch */
    sTxMsgFrame.bBitRateSwitch = 1;
    /*Set the data lenght */
    sTxMsgFrame.u32DLC = 64;
    #elif defined (ENABLE_CAN_NORMAL)
    /*Set CAN FD frame format */
    sTxMsgFrame.bFDFormat = 0;
    /*Set the bitrate switch */
    sTxMsgFrame.bBitRateSwitch = 0;
    /*Set the data lenght */
    sTxMsgFrame.u32DLC = 8;
    #endif

    printf("Send to transmit message 0x%08x (29-bit)\n", sTxMsgFrame.u32Id);

    for (u8Cnt = 0; u8Cnt < sTxMsgFrame.u32DLC; u8Cnt++)
    {
        #if defined (DEVICE_A)
        sTxMsgFrame.au8Data[u8Cnt] = u8Cnt + 0xA0 + count;
        #elif defined (DEVICE_B)
        sTxMsgFrame.au8Data[u8Cnt] = u8Cnt + 0xB0 + count;
        #endif
        // printf("%02d,", sTxMsgFrame.au8Data[u8Cnt]);
        
        printf("0x%2X,", sTxMsgFrame.au8Data[u8Cnt]);

        if ((u8Cnt+1)%8 ==0)
        {
            printf("\r\n");
        }  

    }

    printf("\n\n");

    
    // dump_buffer((unsigned char *) sTxMsgFrame.au8Data,sTxMsgFrame.u32DLC);


    /*
        0 = Tx Message Buffer is currently in use.
        1= Write Tx Message Buffer Successfully.
    */
    /* use message buffer 0 */
    // resp = CANFD_TransmitTxMsg(g_pCanfd, MSG_ID_TX, &sTxMsgFrame);
    // if (resp != 1)
    // {
    //     printf("Failed to transmit message :0x%2X\n" , resp);
    // }

    while (CANFD_TransmitTxMsg(g_pCanfd, MSG_ID_TX , &sTxMsgFrame) != 1);

    printf("transmit done\r\n");    

    count++;
}


void CANFD_Init(void)
{
    CANFD_FD_T sCANFD_Config;

    #if defined (ENABLE_CAN_MONITOR)
    uint8_t u8EnMonMode = 1;
    #endif

    /*Get the CAN FD0 configuration value*/
    #if defined (ENABLE_CAN_FD)
    CANFD_GetDefaultConfig(&sCANFD_Config, CANFD_OP_CAN_FD_MODE);
    sCANFD_Config.sBtConfig.sNormBitRate.u32BitRate = CAN_sNormBitRate;//1000000;
    sCANFD_Config.sBtConfig.sDataBitRate.u32BitRate = CAN_sDataBitRate;//4000000;
    #elif defined (ENABLE_CAN_NORMAL)
    CANFD_GetDefaultConfig(&sCANFD_Config, CANFD_OP_CAN_MODE);
    sCANFD_Config.sBtConfig.sNormBitRate.u32BitRate = 500000;
    sCANFD_Config.sBtConfig.sDataBitRate.u32BitRate = 0;
    #endif

    #if defined (ENABLE_LOOPBACK)
    sCANFD_Config.sBtConfig.bEnableLoopBack = TRUE;
    #endif

    /*Open the CAN FD0 feature*/
    CANFD_Open(g_pCanfd, &sCANFD_Config);

    // g_pCanfd->DBTP |= CANFD_DBTP_TDC_Msk;
    // g_pCanfd->TDCR = 0x500;

    #if defined (DEVICE_A)

    #if defined (ENABLE_CAN_POLLING)
    CANFD_SetXIDFltr(g_pCanfd, 2, CANFD_RX_BUFFER_EXT_LOW(DEVICE_ID_1, MSG_ID_RX), CANFD_RX_BUFFER_EXT_HIGH(DEVICE_ID_1, MSG_ID_RX));
    #elif defined (ENABLE_CAN_INT)
    CANFD_SetXIDFltr(g_pCanfd, 0, CANFD_RX_FIFO0_EXT_MASK_LOW(DEVICE_ID_1), CANFD_RX_FIFO0_EXT_MASK_HIGH(0x1FFFFFFF));
    #endif

    printf("target filter : 0x%4X\r\n" , DEVICE_ID_1);
    #elif defined (DEVICE_B)
    
    #if defined (ENABLE_CAN_POLLING)
    CANFD_SetXIDFltr(g_pCanfd, 2, CANFD_RX_BUFFER_EXT_LOW(DEVICE_ID_2, MSG_ID_RX), CANFD_RX_BUFFER_EXT_HIGH(DEVICE_ID_2, MSG_ID_RX));
    #elif defined (ENABLE_CAN_INT)
    CANFD_SetXIDFltr(g_pCanfd, 0, CANFD_RX_FIFO0_EXT_MASK_LOW(DEVICE_ID_2), CANFD_RX_FIFO0_EXT_MASK_HIGH(0x1FFFFFFF));
    #endif

    printf("target filter : 0x%4X\r\n" , DEVICE_ID_2);
    #endif

    // /*Enable the Bus Monitoring Mode */    
    #if defined (ENABLE_CAN_MONITOR)
    if(u8EnMonMode == 1)
        CANFD0->CCCR |= CANFD_CCCR_MON_Msk;
    else
        CANFD0->CCCR &= ~CANFD_CCCR_MON_Msk;
    #endif


    #if defined (ENABLE_CAN_INT)

    /*
        0= Filter remote frames with 11-bit standard IDs. 
        1= Reject all remote frames with 11-bit standard IDs. 
        
        0= Filter remote frames with 29-bit extended IDs. 
        1= Reject all remote frames with 29-bit extended IDs. 

    */
    CANFD_SetGFC(g_pCanfd, eCANFD_ACC_NON_MATCH_FRM_RX_FIFO0, eCANFD_ACC_NON_MATCH_FRM_RX_FIFO0, 1, 1);    

    CANFD_EnableInt(g_pCanfd, (CANFD_IE_TOOE_Msk | CANFD_IE_DRXE_Msk | CANFD_IE_BOE_Msk  |CANFD_IE_PEAE_Msk  | CANFD_IE_RF0NE_Msk), 0, 0, 0);
    // CANFD_EnableInt(g_pCanfd, CANFD_IE_BOE_Msk , 0, 0, 0);
    NVIC_EnableIRQ(CANFD00_IRQn);
    #elif defined (ENABLE_CAN_POLLING)
    CANFD_EnableInt(g_pCanfd, CANFD_IE_BOE_Msk , 0, 0, 0);
    NVIC_EnableIRQ(CANFD00_IRQn);    
    #endif

    /* CAN FD0 Run to Normal mode  */
    CANFD_RunToNormal(g_pCanfd, TRUE);
}

void CAN_SendMessage(CANFD_FD_MSG_T *psTxMsg, E_CANFD_ID_TYPE eIdType, uint32_t u32Id, uint8_t u8Len)
{
    uint8_t u8Cnt;
    uint8_t resp;
    
    psTxMsg->u32Id = u32Id;
    psTxMsg->eIdType = eIdType;
    psTxMsg->eFrmType = eCANFD_DATA_FRM;
    #if defined (ENABLE_CAN_FD)
    /*Set CAN FD frame format */
    psTxMsg->bFDFormat = 1;
    /*Set the bitrate switch */
    psTxMsg->bBitRateSwitch = 1;
    #elif defined (ENABLE_CAN_NORMAL)
    /*Set CAN FD frame format */
    psTxMsg->bFDFormat = 0;
    /*Set the bitrate switch */
    psTxMsg->bBitRateSwitch = 0;
    #endif
    psTxMsg->u32DLC = u8Len;

    /* use message buffer 0 */
    if (eIdType == eCANFD_SID)
        printf("(Standard ID)Send ID = 0x%08x (11-bit),DLC = %d\n", psTxMsg->u32Id,psTxMsg->u32DLC);
    else
        printf("(Extended ID)Send ID = 0x%08x (29-bit),DLC = %d\n", psTxMsg->u32Id,psTxMsg->u32DLC);

    printf("Message Data : ");
    for (u8Cnt = 0; u8Cnt < psTxMsg->u32DLC; u8Cnt++) 
    {
      psTxMsg->au8Data[u8Cnt] = u8Cnt;
      printf("%02X ,", psTxMsg->au8Data[u8Cnt]);
    }
    // printf("\n\n");
    resp = CANFD_TransmitTxMsg(g_pCanfd, 0, psTxMsg);
    if (resp == 1)
    {
        printf("transmit message OK\n");
    }
    else
    {
        printf("Failed to transmit message (0x%2X)\n",resp);
    }
}


void Bus_off_recovery(void)
{
    // uint8_t result = 0;
    CANFD_FD_MSG_T sTxMsgFrame;
    
    if (FLAG_PROJ_TIMER_PERIOD_BUS_OFF && 
        FLAG_PROJ_BUS_OFF_TRIGGER ) // 50 ms
    {
        FLAG_PROJ_TIMER_PERIOD_BUS_OFF = 0;

        if (canbus_BO_COUNT >= 20)
        {        
            if (((canbus_BO_COUNT) % 20) == 0)
            {
                canbus_off_state = CAN_BO_SLOW_RESP;
                #if 1

                #if defined (DEVICE_A)                
                CAN_SendMessage(&sTxMsgFrame,eCANFD_XID,DEVICE_ID_2,8);
                #elif defined (DEVICE_B)                
                CAN_SendMessage(&sTxMsgFrame,eCANFD_XID,DEVICE_ID_1,8);
                #endif

                #else
                CAN_SendMessage(&sTxMsgFrame,eCANFD_SID,0x666,8);
                #endif

                // CANFD_transmit();
                FLAG_PROJ_BUS_OFF_TRIGGER = 0;
            }	
        }
        else
        {
            if (canbus_off_state == CAN_BO_FAST_RESP)
            {
                #if 1

                #if defined (DEVICE_A)                
                CAN_SendMessage(&sTxMsgFrame,eCANFD_XID,DEVICE_ID_2,8);
                #elif defined (DEVICE_B)                
                CAN_SendMessage(&sTxMsgFrame,eCANFD_XID,DEVICE_ID_1,8);
                #endif

                #else
                CAN_SendMessage(&sTxMsgFrame,eCANFD_SID,0x555,8);
                #endif
                // CANFD_transmit();
                printf("FAST :%d\r\n",canbus_BO_COUNT);
                FLAG_PROJ_BUS_OFF_TRIGGER = 0;
            }
        }
        canbus_BO_COUNT++;

        if ( (g_pCanfd->PSR & 0x05) == 0x00)
        {
            FLAG_PROJ_BUS_OFF_TRIGGER = 0;            
            canbus_off_state = CAN_BO_FAST_RESP;
            canbus_BO_COUNT = 0;

            printf("BOFF:clr\r\n");
        }

    }    

	if (canbus_INT_flag == CAN_BOFF_INT)
    {
        // printf("BOFF:entry\r\n");
        canbus_INT_flag = 0;

        /*   bus-off recovery check*/
        #if 1
        g_pCanfd->CCCR |= ( CANFD_CCCR_INIT_Msk);

        // while((g_pCanfd->PSR&0x07)!=0x00);
        // prevent stuck in loop
        // u32TimeOutCount = 0xFFFF;   //__HSI;
        // while ( (g_pCanfd->PSR & 0x05) == 0x05)
        // {
        //     // printf("BOFF:101 = Bit0Error(1) \r\n");
        //     if (--u32TimeOutCount == 0)
        //     {
        //         // printf("BOFF:101 = Bit0Error(2) - TIMEOUT\r\n");
        //         break;
        //     }
        // }
        
        g_pCanfd->CCCR &= ~( CANFD_CCCR_INIT_Msk);
        CANFD_RunToNormal(g_pCanfd, TRUE);  


        #else
        g_pCanfd->CCCR &= (~(CANFD_CCCR_INIT_Msk | CANFD_CCCR_CCE_Msk));
        while(g_pCanfd->CCCR & CANFD_CCCR_INIT_Msk);
        while((g_pCanfd->PSR & CANFD_PSR_LEC_Msk) == 0x05) 
        {
            if(g_pCanfd->ECR ==0) 
            {
                CANFD_EnableInt(g_pCanfd, (CANFD_IE_TOOE_Msk | CANFD_IE_DRXE_Msk | CANFD_IE_BOE_Msk  |CANFD_IE_PEAE_Msk ), 0, 0, 0);
                
                canbus_INT_flag = 0x00;
                break;
            }
        }
        #endif
	}	
}


//
// check_reset_source
//
uint8_t check_reset_source(void)
{
    uint32_t src = SYS_GetResetSrc();

    /*
        0x02 = M467.  
        0x03 = M463. 
    */

    if ((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x2)  
    {
		printf("PN : M467\r\n");
    }
    else if  ((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x3)
    {
		printf("PN : M463\r\n");
    }

    SYS->RSTSTS |= 0x1FF;
    printf("Reset Source <0x%08X>\r\n", src);

    #if 1   //DEBUG , list reset source
    if (src & BIT0)
    {
        printf("0)POR Reset Flag\r\n");       
    }
    if (src & BIT1)
    {
        printf("1)NRESET Pin Reset Flag\r\n");       
    }
    if (src & BIT2)
    {
        printf("2)WDT Reset Flag\r\n");       
    }
    if (src & BIT3)
    {
        printf("3)LVR Reset Flag\r\n");       
    }
    if (src & BIT4)
    {
        printf("4)BOD Reset Flag\r\n");       
    }
    if (src & BIT5)
    {
        printf("5)System Reset Flag \r\n");       
    }
    if (src & BIT6)
    {
        printf("6)HRESET Reset Flag \r\n");       
    }
    if (src & BIT7)
    {
        printf("7)CPU Reset Flag\r\n");       
    }
    if (src & BIT8)
    {
        printf("8)CPU Lockup Reset Flag\r\n");       
    }
    #endif
    
    if (src & SYS_RSTSTS_PORF_Msk) {
        SYS_ClearResetSrc(SYS_RSTSTS_PORF_Msk);
        
        printf("power on from POR\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_PINRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_PINRF_Msk);
        
        printf("power on from nRESET pin\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSTS_WDTRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_WDTRF_Msk);
        
        printf("power on from WDT Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_LVRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_LVRF_Msk);
        
        printf("power on from LVR Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_BODRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_BODRF_Msk);
        
        printf("power on from BOD Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_MCURF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_MCURF_Msk);
        
        printf("power on from System Reset\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSTS_CPURF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_CPURF_Msk);

        printf("power on from CPU reset\r\n");
        return FALSE;         
    }    
    else if (src & SYS_RSTSTS_CPULKRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_CPULKRF_Msk);
        
        printf("power on from CPU Lockup Reset\r\n");
        return FALSE;
    }   
    
    printf("power on from unhandle reset source\r\n");
    return FALSE;
}

void TMR1_IRQHandler(void)
{
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
            FLAG_PROJ_TIMER_PERIOD_1000MS = 1;//set_flag(flag_timer_period_1000ms ,ENABLE);
		}

		if ((get_tick() % 50) == 0)
		{
            FLAG_PROJ_TIMER_PERIOD_BUS_OFF = 1;
		}	
    }
}

void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void loop(void)
{
	// static uint32_t LOG1 = 0;
	// static uint32_t LOG2 = 0;

    if ((get_systick() % 1000) == 0)
    {
        // printf("%s(systick) : %4d\r\n",__FUNCTION__,LOG2++);    
    }

    if (FLAG_PROJ_TIMER_PERIOD_1000MS)//(is_flag_set(flag_timer_period_1000ms))
    {
        FLAG_PROJ_TIMER_PERIOD_1000MS = 0;//set_flag(flag_timer_period_1000ms ,DISABLE);

        // printf("%s(timer) : %4d\r\n",__FUNCTION__,LOG1++);
        // PH4 ^= 1;             
    }

    if (FLAG_PROJ_CANFD_Transmit_Trigger)
    {
        FLAG_PROJ_CANFD_Transmit_Trigger = 0;

        CANFD_transmit();
    }

    #if defined (ENABLE_CAN_POLLING)
    CANFD_polling();
    #endif

    #if defined (ENABLE_CAN_INT)
    CANFD_IRQHandler_polling();
    #endif
    

    Bus_off_recovery();
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		printf("press : %c\r\n" , res);
		switch(res)
		{
			case '1':
                FLAG_PROJ_CANFD_Transmit_Trigger = 1;
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
                SYS_UnlockReg();
				// NVIC_SystemReset();	// Reset I/O and peripherals , only check BS(FMC_ISPCTL[1])
                // SYS_ResetCPU();     // Not reset I/O and peripherals
                SYS_ResetChip();    // Reset I/O and peripherals ,  BS(FMC_ISPCTL[1]) reload from CONFIG setting (CBS)	
				break;
		}
	}
}

void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART0_IRQn);

	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());    	

//    printf("Product ID 0x%8X\n", SYS->PDID);
	
	#endif	

    #if 0
    printf("FLAG_PROJ_TIMER_PERIOD_1000MS : 0x%2X\r\n",FLAG_PROJ_TIMER_PERIOD_1000MS);
    printf("FLAG_PROJ_REVERSE1 : 0x%2X\r\n",FLAG_PROJ_REVERSE1);
    printf("FLAG_PROJ_REVERSE2 : 0x%2X\r\n",FLAG_PROJ_REVERSE2);
    printf("FLAG_PROJ_REVERSE3 : 0x%2X\r\n",FLAG_PROJ_REVERSE3);
    printf("FLAG_PROJ_REVERSE4 : 0x%2X\r\n",FLAG_PROJ_REVERSE4);
    printf("FLAG_PROJ_REVERSE5 : 0x%2X\r\n",FLAG_PROJ_REVERSE5);
    printf("FLAG_PROJ_REVERSE6 : 0x%2X\r\n",FLAG_PROJ_REVERSE6);
    printf("FLAG_PROJ_REVERSE7 : 0x%2X\r\n",FLAG_PROJ_REVERSE7);
    #endif


    #if defined (ENABLE_EVM)
    printf("ENABLE_EVM\r\n");
    #endif

}

void GPIO_Init (void)
{
    SET_GPIO_PH4();
    SET_GPIO_PH5();
    SET_GPIO_PH6();

	//EVM LED
	GPIO_SetMode(PH,BIT4,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT5,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT6,GPIO_MODE_OUTPUT);
	
}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
   PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);
    
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

   CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
   CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Enable all GPIO clock */
    CLK->AHBCLK0 |= CLK_AHBCLK0_GPACKEN_Msk | CLK_AHBCLK0_GPBCKEN_Msk | CLK_AHBCLK0_GPCCKEN_Msk | CLK_AHBCLK0_GPDCKEN_Msk |
                    CLK_AHBCLK0_GPECKEN_Msk | CLK_AHBCLK0_GPFCKEN_Msk | CLK_AHBCLK0_GPGCKEN_Msk | CLK_AHBCLK0_GPHCKEN_Msk;
    CLK->AHBCLK1 |= CLK_AHBCLK1_GPICKEN_Msk | CLK_AHBCLK1_GPJCKEN_Msk;


    /* Set core clock to 200MHz */
    CLK_SetCoreClock(200000000);

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    #if defined (ENABLE_EVM)
    SET_UART0_RXD_PB12();
    SET_UART0_TXD_PB13(); 
    #endif

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
	

    /* Select CAN FD0 clock source is HCLK */
    CLK_SetModuleClock(CANFD0_MODULE, CLK_CLKSEL0_CANFD0SEL_HCLK, CLK_CLKDIV5_CANFD0(1));

    /* Enable CAN FD0 peripheral clock */
    CLK_EnableModuleClock(CANFD0_MODULE);

    #if defined (ENABLE_EVM)

    #if defined (DEVICE_A)
    SET_CAN0_RXD_PJ11();
    SET_CAN0_TXD_PJ10();   
    #elif defined (DEVICE_B)
    SET_CAN0_RXD_PE15();
    SET_CAN0_TXD_PE14();  
    #endif

    #endif  

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	GPIO_Init();
	UART0_Init();
	TIMER1_Init();
    check_reset_source();

    SysTick_enable(1000);
    #if defined (ENABLE_TICK_EVENT)
    TickSetTickEvent(1000, TickCallback_processA);  // 1000 ms
    TickSetTickEvent(5000, TickCallback_processB);  // 5000 ms
    #endif

    CANFD_Init();

    /* Got no where to go, just loop forever */
    while(1)
    {
        loop();

    }
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
