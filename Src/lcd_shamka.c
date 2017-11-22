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
    static const uint8_t error1[]={0xFE,0};
    static const uint8_t errorFF[]={0xFF};
    static const uint8_t ok[]={0};
    static const uint8_t okUNKLengthEND_0xFF[]={1,1,0xFF};
    static uint16_t stops;
    
    goodCommandFromUartRX=0;
    HAL_UART_Receive_DMA(&huart1, uartInput, MAGIC_UARTRX_BUFF);
    osSemaphoreWait(myBSemU_RXHandle,10);
    
    
    for(;;)
    {
        if(osSemaphoreWait(myBSemU_RXHandle,10)==osOK){
   #ifdef _DEBUG_USB_UART
  printf("UART1 DMA SS %d STAT %d\r\n",goodCommandLengthFromUartRX,goodCommandFromUartRX);
  #endif
          if(goodCommandFromUartRX!=0){
            HAL_UART_Transmit_DMA(&huart1, (uint8_t*)error1, sizeof(error1));
          }else{
            HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,GPIO_PIN_RESET);
            stops=10;
            switch(uartInput[0]){
            case 'I':{
              if(uartInput[1]==0){
                if(goodCommandLengthFromUartRX==5){
                  if(uartInput[4]=='S'){
                    while(HAL_UART_Transmit_DMA(&huart1, (uint8_t*)okUNKLengthEND_0xFF, sizeof(okUNKLengthEND_0xFF))!=HAL_OK){osDelay(10);};
                    for(int i=1;i<128;i++){
                      if(HAL_I2C_IsDeviceReady(&hi2c1,i<<1,3,20)==HAL_OK){
                        while(HAL_UART_Transmit_DMA(&huart1, (uint8_t*)&i, 1)!=HAL_OK){osDelay(10);};
                      }else{
                        //while(HAL_UART_Transmit_DMA(&huart1, (uint8_t*)&i, 1)!=HAL_OK){osDelay(10);};
                      }
                    }
                    while(HAL_UART_Transmit_DMA(&huart1, (uint8_t*)&okUNKLengthEND_0xFF[2], okUNKLengthEND_0xFF[1])!=HAL_OK){osDelay(10);};
                  }
                }
              }else{
                if(uartInput[1]&1){
                  
                }else{
                  if(goodCommandLengthFromUartRX>=4 && HAL_I2C_Master_Transmit(&hi2c1,uartInput[1]&0xFE,&uartInput[4],goodCommandLengthFromUartRX-4,100)==HAL_OK){
                    while(HAL_UART_Transmit_DMA(&huart1, (uint8_t*)ok, sizeof(ok))!=HAL_OK){osDelay(10);};
                  }else{
                    while(HAL_UART_Transmit_DMA(&huart1, (uint8_t*)errorFF, sizeof(errorFF))!=HAL_OK){osDelay(10);};
                  };
                }
                
              };break;}
            default:HAL_UART_Transmit_DMA(&huart1, (uint8_t*)errorFF, sizeof(errorFF));
            }
          }
          goodCommandFromUartRX=goodCommandLengthFromUartRX=0;
          HAL_UART_Receive_DMA(&huart1, uartInput, MAGIC_UARTRX_BUFF);
        }
        if(stops>0){
          stops--;
          if(stops==0){
            HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,GPIO_PIN_SET);
          };
        }
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
    static int16_t commandSize;
   if(huart->hdmarx != NULL)
    {
        if(huart->RxXferSize==0)return;
#ifdef _DEBUG_USB_UART
printf("UART1 DMA EN %d\r\n",huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx));
#endif
         rxXferCount = huart->RxXferSize - __HAL_DMA_GET_COUNTER(huart->hdmarx);
        if(goodCommandFromUartRX==0 && rxXferCount>0){
            if(goodCommandLengthFromUartRX==0)commandSize=-1;
            goodCommandLengthFromUartRX=rxXferCount;
            if(goodCommandLengthFromUartRX>=512){
              goodCommandFromUartRX=2;
            }
            else{
              if(commandSize==-1){
                if(goodCommandLengthFromUartRX>=4){
                  commandSize=*(int16_t*)(&uartInput[2]);
                  if(commandSize<0 || commandSize>=512){
                    goodCommandFromUartRX=2;
                    commandSize=-1;
                  }
                }
                else{
                  return;
                }
              }
              if(commandSize!=-1){
                if(goodCommandLengthFromUartRX<commandSize){
                  return;
                }
                else if(goodCommandLengthFromUartRX>commandSize){
                  goodCommandFromUartRX=2;
                }
              }
            }
        }else{
            goodCommandLengthFromUartRX=0;
            goodCommandFromUartRX=3;
        };
//ABORT DMA
        if(__HAL_DMA_GET_COUNTER(huart->hdmarx)!=0){
            HAL_UART_AbortReceive(huart);
            huart->RxXferSize=0;
            huart->RxXferCount = 0;
            if(huart->gState == HAL_UART_STATE_BUSY_TX_RX){
                huart->gState = HAL_UART_STATE_BUSY_TX;
            }else{
                huart->gState = HAL_UART_STATE_READY;
            }
#ifdef _DEBUG_USB_UART
printf("UART1 DMA ABORT\r\n");
#endif
        }
        osSemaphoreRelease(myBSemU_RXHandle);
    }
}