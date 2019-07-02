
#include <Keypad.h> //libreria de teclado

//******Configuracion y mapeo de teclas del teclado*******
const byte ROWS = 4; // 4 filas del teclado
const byte COLS = 4; // 4 columnas del teclado
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {22, 23, 24, 25}; // Entrada en placa para las filas
byte colPins[COLS] = {26, 27, 28, 29}; // Entrada en placa para las columnas

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS ); //mapeo de las teclas del teclado

String entrada = ""; // Contiene la entrada de teclado
unsigned long temporizadorteclado=5000; //timepo para ingresar datos por teclado, si se excede ese tiempo se resetea la entrada
unsigned long tiempofinteclado=0; //almacena el tiempo en el que finalizara el ingreso de datos por teclado


class teclado_pad{
  
  private:

  public:
  
    String leeteclado(){
      char key = keypad.getKey(); // lee teclado
      if (millis()>tiempofinteclado){
        entrada="";
      }
      //***** verifica si se presiono una tecla
      if (key != NO_KEY){
        //speaker_sonidotecla(3000);//envia sonido a la tecla 3000 para beep
        entrada+=key;
        // verifica si es la primer tecla ingresada para que a partir de ese momento comience a correr el temporizador de ingreso de clave
        if (entrada.length()==1){
          tiempofinteclado=millis()+temporizadorteclado;
        }
      }
      return entrada;
    }
  
};
