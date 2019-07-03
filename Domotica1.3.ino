// incluyo archivos de clases
#include "display.h"
#include "dht.h"
#include "teclado.h"

#define ldr A0
#define entrada_energia A1
#define pir1 A2
#define pir2 A3
#define SIM900 Serial3
#define RESTART asm("jmp 0x0000") //comando para reinciar el arduino
#define luz 36
#define off HIGH
#define on LOW

// inicio instancias de las clases
pantallalcd pantalla;
sensordht12 sensor_th;
teclado_pad teclado;

String dato_temperatura,dato_humedad, dato_sens, dato_humr;

boolean dht12read = true;


//definiciones de tiempos
#define intervalo_chequeoenergia 60000 // tiempo de intervalo en que se chequea el estado del suministro electrico 60000=1 min
unsigned long tiempo_inicial = 0; //guarda el valor actual del tiempo en cada intervalo de lectura
unsigned long intervalo_lectura = 500; //1/2 cada 1 segundo hace lectura de estado de luz, ldr y sensores pir
unsigned long intervalo_encenderluz = 180000; //3 minutos 180000
unsigned long tiempo_actual; //guarda valor de tiempo actual en cada iteracion cuando la luz esta encendida
unsigned long tiempo_inicial_luz = 0; //guarda tiempo actual cuando se enciende la luz

String lecturateclado=""; //guarda lo que la funcion leeteclado devuelve

double tiempo_restanteluz = 0; //para calcular tiempo restante del encendido de la luz
unsigned long chequeo_energiaactual=0;
// fin definiciones de tiempo

int valor_minimoldr = 30; //valor minimo de luminosidad detectada
int valor_minimoenergia = 400; //valor minimo para detectar corte de energia
int valor_minimopir = 300; //valor minimo para deteccion de movimiento
boolean estado_energia; //variable para estado de energia de red
boolean primeracarga;
boolean estado_luz = false;
int valor_pir1;
int valor_pir2;
int valor_ldr;
int pantalla_lcd = 1;

//***Variables para manejo de mensajes y llamadas por sim 900
int respuesta; //respuesta de la conexion grps
char aux_str[50]; //variable para la conexion gprs
char incoming_char; //variable donde recibe datos del SIM900
char* sms; //variable que contiene sms que envia el SIM900
char* celu_responder; //variable para almacenar celular de respuesta (responde al celular que le envia algun codigo valido)
char* celu_llamar; //variable que contiene el celular a quien llamar (celular que le envia codigo de llamar es a quien llama)
int posicion=0; //utilizada para leer caracteres recibidos por el SIM900
char num; //para utilizar con SIM900
String cel_recibido=""; //para armar cadena del numero recibido por SIM900
String cel_recibido_at=""; //para armar comando at del celu recibido
String cel_llamar_at=""; //para armar comando at del celu a llamar
char clave; //variable para clave recibida por sms
String clave_recibida=""; //para armar clave recibida por sms
String celu_aviso="AT+CMGS=\"+543794888545\""; //celular al que se envia msj por alerta
unsigned long chequeosim_actual=0;//almacena el tiempo en que realizara el chequeo del SIM900
#define intervalo_chequeoSIM900 600000 // tiempo de intervalo en que sera chequeado el estado del SIM9000 - 600000=10 min
//Fin variables manejo SIM900*************************************************************


void setup() {
  sensor_th.iniciar();
  pantalla.iniciar();
  pantalla.limpiar();
  primeracarga = true;
  pinMode(entrada_energia, INPUT);
  pinMode(9, OUTPUT); //para encender la tarjeta sim900
  pinMode(ldr, INPUT);
  pinMode(pir1, INPUT);
  pinMode(pir2, INPUT);
  pinMode(luz, OUTPUT);
  Serial.begin(9600);
  estado_luz = false;
  tiempo_inicial = millis();  
  digitalWrite(luz, off);
  SIM900.begin(19200); //Configura velocidad del puerto serie para el SIM9000
}

void loop() {

  //verifica si se reseteo el equipo o es el primer inicio para establecer el valor inicial del estado de energia
  if (primeracarga==true){
    leer_dht12();
    primeracarga=false;
    if (analogRead(entrada_energia)<valor_minimoenergia){
      estado_energia=false;
    } else {
      estado_energia=true;      
    }
    delay(2000);// parada para estabilizar sensores
    encenderSIM900(); //enciende el SIM900 en su primer inicio    
  }
  //fin verificacion del reseteo del equipo

  //lectura de teclado
  delay(50);
  lecturateclado=teclado.leeteclado();
  ingreso_codigo(lecturateclado);

  //fin lectura teclado

  //verifica cada cierto tiempo establecido en intervalo_chequeoSIM900 el estado del modulo SIM9000
  //si lo encuentra apagado envia comando para encenderlo a traves del pin 9
  if (millis() > (chequeosim_actual + intervalo_chequeoSIM900)){ 
    chequeosim_actual = millis();
    encenderSIM900(); 
  }
  //fin verificacion estado del SIM900

  // verifica el estado del suministro electrico y hace lectura del sensor dht12
  if (millis() > (chequeo_energiaactual + intervalo_chequeoenergia) ){
    chequeo_energiaactual = millis();
    leer_dht12();
    if (analogRead(entrada_energia)<valor_minimoenergia){
      // no detecto energia en la lectura
      // consulta si hay cambio de estado
      if (estado_energia==true){
        // hubo cambio en el estado de la energia de "corte de energia"
        estado_energia=false;
        informa_estadoenergia(estado_energia);
      }
    } else {
      // detecto energia en la lectura
      // consulta si hay cambio de estado
      if (estado_energia==false){
        // detecto cambio en el estado de la energia "restablecimiento del servicio"
        estado_energia=true;
        informa_estadoenergia(estado_energia);
      }
    }    
  }
  //fin chequeo estado del suministro electrico

  
  //chequeo de luminosidad exterior, movimiento en sensor pir y control de luces
  if (millis()>(tiempo_inicial + intervalo_lectura)){
    tiempo_inicial = millis();
    //verifica si la luz se encuentra apagada
    if (!estado_luz){
      valor_ldr = analogRead(ldr);
      //si el valor de luz recibido indica poca luminosidad hace lectura del sensor pir
      if (valor_ldr<valor_minimoldr){
        valor_pir1 = analogRead(pir1);
        valor_pir2 = analogRead(pir2);
        if ((valor_pir1<valor_minimopir) || (valor_pir2<valor_minimopir)){
          digitalWrite(luz, on);
          estado_luz = true;
          tiempo_inicial_luz = millis();
        }
      }
    } else {
      //si la luz esta encendida verifica si ya se excedio el tiempo de mantener encendida la luz
      //tiempo_actual = millis();
      //tiempo_restanteluz = (((tiempo_inicial_luz + intervalo_encenderluz)-(tiempo_actual))/60000);
      //Serial.println(tiempo_restanteluz);
      if (millis()>(tiempo_inicial_luz + intervalo_encenderluz)){
        digitalWrite(luz, off);
        tiempo_inicial_luz = 0;
        estado_luz = false;
      }
    } 
  }
  //fin chequeo luminosidad, sensor pir y control de luces

  //RECEPCION DE DATOS POR SMS
  if (SIM900.available()){
    delay(50); //parada de control de flujo
    incoming_char=SIM900.read();
    if ((posicion==0) && (incoming_char=='+')) {
      posicion=1;
      cel_recibido="";
      clave_recibida="";
    }
    if ((posicion==1) && (incoming_char=='C')) {
      posicion=2;
    }
    if ((posicion==2) && (incoming_char=='M')) {
      posicion=3;
    }
    if ((posicion==3) && (incoming_char=='T')) {
      posicion=4;
    }
    if ((posicion==4) && (incoming_char==':')) {
      posicion=5;
    }
    if ((posicion==5) && (incoming_char==' ')) {
      posicion=6;
    }
    if ((posicion==6) && (incoming_char=='"')) {
      cel_recibido="";
      posicion=0;
      for(int i=0;i<13;i++){
        delay(50);
        num = SIM900.read(); //Captura del número remitente.
        if (num == '"') break;
        else 
        cel_recibido += num;
      }
      cel_recibido_at="AT+CMGS=\"" + cel_recibido + "\"";
      cel_llamar_at="ATD" + cel_recibido + ";"; 
      celu_llamar=cel_llamar_at.c_str();
      celu_responder=cel_recibido_at.c_str();
    }
    if (incoming_char=='#'){
      delay(50);
      clave_recibida="";
      clave_recibida+=incoming_char;
      for(int x=0;x<4;x++){
        delay(50);
        clave=SIM900.read();
        clave_recibida+=clave;
      }
      while ( SIM900.available() > 0) SIM900.read(); // Limpia el buffer de entrada
      ingreso_codigo(clave_recibida);
    }
  }
  // FIN RECEPCION DE DATOS POR SMS
  delay(200); //parada para estabilizar flujo

}



void encenderSIM900(){
  if (enviarAT("AT", "OK") == 0){
    digitalWrite(9, HIGH);
    delay(1000);
    digitalWrite(9, LOW);
  }
  iniciarSIM900();
}

void iniciarSIM900(){
  enviarAT("AT+CLIP=1\r", "OK"); // Activamos la identificacion de llamadas
  enviarAT("AT+CMGF=1\r", "OK"); //Configura el modo texto para enviar o recibir mensajes
  enviarAT("AT+CNMI=2,2,0,0,0\r", "OK"); //Configuramos el modulo para que nos muestre los SMS recibidos por comunicacion serie
}


int enviarAT(String ATcommand, char* resp_correcta){
  int x = 0;
  bool correcto = 0;
  char respuesta[100];
  while ( SIM900.available() > 0) SIM900.read(); // Limpia el buffer de entrada
  memset(respuesta, '\0', 100); // Inicializa el string
  delay(100);
  SIM900.println(ATcommand); // Envia el comando AT
  x = 0;
  while (x<10){
    delay(100);
    respuesta[x] = SIM900.read();
    if (strstr(respuesta, resp_correcta) != NULL){
      correcto = 1;
      break;
    }
    x++;
  }
  return correcto;
}

void ingreso_codigo(String data){
  if (data=="#4044"){ //responde el estado en que se encuentra al cel que le mando sms
    respondo_estado();
  }

  if (data=="#0000"){ //envia un reinicio a la placa arduino
    RESTART;
  }

  if (data=="#1111"){
    digitalWrite(luz, on);
    estado_luz = true;
    tiempo_inicial_luz = millis();
  }

  if (data=="#1110"){
    digitalWrite(luz, off);
    estado_luz = false;
    tiempo_inicial_luz = 0;
  }

  if (data=="A"){
    tiempofinteclado = 0;
    pantalla_lcd = 1;
    mostrar_th(pantalla_lcd);
  }

  if (data=="B"){
    tiempofinteclado = 0;
    pantalla_lcd = 2;
    mostrar_th(pantalla_lcd);
  }

  
}

void mensaje_sms(){
  SIM900.println("AT+CMGF=1\r");
  delay(1500);
  sprintf(aux_str, celu_responder, strlen(sms)); //Numero al que vamos a enviar el mensaje
  //sprintf(aux_str, "AT+CMGS=\"+543794888545\"", strlen(sms)); //Numero al que vamos a enviar el mensaje
  SIM900.println(aux_str);
  delay(1500);
  SIM900.println(sms);
}

void respondo_estado(){
  //float lectura = (analogRead(entrada_energia)/204.6);
  float voltage = map((analogRead(entrada_energia)/204.6),0,5,0,215);
  String mensaje_estado="";
  String mensaje_estadoenergia="";
  if (estado_energia==true){
    mensaje_estadoenergia="En estado normal / " + String(voltage) + "v.";
  }else{
    mensaje_estadoenergia="Sin suministro electrico / " + String(voltage) + "v.";
  }
  mensaje_estado="Energia=" + mensaje_estadoenergia + " \x1A \r\n";
  sms=mensaje_estado.c_str();
  mensaje_sms();
}

void informa_estadoenergia(boolean estado_e){
  String mensaje_estado="";
  String mensaje_estadoenergia="";
  if (estado_e==true){
    mensaje_estadoenergia="Energia restablecida";
  } else {
    mensaje_estadoenergia="Corte de suministro electrico";
  }
  mensaje_estado= mensaje_estadoenergia + " \x1A \r\n";
  sms=mensaje_estado.c_str();
  cel_recibido_at=celu_aviso;
  celu_responder=cel_recibido_at.c_str();  
  delay(50);  
  mensaje_sms();
}

void leer_dht12(){
    sensor_th.leer_th();
    dht12read = true;
    if (isnan(t12) || isnan(h12)) {
      dato_temperatura = "ERR";
      dato_humedad = "ERR";
      dato_sens = "ERR";
      dato_humr = "ERR";      
      dht12read = false;
    }      
    if (dht12read){
      sensor_th.compute_dht();
      dato_temperatura = String(t12);
      dato_humedad = String(h12);
      dato_sens = String(hic12);
      dato_humr = String(dpc12);
    }
    mostrar_th(pantalla_lcd);
}

void mostrar_th(int option){ 
  pantalla.limpiar();
  delay(100);
  if (option==1){
    pantalla.escribir("Temp: " + dato_temperatura + "c ",0,0);
    pantalla.escribir("Hum: " + dato_humedad + "% ",1,0);   
  }
  if (option==2){
    pantalla.escribir("Sens T: " + dato_sens + "c ",0,0);
    pantalla.escribir("Hum R: " + dato_humr + "% ",1,0);       
  }
}
