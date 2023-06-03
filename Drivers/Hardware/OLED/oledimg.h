#ifndef __OLEDIMG_H_
#define __OLEDIMG_H_

const uint8_t HEART_RATE_IMAGE[] = 
{
    0x30,0xFC,0xFE,0xFE,0xFE,0x3E,0xFE,0x3C,0x1C,0xFE,0x7E,0xFE,0xFE,0xFE,0xFC,0x78,
    0x00,0x00,0x02,0x06,0x06,0x0F,0x1C,0x3F,0x3F,0x19,0x0E,0x0E,0x06,0x02,0x00,0x00,/*"heart_rate.bmp",0*/
};

const uint8_t STEP_CNT_IMAGE[] = 
{
    0xFF,0xFF,0xFF,0xFF,0xFF,0x3F,0x3F,0xFF,0xFF,0x8F,0x8F,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x7F,0x7F,0x7F,0x7F,0x7F,0x7E,0x78,0x7B,0x7E,0x7E,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,/*"step_cnt.bmp",0*/
};


#endif