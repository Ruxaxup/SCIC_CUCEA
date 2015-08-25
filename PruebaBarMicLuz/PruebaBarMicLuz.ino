#include <Wire.h>
#include <TSL2561.h> //Luminosidad
#include <Adafruit_MPL115A2.h> //Barometro
#include <Ethernet.h>
//#include <TimerOne.h>

//SERIAL USE FOR TESTING
#define DEBUG 1
//El valor hexadecimal es el tiempo de parpadeo
#define LUZ 		0x01	//0000	0001
#define PRESION 	0x02	//0000	0010
#define TEMPERATURA     0x04	//0000	0100
#define RUIDO 		0x08	//0000	1000
#define ETHERNET 	0x10	//0001  0000
#define SERVER	 	0x20	//0010  0000

#define MUESTREO        1000
byte errorFlag = 0x0;

Adafruit_MPL115A2 barometro; //Barometro
TSL2561 luminosidad(TSL2561_ADDR_FLOAT); //Luminosidad
const int sampleWindow = 50; //50 ms = 20 Hz
unsigned int sample;
int ledMic = 7;
int ledLuz = 12;
int ledBar = 13;
int ledError = 8;

/***** TARJETAS GALILEO ******/
#define G_1GTW  0
#define G_1F7R  1
#define G_1DN7  2
#define G_1F2Y  3
#define G_1ELA  4
#define G_1F4K  5
#define G_1FYD  6
#define G_1FCW  7
#define G_1H4N  8
#define G_1FWX  9
/***** Ethernet ******/
#define GALILEO G_1FWX
//#define thingName "Test-Galileo-1"
#define thingName "SC_Galileo_9"
#define MAC_SIZE 6
//MAC esta escrita en la etiqueta del puerto Ethernet
byte mac1GTW[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE5, 0x86 };    //0
byte mac1F7R[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE5, 0xFB };    //1
byte mac1DN7[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE1, 0xA6 };    //2
byte mac1F2Y[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE3, 0x00 };    //3
byte mac1ELA[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE3, 0xB8 };    //4
byte mac1F4K[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE4, 0x5E };    //5
byte mac1FYD[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE1, 0xDD };    //6
byte mac1FCW[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE3, 0xBA };    //7
byte mac1H4N[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE4, 0xC4 };    //8
byte mac1FWX[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE6, 0x42 };    //9

byte mac3M0B[] = { 0x98, 0x4F, 0xEE, 0x02, 0xD7, 0xFF }; // GALILLEO 2

byte actualMac[6];
EthernetClient client;
//char server[] = "www.dweet.io"; 

char server[] = "148.202.23.200";
char sensor_id[] = "G_1FWX";
float longitude;
float latitude;
float location;
char country[] = "Mexico";
char _name[]  = "Gustavo";
char email[] = "gustavo.jimenez.mx@ieee.org";
/********************/

boolean hasError(byte flag){
  return (errorFlag&flag) == flag;
}

void copyArray(byte* array_original, byte* arrayTo, int n){
  int i = 0;
  for(; i<n; i++){
    arrayTo[i] = array_original[i];
  }
}

boolean initializeEthernet(){
  switch(GALILEO){
    case 0:
      copyArray(actualMac,mac1GTW,MAC_SIZE);
    break;
    case 1:
      copyArray(actualMac,mac1F7R,MAC_SIZE);
    break;
    case 2:
      copyArray(actualMac,mac1DN7,MAC_SIZE);
    break;
    case 3:
      copyArray(actualMac,mac1F2Y,MAC_SIZE);
    break;
    case 4:
      copyArray(actualMac,mac1ELA,MAC_SIZE);
    break;
    case 5:
      copyArray(actualMac,mac1F4K,MAC_SIZE);
    break;
    case 6:
      copyArray(actualMac,mac1FYD,MAC_SIZE);
    break;
    case 7:
      copyArray(actualMac,mac1FCW,MAC_SIZE);
    break;
    case 9:
      copyArray(actualMac,mac1FWX,MAC_SIZE);
    break;
  }
  //habilitamos el puerto ethernet de galileo
  if(DEBUG == 1) Serial.print("Setting up ethernet port");
  digitalWrite(ledError,HIGH);
  system("ifup eth0");
  delay(4000); 
  digitalWrite(ledError,LOW);
  if(DEBUG == 1) Serial.print("done");
  
  //Start Ethernet connection
  if(Ethernet.begin(actualMac) == 0){
      if(DEBUG == 1) Serial.print("Failed to configure Ethernet using DHCP");
    return false;
  }
    if(DEBUG == 1) Serial.print("Connecting..."); 
  return true;
}

/** Recopila y manda las lecturas a Dweet **/
void sendDweet(float presion, float temperatura, double ruido,
               uint32_t lumens)
{
  //ruido = 20 * log10(ruido / 5);
  if(DEBUG == 1) Serial.println("Sending...");
  if(client.connect(server,80)){
    if(hasError(SERVER)){
      errorFlag = errorFlag ^ SERVER;
    }
    //Estructura de los datos
    client.print("GET /CICI/IoT/test_mongophp.php");
    //client.print("GET /dweet/for/");
    //client.print(thingName);
    client.print("?sensor_id=");
    client.print(sensor_id);
    //Data to send
    client.print("&pressure=");
    client.print(presion);
    client.print("&temperature=");
    client.print(temperatura);
    client.print("&noise=");
    client.print(ruido);
    client.print("&light=");
    client.print(lumens);
    //Board information
    /*client.print("&name=");
    client.print(_name);
    client.print("&email=");
    client.print(email);
    client.print("&country=");*/
    //Location
    /*client.print(country);
    client.print("&location=");
    client.print(location);
    client.print("&longitude=");
    client.print(longitude);
    client.print("&latitude=");    
    client.print(latitude);*/
    client.println(" HTTP/1.0");
    
    //Indicador de host y cierre de la conexion
    //client.println("Host: dweet.io");
    //client.println("Connection: close");
    client.println();
    client.flush();  
      
    //Leemos la respuesta
    if(client.available()){
      char c = client.read();
      Serial.print(c);
      client.flush();
    }
    client.stop();
      if(DEBUG == 1) Serial.println("Information sent");
    }else{
        errorFlag = errorFlag | SERVER;
        if(DEBUG == 1) Serial.println("Connection to server failed");
    }
}

float getPressure(){
  float presion = barometro.getPressure();
  return presion;
}

float getTemperature(){
  float temperature = barometro.getTemperature();
  return temperature;
}

void printToSerialTempPress(float temp, float pressure){
  //Mostramos datos leidos
  Serial.print("Presion (kPa): ");
  Serial.print(pressure, 4);
  Serial.println(" kPa");
  Serial.print("Temperatura (*C): ");
  Serial.print(temp, 1);
  Serial.println(" *C");
  Serial.println("/*************************/"); 
}

uint32_t getLuminosity(){
  uint32_t luminosidadCompleta = luminosidad.getFullLuminosity();
  luminosidadCompleta = luminosidadCompleta & 0xFFFF;
  return luminosidadCompleta;
}

void printToSerialLum(uint32_t lum){
  //Mostramos la luminosidad completa
  Serial.print("Full: ");
  Serial.print(lum);
  Serial.println(" lumenes");
  Serial.println("/*************************/");
}

float getNoise(){
  //unsigned int peekToPeek = 0;
  double peekToPeek = 0;
  unsigned int signalMax = 0, signalMin = 1024;
  unsigned long startMillis = millis();
  digitalWrite(ledMic, HIGH);
  
  //Se realiza un muestreo de seales y se obtiene un promedio
  while(millis() - startMillis < sampleWindow){
    sample = analogRead(0);
    if(sample < 1024){
      if(sample > signalMax){
        signalMax = sample;
      }else if(sample < signalMin){
        signalMin = sample;
      }
    }
  } 
  peekToPeek = (signalMax - signalMin) / 2;  
  if(sample == 1)
    return 0;
  //double ruido = (peekToPeek * 3.3) / 1024;
//  double ruido = 20 * log10(sample / 2);
  double ruido = 20 * log10(peekToPeek / 10);
  return ruido;
}

void printToSerialNoise(double noise){
  Serial.print("Ruido: ");
  Serial.print(noise);
  Serial.println(" dB");
  Serial.println("/--------------------/");
  Serial.println("");
}

//PARPADEA EL LED DE ERROR SEGUN SEA LA BANDERA
void blinkError(byte flag){
  int i;
  for(i = 0; i < flag; i++){
    digitalWrite(ledError, HIGH);
    delay(200);
    digitalWrite(ledError, LOW);
    delay(200);
  }
  if(DEBUG == 1){
    Serial.print("Blinked ");
    Serial.print(i);
    Serial.println(" times");
  }
}

//METODO PARA CHEQUEO DE ERRORES
void timerIsr(){
  digitalWrite(ledError, HIGH);
  delay(2000);
  digitalWrite(ledError, LOW);
  if( (errorFlag&LUZ) == LUZ){
    blinkError(LUZ);
  }
  
  digitalWrite(ledError, HIGH);
  delay(2000);
  digitalWrite(ledError, LOW);
  
  if( (errorFlag&PRESION) == PRESION){
    blinkError(PRESION);
  }
  
  digitalWrite(ledError, HIGH);
  delay(2000);
  digitalWrite(ledError, LOW);
  
  if( (errorFlag&TEMPERATURA) == TEMPERATURA){
    blinkError(TEMPERATURA);
  }
  
  digitalWrite(ledError, HIGH);
  delay(2000);
  digitalWrite(ledError, LOW);
  
  if( (errorFlag&RUIDO) == RUIDO){
    blinkError(RUIDO);
  }
  
  digitalWrite(ledError, HIGH);
  delay(2000);
  digitalWrite(ledError, LOW);  
  
  if( (errorFlag&ETHERNET) == ETHERNET){
    blinkError(ETHERNET);    
  }
  
  digitalWrite(ledError, HIGH);
  delay(2000);
  digitalWrite(ledError, LOW);
  
  if( (errorFlag&SERVER) == SERVER){
    blinkError(SERVER);
    //REINICIAR EL SISTEMA
    if(DEBUG ==1){
      Serial.println("Rebooting system...");
    }
    rebootLEDs();
    system("reboot");
  }
}

//REBOOT INDICATOR
void rebootLEDs(){
  digitalWrite(ledMic, HIGH);
  digitalWrite(ledLuz, HIGH);
  digitalWrite(ledBar, HIGH);
  digitalWrite(ledError, HIGH);
  delay(2000);
  digitalWrite(ledMic, LOW);
  digitalWrite(ledLuz, LOW);
  digitalWrite(ledBar, LOW);
  digitalWrite(ledError, LOW);
}
//METODO DE CONFIGURACIONES INICIALES
void setup() {  
  if(DEBUG == 1) Serial.begin(9600);  
  //Ethernet initialization
  if(!initializeEthernet()){
    //No hacemos nada
    errorFlag = errorFlag | ETHERNET;
    while(true){
        delay(5000);
        if(initializeEthernet()){
          errorFlag = errorFlag ^ ETHERNET;
          break;
        }
    }
  }  
  // put your setup code here, to run once:
  barometro.begin();
  pinMode(ledMic, OUTPUT);
  digitalWrite(ledMic,LOW);
  pinMode(ledLuz, OUTPUT);
  digitalWrite(ledLuz,LOW);
  pinMode(ledBar, OUTPUT);    
  digitalWrite(ledBar,LOW);
  pinMode(ledError, OUTPUT);
  digitalWrite(ledError, LOW);
  //Timer1.initialize(1000000);
  //Timer1.attachInterrupt(timerIsr);
}

void loop() {
  //Variables para presion y temperatura
  float presion = 0, temperatura = 0;
  //Variables para luminosidad
  uint32_t luminosidadCompleta = -1;
  //Variables microfono
  double ruido = 0;
  
  
  //Lectura de presion y temperatura
  digitalWrite(ledBar, HIGH);
  presion = getPressure();
  if(presion == 0){
    //error de lectura Presion
    errorFlag = errorFlag | PRESION;
  }else{
    if(hasError(PRESION)){
      errorFlag = errorFlag ^ PRESION;
    }
  }
  temperatura = getTemperature();
  if(temperatura == 0){
    //error de lectura temperatura
    errorFlag = errorFlag | TEMPERATURA;
  }else{
    if(hasError(TEMPERATURA)){
      errorFlag = errorFlag ^ TEMPERATURA;
    }
  }
  if(DEBUG == 1) printToSerialTempPress(temperatura, presion);   
  delay(1000);
  digitalWrite(ledBar, LOW);
  /******************************************************/
  //Lectura de luz
  digitalWrite(ledLuz,HIGH);
  luminosidadCompleta = getLuminosity();
  if(luminosidadCompleta == -1){
    //error de lectura Luz
    errorFlag = errorFlag | LUZ;
  }else{
    if(hasError(LUZ)){
      errorFlag = errorFlag ^ RUIDO;
    }
  }
  if(DEBUG == 1) printToSerialLum(luminosidadCompleta);
  delay(1000);
  digitalWrite(ledLuz,LOW);
  /******************************************************/
  //Lectura de ruido
  ruido = getNoise();
  if(ruido == 0){
    //error de lectura Ruido
    errorFlag = errorFlag | RUIDO;
  }else{
    if(hasError(RUIDO)){
      errorFlag = errorFlag ^ RUIDO;
    }
  }
  if(DEBUG == 1) printToSerialNoise(ruido);
  delay(1000);
  digitalWrite(ledMic, LOW);
  
  //Empujar los datos a Dweet
  sendDweet(presion, temperatura, ruido, luminosidadCompleta);
  delay(MUESTREO);
  timerIsr();
  delay(MUESTREO);
}
