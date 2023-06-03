#ifndef __HC06_H__
#define __HC06_H__

#include <stdint.h>
#include <stdbool.h>

#define MSG_SIZE 16
#define MSG_END_DELIMITER '\n'

typedef struct
{
  char ch_buffer;
  char rx_buffer[MSG_SIZE];
  uint8_t rx_buffer_write_pos;
  char current_msg[MSG_SIZE];
  bool has_new_msg;
} hc06_t;

void HC06_Init(hc06_t* obj);
void HC06_SendString(hc06_t* obj, const char* str, uint16_t size);
void HC06_WaitMsg(hc06_t* obj);
const char* HC06_GetRecvedMsg(hc06_t* obj);
bool HC06_IsNewMsgRecved(hc06_t* obj);
void HC06_ComingCallOption(hc06_t* obj, bool isAcceptCall);
bool HC06_ComingCallIsHangupByPeer(hc06_t* obj);

#endif
