#include "adxl345_hal.h"

#define DEVID		        0X00 	// 器件ID
#define THRESH_TAP		  0X1D  // 敲击阀值寄存器
#define OFSX			      0X1E
#define OFSY			      0X1F
#define OFSZ			      0X20
#define DUR				      0X21
#define Latent			    0X22
#define Window  		    0X23 
#define THRESH_ACT		  0X24	// 运动阈值寄存器
#define THRESH_INACT	  0X25 	// 静止阈值寄存器
#define TIME_INACT		  0X26	// 静止时间			比例1 sec /LSB
#define ACT_INACT_CTL	  0X27	// 启用运动/静止检测
#define THRESH_FF		    0X28	// 自由下落阈值	建议采用300 mg与600 mg(0x05至0x09)之间的值 比例62.5 mg/LSB
#define TIME_FF			    0X29 	// 自由下落时间	建议采用100 ms与350 ms(0x14至0x46)之间的值 比例5ms/LSB
#define TAP_AXES		    0X2A  
#define ACT_TAP_STATUS  0X2B 
#define BW_RATE			    0X2C 
#define POWER_CTL		    0X2D 
#define INT_ENABLE		  0X2E	// 设置中断配置
#define INT_MAP			    0X2F
#define INT_SOURCE  	  0X30
#define DATA_FORMAT	    0X31
#define DATA_X0			    0X32
#define DATA_X1			    0X33
#define DATA_Y0			    0X34
#define DATA_Y1			    0X35
#define DATA_Z0			    0X36
#define DATA_Z1			    0X37
#define FIFO_CTL		    0X38
#define FIFO_STATUS		  0X39

#define ADXL345_ADDR    (0X53 << 1)

I2C_HandleTypeDef* adxl345_hi2c = NULL;

void ADXL345_Driver_Init(I2C_HandleTypeDef* hi2c)
{
  
  uint8_t id;
  adxl345_hi2c = hi2c;
  HAL_I2C_Mem_Read(adxl345_hi2c, ADXL345_ADDR, DEVID,I2C_MEMADD_SIZE_8BIT, &id, 1, 0xff);
  if (0XE5 == id)
  {
    ADXL345_Driver_Write(INT_ENABLE, 0x00);
    ADXL345_Driver_Write(DATA_FORMAT, 0x0B);
    ADXL345_Driver_Write(BW_RATE, 0x08);
    ADXL345_Driver_Write(POWER_CTL, 0x08);
    ADXL345_Driver_Write(OFSX, 0X00);
    ADXL345_Driver_Write(OFSY, 0X00);
    ADXL345_Driver_Write(OFSZ, 0X00);
  }
}

void ADXL345_Driver_Write(uint8_t addr, uint8_t value)
{
  if (addr > 63)  addr = 63;
  addr &= ~(0x80);
	HAL_I2C_Mem_Write(adxl345_hi2c, ADXL345_ADDR, addr, I2C_MEMADD_SIZE_8BIT, &value, 1, 0xFF);
}

void ADXL345_Driver_Read(uint8_t addr, uint8_t* vvalue)
{
  if (addr > 63)  addr = 63;
	addr &= ~(0x40);
  addr |= (0x80);
  HAL_I2C_Mem_Read(adxl345_hi2c, ADXL345_ADDR, addr, I2C_MEMADD_SIZE_8BIT, vvalue, 1, 0xFF);
}

void ADXL345_Driver_Axis_Data(float* acc_x, float* acc_y, float* acc_z)
{
  uint8_t l, h;
	{
    ADXL345_Driver_Read(DATA_X0, &l);
    ADXL345_Driver_Read(DATA_X1, &h);
    *acc_x =  (float)((int16_t)(((uint16_t)h << 8) | l)) / 256.0f;
  }
  {
    ADXL345_Driver_Read(DATA_Y0, &l);
    ADXL345_Driver_Read(DATA_Y1, &h);
    *acc_y =  (float)((int16_t)(((uint16_t)h << 8) | l)) / 256.0f;
  }
  {
    ADXL345_Driver_Read(DATA_Z0, &l);
    ADXL345_Driver_Read(DATA_Z1, &h);
    *acc_z =  (float)((int16_t)(((uint16_t)h << 8) | l)) / 256.0f;
  }
}
