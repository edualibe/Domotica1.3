class speaker{

  private:

    int pin_spk;

  public:

    speaker(int pin_spkp){
      pin_spk = pin_spkp;
    }

    void sonido(int tono, int tiempo){
      tone(pin_spk, tono, tiempo);
    }
  
    void sonidoinicio(){
      for (int x=0;x<3;x++){
        delay(100);
        tone(pin_spk, 1900);
        delay(500);
        noTone(pin_spk);
      }
    }

    void sonido_sinenergia(){
      for (int x=0;x<3;x++){
        delay(100);
        tone(pin_spk, 2100);
        delay(800);
        noTone(pin_spk);
      }      
    }

};
