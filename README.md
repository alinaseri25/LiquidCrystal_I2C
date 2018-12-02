# LiquidCrystal_I2C

to use this lib you should be enable C99 Mode and Add --cpp to misc controls (in options for target --> C/C++ tab)

then you can add library to your project and use it

#include "dwt_stm32_delay.h"
#include "LCD_I2C.hpp"


LCDI2C LCD(0x4E,2,16,&hi2c1,LCD_5x8DOTS); //for example use hi2c1


DWT_Delay_Init();
LCD.begin();


LCD.setCursor(0,0);
LCD.printstr(data); // char data[80];


you can use any fucntion in lcdi2c for arduino libraries lik:

	void clear(void);
	void home(void);
	void noDisplay(void);
	void display(void);
	void noBlink(void);
	void blink(void);
	void noCursor(void);
	void cursor(void);
	void scrollDisplayLeft(void);
	void scrollDisplayRight(void);
	void printLeft(void);
	void printRight(void);
	void leftToRight(void);
	void rightToLeft(void);
	void noBacklight(void);
	void backlight(void);
	int getBacklight(void);
	void autoscroll(void);
	void noAutoscroll(void);
	void createChar(uint8_t, uint8_t[]);
	void setCursor(uint8_t, uint8_t);

  void load_custom_character(uint8_t char_num, uint8_t *rows);	// alias for createChar()
	void printstr(char *str);
	
	HAL_StatusTypeDef getLCDI2CState(void);
	
	uint8_t getCurrentCol(void);
	uint8_t getCurrentRow(void);
	
