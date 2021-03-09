#include <avr/io.h>
#include<util/delay.h>
#include<stdint.h>

#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])
const uint8_t leds[] = {PD2,PD3,PD4,PD5,PD6};

#define RX PD0
//do uart
#define FOSC 12000000UL                //czestotliwosc zegara 12MHz
#define BAUD 9600UL                   //szybkosc transmisji
#define MYUBRR ((FOSC+8UL*BAUD)/16UL/BAUD)-1       //obliczenie UBRR//+8*BAUD ->żeby dobrze działało zakrąglanie

void USART_Init( unsigned int ubrr)
{
    /* ustawienie baud */
    UBRR0H = (unsigned char)(ubrr>>8);
    UBRR0L = (unsigned char)ubrr;
    /* odblokowanie transmisji i retransmisji */
    UCSR0B = (1<<RXEN0);//|(1<<TXEN0);
    /* Ustawienie parametrów ramki: 8data bit, 1stop bit, można tu próbować kombinować */
    UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);//(3<<UCSZ00)
}

void TurnOff()
{
  for(int i=0;i<ARRAY_SIZE(leds)-1;i++)
    PORTD&=~(1<<leds[i]);
}

//ISR(USART_RXC_vect)        //przerwanie od odbioru danej 
//{ 
//    static char a;        //zmienna pomocnicza 
//    a = UDR;            //zapis odebranej danej 
//    a ^= 0xff;                //operacja bitowa XOR 
//    UDR = a;            //wysłanie danej zwrotnej 
//}

//Funkcja do wyświetlenia wartości chara na diodach 
void show(char sign){
   TurnOff();
   PORTD |= ((sign>>2)&(15<<2)); 
  _delay_ms(1000);
  TurnOff();
  PORTD |= ((sign<<2)&(15<<2));
  _delay_ms(1000);
}

int main()
{
  for(int i=0;i<ARRAY_SIZE(leds);i++)
  {
    DDRD|=(1<<leds[i]);
  }
  DDRD&=~(1<<RX);
    
//    sei();

//UART ze strony
USART_Init ( MYUBRR );         //wywolanie inicjalizacji UART
char test = 'P';//P: 0101 0000
show(test);
while (1)
{
  //błąd ramki
  if (bit_is_set(UCSR0A, FE0))
    {
      PORTD |= (1<<leds[4]);
      _delay_ms(1000);
    }    
  else
   PORTD&=~(1<<leds[4]);
   
  
  if(bit_is_set(UCSR0A, RXC0))     //jeśli są do odebrania dane 
  {
    char uart = UDR0;            //  zapisz dane do zmiennej
    if(uart >= 0 && uart<= 127) //TAblica ASCII
   {        
      show (uart);
 
  //Sprawdzenie, czy uart działa
////  if (bit_is_set(UCSR0A, DOR0))
////    {
//      PORTD |= (1<<leds[1]);//działa na razie przy wejściu w pętlę
////    }
//  if(uart>40)
//    {
//      PORTD |= (1<<leds[2]);
//    }
//  if(uart=='B')
//    {
//      PORTD |= (1<<leds[3]);
//    }
//    
//  _delay_ms(500);
  
  }
  TurnOff();
  }
//_delay_ms(20);
  }
}
