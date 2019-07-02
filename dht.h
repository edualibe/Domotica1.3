#include <DHT12.h>

#include "Arduino.h"

DHT12 dht12(5, true);
float t12, h12, hic12, dpc12 = 0;


class sensordht12{
  
  private:

  public:
    void iniciar(){
        dht12.begin();
    }

    void leer_th(){
      t12 = dht12.readTemperature();
      h12 = dht12.readHumidity();
      /*return t12, h12;*/
    }

    void compute_dht(){
      // Compute heat index in Celsius (isFahreheit = false)
      hic12 = dht12.computeHeatIndex(t12, h12, false);
      // Compute dew point in Celsius (isFahreheit = false)
      dpc12 = dht12.dewPoint(t12, h12, false);
      /*return hic12, dpc12;*/
    }
  
};
