#include <Wire.h>
#include <TSL2561.h> //Luminosidad
#include <Adafruit_MPL115A2.h> //Barometro
#include <SD.h>

//#include <TimerOne.h>

//SERIAL USE FOR TESTING
#define DEBUG 0
//El valor hexadecimal es el tiempo de parpadeo para errores
#define LUZ 		0x01	//0000	0001
#define PRESION 	0x02	//0000	0010
#define TEMPERATURA     0x04	//0000	0100
#define RUIDO 		0x08	//0000	1000
#define GAS	 		0x10
#define HUMEDAD 	0x20	//0001  0000
#define AGUA	 	0x40	//0010  0000
byte errorFlag = 0x0;
byte readingFlag = 0x0;
byte estatus_sensores = 0x0;

//FRECUENCIA DE MUESTREO
#define SAMPLING        1000 * 60 // 30 segundos antes y despues de chequeo de errores
#define TIME_TEMP 	10              //minutos
#define TIME_PRESS 	10             //minutos
#define TIME_LIGHT 	1              //minutos
#define TIME_HUM 	1              //minutos
#define TIME_GAS	1              //minutos
#define TIME_AGUA 	1              //minutos
#define TIME_NOISE 	1              //minutos

//LEDS
#define LED_ERROR 13
#define LED_WARNING 12
#define LED_OK 11

//SENSOR LEYENDO
#define LED_SENS_1 10
#define LED_SENS_2 9
#define LED_SENS_3 8

//CALIBRACION
#define CAL_TEMP 5
#define CAL_NOISE 0

//SENSORES
Adafruit_MPL115A2 barometro; //Barometro
TSL2561 luminosidad(TSL2561_ADDR_FLOAT); //Luminosidad
const int sampleWindow = 50; //50 ms = 20 Hz
unsigned int sample;
int ledMic = 7;
int ledLuz = 12;
int ledBar = 13;
int ledError = 8;


//Variables para presion y temperatura
float presion = 0, temperatura = 0;
//Variables para luminosidad
uint32_t luminosidadCompleta = -1;
//Variables microfono
double ruido = 0;  


//ESTADOS
enum states{
  start,
  idle,
  readSensors,
  sendMetadata,
  sendDataMQTT,
  error
};

states state = start;

////////////////   FUNCIONES PRINCIPALES  /////////////////////
boolean hasError(byte flag){
  return (errorFlag&flag) == flag;
}

void copyArray(byte* array_original, byte* arrayTo, int n){
  int i = 0;
  for(; i<n; i++){
    arrayTo[i] = array_original[i];
  }
}

float getPressure(){
  float presion = barometro.getPressure();
  return presion;
}

float getTemperature(){
  float temperature = barometro.getTemperature();
  temperature -= CAL_TEMP;
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
    
  double ruido = 20 * log10(peekToPeek / 10);
  ruido += CAL_NOISE;
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

//PARPADEA EL LED DE ERROR SEGUN SEA LA BANDERA
void blinkRedLED(int cantidad){
  int i;
  for(i = 0; i < cantidad; i++){
    digitalWrite(ledError, HIGH);
    delay(50);
    digitalWrite(ledError, LOW);
    delay(50);
  }
  if(DEBUG == 1){
    Serial.print("Blinked ");
    Serial.print(i);
    Serial.println(" times");
  }
}

//METODO PARA CHEQUEO DE ERRORES
void timerIsr(){

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


//////////////////////////
///////// MQTT //////////

void _sendMetaData(){
  system("python /home/root/PythonMQTT/sendMetadata.py");
}

void _sendDataMQTT(String tipoLectura){
  char data[25];
  String path = String("python /home/root/PythonMQTT/sendDataMQTT.py " + tipoLectura);
  galileoCreateFile("values.txt");
  if(DEBUG == 1) Serial.println(tipoLectura);
  if(tipoLectura.equals("temp")){
    addValueToFile("values.txt",temperatura);
  }else if(tipoLectura.equals("press")){
    addValueToFile("values.txt",presion);
  }else if(tipoLectura.equals("light")){
    addValueToFile("values.txt",luminosidadCompleta);
  }else if(tipoLectura.equals("noise")){
    addValueToFile("values.txt",ruido);
  }else if(tipoLectura.equals("gas")){
    addValueToFile("values.txt",ruido);
  }else if(tipoLectura.equals("water")){
    addValueToFile("values.txt",ruido);
  }else if(tipoLectura.equals("hum")){
    addValueToFile("values.txt",ruido);
  }
  
  char command [path.length() + 1  ];
  path.toCharArray(command, sizeof(command)); 
  if(DEBUG == 1) Serial.println(command);
  blinkRedLED(20);
  system(command);
  /*temperatura = 0;
  presion = 0;
  luminosidadCompleta = 0;
  ruido = 0;*/
}
/////////////////////////
/////////////////////////

void _readSensors(){
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

}


/******************************** SD **********************************/
//SD.open retrieves the file in append more. 
void addValueToFile(String fileName, float content) {
  char charFileName[fileName.length() + 1];
  fileName.toCharArray(charFileName, sizeof(charFileName));

  if (SD.exists(charFileName)) {
    File targetFile = SD.open(charFileName, FILE_WRITE);
    targetFile.print(content);
    targetFile.close();

    /*File readFile = SD.open(charFileName);
    while (readFile.available()) {
      status_message += (char)readFile.read();
    }
    status_message += "\nDone printing.";
    readFile.close();*/
  }
}

void addValueToFile(String fileName, double content) {
  char charFileName[fileName.length() + 1];
  fileName.toCharArray(charFileName, sizeof(charFileName));

  if (SD.exists(charFileName)) {
    File targetFile = SD.open(charFileName, FILE_WRITE);
    targetFile.print(content);
    targetFile.close();

    /*File readFile = SD.open(charFileName);
    while (readFile.available()) {
      status_message += (char)readFile.read();
    }
    status_message += "\nDone printing.";
    readFile.close();*/
  }
}

void addValueToFile(String fileName, uint32_t content) {
  char charFileName[fileName.length() + 1];
  fileName.toCharArray(charFileName, sizeof(charFileName));

  if (SD.exists(charFileName)) {
    File targetFile = SD.open(charFileName, FILE_WRITE);
    targetFile.print(content);
    targetFile.close();

    /*File readFile = SD.open(charFileName);
    while (readFile.available()) {
      status_message += (char)readFile.read();
    }
    status_message += "\nDone printing.";
    readFile.close();*/
  }
}

void galileoCreateFile(String fileName) {
  String status_message = String();
  status_message = fileName;
  char charFileName[fileName.length() + 1];
  fileName.toCharArray(charFileName, sizeof(charFileName));

  if (SD.exists(charFileName)) { 
    status_message += " exists already.";
  }
  else {
    char system_message[256];
    char directory[] = "/media/realroot";
    sprintf(system_message, "touch %s/%s", directory, charFileName);
    system(system_message);
    if (SD.exists(charFileName)) {
      status_message += " created.";
    } 
    else {
      status_message += " creation tried and failed.";
    }
  }
}
/**********************************************************************/


//METODO DE CONFIGURACIONES INICIALES
void setup() {  
  if(DEBUG == 1) Serial.begin(9600);  
  
  // put your setup code here, to run once:
  barometro.begin();
  pinMode(LED_ERROR, OUTPUT);
  digitalWrite(LED_ERROR,LOW);
  pinMode(LED_WARNING, OUTPUT);
  digitalWrite(LED_WARNING,LOW);
  pinMode(LED_OK, OUTPUT);    
  digitalWrite(LED_OK,LOW);
  pinMode(LED_SENS_1, OUTPUT);
  digitalWrite(LED_SENS_1, LOW);
  pinMode(LED_SENS_2, OUTPUT);
  digitalWrite(LED_SENS_2, LOW);
  pinMode(LED_SENS_3, OUTPUT);
  digitalWrite(LED_SENS_3, LOW);
}

int cont_temp = 0;
int cont_press = 0;
int cont_light = 0;
int cont_noise = 0;
int cont_gas = 0;
int cont_hum = 0;
int cont_water = 0;

void loop() {
  String tipoLectura = String();
  
  switch(state){
    case start:
    if(DEBUG == 1) Serial.println("state:Start");
    //Verificar conexiones
      rebootLEDs();
      state = sendMetadata;
    break;
    case idle:      
      //timerIsr();
      if(DEBUG == 1) Serial.println("state:Duermo 1 min"); 
      delay(MUESTREO); //Duermete y cuando despiertes cuenta
      if( (estatus_sensores & LUZ) == LUZ ){
      	cont_light++;	
      }
      if( (estatus_sensores & TEMPERATURA) == TEMPERATURA ){
      	cont_temp++;	
      }
      if( (estatus_sensores & PRESION) == PRESION ){
      	cont_press++;	
      }
      if( (estatus_sensores & RUIDO) == RUIDO ){
      	cont_noise++;	
      }
      if( (estatus_sensores & GAS) == GAS){
      	cont_gas++;	
      }
      if( (estatus_sensores & HUMEDAD) == HUMEDAD ){
      	cont_hum++;	
      }
      if( (estatus_sensores & AGUA) == AGUA ){
      	cont_water++;	
      }

      if(DEBUG == 1){
        Serial.print("cont_temp = ");
        Serial.println(cont_temp); 
        Serial.print("cont_press = ");
        Serial.println(cont_press);
        Serial.print("cont_light = ");
        Serial.println(cont_light);
        Serial.print("cont_noise = ");
        Serial.println(cont_noise);
        Serial.print("cont_gas = ");
        Serial.println(cont_gas);
        Serial.print("cont_hum = ");
        Serial.println(cont_hum);
        Serial.print("cont_water = ");
        Serial.println(cont_water);
      }
      if(DEBUG == 1) Serial.println("state:Idle");      
      if(cont_temp >= TIME_TEMP){
        readingFlag = readingFlag | TEMPERATURA;        
      }
      if(cont_press >= TIME_PRESS){
        readingFlag = readingFlag | PRESION;
      }
      if(cont_light >= TIME_LIGHT){
        readingFlag = readingFlag | LUZ;
      }
      if(cont_noise >= TIME_NOISE){
        readingFlag = readingFlag | RUIDO;
      }
      if(cont_noise >= TIME_GAS){
        readingFlag = readingFlag | GAS;
      }
      if(cont_noise >= TIME_HUM){
        readingFlag = readingFlag | HUMEDAD;
      }
      if(cont_noise >= TIME_AGUA){
        readingFlag = readingFlag | AGUA;
      }
      if(readingFlag != 0x00){
        state = readSensors;
      }      
      
    break;
    case sendMetadata:
      if(DEBUG == 1) Serial.println("state:sendMetadata");
      //_sendMetaData();
      state = idle;
    break;
    case sendDataMQTT:
      if(DEBUG == 1) Serial.println("state:sendDataMQTT");
      if( (readingFlag & TEMPERATURA) == TEMPERATURA){
        _sendDataMQTT("temp");
        readingFlag = readingFlag ^ TEMPERATURA;
        cont_temp = 0;
      }
      if( (readingFlag & PRESION) == PRESION){
        _sendDataMQTT("press");
        readingFlag = readingFlag ^ PRESION;
        cont_press = 0;
      }
      if( (readingFlag & LUZ) == LUZ){
        _sendDataMQTT("light");
        readingFlag = readingFlag ^ LUZ;
        cont_light = 0;
      }
      if( (readingFlag & RUIDO) == RUIDO){
        _sendDataMQTT("noise");
        readingFlag = readingFlag ^ RUIDO;
        cont_noise = 0;
      }
      state = idle;
    break;
    case readSensors:
      if(DEBUG == 1) Serial.println("state:readSensors");
      _readSensors();
      state = sendDataMQTT;
    break;
    case error:
      if(DEBUG == 1) Serial.println("state:Error");
    break;
    default:
      if(DEBUG == 1) Serial.println("state:Default");
    break;  
  }
  
  
}
