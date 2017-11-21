#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "gpio.h"


#define MAGIC_UARTRX_BUFF 64
#define MAGIC_UARTTX_BUFF 64

uint8_t uartBuff,uartInput[MAGIC_UARTRX_BUFF*2],uartOutut[MAGIC_UARTTX_BUFF];
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;









void StartDefaultTask(void const * argument)
{
    uartBuff=0;
    HAL_UART_Receive_DMA(&huart1, &uartInput[uartBuff], MAGIC_UARTRX_BUFF);
    
    
    
  for(;;)
  {
    osDelay(1000);
    //HAL_UART_Transmit_DMA(&huart1, (uint8_t*)ttt, sizeof(ttt)-1);
    HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
  }
}



//USART
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    HAL_UART_Transmit_DMA(&huart1, &uartInput[uartBuff], MAGIC_UARTRX_BUFF);
    if(uartBuff==0){
        uartBuff=MAGIC_UARTRX_BUFF;
    }
    else{
        uartBuff=0;
    }
    HAL_UART_Receive_DMA(&huart1, &uartInput[uartBuff], MAGIC_UARTRX_BUFF);
#ifdef _DEBUG_USB_UART
    printf("UART3 DMA COMPLETE\r\n");
#endif

}
void HAL_UART_RxIdleCallback(UART_HandleTypeDef* huart){
    static uint32_t rxXferCount = 0;
    if(huart->RxXferSize==0)return;
    if(huart->hdmarx != NULL)
    {
        rxXferCount = huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx);
        if(rxXferCount==0){

#ifdef _DEBUG_USB_UART
            printf("UART3 IDLE 64\r\n");
#endif
            return;
        };
        HAL_UART_Transmit_DMA(&huart1, &uartInput[uartBuff], rxXferCount);
        /* Determine how many items of data have been received */
        //rxXferCount = huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx);
        HAL_UART_AbortReceive(huart);
        huart->RxXferCount = 0;
        /* Check if a transmit process is ongoing or not */
        if(huart->gState == HAL_UART_STATE_BUSY_TX_RX)
        {
            huart->gState = HAL_UART_STATE_BUSY_TX;
        }
        else
        {
            huart->gState = HAL_UART_STATE_READY;
        }
        if(uartBuff==0){
            uartBuff=MAGIC_UARTRX_BUFF;
        }
        else{
            uartBuff=0;
        }
        HAL_UART_Receive_DMA(&huart1, &uartInput[uartBuff], MAGIC_UARTRX_BUFF);
#ifdef _DEBUG_USB_UART
    printf("UART3 IDLE\r\n");
#endif

    }
}