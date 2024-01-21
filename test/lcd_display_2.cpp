#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

extern "C"{
#include <errno.h>
#include <i2c/smbus.h>
}

#define OK 0
#define FAILED 1

#define LCD_SLAVE_ADDR (0x27)

// Command
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En 0b00000100  // Enable bit
#define Rw 0b00000010  // Read/Write bit
#define Rs 0b00000001  // Register select bit

///////////////////////////////////
//         description           //
///////////////////////////////////
class LCD{
private:
    int fd;
    int i2c_bus = 1;
    char filename[11];
	uint8_t _cols = 16;
	uint8_t _rows = 2;
    uint8_t _addr = LCD_SLAVE_ADDR;
	uint8_t _backlightval = LCD_BACKLIGHT;
    uint8_t _displayfunction;
    uint8_t _displaymode;
    uint8_t _displaycontrol;
public:
    LCD(void);
    ~LCD();
    int init(const char* filename, int slave_addr = LCD_SLAVE_ADDR);
    void begin();
    void noDisplay();
    void display();
    void clear();
    void home();
    void setcursor(uint8_t col, uint8_t row);
    void noCursor();
    void cursor();
    void noBlink();
    void blink();
    void scrollDisplayLeft();
    void scrollDisplayRight();
    void leftToRight();
    void rightToLeft();
    void autoscroll();
    void noAutoscroll();
    void createChar(uint8_t location, uint8_t charmap[]);
    void noBacklight();
    void backlight();
    void setBacklight(uint8_t new_val);
    int writeString();
private:
    void write4bits(uint8_t value);
    int expanderWrite(uint8_t data);
    void pulseEnable(uint8_t data);
    void command(uint8_t value, uint8_t mode = 0);
private:
    void sleep(uint16_t ms_time);
    void reportError(int error, std::string info = "Errno");
};

int main(){
    LCD lcd;
    std::cout << "Press Enter to exit." << std::endl;
    lcd.cursor();
    lcd.blink();
    lcd.backlight();  
    lcd.writeString();  
    getchar();
    lcd.clear();
    lcd.backlight();
    return 0;
}

///////////////////////////////////
//         definition            //
///////////////////////////////////
LCD::LCD(void){
    std::cout << "The file has been launched." << std::endl;
    snprintf(filename, 11, "/dev/i2c-%d", i2c_bus);
    if(init(filename, LCD_SLAVE_ADDR)){
        exit(-1);
    }
    begin();
}

LCD::~LCD(){
    close(fd);
}

int LCD::init(const char* filename, int slave_addr){
    fd = open(filename, O_RDWR);
    if(fd < 0){
        reportError(errno, "Failed  to open I2C device.");
        return FAILED;
    }
    if(ioctl(fd, I2C_SLAVE, slave_addr)){
        reportError(errno, "Failed to connect to the I2C device");
        close(fd);
        return FAILED;
    }
    return OK;
}

void LCD::begin(){
    _displayfunction = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
    sleep(50);
    expanderWrite(_backlightval);
    sleep(1000);

    write4bits(0x03 << 4);
    sleep(5);
    write4bits(0x03 << 4);
    sleep(5);
    write4bits(0x03 << 4);
    sleep(5);
    write4bits(0x02 << 4);
    command(LCD_FUNCTIONSET | _displayfunction);
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();
    clear();
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    command(LCD_ENTRYMODESET | _displaymode);
    home();
}

void LCD::noDisplay(){
    _displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD::display(){
    _displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD::clear(){
    command(LCD_CLEARDISPLAY);
	sleep(2);
}

void LCD::home(){
    command(LCD_RETURNHOME);
	sleep(2);
}

void LCD::setcursor(uint8_t col, uint8_t row){
    int row_offsets[] = { 0x00, 0x40 };
    if(row > _rows && row < 0){
        row = _rows-1;
    }
    command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void LCD::noCursor(){
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD::cursor(){
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD::noBlink(){
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD::blink(){
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD::scrollDisplayLeft(){
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void LCD::scrollDisplayRight(){
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void LCD::leftToRight(){
    _displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LCD::rightToLeft(){
    _displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LCD::autoscroll(){
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LCD::noAutoscroll(){
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LCD::createChar(uint8_t location, uint8_t charmap[]){
    location &= 0x7; 
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		command(charmap[i], Rs);
	}
}

void LCD::noBacklight() {
	_backlightval = LCD_NOBACKLIGHT;
	expanderWrite(0);
}

void LCD::backlight() {
	_backlightval = LCD_BACKLIGHT;
	expanderWrite(_backlightval);
}

void LCD::setBacklight(uint8_t new_val){
	if (new_val) {
		backlight();		// turn backlight on
	} else {
		noBacklight();		// turn backlight off
	}
}

int LCD::writeString(){
    char * text = "deneme";
    unsigned char ucTemp[2];
    int i = 0;
    if(fd < 0 || text == NULL){
        return FAILED;
    }

    while (i < 16 && *text) {
        ucTemp[0] = _backlightval | 1 | (*text & 0xf0);
		write(fd, ucTemp, 1);
        sleep(5);
        ucTemp[0] |= 4;
        write(fd, ucTemp, 1);
		sleep(5);
		ucTemp[0] &= ~4;
		write(fd, ucTemp, 1);
		sleep(5);
        ucTemp[0] = _backlightval | 1 | (*text << 4);
		write(fd, ucTemp, 1);
		ucTemp[0] |= 4; // pulse E
        write(fd, ucTemp, 1);
        sleep(5);
        ucTemp[0] &= ~4;
        write(fd, ucTemp, 1);
        usleep(5);
		text++;
		i++;
    }
    

}

//////////////////////////////////////
void LCD::write4bits(uint8_t value){
    expanderWrite(value);
	pulseEnable(value);
}

int LCD::expanderWrite(uint8_t data){
    if(write(fd, &data, 1) != 1){
        reportError(errno, "Error writing data over I2C");
        return FAILED;
    }
    return OK;
}

void LCD::pulseEnable(uint8_t data){
    expanderWrite(data | En);
    usleep(1);
    expanderWrite(data & ~En);
    usleep(50);
}

void LCD::command(uint8_t value, uint8_t mode){
    uint8_t highnib = value & 0xf0;
	uint8_t lownib = (value<<4) & 0xf0;
	write4bits((highnib) | mode);
	write4bits((lownib) | mode);
}

/////////////////////////////////////////
void LCD::sleep(uint16_t ms_time){
    usleep(ms_time * 1000);
}

void LCD::reportError(int error, std::string info){
    std::cerr << "Error! " << info << " : " << strerror(error);
}