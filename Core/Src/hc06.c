#include "hc06.h"
#include <string.h>
#include <stdio.h>
#include "usart.h"

void HC06_Init(hc06_t* obj)
{
  obj->rx_buffer_write_pos = 0;
  obj->has_new_msg = false;
  HC06_WaitMsg(obj);
}

void HC06_SendString(hc06_t* obj, const char* str, uint16_t size)
{
  HAL_UART_Transmit(&huart2, (const uint8_t*)str, size, 0xffff);
}

void HC06_WaitMsg(hc06_t* obj)
{
  HAL_UART_Receive_IT(&huart2, (uint8_t*)&obj->ch_buffer, 1);
}

const char* HC06_GetRecvedMsg(hc06_t* obj)
{
  return obj->current_msg;
}

bool HC06_IsNewMsgRecved(hc06_t* obj)
{
  return obj->has_new_msg;
}

void HC06_ComingCallOption(hc06_t* obj, bool isAcceptCall)
{
  if (isAcceptCall) printf("Accepted");
  else  printf("Rejected");
}

bool HC06_ComingCallIsHangupByPeer(hc06_t* obj)
{
  return false;
}
