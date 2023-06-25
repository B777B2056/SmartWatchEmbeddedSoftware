#ifndef __HC06_H__
#define __HC06_H__

#include <stdint.h>
#include <stdbool.h>

#define MSG_SIZE 32

#define MSG_TYPE_NOT_COMPLETED        0X00
#define MSG_TYPE_COMING_CALL_NOTIFY   0X01
#define MSG_TYPE_CALL_HANGUP_NOTIFY   0X02
#define MSG_TYPE_STEPCNT              0X03
#define MSG_TYPE_KAL                  0x04
#define MSG_TYPE_HEART_RATE           0x05
#define MSG_TYPE_SPO2                 0x06
#define MSG_TYPE_CALL_REJECT_NOTIFY   0x07
#define MSG_TYPE_CALL_ACCEPT_NOTIFY   0X08

#define MSG_STATUS_ON_HEADER_TYPE     0X00
#define MSG_STATUS_ON_HEADER_LENGTH   0X01
#define MSG_STATUS_ON_BODY_RECV       0X02

typedef struct
{
  char ch_buffer;
  uint8_t msg_type;
  uint8_t msg_len;
  uint8_t msg_status;
  char rx_buffer[MSG_SIZE];
  uint8_t rx_buffer_write_pos;
  char current_msg[MSG_SIZE];
} hc06_t;

void HC06_Init(hc06_t* obj);
void HC06_SendMsg(hc06_t* obj, const char* str, uint16_t size);
uint8_t HC06_HandleMsg(hc06_t* obj);

typedef struct
{
  bool is_handled;
  bool has_new_msg;
  char content[MSG_SIZE];
  uint8_t missed_call_count;

  hc06_t* p_driver;
} coming_call_handler_t;

void ComingCallHandler_Init(coming_call_handler_t* this, hc06_t* driver);
void ComingCallHandler_NewCallNotify(coming_call_handler_t* this);
void ComingCallHandler_AcceptCallNotify(coming_call_handler_t* this);
void ComingCallHandler_PeerHangupNotify(coming_call_handler_t* this);
bool ComingCallHandler_IsNewCallComing(coming_call_handler_t* this);
const char* ComingCallHandler_GetContent(const coming_call_handler_t* this);
void ComingCallHandler_SetChoice(coming_call_handler_t* this, bool isAcceptCall);
bool ComingCallHandler_IsCallHandled(coming_call_handler_t* this);

#endif
