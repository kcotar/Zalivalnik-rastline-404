// deluje z AttinyCore jedrom za attiny84 na frekvenci 1 ter 4 MHz (internal)
// za oznacevanjev portov se uporablja priporocena izbira clockwise 

// vkljucitev dodatnih knjiznic, ki na omogocajo lazje rokovanje z LCD zaslonom in servo motorckom
#include <Servo_ATTinyCore.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// port in inicializacija za servo
Servo servo;
#define SERV 7

// port za temp senzor
#define TMP A3

// port za pumpo vode
#define PUMP 8

// port za vlagomer, ter kalibracijske nastavitve
#define HUM A2
int hum_voda = 1; // odcitek v vodi
int hum_zrak = 2; // odcitev na zraku
int hum_prst = 3; // odcitek v suhi prsti

// porti za tipke
#define T_OPT 10
#define T_PLU 9
#define T_MIN 1

// nastavitev velikosti (16x2) in naslov (dolocen preko A0,A1,A2 padov na zadnji strani) zaslona
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

// definicija ostalih globalnih spremenljivk


// funkcija setup, se pozene le enkrat ob vklopu
void setup() 
{
  // inicializacija LCD-ja
  lcd.init();
  lcd.backlight();

  // nastavitev delovanja (vhod/izhod) portov
  pinMode(TMP, INPUT);
  pinMode(HUM, INPUT);
  pinMode(SERV, OUTPUT);
  pinMode(T_OPT, OUTPUT);
  pinMode(T_PLU, OUTPUT);
  pinMode(T_MIN, OUTPUT);
  pinMode(PUMP, OUTPUT);
}


// funkcija za veckratno zaporednje branje analognega senzorja
// vrne povprecje vecih meritev
float analog_beri_veckrat(int pin, int ponovitev)
{
  float analog_vrednost = 0;
  
  for(int i = 0; i < ponovitev; i++)
  {
    analog_vrednost += analogRead(pin);
  }
  
  return analog_vrednost / ponovitev;
}


//
void servo_max()
{
  servo.attach(SERV);
  delay(250);
  servo.write(180);
  delay(1000);
  servo.detach();
  delay(250);
}


//
void servo_min()
{
  servo.attach(SERV);
  delay(250);
  servo.write(0);
  delay(1000);
  servo.detach();
  delay(250);
}


//
void pozeni_pumpo(float moc, float cas)
{
  analogWrite(PUMP, 255 * moc);
  delay(cas * 1000);
  digitalWrite(PUMP, LOW);
}


// funkcija za pretvarjanje vrednosti analognega odcitka v stopinje C za temperaturni senzor
float analog_v_c(float vrednost)
{
  // nicne kalibrirane vrednosti pobrane iz tehnicne dokumentacije
  int T_0 = 25;  // v enotah C
  int i_0 = 750;  // v enotah mV
  int k_iT = 10;  // naklon premice v enotah mV/C, veljaven za razpon med -40 do 125 stopinj C

  float napetost = vrednost / 1023. * 5.;  // vrednost ADC-ja v napetost 
  float temp = (napetost - i_0/1000.) / (k_iT/1000.) + T_0;
  return temp;
}


// funkcija loop tece ves cas in se ponavlja
void loop()
{

  float temp_analog = analog_beri_veckrat(TMP, 100);
  float temp_c = analog_v_c(temp_analog);
  lcd.setCursor(0, 0);
  lcd.print(temp_c);

  int prit_tipke = digitalRead(T_OPT)*1 + digitalRead(T_PLU)*2 + digitalRead(T_MIN)*4;
  lcd.setCursor(8, 0);
  lcd.print(prit_tipke);

  if(digitalRead(T_PLU))
  {
    servo_max();
  }

  if(digitalRead(T_MIN))
  {
    servo_min();
  }

  if(digitalRead(T_OPT))
  {
    float pump_sekund = 2.;
    float pump_moc = 0.5;
    pozeni_pumpo(pump_moc, pump_sekund);
  }
  
  lcd.setCursor(0, 1);
  lcd.print(analog_beri_veckrat(HUM, 25));

  delay(250);
}
