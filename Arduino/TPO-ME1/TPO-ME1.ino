/*******************************
 ****     Add library      *****
 *******************************/
#include <LiquidCrystal_I2C.h>
#include <TimerOne.h>
#include "TimerSW.h"

LiquidCrystal_I2C lcd(0x27,16,2);
TimerSW maqTimer;

#define At200mV 1.0
#define At2V    10.0
#define EmV  1000.0
#define EV   1.0  
/*******************************
 **** Analog Read & Filter *****
 *******************************/
const int samples = 30;
float adc_filter = 0.0;
char buffer[6];
const float cuentaMax = 1023;
const float cuentaMin = 2;

const float a = 1.1734;
const float b = -21.299;
#define alpha 0.2

/*******************************
 ***     Encoder Pin         ***
 *******************************/
int SW  = 2;
int DT  = 3;
int CLK = 4;
int ANTERIOR = 1;		
volatile int POSICION = 2;

/*******************************
 ***     Encoder Mux         ***
 *******************************/
uint8_t SEL0 = 8;
uint8_t SEL1 = 7;

struct MiEstructura {
  char buffer[6];
  float At;
  float Av;
  float Escala;
};
MiEstructura Datos;

float R1 = 10000.0;
float R2 = 470000.0;
float R34 = 9000.0;
float R58 = 1002.0; 
float At = (R34+R58)/R58;
float Ga = (1 + R2/R1); 

void setup() {
  lcd.init();                 // inicializa el LCD 
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Iniciando ......");
  Serial.begin(9600);
  Serial.print("Iniciando ......");

  pinMode(DT, INPUT);
  pinMode(CLK, INPUT);	

  pinMode(SEL0, OUTPUT);
  pinMode(SEL1, OUTPUT);

  digitalWrite(SEL0, HIGH);
  digitalWrite(SEL1, LOW);

  Datos.At = At2V;
  Datos.Av = Ga;
  Datos.Escala = EV;

  Timer1.initialize(10000);
  Timer1.attachInterrupt(blink);
  attachInterrupt(digitalPinToInterrupt(DT), encoder, LOW);
  maqTimer.TimerStart(1, 1, adcRead, BASE_DEC);
  maqTimer.TimerStart(3,100, DisplayLCD, BASE_DEC);
}

void loop(){
  maqTimer.TimerEvent();
}

void blink(void)
{
  maqTimer.AnalizarTimers();
}

void adcRead(){
  static int aux = 0;
  static float adc_raw = 0.0;
  float Cuentas = analogRead(A0);
  float Vrms = 0;
  adc_raw = adc_raw + Cuentas;
  if(aux > samples){
    adc_raw = adc_raw/samples;
    Vrms = Datos.Escala*(filter(adc_raw) * 5.0/1024)*Datos.At/Datos.Av;
    if(Datos.Escala == EV ){
      if(Vrms > 1.10){
        strcpy(Datos.buffer, "- - - "); 
      } else{
        if(Vrms < 0 )
          Vrms = 0;
        dtostrf(Vrms, 6, 3, Datos.buffer);  // 6 = ancho total, 2 = número de decimales
      }
    } else {
  /***************** Ajuste de calibración ****************
  ***/Vrms = a * Vrms + b;/********************************
  ********************************************************/
      if(Vrms > 104){
        strcpy(Datos.buffer, "- - - ");
      } else{
        if(Vrms < 0 )
          Vrms = 0;
        dtostrf(Vrms, 6, 1, Datos.buffer);  // 6 = ancho total, 2 = número de decimales
      }
    }
    adc_raw = 0;
    aux = 0;
  }
  aux++;    
  maqTimer.TimerStart(1,1, adcRead, BASE_DEC); 
}

void encoder()  {
  static unsigned long ultimaInterrupcion = 0;	
  unsigned long tiempoInterrupcion = millis();
  if (tiempoInterrupcion - ultimaInterrupcion > 5) {
    if (digitalRead(CLK) == HIGH)
    {
      POSICION++ ;				
    }
    else {				
      POSICION-- ;
    }
    POSICION = min(2, max(1, POSICION));	// establece limite inferior de 0 y
						// superior de 100 para POSICION
    maqTimer.TimerStart(4, 2, Escala, DEC);
    ultimaInterrupcion = tiempoInterrupcion;	// guarda valor actualizado del tiempo
  }						// de la interrupcion en variable static
}

void Escala(){
  lcd.clear();
  switch (POSICION){
    case 1: 
      digitalWrite(SEL0, LOW);
      digitalWrite(SEL1, LOW);
      Serial.println("Escala 100mV");
      lcd.setCursor(0,0);
      lcd.print("  Escala 100mV  "); 
      Datos.Escala = EmV;
      Datos.At = At200mV;
      break;
    case 2: 
      digitalWrite(SEL0, HIGH);
      digitalWrite(SEL1, LOW);
      Serial.println("Escala de 1V");
      lcd.setCursor(0,0);
      lcd.print("   Escala 1V    "); 
      Datos.Escala = EV;
      Datos.At = At2V;
      break;

  }
}

void DisplayLCD(){
  lcd.setCursor(0,0);
  lcd.print(" VOLTIMETRO RMS "); 
  lcd.setCursor(0,1);
  lcd.print("   ");
  lcd.print(Datos.buffer);
  Serial.println(Datos.buffer);
  if(Datos.Escala == EV ){
    lcd.setCursor(11,1);
    lcd.print(" Vrms");
  } else {
    lcd.setCursor(11,1);
    lcd.print("mVrms");
  }
  maqTimer.TimerStart(3,100, DisplayLCD, DEC);
}

float filter( float adc_raw) {
 	adc_filter = (alpha*adc_raw) + ((1-alpha)*adc_filter);
 	return adc_filter;
}
