#include "LCD_I2C.hpp"
// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 8-bit interface data
//    N = 0; 1-line display
//    F = 0; 5x8 dot character font
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//
// Note, however, that resetting the Micro doesn't reset the LCD, so we
// can't assume that its in that state when a program starts (and the
// LiquidCrystal constructor is called).


LCDI2C::LCDI2C(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows, I2C_HandleTypeDef *_hi2c, uint8_t charsize)
{
	__disable_irq();
	_addr = lcd_addr;
	_cols = lcd_cols;
	_rows = lcd_rows;
	_charsize = charsize;
	_backlightval = LCD_BACKLIGHT;
	hi2c = _hi2c;
	Cur_Row = Cur_Col = 0;
	
	LCD_State = HAL_OK;
	__enable_irq();
}

void LCDI2C::begin() {
	DWT_Delay_Init();
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

	if (_rows > 1) {
		_displayfunction |= LCD_2LINE;
	}

	// for some 1 line displays you can select a 10 pixel high font
	if ((_charsize != 0) && (_rows == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	HAL_Delay(50);

	// Now we pull both RS and R/W low to begin commands
	expanderWrite(_backlightval);	// reset expanderand turn backlight off (Bit 8 =1)
	HAL_Delay(1000);
	HAL_IWDG_Refresh(&hiwdg);

	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46

	// we start in 8bit mode, try to set 4 bit mode
	write4bits(0x03 << 4);
	DWT_Delay_us(4500); // wait min 4.1ms

	// second try
	write4bits(0x03 << 4);
	DWT_Delay_us(4500); // wait min 4.1ms

	// third go!
	write4bits(0x03 << 4);
	DWT_Delay_us(150);

	// finally, set to 4-bit interface
	write4bits(0x02 << 4);
	HAL_IWDG_Refresh(&hiwdg);

	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);

	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();
	HAL_IWDG_Refresh(&hiwdg);

	// clear it off
	clear();

	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);

	home();
}

/********** high level commands, for the user! */
void LCDI2C::clear(){
	__disable_irq();
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	DWT_Delay_us(2000);  // this command takes a long time!
	__enable_irq();
}

void LCDI2C::home(){
	__disable_irq();
	command(LCD_RETURNHOME);  // set cursor position to zero
	DWT_Delay_us(2000);  // this command takes a long time!
	__enable_irq();
}

void LCDI2C::setCursor(uint8_t col, uint8_t row){
	__disable_irq();
	Cur_Col = col;
	Cur_Row = row;
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if (row > _rows) {
		row = _rows-1;    // we count rows starting w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
	__enable_irq();
}

// Turn the display on/off (quickly)
void LCDI2C::noDisplay() {
	__disable_irq();
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
	__enable_irq();
}
void LCDI2C::display() {
	__disable_irq();
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
	__enable_irq();
}

// Turns the underline cursor on/off
void LCDI2C::noCursor() {
	__disable_irq();
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
	__enable_irq();
}
void LCDI2C::cursor() {
	__disable_irq();
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
	__enable_irq();
}

// Turn on and off the blinking cursor
void LCDI2C::noBlink() {
	__disable_irq();
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
	__enable_irq();
}
void LCDI2C::blink() {
	__disable_irq();
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
	__enable_irq();
}

// These commands scroll the display without changing the RAM
void LCDI2C::scrollDisplayLeft(void) {
	__disable_irq();
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
	__enable_irq();
}
void LCDI2C::scrollDisplayRight(void) {
	__disable_irq();
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
	__enable_irq();
}

// This is for text that flows Left to Right
void LCDI2C::leftToRight(void) {
	__disable_irq();
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
	__enable_irq();
}

// This is for text that flows Right to Left
void LCDI2C::rightToLeft(void) {
	__disable_irq();
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
	__enable_irq();
}

// This will 'right justify' text from the cursor
void LCDI2C::autoscroll(void) {
	__disable_irq();
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
	__enable_irq();
}

// This will 'left justify' text from the cursor
void LCDI2C::noAutoscroll(void) {
	__disable_irq();
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
	__enable_irq();
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LCDI2C::createChar(uint8_t location, uint8_t charmap[]) {
	__disable_irq();
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
	__enable_irq();
}

// Turn the (optional) backlight off/on
void LCDI2C::noBacklight(void) {
	__disable_irq();
	_backlightval=LCD_NOBACKLIGHT;
	expanderWrite(0);
	__enable_irq();
}

void LCDI2C::backlight(void) {
	__disable_irq();
	_backlightval=LCD_BACKLIGHT;
	expanderWrite(0);
	__enable_irq();
}
int LCDI2C::getBacklight() {
  return _backlightval == LCD_BACKLIGHT;
}


/*********** mid level commands, for sending data/cmds */

void LCDI2C::command(uint8_t value) {
	send(value, 0);
}

uint8_t LCDI2C::write(uint8_t value) {
	send(value, Rs);
	return 1;
}


/************ low level data pushing commands **********/

// write either command or data
void LCDI2C::send(uint8_t value, uint8_t mode) {
	uint8_t highnib=value&0xf0;
	uint8_t lownib=(value<<4)&0xf0;
	write4bits((highnib)|mode);
	write4bits((lownib)|mode);
}

void LCDI2C::write4bits(uint8_t value) {
	expanderWrite(value);
	pulseEnable(value);
}

void LCDI2C::expanderWrite(uint8_t _data){
	if(LCD_State == HAL_OK)
	{
		uint8_t data = ((int)(_data) | _backlightval);
		LCD_State = HAL_I2C_Master_Transmit(hi2c,_addr,&data,1,1);
	}
}

void LCDI2C::pulseEnable(uint8_t _data){
	expanderWrite(_data | En);	// En high
	DWT_Delay_us(1);		// enable pulse must be >450ns

	expanderWrite(_data & ~En);	// En low
	DWT_Delay_us(50);		// commands need > 37us to settle
}

void LCDI2C::load_custom_character(uint8_t char_num, uint8_t *rows){
	createChar(char_num, rows);
}

void LCDI2C::setBacklight(uint8_t new_val){
	if (new_val) {
		backlight();		// turn backlight on
	} else {
		noBacklight();		// turn backlight off
	}
}

void LCDI2C::printstr(char *str){
	//This function is not identical to the function used for "real" I2C displays
	//it's here so the user sketch doesn't have to be changed
	__disable_irq();
	while (*str) 
	{
		if(*str == '\n')
		{
			Cur_Row++;
			setCursor(0,Cur_Row);
			str++;
		}
		else if(*str == '\r')
		{
			str++;
		}
		else
		{
			write(*str++);
			Cur_Col++;
		}
	}
	__enable_irq();
}
void LCDI2C::blink_on(void) 
{
	blink();
}
void LCDI2C::blink_off(void) 
{
	noBlink(); 
}
void LCDI2C::cursor_on(void) 
{ 
	cursor();
}
void LCDI2C::cursor_off(void) 
{
	noCursor(); 
}

HAL_StatusTypeDef LCDI2C::getLCDI2CState(void)
{
	return LCD_State;
}

uint8_t LCDI2C::getCurrentCol(void)
{
	return Cur_Col;
}

uint8_t LCDI2C::getCurrentRow(void)
{
	return Cur_Row;
}
