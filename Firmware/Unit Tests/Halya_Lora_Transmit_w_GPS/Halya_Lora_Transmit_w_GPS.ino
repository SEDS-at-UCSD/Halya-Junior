#define GPSRX 3
#define GPSTX 4

#define RX2 6
#define TX2 5


#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <TinyGPS.h>
#include <Wire.h>               
#include "HT_SSD1306Wire.h"


SSD1306Wire I2cdisplay(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst
#include <FuGPS.h>


#define RF_FREQUENCY                                915000000 // Hz
#define TX_OUTPUT_POWER                             5        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 256 // Define the payload size here MAX 256

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;

bool lora_idle=true;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );

FuGPS fuGPS(Serial2);
bool gpsAlive = false;


static int progress;


void setup() {
  delay(100);

  Serial.begin(115200);
  Mcu.begin();
  delay(100);

  VextON();
  I2cdisplay.init();
  I2cdisplay.setFont(ArialMT_Plain_10);
  I2cdisplay.clear();
  
  // Initialising the UI will init the I2cdisplay too.

  progress = 0;
  I2cdisplay.clear();
  I2cdisplay.drawProgressBar(0, 32, 120, 10, progress);
  I2cdisplay.setTextAlignment(TEXT_ALIGN_CENTER);
  I2cdisplay.drawString(64, 15, String(progress) + "%");
  I2cdisplay.display();

  txNumber=0;

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 
    
  progress = 50;
  I2cdisplay.clear();
  I2cdisplay.drawProgressBar(0, 32, 120, 10, progress);
  I2cdisplay.setTextAlignment(TEXT_ALIGN_CENTER);
  I2cdisplay.drawString(64, 15, String(progress) + "%");
  I2cdisplay.display();
  delay(100);

  Serial2.begin(9600,SERIAL_8N1,GPSRX,GPSTX);
  // fuGPS.sendCommand(FUGPS_PMTK_SET_NMEA_BAUDRATE_9600);
  // fuGPS.sendCommand(FUGPS_PMTK_SET_NMEA_UPDATERATE_1HZ);
  fuGPS.sendCommand(FUGPS_PMTK_API_SET_NMEA_OUTPUT_RMCGGA);

  progress = 100;
  I2cdisplay.clear();
  I2cdisplay.drawProgressBar(0, 32, 120, 10, progress);
  I2cdisplay.setTextAlignment(TEXT_ALIGN_CENTER);
  I2cdisplay.drawString(64, 15, String(progress) + "%");
  I2cdisplay.display();
  delay(100);

}

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}


void loop()
{
  String GPSinfo = "";
  
  // Valid NMEA message
  if (fuGPS.read())
  {
    // We don't know, which message was came first (GGA or RMC).
    // Thats why some fields may be empty.

    gpsAlive = true;

    GPSinfo += "{SATS:";
    GPSinfo += String(fuGPS.Satellites);

    GPSinfo += ", QUALITY:";
    GPSinfo += String(fuGPS.Quality);

    if (fuGPS.hasFix() == true)
    {
      // Data from GGA message
      GPSinfo += ", LAT:";
      GPSinfo += String(fuGPS.Latitude, 6);
      GPSinfo += ", LONG:";
      GPSinfo += String(fuGPS.Longitude, 6);
      GPSinfo += ", ALT(M):";
      GPSinfo += String(fuGPS.Altitude, 6);

      GPSinfo += ", Accuracy HDOP:";
      GPSinfo += String(fuGPS.Accuracy);

      GPSinfo += "}";
      // Data from GGA or RMC
      //Serial.print("Location (decimal degrees): ");
      //Serial.println("https://www.google.com/maps/search/?api=1&query=" + String(fuGPS.Latitude, 6) + "," + String(fuGPS.Longitude, 6));
    }


    Serial.println(GPSinfo);
    Serial.println(GPSinfo.length());

    I2cdisplay.clear();
    I2cdisplay.setFont(ArialMT_Plain_10);
    I2cdisplay.setTextAlignment(TEXT_ALIGN_LEFT);
    I2cdisplay.drawStringMaxWidth(0, 0, 128,GPSinfo);
    I2cdisplay.display();

    if(lora_idle == true)
    {
      txNumber += 0.01;
      GPSinfo.toCharArray(txpacket,BUFFER_SIZE);
      //sprintf(txpacket,"Hello world number %0.2f",txNumber);  //start a package
    
      //Serial.printf("\r\nsending packet \"%s\" , length %d\r\n",txpacket, strlen(txpacket));

      Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out	
      lora_idle = false;
    }
    Radio.IrqProcess( );
  }

  // Default is 10 seconds
  if (fuGPS.isAlive() == false)
  {
      if (gpsAlive == true)
      {
          gpsAlive = false;
          Serial.println("GPS module not responding with valid data.");
          Serial.println("Check wiring or restart.");
      }
  }

  
  delay(1);
  
}

void OnTxDone( void )
{
	Serial.println("TX done......");
	lora_idle = true;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.println("TX Timeout......");
    lora_idle = true;
}