/*
 * ControlRejillas_08.06.2019.6.ino - 
 * Copyright 2020 FranBoza
 *   
 *     8-4-2020   const String Version = "ControlRejillas_08.06.2019.7 UDP "; Se incluye control dell LED Virtual V8 para indicar si esta viva la placa o comunicacion Heart
 *     
 */

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <BlynkSimpleEsp8266.h>
#include <SPI.h>
#include <DHT.h>
#include <EEPROM.h>
#include <Streaming.h> // https://github.com/kachok/arduino-libraries/blob/master/Streaming/Streaming.h
#include "avdweb_Switch.h"
#include <SimpleTimer.h> // Timers 
#include <stdlib.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

ESP8266WiFiMulti wifiMulti;      // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
//NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 60000);

const char* ssid = "Unifi";
const char* password = "tel3dixi029574unifi";

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#define DHTPIN 2 //
#define DHTTYPE DHT22   // DHT 22 Change this if you have a DHT11
DHT dht(DHTPIN, DHTTYPE);

SimpleTimer TimerDHT;
SimpleTimer Temporizador1;
SimpleTimer TreintaSeg;
SimpleTimer Temporizador3;

//int Repeticion;

const byte toggleSwitchpin = 14; //3      en ardui 3   wemos  14
const byte buttonGNDpin = 12; //4         en ardu 4    wemos 1242356a7a30254d3792b0956ee17d71b9
//const byte buttonGNDpin2 = 16; //                      wemos 16
const byte ReleRejilla = 5; //D1 GPIO5
const byte Control_3v = 13; // GPIo13  Para alimentar el sensor y poder resetearlo si se bloquea el maldito DTH22

int CuentaHeart = 1;

//const byte ButtonVCCpin = 6;
//const byte Button10mspin = 8;
boolean FlagReles = false ; // Si se activa rele este flag es true
int ContadorErroresDHT ;// cuanta las veces que entra en la rutina timer de 30 segundos
int ContadorErroresDHTotal ;
int ContadorFlagActivo ;// cuanta las veces que entra en la rutina timer de 30 segundos con el FlagReles ACTIVO

String minutosString;
int minutos;
String horasString;
int horas;
String segundosString;
String segundos;
String reloj;
String FechaString;

int TFijada;
bool Modo; //0 verano 1 invierno
bool EstadoRejilla;
bool ManualAutomatico;
int address; //direccion memoria EEPROM
String MensajeInicio = "";
float TemperaturaReal;
float HumedadReal;
int Contador;

const String Version = "ControlRejillas_08.06.2019.7 UDP";
//char auth[] = "4-M8CI1jYpE3QQZv9tTU67E-bp_hUqno";  int Habitacion = 1;  IPAddress Ip(192,168,1,81);//  SALON
char auth[] = "NC2RQt53Zpg395PK-3jMi1PRMFQm3MMl"; int Habitacion = 2;  IPAddress Ip(192, 168, 1, 82); //  IRIS
//char auth[] = "Kp2asQl6LHTCFg6fIQ8OqRSGrq7hIBZf";  int Habitacion = 3;  IPAddress Ip(192, 168, 1, 83); // PRINCIPAL
//char auth[] = "C9mbucXz92IJtFQL_Ywxb6wixp_IR0XR";   int Habitacion = 4; IPAddress Ip(192,168,1,84); // DESPACHO
//char auth[] = "9C-_31Qw-t-YA1xFFKhMQukTuot2kPlr";   int Habitacion = 5; IPAddress Ip(192,168,1,85); // TERRAZA
//char auth[] = "MBct4N3QgdHwIOV4xYj_bxW1nc4PDhyl";   int Habitacion = 6; IPAddress Ip(192,168,1,86); // COCINA Y PASILLO



Switch buttonGND = Switch(buttonGNDpin); // button to GND, use internal 20K pullup resistor
//Switch buttonGND2 = Switch(buttonGNDpin2); // button to GND, use internal 20K pullup resistor
Switch toggleSwitch = Switch(toggleSwitchpin);
//Switch buttonVCC = Switch(ButtonVCCpin, INPUT, HIGH); // button to VCC, 10k pull-down resistor, no internal pull-up resistor, HIGH polarity
//Switch button10ms = Switch(Button10mspin, INPUT_PULLUP, LOW, 1); // debounceTime 1ms

WidgetLED Heart(V8);  // para indicar que la placa esta viva
WidgetLED LedReles(V14);
WidgetLED LedRejilla(V3);
WidgetBridge bridge1(V21);
WidgetBridge bridge2(V22);
WidgetBridge bridge3(V23);
WidgetBridge bridge4(V24);
WidgetTerminal TerminalRemoto(V30);

unsigned int PuertoRemoto = 8889;
//IPAddress remote(192,168,1,15); // Lenovo Wifi
IPAddress remote(192, 168, 1, 100); // Pc Salon por cable

// UDP variables
unsigned int localPort = 8888;
WiFiUDP UDP;
boolean udpConnected = false;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char ReplyBuffer[10]; // = "Devolucion paquete "; // a string to send back
String  CadenaAEnviar;
char CadenaEnChar[7];

BLYNK_WRITE(V1) // Selector de temperatura
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  // You can also use:
  // String i = param.asStr();
  // double d = param.asDouble();
  Serial.print("V1 value is: ");
  Serial.println(pinValue);
  TFijada = pinValue;
  EEPROM.write(0, pinValue);
  //   delay(200);
  EEPROM.commit();
  Blynk.virtualWrite(V1, TFijada);
  RutinaTreintaSeg();
}

BLYNK_WRITE(V2) //Boton Modo Invierno/Verano
{
  int ModoValue = param.asInt(); // assigning incoming value from pin V2 to a variable
  // You can also use:
  // String i = param.asStr();
  // double d = param.asDouble();
  Serial.print("V2 modo is: ");
  Serial.println(ModoValue);
  Modo = ModoValue;
  EEPROM.write(1, ModoValue);
  ////    delay(200);
  EEPROM.commit();
  RutinaTreintaSeg();
}

BLYNK_WRITE(V4) //Boton Auto/Manual
{
  int MAuto = param.asInt(); // assigning incoming value from pin V4 to a variable
  Serial.print("V4 modo is: ");
  Serial.println(MAuto);
  ManualAutomatico = MAuto;
  EEPROM.write(3, MAuto);
  delay(200);
  EEPROM.commit();
  RutinaTreintaSeg();
}

BLYNK_WRITE(V5) //Boton Rejilla
{
  int EstadoRejilla = param.asInt(); // assigning incoming value from pin V1 to a variable
  if (ManualAutomatico == 0) {

    if (EstadoRejilla == 1)
    { digitalWrite(ReleRejilla, LOW);
      Blynk.setProperty(V3, "color", "#0c990c");// Setting color VERDE
      LedRejilla.on();
      Serial << "LED encendido" << " " << EstadoRejilla << " " << endl;
      Blynk.virtualWrite(V6, ("Abriendo Manual Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
    }
    else
    { digitalWrite(ReleRejilla, HIGH);
      Blynk.setProperty(V3, "color", "#ff0000");// Setting color ROJO
      LedRejilla.on();
      Serial << "LED apagado" << " " << EstadoRejilla << " " << endl;
      Blynk.virtualWrite(V6, ("Cerrando Manual Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
    }


    //int ModoValue = param.asInt(); //
    // You can also use:
    // String i = param.asStr();
    // double d = param.asDouble();
    Serial.print("V5: ");
    //Serial.println(ModoValue);
    //Modo= ModoValue;
    EEPROM.write(2, EstadoRejilla);
    ////    delay(200);
    EEPROM.commit();
    Serial.println(EstadoRejilla);
  }
  else {
    Blynk.virtualWrite(V6, ("Rejilla " + MensajeInicio + " esta en Automatico " + reloj ));
    if (EstadoRejilla == 0) {
      Blynk.virtualWrite (V5, 1);
    }
    if (EstadoRejilla == 1) {
      Blynk.virtualWrite (V5, 0);
    }
  }
}

BLYNK_WRITE(V7) // TODOS ON
{
  if (Habitacion == 1) { // Solo funcionara desde SALON
    Serial.print("V7 ABRIR TODAS DESDE SALON");
    bridge2.virtualWrite(V4, LOW); // PONE EN MANUAL
    bridge2.virtualWrite(V5, HIGH); // ABRE..
    bridge3.virtualWrite(V4, LOW);
    bridge3.virtualWrite(V5, HIGH);
    bridge4.virtualWrite(V4, LOW);
    bridge4.virtualWrite(V5, HIGH);

    Blynk.virtualWrite(V4, LOW);
    ManualAutomatico = 0;
    EEPROM.write(3, 0);
    delay(200);
    EEPROM.commit();
    AbrirRejilla();
    RutinaTreintaSeg();
  }
}

//BLYNK_WRITE(V20) // A dormir
//{
//
//  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
//  if (ON == 1) {
//
//    MandarAPC("#*FA5#"); // Macro a dormir
//  }
//}




BLYNK_WRITE(V30) {  //TerminalRemoto
  int Integer;
  String Cadena = param.asStr();
  // if (String("TMP") == param.asStr()){

  // TerminalRemoto.println("Indica Temperatura Fijada: " + Cadena);
  if ((Contador == 0) && (Cadena.startsWith("TMP", 0))) {
    TerminalRemoto.println("Introduce Valor para TMP Fijada " + Cadena + " " + Contador);
    Contador ++ ;
  } else {
    // Send it back
    //    TerminalRemoto.print("Tu comando: ");
    //    TerminalRemoto.write(param.getBuffer(), param.getLength());
    //   TerminalRemoto.println();
    // }

    // Ensure everything is sent


    Integer = (Cadena.toInt()); // Si no es un numero va a dar como conversion un 0
    TerminalRemoto.println("Cadena String " + Cadena + " - Pasada a INT : " + Integer);
  }
  //   TerminalRemoto.println(Cadena.toInt());}
  TerminalRemoto.flush();
}


BLYNK_CONNECTED() {
  bridge1.setAuthToken("8d3ebab2a31a4cdc86d9a9efb31d66dd"); // Token of the hardware SALON
  bridge2.setAuthToken("d8a80e1e360c42ee9a03e9f0a7e15158"); // Token of the hardware IRIS
  bridge3.setAuthToken("26f70398d1cd481591303b9d2e14ec38"); // Token of the hardware PRINCIPAL
  bridge4.setAuthToken("9b9866c193b04836840bd7492d6fdcc6"); // Token of the hardware DESPACHO

}




BLYNK_WRITE(V31) // Luz 1 ON
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPC("#*KC3#"); // Salon 1
        break;
      case 2:
        MandarAPC("#*KC6#"); // Iris Techo
        break;
      case 3:
        MandarAPC("#*KC8#"); //  Principal
        break;
      case 4:
        MandarAPC("#*KC12#"); //  Despacho
        break;
      case 5:
        MandarAPC("#*KC2#"); //  Terraza
        break;
      case 6:
        MandarAPC("#*KC14#"); //  Cocina
        MandarAPC("#*KC15#"); // Lavadero
        break;
    }
  }
}
BLYNK_WRITE(V32) // Luz 1 OFF
{
  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPC("#*FC3#"); // Salon 1
        break;
      case 2:
        MandarAPC("#*FC6#"); // Iris Techo
        break;
      case 3:
        MandarAPC("#*FC8#"); // Principal
        break;
      case 4:
        MandarAPC("#*FC12#"); // Despacho
        break;
      case 5:
        MandarAPC("#*FC2#"); // Terraza
        break;
      case 6:
        MandarAPC("#*FC14#"); // Cocina
        MandarAPC("#*FC15#"); // Lavadero
        break;
    }
  }
}
BLYNK_WRITE(V33) // Luz 2 ON +25%
{
  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPCPersianas("#*BC5#"); // Salon Pie
        break;
      case 2:
        MandarAPCPersianas("#*BC7#"); // Iris Mesita
        break;
      case 3:
        MandarAPCPersianas("#*BC9#"); // Principal Mesita
        break;
      case 4:
        //MandarAPC("#*FC12#"); // Despacho
        break;
      case 5:
        //MandarAPC("#*FC2#"); // Terraza
        break;
      case 6:
        // MandarAPC("#*FC14#"); // Cocina
        //MandarAPC("#*FC15#"); // Lavadero
        break;
    }
  }
}
BLYNK_WRITE(V34) // Luz 2 OFF
{
  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPC("#*FC5#"); // Salon Pie
        break;
      case 2:
        MandarAPC("#*FC7#"); // Iris Mesita
        break;
      case 3:
        MandarAPC("#*FC9#"); // Principal Mesita
        break;
      case 4:
        //MandarAPC("#*FC12#"); // Despacho
        break;
      case 5:
        //MandarAPC("#*FC2#"); // Terraza
        break;
      case 6:
        // MandarAPC("#*FC14#"); // Cocina
        //MandarAPC("#*FC15#"); // Lavadero
        break;
    }
  }
}
BLYNK_WRITE(V39) // Luz 2 OFF  AL 25 %
{
  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPCPersianas("#*DC5#"); // Salon Pie
        break;
      case 2:
        MandarAPCPersianas("#*DC7#"); // Iris Mesita
        break;
      case 3:
        MandarAPCPersianas("#*DC9#"); // Principal Mesita
        break;
      case 4:
        //MandarAPC("#*FC12#"); // Despacho
        break;
      case 5:
        //MandarAPC("#*FC2#"); // Terraza
        break;
      case 6:
        // MandarAPC("#*FC14#"); // Cocina
        //MandarAPC("#*FC15#"); // Lavadero
        break;
    }
  }
}
BLYNK_WRITE(V35) // Luz 3 ON
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPC("#*KC4#"); // Salon 2
        break;
      case 2:
        MandarAPC("#*KC13#"); // Baño Iris
        break;
      case 3:
        MandarAPC("#*KC10#"); //  Baño Principal
        break;
      case 4:
        //MandarAPC("#*KC12#"); // despacho
        break;
      case 5:
        //MandarAPC("#*KC1#"); // Terrza
        break;
      case 6:
        MandarAPC("#*KC1#"); // Pasillo 1
        MandarAPC("#*KC11#"); // Pasillo 2
        break;
    }
  }
}
BLYNK_WRITE(V36) // Luz 3 OFF
{
  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPC("#*FC4#"); // Salon 2
        break;
      case 2:
        MandarAPC("#*FC13#"); // Baño Iris
        break;
      case 3:
        MandarAPC("#*FC10#"); // Baño Principal
        break;
      case 4:
        //MandarAPC("#*FC12#"); // Despacho
        break;
      case 5:
        //MandarAPC("#*FC12#"); // Terraza
        break;
      case 6:
        MandarAPC("#*FC1#"); // Pasillo 1
        MandarAPC("#*FC11#"); // Pasillo 2
        break;
    }
  }
}
BLYNK_WRITE(V37) // Luz 4 ON
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        // MandarAPC("#*KC4#"); // Terraza
        break;
      case 2:
        //MandarAPC("#*KC6#"); //
        break;
      case 3:
        //MandarAPC("#*KC8#"); //
        break;
      case 4:
        //MandarAPC("#*KC12#"); //
        break;
      case 5:
        //MandarAPC("#*KH1#"); //
        break;
      case 6:
        MandarAPC("#*KH1#"); // Pasillo Piloto
        break;
    }
  }
}
BLYNK_WRITE(V38) // Luz 4 OFF
{
  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        //MandarAPC("#*FC4#"); // Terraza
        break;
      case 2:
        //MandarAPC("#*FC6#"); //
        break;
      case 3:
        //MandarAPC("#*FC8#"); //
        break;
      case 4:
        //MandarAPC("#*FC12#"); //
        break;
      case 5:
        //MandarAPC("#*FC12#"); //
        break;
      case 6:
        MandarAPC("#*FH1#"); // Pasillo Piloto
        break;
    }
  }
}
BLYNK_WRITE(V41) // Pers. 1 ON
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPCPersianas("#*KF2#"); // Salon 1
        break;
      case 2:
        MandarAPCPersianas("#*KG5#"); // Iris Techo
        break;
      case 3:
        MandarAPCPersianas("#*KG7#"); //  Principal
        break;
      case 4:
        //   MandarAPCPersianas("#*KC12#"); //  Despacho
        break;
      case 5:
        MandarAPCPersianas("#*KF1#"); //  Terraza  Puerta
        break;
    }
  }
}
BLYNK_WRITE(V42) // Pers. 1 OFF
{
  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPCPersianas("#*FF2#"); // Salon 1
        break;
      case 2:
        MandarAPCPersianas("#*FG5#"); // Iris Techo
        break;
      case 3:
        MandarAPCPersianas("#*FG7#"); // Principal
        break;
      case 4:
        //  MandarAPCPersianas("#*FC12#"); // Despacho
        break;
      case 5:
        MandarAPCPersianas("#*FF1#"); // Terraza Puerta
        break;
    }
  }
}
BLYNK_WRITE(V43) // Pers. 2 ON
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPCPersianas("#*KF3#"); // Salon 1
        break;
      case 2:
        // MandarAPCPersianas("#*KG5#"); // Iris Techo
        break;
      case 3:
        //MandarAPCPersianas("#*KG7#"); //  Principal
        break;
      case 4:
        //   MandarAPCPersianas("#*KC12#"); //  Despacho
        break;
      case 5:
        MandarAPCPersianas("#*KG8#"); //  Toldo Techo
        break;
    }
  }
}
BLYNK_WRITE(V44) // Pers. 2 OFF
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPCPersianas("#*FF3#"); // Salon 1
        break;
      case 2:
        // MandarAPCPersianas("#*FG5#"); // Iris Techo
        break;
      case 3:
        MandarAPCPersianas("#*FG9#"); //  Principal
        break;
      case 4:
        //   MandarAPCPersianas("#*FC12#"); //  Despacho
        break;
      case 5:
        MandarAPCPersianas("#*FG8#"); //  Toldo Techo
        break;
    }
  }
}
BLYNK_WRITE(V45) // Pers. 3 ON
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPCPersianas("#*KF4#"); // Toldo Salon-Terraza
        break;
      case 2:
        // MandarAPCPersianas("#*KG5#"); // Iris Techo
        break;
      case 3:
        //MandarAPCPersianas("#*KG7#"); //  Principal
        break;
      case 4:
        //   MandarAPCPersianas("#*KC12#"); //  Despacho
        break;
      case 5:
        MandarAPCPersianas("#*KF4#"); //  Toldo Ventana
        break;
    }
  }
}
BLYNK_WRITE(V46) // Pers. 3 OFF
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
  if (ON == 1) {

    switch (Habitacion) {
      case 1:
        MandarAPCPersianas("#*FF4#"); // Toldo Salon Terraza
        break;
      case 2:
        // MandarAPCPersianas("#*FG5#"); // Iris Techo
        break;
      case 3:
        //MandarAPCPersianas("#*FG7#"); //  Principal
        break;
      case 4:
        //   MandarAPCPersianas("#*FC12#"); //  Despacho
        break;
      case 5:
        MandarAPCPersianas("#*FF4#"); //  Toldo Ventana
        break;
    }
  }
}
BLYNK_WRITE(V50) // A dormir
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
 //if (ON == 1 && Habitacion ==1) {
if (ON == 1 ) {

    MandarAPC("#*FA5#"); // Macro a dormir
  }
}

BLYNK_WRITE(V51) // Ver TV NOCHE
{

  int ON = param.asInt(); // assigning incoming value from pin V10 to a variable
 //if (ON == 1 && Habitacion ==1) {
if (ON == 1 ) {

    MandarAPC("#*KA5#"); // Macro Ver TV Noche
  }
}

//------------------------------  SETUP  --------------------------------
void setup()

{ Serial.begin(115200);
  EEPROM.begin(512);
  //startWiFi();                   // Try to connect to some given access points. Then wait for a connection
  TFijada = EEPROM.read(0); // Temperatura Fijada
  Modo = EEPROM.read(1); // modo Verano invierno
  EstadoRejilla = EEPROM.read(2); //Estado de la rejilla cerrada 0 abierta 1
  ManualAutomatico = EEPROM.read(3); // Modo Automatico / Manual
   timeClient.setTimeOffset(3600); //    7200 = + 2 HORAS  ' 7200 es horario de verano, 3600 horario de invierno
  delay(1000);
  TerminalRemoto.println(Version);
  // -------------------------------------------
  switch (Habitacion) {
    case 1:
      MensajeInicio = "SALON";
      ArduinoOTA.setHostname("SALON");
      break;
    case 2:
      MensajeInicio = "IRIS";
      ArduinoOTA.setHostname("IRIS");
      break;
    case 3:
      MensajeInicio = "PRINCIPAL";
      ArduinoOTA.setHostname("PRINCIPAL");
      break;
    case 4:
      MensajeInicio = "DESPACHO";
      ArduinoOTA.setHostname("DESPACHO");
      break;
    case 5:
      MensajeInicio = "TERRAZA";
      ArduinoOTA.setHostname("TERRAZA");
      break;
    case 6:
      MensajeInicio = "COCINA Y PASILLO";
      ArduinoOTA.setHostname("COCINA Y PASILLO");
      break;
  }
  // ------------------------------------------------
  Serial.print("Arrancando.....  ");

  //Blynk.begin(auth,ssid, password); //insert here your SSID and password
  Blynk.begin(auth, "Unifi", "tel3dixi029574unifi"); //insert here your SSID and password
  // Blynk.begin(auth, "FWIFI24", "tel3dixi029574-24"); //insert here your SSID and password
  // Blynk.begin(auth, "tbja_sl", "quenotepasena"); //insert here your SSID and password
  while (Blynk.connect() == false) {
    // Wait until Blynk is connected
  }

  udpConnected = connectUDP(); // CONEXION UDP
  //Blynk.notify(Version);
  TerminalRemoto.println("Iniciado " + Version );
  TerminalRemoto.println(reloj) ;
  TerminalRemoto.flush();
  //WiFi.mode(WIFI_STA); //para que no inicie solamente en modo estaciÃ³n
  //  WiFi.begin(ssid, password);
  //  while (WiFi.status() != WL_CONNECTED and contconexion <50) { //Cuenta hasta 50 si no se puede conectar lo cancela
  //    ++contconexion;
  //    Serial.print(".");
  //    delay(500);
  //  }
  //  if (contconexion <50) {
  //      //para usar con ip fija
  //     // IPAddress Ip(192,168,1,222); //Elegida al princicio al seleccionar la habitacion
  //      IPAddress Gateway(192,168,1,1);
  //      IPAddress Subnet(255,255,255,0);
  //      WiFi.config(Ip, Gateway, Subnet);
  //
  //      Serial.println("");
  //      Serial.println("WiFi conectado");
  //      Serial.println(WiFi.localIP());
  //
  //  }
  //  else {
  //      Serial.println("");
  //      Serial.println("Error de conexion");
  //  }

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print(MensajeInicio + " OTA");


  TimerDHT.setInterval(6000, sendUptime);     //aqui se define cada cuanto tiempo se lee a temperatura
  Temporizador1.setInterval(1000, RepeatTask); // Rutina de repeticion continua
  TreintaSeg.setInterval(60000, RutinaTreintaSeg); // Rutina
  Temporizador3.setTimer(10000, TreeTimesTask, 1); // Repeticion de rutina tres veces
  timeClient.begin();
  timeClient.setTimeOffset(3600); //    7200 =  + 2 HORAS
  //Temporizador3.disable(Repeticion);

  pinMode (ReleRejilla, OUTPUT);
  digitalWrite(ReleRejilla, LOW);
  Blynk.notify("INICIADO " + MensajeInicio);
  pinMode (Control_3v, OUTPUT);
  digitalWrite(Control_3v, HIGH);
  TerminalRemoto.println("INICIADO ESP "  + MensajeInicio);
  TerminalRemoto.flush();

  //-----------------------------  //Estado de la Rejilla
  if (EstadoRejilla == 1) // Valor leido de la Eeprom
  { digitalWrite(ReleRejilla, LOW);
    Blynk.setProperty(V3, "color", "#0c990c");// Setting color VERDE
    LedRejilla.on();
    Blynk.virtualWrite(V5, 1);
    Blynk.virtualWrite(V6, ("Abriendo Rejilla " + MensajeInicio + " " + (timeClient.getFormattedTime()) + "  ErrHora  " + ContadorErroresDHTotal));
    Serial << "Rejilla ABIERTA " << endl;
    //   EEPROM.write(2, 1);
    //   EEPROM.commit();
  }
  else
  { digitalWrite(ReleRejilla, HIGH);
    Blynk.setProperty(V3, "color", "#ff0000");// Setting color ROJO
    LedRejilla.on();
    Blynk.virtualWrite(V5, 0);
    Blynk.virtualWrite(V6, ("Cerrando Rejilla " + MensajeInicio + " " + (timeClient.getFormattedTime()) + "  ErrHora  " + ContadorErroresDHTotal));
    Serial << "REJILLA CERRADA " << endl;
  }
  //---------------------------- //Automatico Manual
  if (ManualAutomatico == 1)
  {
    Blynk.virtualWrite(V4, 1);
  }
  else
  { Blynk.virtualWrite(V4, 0);
  }
  //---------------------------   //InviernoVerano
  if (Modo == 1) // Verano
  {
    Blynk.virtualWrite(V2, 1);
  }
  else
  { Blynk.virtualWrite(V2, 0);
  }
} // Final del SETUP

void AbrirRejilla()
{
  EstadoRejilla = 1;
  digitalWrite(ReleRejilla, LOW);
  Blynk.setProperty(V3, "color", "#0c990c");// Setting color VERDE
  LedRejilla.on();
  Blynk.virtualWrite(V5, 1);
  //   Blynk.virtualWrite(V6, ("Abriendo Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
  Serial << "Rejilla ABIERTA " << endl;
}
void CerrarRejilla()
{
  EstadoRejilla = 0;
  digitalWrite(ReleRejilla, HIGH);
  Blynk.setProperty(V3, "color", "#ff0000");// Setting color ROJO
  LedRejilla.on();
  Blynk.virtualWrite(V5, 0);
  //  Blynk.virtualWrite(V6, ("Cerrando Rejilla " + MensajeInicio + " " + (timeClient.getFormattedTime()) + "  ErrHora  " + ContadorErroresDHTotal));
  Serial << "REJILLA CERRADA " << endl;
}
void TreeTimesTask()
{ // Timer con limitacion de repeticiones
  static int k = 0;
  k++;
  Serial.print("called ");
  Serial.print(k);
  Serial.println(" / 3times.");
  //   MandarAPCPersianas("#*QQG7#"); //  Principal
  FechaString=(timeClient.getFormattedDate());
  TerminalRemoto.println(FechaString + "  " + Habitacion );
    TerminalRemoto.flush();
  if (k == 3) {
    // Serial << "Reset 3 " << " " << Anemo << " " << endl;
    // Serial << "Reset 3 " << " " << Pluvio << " " << endl;
  }
}

void RepeatTask()  //Consulta Reloj y escribe en Pin Virtual V0,V1,V2....
{ // Timer infinito
 timeClient.setTimeOffset(3600); //    7200 = + 2 HORAS  ' 7200 es horario de verano, 3600 horario de invierno
  timeClient.update();
  //reloj = (timeClient.getFormattedTime());
  Serial.println(timeClient.getFormattedTime());
  minutosString = (timeClient.getMinutes());
  horasString = (timeClient.getHours());
  segundosString = (timeClient.getSeconds());

  horas = (horasString.toInt());
  horas = horas + 1;
  if (horas == 24)  {
    horas = 0;
  }
  minutos = (minutosString.toInt());
  //segundos=(segundosString.toInt());
  //segundosCero= String (segundos);
  //if (segundosCero.length() == 1)
  //{ segundosCero = ("0" + segundosCero);}
  //   Serial << horas << ":" << minutos << ":" << segundos << endl;

  reloj =  (String (horas) + ":" + String (minutos));
  Serial << reloj << endl;

  if (minutos == 0) {
    Serial.println(timeClient.getMinutes());
    ContadorErroresDHTotal = 0;
  }

  if (CuentaHeart % 2 == 0) {
    Heart.on(); // Miro si CuentaHeart es par o impar y actuo (al final del sub hay un incremento de la variable CuentaHeart con antidesboramiento
  }
  else {
    Heart.off();
  }

 ++CuentaHeart;
  if (CuentaHeart == 10000) {
    CuentaHeart = 1;
  }

  // Serial << "Timer de 1 segundo " << endl;
  Blynk.virtualWrite(V0, TFijada); // virtual pin 0
  Blynk.virtualWrite(V2, Modo); // virtual pin 2
  Blynk.virtualWrite(V1, TFijada); // virtual pin 1 Deslizante para actualizar el valor

  switch (Habitacion)
  {
    case 1:
      Blynk.virtualWrite(V40, "SALON");
      Blynk.setProperty(V31, "offLabel", "LUZ SALÓN 1");              // ON
      Blynk.setProperty(V33, "offLabel", "LÁMPARA PIE SALÓN"); // ON
      Blynk.setProperty(V35, "offLabel", "LUZ SALÓN 2");               // ON
      Blynk.setProperty(V37, "offLabel", " ");                                  // ON
      Blynk.setProperty(V32, "offLabel", "OFF");                            // OFF
      Blynk.setProperty(V34, "offLabel", "OFF");                            // OFF
      Blynk.setProperty(V36, "offLabel", "OFF");                           // OFF
      Blynk.setProperty(V38, "offLabel", " ");                               // OFF
      Blynk.setProperty(V41, "offLabel", "Subir Pers. 1");    Blynk.setProperty(V41, "label", " VENTANA IZQUIERDA");              // ON
      Blynk.setProperty(V42, "offLabel", "Bajar Pers. 1");    Blynk.setProperty(V42, "label", " VENTANA IZQUIERDA");         //OFF
      Blynk.setProperty(V43, "offLabel", "Subir Pers. 2");    Blynk.setProperty(V43, "label", " VENTANA DERECHA");           // ON
      Blynk.setProperty(V44, "offLabel", "Bajar Pers. 2");    Blynk.setProperty(V44, "label", " VENTANA DERECHA");          // OFF
      Blynk.setProperty(V45, "offLabel", "Subir TOLDO");    Blynk.setProperty(V45, "label", " TOLDO VENTANA SALÓN");          // ON
      Blynk.setProperty(V46, "offLabel", "Bajar TOLDO");     Blynk.setProperty(V46, "label", " TOLDO VENTANA SALÓN");       // OFF
      break;
    case 2:
      Blynk.virtualWrite(V40, "IRIS");
      Blynk.setProperty(V31, "offLabel", "LUZ IRIS TECHO");                    // ON
      Blynk.setProperty(V33, "offLabel", "MESITA IRIS");                         // ON
      Blynk.setProperty(V35, "offLabel", "BAÑO IRIS");                           // ON
      Blynk.setProperty(V37, "offLabel", " ");                                   // ON
      Blynk.setProperty(V32, "offLabel", "OFF");                            // OFF
      Blynk.setProperty(V34, "offLabel", "OFF");                           // OFF
      Blynk.setProperty(V36, "offLabel", "OFF");                        // OFF
      Blynk.setProperty(V38, "offLabel", " ");                                // OFF
      Blynk.setProperty(V41, "offLabel", "Subir Pers. Iris");  Blynk.setProperty(V41, "label", " VENTANA IRIS");        // ON
      Blynk.setProperty(V42, "offLabel", "Bajar Pers. Iris");   Blynk.setProperty(V42, "label", " VENTANA IRIS");        //OFF
      Blynk.setProperty(V43, "offLabel", " "); Blynk.setProperty(V43, "label", " ");     // ON
      Blynk.setProperty(V44, "offLabel", " ");  Blynk.setProperty(V44, "label", " ");    // OFF
      Blynk.setProperty(V45, "offLabel", " ");  Blynk.setProperty(V45, "label", " ");    // ON
      Blynk.setProperty(V46, "offLabel", " "); Blynk.setProperty(V46, "label", " ");    // OFF
      break;
    case 3:
      Blynk.virtualWrite(V40, "DORM. PRINCIPAL");
      Blynk.setProperty(V31, "offLabel", "LUZ PRINCIPAL TECHO");        // ON
      Blynk.setProperty(V33, "offLabel", "MESITA PRINCIPAL");                // ON
      Blynk.setProperty(V35, "offLabel", "BAÑO PRINCIPAL");                // ON
      Blynk.setProperty(V37, "offLabel", " ");                                         // ON
      Blynk.setProperty(V32, "offLabel", "OFF");                                  //OFF
      Blynk.setProperty(V34, "offLabel", "OFF");                                 //OFF
      Blynk.setProperty(V36, "offLabel", "OFF");                                //OFF
      Blynk.setProperty(V38, "offLabel", " ");                                      //OFF
      Blynk.setProperty(V41, "offLabel", "Subir Pers. PPAL");  Blynk.setProperty(V41, "label", "PERSIANA PRINCIPAL");       // ON
      Blynk.setProperty(V42, "offLabel", "Bajar Pers. PPAL");   Blynk.setProperty(V42, "label", "PERSIANA PRINCIPAL");       //OFF
      Blynk.setProperty(V43, "offLabel", " ");                            Blynk.setProperty(V43, "label", " ");                                  // ON
      Blynk.setProperty(V44, "offLabel", " Bajar en Verano");   Blynk.setProperty(V44, "label", "Bajada Temporizada");   //OFF
      Blynk.setProperty(V45, "offLabel", " ");                          Blynk.setProperty(V45, "label", " ");        // ON
      Blynk.setProperty(V46, "offLabel", " ");                          Blynk.setProperty(V46, "label", " ");         //OFF

      break;
    case 4:
      Blynk.virtualWrite(V40, "DESPACHO");
      Blynk.setProperty(V31, "offLabel", "LUZ DESPACHO TECHO");        // ON
      Blynk.setProperty(V33, "offLabel", " ");                                        // ON
      Blynk.setProperty(V35, "offLabel", " ");                                        // ON
      Blynk.setProperty(V37, "offLabel", " ");                                        // ON
      Blynk.setProperty(V32, "offLabel", "OFF");                                 // OFF
      Blynk.setProperty(V34, "offLabel", " ");                                         // OFF
      Blynk.setProperty(V36, "offLabel", " ");                                         // OFF
      Blynk.setProperty(V38, "offLabel", " ");                                         // OFF
      Blynk.setProperty(V41, "offLabel", "Subir Pers. Despacho");  Blynk.setProperty(V41, "label", "PERSIANA DESPACHO");       // ON
      Blynk.setProperty(V42, "offLabel", "Bajar Pers. Despacho");  Blynk.setProperty(V42, "label", "PERSIANA DESPACHO");         // OFF
      Blynk.setProperty(V43, "offLabel", " "); Blynk.setProperty(V43, "label", " ");    // ON
      Blynk.setProperty(V44, "offLabel", " "); Blynk.setProperty(V44, "label", " ");   // OFF
      Blynk.setProperty(V45, "offLabel", " "); Blynk.setProperty(V45, "label", " ");    // ON
      Blynk.setProperty(V46, "offLabel", " ");  Blynk.setProperty(V46, "label", " ");    // OFF
      break;
    case 5:
      Blynk.virtualWrite(V40, "TERRAZA");
      Blynk.setProperty(V31, "offLabel", "LUZ TERRAZA");                // ON
      Blynk.setProperty(V33, "offLabel", " ");                                // ON
      Blynk.setProperty(V35, "offLabel", " ");                                // ON
      Blynk.setProperty(V37, "offLabel", " ");                                // ON
      Blynk.setProperty(V32, "offLabel", "OFF");                         // OFF
      Blynk.setProperty(V34, "offLabel", " ");                                 // OFF
      Blynk.setProperty(V36, "offLabel", " ");                                 // OFF
      Blynk.setProperty(V38, "offLabel", " ");                                 // OFF
      Blynk.setProperty(V41, "offLabel", "Subir Pers. Terraza");   Blynk.setProperty(V41, "label", "PERSIANA PUERTA TERRAZA");       // ON
      Blynk.setProperty(V42, "offLabel", "Bajar Pers. Terraza");   Blynk.setProperty(V42, "label", "PERSIANA PUERTA TERRAZA");       // OFF
      Blynk.setProperty(V43, "offLabel", "Abrir Techo");               Blynk.setProperty(V43, "label", "TOLDO TECHO"); // ON
      Blynk.setProperty(V44, "offLabel", "Cerrar Techo");             Blynk.setProperty(V44, "label", "TOLDO TECHO");     // OFF
      Blynk.setProperty(V45, "offLabel", "Subir TOLDO");            Blynk.setProperty(V45, "label", "TOLDO VENTANA SALON");  // ON
      Blynk.setProperty(V46, "offLabel", "Bajar TOLDO");            Blynk.setProperty(V46, "label", "TOLDO VENTANA SALON");   // OFF
      break;
    case 6:
      Blynk.virtualWrite(V40, "COCINA Y PASILLO");
      Blynk.setProperty(V31, "offLabel", "LUZ COCINA / LAVADERO");        // ON
      Blynk.setProperty(V33, "offLabel", " ");                                                // ON
      Blynk.setProperty(V35, "offLabel", "LUCES PASILLO ");                        // ON
      Blynk.setProperty(V37, "offLabel", "LUZ PILOTO PASILLO ");           // ON
      Blynk.setProperty(V32, "offLabel", "OFF");                                 // OFF
      Blynk.setProperty(V34, "offLabel", " ");                                         // OFF
      Blynk.setProperty(V36, "offLabel", "OFF");                                 // OFF
      Blynk.setProperty(V38, "offLabel", "OFF");                                 // OFF
      Blynk.setProperty(V41, "offLabel", " ");   Blynk.setProperty(V41, "label", " ");                                      // ON
      Blynk.setProperty(V42, "offLabel", " ");   Blynk.setProperty(V42, "label", " ");                                        // OFF
      Blynk.setProperty(V43, "offLabel", " ");  Blynk.setProperty(V43, "label", " ");                                       // ON
      Blynk.setProperty(V44, "offLabel", " ");  Blynk.setProperty(V44, "label", " ");                                       // OFF
      Blynk.setProperty(V45, "offLabel", " ");   Blynk.setProperty(V45, "label", " ");                                      // ON
      Blynk.setProperty(V46, "offLabel", " ");  Blynk.setProperty(V46, "label", " ");                                         // OFF
      break;
    default:
      Blynk.setProperty(V31, "offLabel", " ");
      Blynk.setProperty(V32, "offLabel", " ");
      Blynk.setProperty(V33, "offLabel", " ");
      Blynk.setProperty(V34, "offLabel", " ");
      Blynk.setProperty(V35, "offLabel", " ");
      Blynk.setProperty(V36, "offLabel", " ");
      Blynk.setProperty(V37, "offLabel", " ");
      Blynk.setProperty(V38, "offLabel", " ");
      Blynk.setProperty(V39, "offLabel", " ");
      Blynk.setProperty(V40, "offLabel", " ");
      Blynk.setProperty(V41, "offLabel", " ");
      Blynk.setProperty(V42, "offLabel", " ");
      Blynk.setProperty(V43, "offLabel", " ");
      Blynk.setProperty(V44, "offLabel", " ");
      Blynk.setProperty(V45, "offLabel", " ");
      Blynk.setProperty(V46, "offLabel", " ");
      break;
  }
}

void sendUptime() // Rutina de lectura del sensor de temperatura
{
  Blynk.virtualWrite(V1, TFijada);
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  //Read the Temp and Humidity from DHT
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  TemperaturaReal = t;
  HumedadReal = h;

  if (isnan(h) || isnan(t)) {
    ContadorErroresDHT ++;
    Serial.println("Error al Leer Sensor DHT");
    // Blynk.virtualWrite(V6, ("Error en Sonda " + MensajeInicio + " " + ContadorErroresDHT + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
    if (ContadorErroresDHT >= 10) {
      //Blynk.notify("Error en Sonda " + MensajeInicio);
      // Blynk.virtualWrite(10, 0); //
      // Blynk.virtualWrite(11, 0); //

      digitalWrite(Control_3v, LOW);
      delay(500);
      digitalWrite(Control_3v, HIGH);
      ContadorErroresDHT = 0;
      //ESP.restart();
    }

  } else {
    ContadorErroresDHT = 0;
    Serial << "T: " << " " << (t) << " " << endl;
    Serial << "H %  " << " " << (h) << " " << endl;
    Blynk.virtualWrite(10, t); //
    Blynk.virtualWrite(11, h); //
  }
}

void RutinaTreintaSeg() // Comprobacion de Modo, temperatura y comparacion, para abrir o cerrar
{ // Entra cada 30 segundos

  if (ManualAutomatico == 1) { //1Âº verifico si el sistema esta en automatico o manual, si es auto hago la rutina
    Serial << "Automatico" << endl;
    switch (Modo) { //Modo Invierno Verano

      case 0: // Invierno
        Serial << "Invierno " << endl;
        // --------------------------------------------------------------------------------------------
        if (EstadoRejilla == 0)      { // Si la rejilla esta cerrada
          Serial << "Invierno " << "Cerrada " << TemperaturaReal << " C " << endl;
          if (TemperaturaReal <= (TFijada - 1)) { // Si la t es menor o igual a la Fijada -1...
            AbrirRejilla();// Llamo a un sub para abrir y mandar a Blynk
            Blynk.virtualWrite(V6, ("Abriendo Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
            //  Blynk.virtualWrite(V6, ("Abriendo Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
            EEPROM.write(2, 1);
            ////    delay(200);
            EEPROM.commit();
          }  // Fin comprobar temperatura

          if (TemperaturaReal >= (TFijada + 1)) { // Si la t es mayor o igual a la Fijada +1...
            CerrarRejilla();// Llamo a un sub para cerrar y mandar a Blynk
            Blynk.virtualWrite(V6, ("Cerrando Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
          }  // Fin comprobar temperatura

        }  // Fin si la rejilla esta cerrada
        // -------------------------------------------------------------------------------------------------
        if (EstadoRejilla == 1)        { //  Si la rejilla esta abierta
          Serial << "Invierno " << "Abierta " << TemperaturaReal << " C " << endl;
          if (TemperaturaReal <= (TFijada - 1)) { // Si la t es menor o igual a la Fijada -1...
            AbrirRejilla();// Llamo a un sub para abrir y mandar a Blynk
            Blynk.virtualWrite(V6, ("Abriendo Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
            //Blynk.virtualWrite(V6, ("Abriendo Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
          }  // Fin comprobar temperatura

          if (TemperaturaReal >= (TFijada + 1)) { // Si la t es mayor o igual a la Fijada +1...
            CerrarRejilla();// Llamo a un sub para cerrar y mandar a Blynk
            Blynk.virtualWrite(V6, ("Cerrando Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
            EEPROM.write(2, 0);
            ////    delay(200);
            EEPROM.commit();
          }  // Fin comprobar temperatura

        } //  Fin si la rejilla esta abierta

        // -----------------------------------------------------------------------------------------------
        break; // Salgo del modo Invierno

      case 1: // Verano    -------------------------------------------   V E R A N O   ---------------------------------

        Serial << "Verano " << endl;
        // --------------------------------------------------------------------------------------------

        if (EstadoRejilla == 0)      { // Si la rejilla esta cerrada
          Serial << "Verano " << "Cerrada " << TemperaturaReal << " C " << endl;
          if (TemperaturaReal <= (TFijada - (0.5))) { // Si la t es menor o igual a la Fijada -1...
            CerrarRejilla();// Llamo a un sub para cerrar y mandar a Blynk
            Blynk.virtualWrite(V6, ("Cerrando Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
          }  // Fin comprobar temperatura

          if (TemperaturaReal >= (TFijada + (0.5))) { // Si la t es mayor o igual a la Fijada +1...
            AbrirRejilla();// Llamo a un sub para abrir y mandar a Blynk
            Blynk.virtualWrite(V6, ("Abriendo Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
            EEPROM.write(2, 1);
            ////    delay(200);
            EEPROM.commit();

          }  // Fin comprobar temperatura

        }  // Fin si la rejilla esta cerrada
        // -------------------------------------------------------------------------------------------------
        if (EstadoRejilla == 1)        { //  Si la rejilla esta abierta
          Serial << "Verano " << "Abierta " << TemperaturaReal << " C " << endl;
          if (TemperaturaReal <= (TFijada - (0.5))) { // Si la t es menor o igual a la Fijada -1...
            CerrarRejilla();// Llamo a un sub para cerrar y mandar a Blynk
            Blynk.virtualWrite(V6, ("Cerrando Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
            EEPROM.write(2, 0);
            ////    delay(200);
            EEPROM.commit();
          }  // Fin comprobar temperatura

          if (TemperaturaReal >= (TFijada + (0.5))) { // Si la t es mayor o igual a la Fijada +1...
            AbrirRejilla();// Llamo a un sub para abrir y mandar a Blynk
            Blynk.virtualWrite(V6, ("Abriendo Rejilla " + MensajeInicio + " " + reloj + "  ErrHora  " + ContadorErroresDHTotal));
          }  // Fin comprobar temperatura
        } //  Fin si la rejilla esta abierta

        break; // Salgo del modo Verano

    } // Fin case Modo Invierno Verano
  } //Fin if Manual/Auto
} // ----------- Fin RutinaTreinta

void MandarAPC(String CadenaDeEnvioString) {
  CadenaDeEnvioString.toCharArray(CadenaEnChar, 8);
  UDP.beginPacket(remote, PuertoRemoto);
  UDP.write(CadenaEnChar);
  UDP.write(packetBuffer);
  UDP.endPacket();


          UDP.beginPacket(remote, PuertoRemoto);
          UDP.write(CadenaEnChar);
          UDP.write(packetBuffer);
          UDP.endPacket();


  memset(packetBuffer, 0, sizeof(packetBuffer)); // vaciar Buffer de entrada
  //Blynk.notify("Enciende Luz  ");
  //TerminalRemoto.println(reloj + "  " + Habitacion + "  " + CadenaDeEnvioString );
  FechaString=(timeClient.getFormattedDate());
  TerminalRemoto.println(FechaString + "  " + Habitacion + "  " + CadenaDeEnvioString ); 
  TerminalRemoto.flush();
}

void MandarAPCPersianas(String CadenaDeEnvioString) { // Sin doble envio
  CadenaDeEnvioString.toCharArray(CadenaEnChar, 8);
  UDP.beginPacket(remote, PuertoRemoto);
  UDP.write(CadenaEnChar);
  UDP.write(packetBuffer);
  UDP.endPacket();

  memset(packetBuffer, 0, sizeof(packetBuffer)); // vaciar Buffer de entrada
  //Blynk.notify("Enciende Luz  ");
  FechaString=(timeClient.getFormattedDate());
  TerminalRemoto.println(FechaString + "  " + Habitacion + "  " + CadenaDeEnvioString );
  TerminalRemoto.flush();
}

void loop()
{
  ArduinoOTA.handle();
  if (toggleSwitch.poll()) Serial << toggleSwitch.on() << endl;
  if (toggleSwitch.longPress()) Serial << "longPress1 " << endl;
  if (toggleSwitch.longPress()) Serial << "longPress2\n" << endl;
  if (toggleSwitch.doubleClick()) Serial << "doubleClick1 " << endl;
  if (toggleSwitch.doubleClick()) Serial << "doubleClick2\n" << endl;

  Blynk.run();
  TimerDHT.run();
  Temporizador1.run();
  TreintaSeg.run();
  Temporizador3.run();

  // check if the WiFi and UDP connections were successful
  //if (wifiConnected) {
  if (udpConnected) {
    // Si hay datos disponibles, lee un paquete
    int packetSize = UDP.parsePacket();
    if (packetSize)
    {

      Serial.println("");
      Serial.print("Recibido paquete de tamaño ");
      Serial.println(packetSize);
      Serial.print("De  ");

      //      IPAddress remote = UDP.remoteIP();
      //  unsigned int PuertoRemoto = 8889;
      //   IPAddress remote(192,168,1,15);


      for (int i = 0; i < 4; i++)
      {
        Serial.print(remote[i], DEC);
        if (i < 3)
        {
          Serial.print(".");
        }
      }
      Serial.print(", Puerto ");
      Serial.println(UDP.remotePort());

      // read the packet into packetBufffer
      UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      Serial.println("Contents: ");
      int value = packetBuffer[0] * 256 + packetBuffer[1];
      Serial.println(value);
      Serial.println(packetBuffer);  //añadido por mi
      String DatoEnString = packetBuffer; //IMPORTANTISIMO... ESTO CONVIERTE EL ARRAY EN STRING


      if (strlen(packetBuffer) == 7) {
        Serial.println("EEEE Correcto");
        Serial.println("Este es el dato: " + DatoEnString);
      }

      ///  ALGUNAS PRUEBAS DE MANEJO DE CADENAS .....

      int posicion = DatoEnString.indexOf("L");            //Buscar un caracter en la cadena
      Serial.println(posicion); // Si no encuentra el valor devuelve -1 , el valor mas a la izq es 0


      if (DatoEnString.startsWith("#")) {
        Serial.println("OK Dato comienza por el caracter buscado ...");
      }

      String subStr = DatoEnString.substring(2, 4);
      Serial.println("SubString " + subStr);

      /// -----------------------------------------------------------------------------------------------
      // send a reply, to the IP address and port that sent us the packet we received
      //       UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
      UDP.beginPacket(remote, PuertoRemoto);
      UDP.write("Devuelto paquete: ");
      UDP.write(packetBuffer);
      UDP.endPacket();

      UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
      UDP.write("Devuelto paquete: ");
      UDP.write(packetBuffer);
      UDP.endPacket();

      memset(packetBuffer, 0, sizeof(packetBuffer)); // vaciar Buffer de entrada


    }
  }
  // buttonGND.poll();
  // if (buttonGND.switched());         // Serial << "switched ";
  // if (buttonGND.pushed()) ++Anemo;   //Serial << "pushed " << ++Anemo << " ";
  // if (buttonGND.released());         // Serial << "released\n";

  // buttonGND2.poll();
  //  if (buttonGND2.switched());         //Serial << "switched2 ";
  //  if (buttonGND2.pushed()) Pluvio = Pluvio + 3; //
  // Serial << "pushed2 " << ++PluvioTMP << " ";
  //  if (buttonGND2.released());        //Serial << "released2\n";

} // -------------- Final LOOP ---------------------------------------------------

/***********************************
    esto es una maravilla, detecta una pulsacion, pulsado y dobleclick....
    No he probado las opciones
    const byte ButtonVCCpin = 6;
    const byte Button10mspin = 8;
    Pero las otras funciona perfecto.
    Ademas tiene una libreria Streaming que sirve para escribir en el puerto serie de una manera mas directa y con menos lineas..


    Prueba de libreria SimpleTimer que permite hacer diferentes tipos de temporizaciones. Infinita o con limite de repeticiones.
*/

// 26-09-2018         Se aÃ±ade una notificacion en el Setup para controlar si hay reinicios de placa o si se apaga cuando se inicia de nuevo.

// 27-11-2018          He aÃ±adido un pequeÃ±o delay de 200 ms despues de escribir en la eeprom y antes del commit de confirmacion
// He dejado ya en 30 sg la comparativa de temperatura ambiente con la fijada
// Ademas si esta en automatico blynk envia el estado corecto del boton rejilla que hemos pulsado y no actua por estar en auto



//   ----------------  DECLARACION DE FUNCIONES - ------------------------------

// connect to UDP y returns true if successful or false if not
boolean connectUDP() {
  boolean state = false;

  Serial.println("");
  Serial.println("Connecting to UDP");

  if (UDP.begin(localPort) == 1) {
    Serial.println("Conexion Correcta");
    state = true;
  }
  else {
    Serial.println("Conexion Fallida");
  }

  return state;
}




