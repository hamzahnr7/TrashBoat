//---- Daftar librari yang digunakan ----//
//---- librari oled lcd ----//
#include "U8glib.h"
//---- librari apc220 ----//            
#include <SoftwareSerial.h> 
//---- librari load cell ----//
#include "HX711.h"

//---- setting load cell ----//
HX711 scale(9, 8); //(DT, SCK)//
       
//---- setting apc220 ----//
SoftwareSerial mySerial(2, 3); 

//---- setting pin motor driver ----//
// 6, relay panel, 4, relay Trash Boat, 7, Relay Conveyor
int SolarCell = 6; // Charger Controller
int Conveyor = 7; // Pin Relay buat ke Conveyor
int TrashBoat = 4; //Trash Boat
int ML = 5; // Motor Kiri
int MR = 6; // Motor Kanan
int spd = 255;
//int IN_4 = 7;

//---- setting oled ----//
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NO_ACK); 

//---- setting sensor arus -----//
int arusPin[3] = {A0, A1, A2}; // A0 untuk Panel Surya, A1 untuk Arduino, A2 untuk Trash Boat
//int arusPin2 = A1; 
//int arusPin3 = A2;
float hasilSensorArus[3] = {0.0, 0.0, 0.0};
float nilaiSensorArus[3] = {0.0, 0.0, 0.0};

//---- setting sensor tegangan ----//
int voltPin[3] = {A3, A4, A5}; // A3 untuk Panel Surya, A4 untuk Baterai Utama, A5 untuk Baterai Cadangan
//int voltPin2 = A4;
//int voltPin3 = A5;
int avgValue = 500;
 
float Vmodul[3] = {0.0, 0.0, 0.0}; 
float hasilSensorTegangan[3] = {0.0, 0.0, 0.0};
float vcc = 4.7; // VCC pada Arduino hasur dikalibrasi ulang
float R1 = 30000.0; //30k
float R2 = 7500.0; //7500 ohm resistor, 
float nilaiSensorTegangan[3] = {0.0, 0.0, 0.0};

//----setting timer ----//
unsigned long interval=1000;
unsigned long previousMillis=0;

//---- variabel data ----//
String dataX = "";
String dataY = "";
int x = 0;
int y = 0;
String textX = "";
String textY = "";
String textBaterai = "";

//---- variabel load cell ----//
float calibration_factor = 1000; // this calibration factor is adjusted according to my load cell
float units;
float ounces;


        
void setup()
{
  Serial.begin(9600);  
  mySerial.begin(9600); 
  //mendefinisikan control pin sebagai output
  pinMode(ML, OUTPUT);
  pinMode(MR, OUTPUT);
  pinMode(Conveyor, OUTPUT);
  pinMode(TrashBoat, OUTPUT);
  pinMode(SolarCell, OUTPUT);
//  pinMode(IN_4, OUTPUT); 

  //---- setting sesnsor tegangan ----//
  for (int i = 0; i < 3; i++){
    pinMode(arusPin[i], INPUT);  
    pinMode(voltPin[i], INPUT);
  }
  
  //---- kalibrasi loadcell jika diperlukan ----//
  
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");

  scale.set_scale();
  scale.tare();  //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

}
void loop(){   
  u8g.firstPage();  
  do 
    {
     draw();      
    }
  while( u8g.nextPage() );
  delay(100);
}

void draw(void) 
{
  u8g.setFont(u8g_font_profont12);        
  u8g.drawStr(0, 10, "ARAH MOTOR : "); 
  
  u8g.drawRFrame(0, 20, 40, 40, 5);     
 
  //---- menerima data setiap beberapa detik sekali ----//
  unsigned long currentMillis = millis();
  if ((unsigned long)(currentMillis - previousMillis) >= interval) {   
 
    
    String dataTransmitter= mySerial.readString();// read the incoming data as string  
    dataTransmitter.trim();  
    if(dataTransmitter != ""){
      Serial.println("Data Terbaca dari Transmitter : "+dataTransmitter);
      
      String flagDataBoat = getValue(dataTransmitter,'*',0);
      //---- membaca databoat, kalau flag 1 berarti valid ----//
      if(flagDataBoat == "1"){
        dataX = getValue(dataTransmitter,'*',1);
        dataY = getValue(dataTransmitter,'*',2);
        x = dataX.toInt();
        y = dataY.toInt();
        textX = "X = "+String(x);
        textY = "Y = "+String(y);

        //Serial.println("dataX : "+dataX+" dataY : "+dataY);
        
       
      } 
      
    }
    
    //---- Nilai Sensor Arus ----//
    for (int i = 0; i < avgValue; i++){
      hasilSensorArus[0] += analogRead(arusPin[0]);
      hasilSensorArus[1] += analogRead(arusPin[1]);
      hasilSensorArus[2] += analogRead(arusPin[2]);
      delay(1);
    }

    for (int i = 0; i < 3; i++){
      hasilSensorArus[i] /= avgValue;
      float voltIn = hasilSensorArus[i] * 5.0 / 1024.0; // 5.0 bisa diganti var vcc
      nilaiSensorArus[i] = (voltIn - 2.5) / 0.066;
    }

    //---- kapasitas baterai ----//
    for(int i = 0; i < 3; i++){
      nilaiSensorTegangan[i] = analogRead(voltPin[i]);      
      Vmodul[i] = (nilaiSensorTegangan[i] * vcc) / 1024.0;
      hasilSensorTegangan[i] = Vmodul[i] / (R2/(R1+R2));
    }
    
//    int kapasitasBaterai = hasilSensorTegangan[1]/random(30, 90);
    int kapasitasBaterai = hasilSensorTegangan[1];
    String dataKirim = "2*"+String(kapasitasBaterai); 
    textBaterai = "Batt = "+String(kapasitasBaterai)+"%";   
    dataKirim.trim();
    mySerial.println(dataKirim); // Ngirim lewat APC   

    //---- load cell ----//
    scale.set_scale(calibration_factor); //Adjust to this calibration factor
    units = scale.get_units(), 1;
    if (units < 0)
    {
      units = 0.00;
    }
    Serial.println("Kapasitas Baterai: "+String(kapasitasBaterai));
    Serial.println("Unit Terbaca Load Cell: "+String(units));
    
    previousMillis = millis();
  }
  
  u8g.setPrintPos(50,52); 
  u8g.println(textBaterai);
  
  u8g.setPrintPos(50,28); 
  u8g.println(textX);
  u8g.setPrintPos(50,40); 
  u8g.println(textY);
  

  if(x<=10 && y!=0){
    u8g.drawStr(80, 10, "KIRI ");
    gerakKiri();
  }
  if(x>=1020 && y!=0){
    u8g.drawStr(80, 10, "KANAN");
    gerakKanan();
  }
  if(y<=10 && x!=0){
    u8g.drawStr(80, 10, "ATAS ");
    gerakMaju();
  }
  if(y>=1020 && x!=0){
    u8g.drawStr(80, 10, "BAWAH");
    digitalWrite(Conveyor, HIGH);
  }
  if(x>=500 && x<=510 && y>=535 && y<=545){
    u8g.drawStr(80, 10, "STOP");
    diam();
  }
  
  int coordinateX1 = map(x, 0, 1023, 2, 36);
  int coordinateY1 = map(y, 0, 1023, 28, 62);
  u8g.setPrintPos(coordinateX1,coordinateY1);                      
  u8g.println("*");
   
}

void gerakMaju(){
 // Untuk Motor Kiri & Kanan Berputas Bersamaan
 analogWrite(ML, spd);
 analogWrite(MR, spd);
 delay(1000);
}

void gerakMundur(){
// Gak Ada Gerak Mundur
 delay(1000); 
}

void gerakKiri(){
// Hanya Motor Kanan yang menyala
 analogWrite(MR, spd);
 delay(1000);  
}

void gerakKanan(){
// Hanya Motor Kiri yang menyala
 analogWrite(ML, spd);
 delay(1000);  
}

void diam(){
// Semua Motor Berhenti
 analogWrite(MR, 0);
 analogWrite(ML, 0);
// delay(1000);  
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
