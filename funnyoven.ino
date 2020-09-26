#define high true
#define low false
#define output true
#define input false

#define UART_DELAY_US 100

#include <LiquidCrystal.h>

//change steinhart-hart coefficients to match your thermistor
const float abc[] = {0.0010426455642216109, 0.00009633115006360066, 7.731725468701475e-7};

uint8_t ignoreCount = 0;
const unsigned int R2 = 1371;
uint8_t pressedCount;
bool cState = false;
bool eligible = true;
unsigned int ticks = 0;
uint8_t bPrev;
char buf[16];
unsigned int sTime = 0;
unsigned int stage = 0;

//change to fit solder
unsigned int soakTime = 30;
unsigned int soakTemp = 150;
unsigned int reflowTime = 10;
unsigned int reflowTemp = 225;
unsigned int coolTemp = 65;

//adjust if somehow the pinout for the lcd is different
const int rs = 0, en = 1, d4 = 2, d5 = 3, d6 = 4, d7 = 5;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  aInit();
  pinDir(8, output);
  pinDir(9, output);
  pinDir(10, output);
  lcd.begin(16, 2);
  pinState(9, high);
  pinState(10, high);
}

void loop() {
  lcd.setCursor(0, 0);
  ltoa(lrint(getTemp(6)), buf, 10);
  lcd.print(buf);
  lcd.print('C');
  lcd.print(' ');
  bPrev = (PINB & (1 << PINB2));
  _delay_ms(100);
  if(bPrev == (PINB & (1 << PINB2)) && bPrev == 0){
    if(ignoreCount <= 0){
      pressedCount++;
    } else {
      ignoreCount--;
    }
  } else {
    pressedCount = 0;
    if(ignoreCount > 0){
      ignoreCount--;
    }
  }
  if(pressedCount >= 4){
    pressedCount = 0;
    ignoreCount = 30;
    cState = !cState;
  }

  if(cState){
    lcd.setCursor(0, 1);
    //for more messages look in the initial commits
    lcd.print("generating pogs"); 
    if(eligible){
      stage = 0;
      eligible = false;
    }
    ticks++;
  } else {
//    lcd.setCursor(0, 1);
//    lcd.print(F("                "));
//    lcd.setCursor(0, 1);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(F("cringe"));
    ticks = 0;
    stage = 5;
    eligible = true;
    pinState(8, low);
    pinState(9, low);
    pinState(10, low);
  }
  pinState(8, pogFunc(ticks / 10));
}

bool pogFunc(unsigned int sec){
  int temp = getTemp(6)+0.5f;
  switch(stage){
    case 0:
    if(temp >= 150){
      stage = 1;
      sTime = sec;
      break;
    }
    lcd.setCursor(9, 0);
    lcd.print(F("Preheat"));
    return true;
    case 1:
    if(sec - sTime >= soakTime){
      stage = 2;
      break;
    }
    lcd.setCursor(9, 0);
    lcd.print(F("   Soak"));
    return (temp < soakTemp) ? true : false;
    case 2:
    if(temp >= reflowTemp){
      stage = 3;
      sTime = sec;
      break;
    }
    lcd.setCursor(5, 0);
    lcd.print(F("Pre-Reflowo"));
    return true;
    case 3:
    if(sec - sTime >= reflowTime){
      stage = 4;
      break;
    }
    lcd.setCursor(5, 0);
    lcd.print(F("    Reflowo"));
    return (temp < reflowTemp) ? true : false;
    case 4:
    if(temp < coolTemp){
      stage = 5;
      break;
    }
    lcd.setCursor(9, 0);
    lcd.print(F("Cooling"));
    return false;
    default:
    eligible = true;
    lcd.setCursor(9, 0);
    lcd.print(F("   Idle"));
    return false;
  }
}

void aInit(){
  ADMUX = 0x00;
  //use internal 1.1V reference
  //ADMUX |= (1 << REFS1);
  ADCSRA |= (1 << ADEN) | 0b111;
}

uint16_t analogGet(uint8_t pin){
  //pin &= 0b00000111;
  //ADMUX = (ADMUX & 0x11111000) | pin;
  ADMUX = 0b00000110;
  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC));
//  return (ADCH << 8) | ADCL;
  return ADC;
}

float getTemp(int pin){
  float volt = ((float)5 / 1023) * analogRead(pin);
//  ltoa(lrint(volt * 1000), buf, 10);
//  uartPrint(buf, 1);
//  uartSend(1, 'm');
//  uartSend(1, 'V');
//  uartSend(1, 0xA);
//  uartSend(1, 0xD);
  float resistance = R2 * (((float)5 / volt) - 1);
//  ltoa(lrint(resistance), buf, 10);
//  uartPrint(buf, 1);
//  uartSend(1, 'R');
//  uartSend(1, 0xA);
//  uartSend(1, 0xD);
//  F
//  return ((float)1 / (abc[0] + abc[1] * log(resistance) + abc[2] * log(resistance) * log(resistance) * log(resistance)) - 273.15) * 1.8 + 32;
//  C
  return ((float)1 / (abc[0] + abc[1] * log(resistance) + abc[2] * log(resistance) * log(resistance) * log(resistance))) - 273.15;
}

float getTempB(int pin){
  float volt = ((float)5 / 1023) * analogGet(pin);
//  ltoa(lrint(volt * 1000), buf, 10);
//  uartPrint(buf, 1);
//  uartSend(1, 'm');
//  uartSend(1, 'V');
//  uartSend(1, 0xA);
//  uartSend(1, 0xD);
  float resistance = R2 * (((float)5 / volt) - 1);
//  ltoa(lrint(resistance), buf, 10);
//  uartPrint(buf, 1);
//  uartSend(1, 'R');
//  uartSend(1, 0xA);
//  uartSend(1, 0xD);
  return ((5) - 273.15) * 1.8 + 32;
}

void pinState(uint8_t pin, bool state){
  if(pin < 12 && pin > -1){
    if(pin < 8){
      if(state){ //input
        PORTA |= (1 << pin);
      } else { //output
        PORTA &= ~(1 << pin);
      }
    } else {
      pin -= 8;
      if(state){ //input
        PORTB |= (1 << pin);
      } else { //output
        PORTB &= ~(1 << pin);
      }
    }
  }
}

void pinDir(uint8_t pin, bool state){
  if(pin < 12 && pin > -1){
    if(pin < 8){
      if(state){ //output
        PORTA |= (0b10000000 >> pin);
      } else { //input
        PORTA &= ~(0b10000000 >> pin);
      }
    } else {
      pin -= 8;
      if(state){ //output
        PORTB |= (0b10000000 >> pin);
      } else { //input
        PORTB &= ~(0b10000000 >> pin);
      }
    }
  }
}

//not used but here if you want
void uartSend(uint8_t pin, uint8_t ascii){
  PORTA &= ~(1 << pin);
  _delay_us(UART_DELAY_US);
  for(int i = 0; i < 8;  i++){
    if((ascii & 1) == 1){
      PORTA |= (1 << pin);
      } else {
      PORTA &= ~(1 << pin);
    }
    ascii >>= 1;
    _delay_us(UART_DELAY_US);
  }
  PORTA |= (1 << pin);
  _delay_us(UART_DELAY_US);
}

void uartPrint(char *str, uint8_t pin){
  while(*str != 0){
    uartSend(1, *str);
    str++;
  }
}
