#ifndef __HC06_H__
#define __HC06_H__

#include <stdint.h>
#include <stdbool.h>

void HC06_SendString(const char* str, uint16_t size);
void HC06_WaitMsg();
const char* HC06_GetRecvedMsg();
bool HC06_IsNewMsgRecved();
void HC06_ComingCallOption(bool isAcceptCall);
bool HC06_ComingCallIsHangupByPeer();

#endif
