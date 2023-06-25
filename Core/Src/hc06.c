#include "hc06.h"
#include <string.h>
#include "usart.h"

static void HC06_WaitMsg(hc06_t* obj)
{
  HAL_UART_Receive_IT(&huart2, (uint8_t*)&obj->ch_buffer, 1);
}

void HC06_Init(hc06_t* obj)
{
  obj->msg_type = MSG_TYPE_NOT_COMPLETED;
  obj->msg_len = 0;
  obj->msg_status = MSG_STATUS_ON_HEADER_TYPE;
  obj->rx_buffer_write_pos = 0;
  HC06_WaitMsg(obj);
}

void HC06_SendMsg(hc06_t* obj, const char* str, uint16_t size)
{
  uint16_t i;
  for (i = 0; i < size; ++i)
  {
    HAL_UART_Transmit(&huart2, (const uint8_t*)(str+i), 1, 0xffff);
  }
}

uint8_t HC06_HandleMsg(hc06_t* obj)
{
  uint8_t msg_type = MSG_TYPE_NOT_COMPLETED;
  switch (obj->msg_status)
  {
  case MSG_STATUS_ON_HEADER_TYPE:
    obj->msg_type = obj->ch_buffer;
    obj->msg_status = MSG_STATUS_ON_HEADER_LENGTH;
    break;

  case MSG_STATUS_ON_HEADER_LENGTH:
    obj->msg_len = obj->ch_buffer;
    if (0 == obj->msg_len)
    {
      msg_type = obj->msg_type;
      obj->msg_status = MSG_STATUS_ON_HEADER_TYPE;
    }
    else
    {
      obj->msg_status = MSG_STATUS_ON_BODY_RECV;
    }
    break;

  case MSG_STATUS_ON_BODY_RECV:
    obj->rx_buffer[obj->rx_buffer_write_pos++] = obj->ch_buffer;
    if (obj->rx_buffer_write_pos == obj->msg_len)
    {
      msg_type = obj->msg_type;
      obj->rx_buffer_write_pos = 0;
      obj->msg_status = MSG_STATUS_ON_HEADER_TYPE;
    }
    break;
  
  default:
    break;
  }
  HC06_WaitMsg(obj);
  return msg_type;
}

void ComingCallHandler_Init(coming_call_handler_t* this, hc06_t* driver)
{
  this->has_new_msg = false;
  this->is_handled = false;
  this->p_driver = driver;
  memset(this->content, 0, sizeof(this->content));
  this->missed_call_count = 0;
}

bool ComingCallHandler_IsNewCallComing(coming_call_handler_t* this)
{
  if (this->has_new_msg)
  {
    this->has_new_msg = false;
    return true;
  }
  else
    return false;
}

void ComingCallHandler_NewCallNotify(coming_call_handler_t* this)
{
  memset(this->content, 0, sizeof(this->content));
  memcpy(this->content, this->p_driver->rx_buffer, this->p_driver->msg_len);
  this->content[this->p_driver->msg_len] = '\0';
  this->has_new_msg = true;
  this->is_handled = false;
}

void ComingCallHandler_AcceptCallNotify(coming_call_handler_t* this)
{
  this->has_new_msg = false;
  this->is_handled = true;
}

void ComingCallHandler_PeerHangupNotify(coming_call_handler_t* this)
{
  this->has_new_msg = false;
  this->is_handled = true;
  ++this->missed_call_count;
}

const char* ComingCallHandler_GetContent(const coming_call_handler_t* this)
{
  return this->content;
}

void ComingCallHandler_SetChoice(coming_call_handler_t* this, bool isAcceptCall)
{
  if (!isAcceptCall)
  {
    char msg[2];
    msg[0] = MSG_TYPE_CALL_REJECT_NOTIFY;
    msg[1] = 0x00;
    HC06_SendMsg(this->p_driver, msg, 2);
  }
}

bool ComingCallHandler_IsCallHandled(coming_call_handler_t* this)
{
  if (this->is_handled)
  {
    this->is_handled = false;
    return true;
  }
  else
    return false;
}
