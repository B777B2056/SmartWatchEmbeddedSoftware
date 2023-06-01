#include "hc06.h"
#include <string.h>
#include <stdio.h>
#include "usart.h"

// #define RX_BUFFER_SIZE 16
#define MSG_END_DELIMITER '\n'
#define MSG_SIZE 16

char ch_buffer;
char rx_buffer[MSG_SIZE];
uint8_t rx_buffer_write_pos = 0;
char current_msg[MSG_SIZE];
bool has_new_msg = false;

void HC06_SendString(const char* str, uint16_t size)
{
  HAL_UART_Transmit(&huart2, (const uint8_t*)str, size, 0xffff);
}

void HC06_WaitMsg()
{
  HAL_UART_Receive_IT(&huart2, (uint8_t*)&ch_buffer, 1);
}

const char* HC06_GetRecvedMsg()
{
  printf("recved msg: %s\n", current_msg);
  return current_msg;
}

bool HC06_IsNewMsgRecved()
{
  return has_new_msg;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (rx_buffer_write_pos >= MSG_SIZE)  return; // 缓冲区满
  has_new_msg = (MSG_END_DELIMITER == ch_buffer);
  if (has_new_msg)
  {
    memcpy(current_msg, rx_buffer, rx_buffer_write_pos);
    current_msg[rx_buffer_write_pos] = '\0';
    rx_buffer_write_pos = 0;
  }
  else
  {
    rx_buffer[rx_buffer_write_pos++] = ch_buffer;
  }
}

void HC06_ComingCallOption(bool isAcceptCall)
{
  if (isAcceptCall) printf("Accepted");
  else  printf("Rejected");
}

bool HC06_ComingCallIsHangupByPeer()
{
  return false;
}
