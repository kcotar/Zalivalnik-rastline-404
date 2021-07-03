#include <WiFi.h>
#include <DHT.h>
#include <math.h>
#include <ThingSpeak.h>

// nastavitve za wifi povezavo
const char* ssid = "Hogwarts";
const char* password = "RavenClaw";
WiFiClient  client;

// nastavitve za posiljanje podatkov na sistem ThingSpeak
unsigned long myChannelNumber = 1396646;
const char * myWriteAPIKey = "VJCMJ5222VC3WVSW";

// nastavitve za dth
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// svetlost zunaj
#define pulse_ip 27

void setup() {
  // put your setup code here, to run once:
  // vzpostavimo serijsko povezavo
  Serial.begin(115200);

  dht.begin();

  // vzpostavi wifi povezavo s prej doloceno dostopno tocko
  WiFi.begin(ssid, password);
  // povezava lahko traja nekaj casa, zato vmes izpisujemo pikice
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Izpisemo pridobljen IP naslov
  Serial.print("Use this URL to connect: ");
  Serial.println(WiFi.localIP());

  // nastavimo povezavo do sistema ThingSpeak
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  
  pinMode(32, INPUT);
  pinMode(34, INPUT);
  pinMode(35, INPUT);
  pinMode(pulse_ip, INPUT);
}

float get_sensor_reading(int pin, int n_reads)
{
  float sensor_reading = 0;
  for(int i_r=0; i_r < n_reads; i_r++)
  {
    sensor_reading += analogRead(pin);
    delay(10);
  }
  return sensor_reading / n_reads;
}

float get_pulse_length(int pin, int n_reads)
{
  float offtime = n_reads;
  for(int io=0; io<20; io++)
  {
    offtime += pulseIn(pin, LOW);
  }
  return log10(offtime / n_reads);
}

void loop() {
  // put your main code here, to run repeatedly:
  int pins[] = {32, 34, 35};
  float vrednosti[3];

  // sprehodimo se cez pridobljen string in poisceno lokacije vejic, ki nam medsebojno delijo poslane vrednosti
  for(int kanal=1; kanal<4; kanal++)
  {
    vrednosti[kanal-1] = get_sensor_reading(pins[kanal-1], 25);

    // doloci vrednost, ki bo zapisana na izbrano polje
    ThingSpeak.setField(kanal, vrednosti[kanal-1]);
  }

  ThingSpeak.setField(4, dht.readTemperature());
  ThingSpeak.setField(5, dht.readHumidity());
  ThingSpeak.setField(6, get_pulse_length(pulse_ip, 25));

  // zapisi vse vrednosti polj v nas ThingSpeak kanal
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) 
  {
    // izpis če je zapis vrednosti uspesen
    Serial.println("Channel update successful.");
  }
  else 
  {
    // izpis če zapis vrednosti ni bil uspesen
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

  delay(15000);

}
