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
int hum_voda = 300; // odcitek v vodi
int hum_prst = 980; // odcitek v suhi prsti
int hum_zrak = 1023; // odcitev na zraku

// porti za tipke
#define T_OPT 10
#define T_MIN 9
#define T_PLU 1

// nastavitev velikosti (16x2) in naslov zaslona (dolocen preko A0,A1,A2 padov na zadnji strani)
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

// definicija ostalih globalnih spremenljivk
byte zaslon = 0;
byte servo_poz = 0;
unsigned long zadnji_pritisk = 0;
unsigned long nazadnje_zalito = 0;
float temp_c, hum_perc;
char str_vred[3];
char str_vred_float[5];

// definicije spremenljivk delovanja, ki jih lahko spreminjamo preko tipk in zaslona
byte temp_lim = 33;
byte hum_lim = 50;
byte pumpa_moc = 70;  // v % - duty cycle
int pumpa_cas = 5;  // v sekundah
int pumpa_presledek = 30;  // v minutah


// funkcija setup, se pozene le enkrat ob vklopu mikrokrmilnika
void setup() 
{
  // inicializacija LCD-ja
  lcd.init();
  lcd.backlight();
  lcd.noCursor();

  // nastavitev delovanja (vhod/izhod) portov
  pinMode(TMP, INPUT);
  pinMode(HUM, INPUT);
  pinMode(SERV, OUTPUT);
  pinMode(T_OPT, OUTPUT);
  pinMode(T_PLU, OUTPUT);
  pinMode(T_MIN, OUTPUT);
  pinMode(PUMP, OUTPUT);

  // servo na zacetno vrednost
  premakni_servo();
}


// funkcija za veckratno zaporednje branje analognega senzorja
// vrne povprecje vecih meritev
float analog_beri_veckrat(int pin, int ponovitev)
{
  float analog_vrednost = 0;
  
  for (int i = 0; i < ponovitev; i++)
  {
    analog_vrednost += analogRead(pin);
  }
  
  return analog_vrednost / ponovitev;
}


//
void servo_pojdi(byte pozicija)
{
  servo.attach(SERV);
  delay(100);
  // TODO: pocasnejse premikanje servota ce bo potrebno
  servo.write(pozicija);
  delay(5000);
  servo.detach();
  delay(100);
}


//
void premakni_servo()
{
  if (servo_poz)
  {
    servo_pojdi(180);
  }
  else
  {
    servo_pojdi(0);
  }
}


// vklopi vodno pumpico za izbran čas z izbrano mocjo (PWM signal)
void pozeni_pumpo(byte moc, int cas)
{
  analogWrite(PUMP, 255 * (moc / 100.));
  delay(cas * 1000);
  digitalWrite(PUMP, LOW);
}


// funkcija za pretvarjanje vrednosti analognega odcitka v stopinje C za temperaturni senzor
float analog_v_c(float vrednost)
{
  // nicne kalibrirane vrednosti pobrane iz tehnicne dokumentacije
  int T_0 = 25;  // v enotah C
  int i_0 = 750;  // v enotah mV
  int k_iT = 10;  // naklon premice v enotah mV/C, veljaven v razponu od -40 do 125 stopinj C

  float napetost = vrednost / 1023 * 5;  // vrednost ADC-ja v napetost 
  float temp = (napetost - i_0/1000.) / (k_iT/1000.) + T_0;
  return temp;
}


// funkcija za pretvarjanje vrednosti analognega odcitka vlaznosti v kvazi vlaznost
float analog_v_perc(float vrednost)
{
  if (vrednost > hum_prst)
  {
    // senzor sploh ni postavljen v prst
    return 0;
  }
  else
  {
    // senzor naj bi bil v prsti
    float humid = (hum_prst - vrednost) / (hum_prst - hum_voda) * 100;
    return humid;
  }
}


// ob kliku na nastavitev popravi trenutno vrednost spremenljivke
float popravi_vrednost(int vrednost, int plus, int minus, int korak, int min_vred, int max_vred)
{
  if (plus)
  {
    vrednost += korak;
    if (vrednost > max_vred)
    {
      vrednost = max_vred;
    }
  }
  
  if (minus)
  {
    vrednost -= korak;
    if (vrednost < min_vred)
    {
      vrednost = min_vred;
    }
  }
  
  return vrednost;
}


// funkcija loop tece ves cas in se neprestano izvaja/ponavlja
void loop()
{
  // preveri ce je bila katera od tipk pritisnjena
  int opt = digitalRead(T_OPT);
  int plus = digitalRead(T_PLU);
  int minu = digitalRead(T_MIN);

  // premakni zaslon ob pritisku na tipko O
  if (opt)
  {
    zaslon++;
    if(zaslon > 5)
    {
      zaslon = 1;
    }
  }

  // ce je bila katera od tipk pritisnjena, resetiraj stevec za prikaz nastvativenega zaslona
  if (opt || plus || minu)
  {
    zadnji_pritisk = millis();
    delay(250);
  }
  else
  {
    // po 15 sekundah neaktivnost premakni na zacetni zaslon ter delovanje
    if ((millis() - zadnji_pritisk) > 15000 && zaslon != 0)
    {
      zaslon = 0;
      lcd.clear();
    }
  }

  // if stavki za izbiranje zaslona, ker switch ni deloval pravilno - nimam pojma zakaj in kako, le 0 je delal, no ni kaj

  lcd.home();
  if (zaslon == 0)
  {
    float temp_analog = analog_beri_veckrat(TMP, 150);
    temp_c = analog_v_c(temp_analog);
    float hum_analog = analog_beri_veckrat(HUM, 25);
    hum_perc = analog_v_perc(hum_analog);

    if (hum_perc <= 0)
    {
      lcd.print("Ni v prsti  ");
    }
    else
    {
      lcd.print("Vlaz:");
      lcd.setCursor(5, 0);
      sprintf(str_vred_float, "%3d.%1d", int(hum_perc), int(hum_perc*10)%10);
      lcd.print(str_vred_float);
      lcd.setCursor(11, 0);
      lcd.print("%");
    }
    
    lcd.setCursor(0, 1);
    lcd.print("Temp:");
    lcd.setCursor(5, 1);
    sprintf(str_vred_float, "%3d.%1d", int(temp_c), int(temp_c*10)%10);
    lcd.print(str_vred_float);
    lcd.setCursor(11, 1);
    lcd.print("C");
    
  }
  
  if (zaslon == 1)
  {
    lcd.print("Vklop pumpe ob");
    lcd.setCursor(0, 1);
    lcd.print("vlaznosti ");
    lcd.setCursor(10, 1);
    sprintf(str_vred, "%3d", hum_lim);
    lcd.print(str_vred);
    lcd.setCursor(13, 1);
    lcd.print(" %");
  }

  if (zaslon == 2)
  {
    lcd.print("Vklopi pumpo za");
    lcd.setCursor(0, 1);
    sprintf(str_vred, "%3d", pumpa_cas);
    lcd.print(str_vred);
    lcd.setCursor(3, 1);
    lcd.print(" sekund     ");
  }

  if (zaslon == 3)
  {
    lcd.print("Moc pumpe je   ");
    lcd.setCursor(0, 1);
    sprintf(str_vred, "%3d", pumpa_moc);
    lcd.print(str_vred);
    lcd.setCursor(3, 1);
    lcd.print(" %     ");
  }

  if (zaslon == 4)
  {
    lcd.print("Med zalivanji");
    lcd.setCursor(0, 1);
    lcd.print("vsaj ");
    lcd.setCursor(5, 1);
    sprintf(str_vred, "%3d", pumpa_presledek);
    lcd.print(str_vred);
    lcd.setCursor(8, 1);
    lcd.print(" minut");
  }

  if (zaslon == 5)
  {
    lcd.print("Najvecja temp");
    lcd.setCursor(0, 1);
    sprintf(str_vred, "%3d", temp_lim);
    lcd.print(str_vred);
    lcd.setCursor(3, 1);
    lcd.print(" C         ");
  }

  // izbira med delovanjem ter spreminjanjem nastavitev
  if (zaslon == 0)
  {
    // naprava deluje le ce je prikazan zacetni informativni zaslon

    // logika za priziganje servo motorja
    if ((temp_c > temp_lim) && servo_poz == 0)
    {
      servo_poz = 1;
      premakni_servo();
    }
    if ((temp_c <= temp_lim) && servo_poz == 1)
    {
      servo_poz = 0;
      premakni_servo();
    }

    // logika za delovanje vodne pumpice
    if((hum_perc < hum_lim) && (millis() - nazadnje_zalito > pumpa_presledek*60*1000))
    {
      pozeni_pumpo(pumpa_moc, pumpa_cas);
      nazadnje_zalito = millis();
    }
  }
  else
  {
    // korigiranje nastavitve delovanja prikazane na izbranem zaslonu, ki ni zacetni
    // izvedi korekcijo le ce je bila pritisnjena katera od tipk za spremembo
    if (plus || minu)
    {
      switch (zaslon)
      {
        case 1:
          hum_lim = popravi_vrednost(hum_lim, plus, minu, 5, 0, 100);
          break;

        case 2:
          pumpa_cas = popravi_vrednost(pumpa_cas, plus, minu, 10, 0, 999);
          break;

        case 3:
          pumpa_moc = popravi_vrednost(pumpa_moc, plus, minu, 10, 0, 100);
          break;

       case 4:
          pumpa_presledek = popravi_vrednost(pumpa_presledek, plus, minu, 10, 1, 999);
          break;

       case 5:
          temp_lim = popravi_vrednost(temp_lim, plus, minu, 1, 0, 100);
          break;
      
        default:
          break;  
      }
    }
  }

  delay(100);
}
