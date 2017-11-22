#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "gpio.h"


#define MAGIC_UARTRX_BUFF 512

extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;
extern osSemaphoreId myBSemU_RXHandle;
volatile uint8_t goodCommandFromUartRX;
volatile int16_t goodCommandLengthFromUartRX;
uint8_t uartInput[MAGIC_UARTRX_BUFF];






void StartDefaultTask(void const * argument)
{
    static const uint8_t error1[]={1};
    static const uint8_t error2[]={2};
    static const uint8_t errorFF[]={0xFF};
    static const uint8_t ok[]={0};
    goodCommandFromUartRX=0;
    HAL_UART_Receive_DMA(&huart1, uartInput, MAGIC_UARTRX_BUFF);
    osSemaphoreWait(myBSemU_RXHandle,10);
    
    
  for(;;)
  {
      if(osSemaphoreWait(myBSemU_RXHandle,10)==osOK){
        
          if(goodCommandLengthFromUartRX>0){
            HAL_UART_Transmit_DMA(&huart1, (uint8_t*)ok, sizeof(ok));
          }else{
            HAL_UART_Transmit_DMA(&huart1, (uint8_t*)error1, sizeof(error1));
          }
#ifdef _DEBUG_USB_UART
printf("UART1 DMA SS %d\r\n",goodCommandLengthFromUartRX);
#endif
          HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
          goodCommandFromUartRX=0;
          HAL_UART_Receive_DMA(&huart1, uartInput, MAGIC_UARTRX_BUFF);
      }
    osDelay(100);
  }
}



//USART
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    goodCommandFromUartRX=1;
    HAL_UART_Receive_DMA(&huart1, uartInput, MAGIC_UARTRX_BUFF);
#ifdef _DEBUG_USB_UART
    printf("UART1 DMA COMPLETE\r\n");
#endif
}
void HAL_UART_RxIdleCallback(UART_HandleTypeDef* huart){
    static uint32_t rxXferCount = 0;
#ifdef _DEBUG_USB_UART
printf("UART1 DMA EN %d\r\n",huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx));
#endif
    if(huart->hdmarx != NULL)
    {
        if(huart->RxXferSize==0)return;
        rxXferCount = huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx);
        huart->RxXferSize=0;
        if(rxXferCount!=0){
            HAL_UART_AbortReceive(huart);
            huart->RxXferCount = 0;
#ifdef _DEBUG_USB_UART
printf("UART1 DMA ABORT\r\n");
#endif
        }
        
        if(huart->gState == HAL_UART_STATE_BUSY_TX_RX){
            huart->gState = HAL_UART_STATE_BUSY_TX;
        }else{
            huart->gState = HAL_UART_STATE_READY;
        }
        if(goodCommandFromUartRX==0){
            goodCommandLengthFromUartRX=rxXferCount;
        }else{
            goodCommandLengthFromUartRX=0;
        };
#ifdef _DEBUG_USB_UART
    printf("UART1 DMA goodCLFURX=%d\r\n",goodCommandLengthFromUartRX);
#endif
        osSemaphoreRelease(myBSemU_RXHandle);
    }
}