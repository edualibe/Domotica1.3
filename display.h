#include <LiquidCrystal_I2C.h>

#include <Wire.h>

LiquidCrystal_I2C lcd(0x27,16,2);

class pantallalcd{

  private:  

	public:


    void iniciar(){
      lcd.init(); 
      lcd.backlight();
      lcd.setCursor(0,0);            
    }

    void escribir(String texto, int fila, int columna){
      lcd.setCursor(columna,fila);
      lcd.print(texto);    
    }

    void limpiar(){
      lcd.clear();
    }
		
};
