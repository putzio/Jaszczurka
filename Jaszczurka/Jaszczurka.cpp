#include <avr/io.h>
#include<util/delay.h>
#include<stdint.h>

#define sPin1 PB1 //przód
#define sPin2 PB2 //środek
#define sPin3 PB3 //tył

#define led1 PD2
#define led2 PD3

#define RX PD0
#define TX PD1

//do uart
#define FOSC 16000000                //czestotliwosc zegara 16MHz nie pytajcie dlaczego nie 12MHz
#define BAUD 9600                   //szybkosc transmisji
#define MYUBRR (FOSC/16/BAUD)-1       //obliczenie UBRR

uint8_t step=0;
uint8_t pos1=0,pos2=90,pos3=180; 

void USART_Init( unsigned int ubrr)
{
    /* ustawienie baud */
    UBRR0H = (unsigned char)(ubrr>>8);
    UBRR0L = (unsigned char)ubrr;
    /* odblokowanie transmisji i retransmisji */
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);
    /* Ustawienie parametrów ramki: 8data, 2stop bit */
    UCSR0C = (1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);//(3<<UCSZ0), bo 3 to w dwójkowym 11, czyli na 1 jest UCSZ0 i UCSZ1
}

//ISR(USART_RXC_vect)        //przerwanie od odbioru danej 
//{ 
//    static char a;        //zmienna pomocnicza 
//    a = UDR;            //zapis odebranej danej 
//    a ^= 0xff;                //operacja bitowa XOR 
//    UDR = a;            //wysłanie danej zwrotnej 
//}

int main()
{
  DDRB |= (1<<sPin1);
  DDRB |= (1<<sPin2);
  DDRB |= (1<<sPin3);

  DDRD |= (1<< led1);
  DDRD |= (1<< led2);
  
  //Fast PWM (5); Chosen output mode (non inverting)
  TCCR1A |= (1<<COM1A1)|(1<<COM1B1);
  //Waveform generation bit (fast PWM 10 bit - 1023)
  TCCR1A |= (1<<WGM10);
  TCCR1B |= (1<<WGM12);

  //PRESCALER 12MHz/1024=12kHz
  TCCR1B |= (1<<CS10) |(1<<CS12);


  
  //Dla Timera 2
  TCCR2A |= (1<<COM2A1);

  //Wybieramy Fast PWM z dużej tabeli
  TCCR2A |= (1<<WGM20)|(1<<WGM21);
  
  //Preskaler /1024
  TCCR2B |= ( 1<<CS22) | ( 1<<CS21) | (1<<CS20); 
  
  
  
    
  //  1:30 - 0
  //  2:58 - 180
    
//    sei();

////UART od Dorny
//UDR0 = 0x67;          //ustawienie prędkości transmisji na 9600 baud
//UCSR0C |= (1<<UCSZ01) | (1<< UCSZ00);  //ustawienie liczby bitów przesyłanej informacji na 8
//UCSR0B |= (1<<RXEN0);     //ustawienie Atmegi jako odbiornika
//char uart = 0;        //zmienna pomocnicza

DDRD |= (1<<RX);
PORTD |= (1<<TX);


//UART ze strony
USART_Init ( MYUBRR );         //wywolanie inicjalizacji UART
char uart=0;
bool done;


while (1)
{
  
  if(bit_is_set(UCSR0A, RXC0))     //jeśli są do odebrania dane 
  {
    uart = UDR0;            //  zapisz dane do zmiennej
    done=false;
    if(uart >= 49 && uart<= 127)           
{
  if(uart=='A')
    PORTD |= (1<<led1);
  else
   // PORTD &= ~(1<<led1);
    
  if ((int)uart>(int) 'Z')
    PORTD |= (1<<led1);
  else
   PORTD &= ~(1<<led1);
}  
  }



  _delay_ms(20);
  }
}

void pos (uint8_t pin, uint8_t *angle)
{ 
  int range [] = {8,38};//Zmierzone, żeby działały serwo od 0 do 180*
  switch (pin)
    {
      case sPin1:
        {
          if (*angle>=0&&*angle<=180)
          {
             OCR1A = *angle*(range[1]-range[0])/180+range[0];
          }
          break;   
        }  
  
      case sPin2:
        {
          if (*angle>=0&&*angle<=180)
          {
             OCR1B = *angle*(range[1]-range[0])/180+range[0];
          }
          break;
        }  
      
      case sPin3:
        {
          if (*angle>=0&&*angle<=180)
          {
             OCR2A = *angle*(range[1]-range[0])/180+range[0];
          }
          break;
        }  
      
      defaullt:
        {
        }
    }
}


void forward()
{
  switch(step)
    {
      case 0:   //zerowanie
        {
            pos1=90;
            pos2=90;
            pos3=90;
            
            step=1;
            break;        
         }
      case 1:   //obrót środka
        {
          pos2=105;
          if(pos2>=105)
            {
              step=2;
            }
           break;
        }
      case 2:   // prawa przód, lewa tył 
        {
           pos1++;
           pos3--;
        
          if(pos3 <=45 && pos1>=135)
            {
            step=3;
            }
          break;
        }
      case 3:   //obrót środka w drugą
        {
          pos2=75;
          
          if(pos2 <= 75);
            {
              step=4;
            }
          break;    
        }
      case 4:   // lewa przód,  prawa tył 
        {
          pos1--;
          pos3++;
      
          if(pos1 <=45 && pos3>=135)
            {
              step = 1;
            }
          break;
        }
    
      
      default:
        {
          step = 1; 
        } 
    }
  pos (sPin1, &pos1);  
  pos (sPin2, &pos2);  
  pos (sPin3, &pos3);
}

void backward()
{
  switch(step)
    {
      case 0:   //zerowanie
        {
            pos1=90;
            pos2=90;
            pos3=90;
            
            step=1;
            break;        
         }
      case 1:   //obrót środka
        {
          pos2=75;
          if(pos2>=75)
            {
              step=2;
            }
           break;

        }
      case 2:   // prawa przód, lewa tył 
        {
           pos1++;
           pos3--;
        
          if(pos3 <=45 && pos1>=135)
            {
            step=3;
            }
          break;
        }
      case 3:   //obrót środka w drugą
        {
      pos2=105;
          
          if(pos2 >= 105);
            {
              step=4;
            }
       
          break;    
        }
      case 4:   // lewa przód,  prawa tył 
        {
          pos1--;
          pos3++;
      
          if(pos1 <=45 && pos3>=135)
            {
              step = 1;
            }
          break;
        }
    
      
      default:
        {
          step = 1; 
        } 
    }
  pos (sPin1, &pos1);  
  pos (sPin2, &pos2);  
  pos (sPin3, &pos3);
}









void links( )
{
  switch(step)
    {
      case 0:   //zerowanie
        {
            pos1=90;
            pos2=90;
            pos3=90;
            
            step=1;
            break;        
         }
      case 1:   //obrót środka w prawo
        {
          pos2++;
          if(pos2>=105)
            {
              step=2;
            }
           break;

        }
      case 2:   // prawa przednia noga, lewa tylnia noga 
        {
           pos1++;
           pos3--;
        
          if(pos3 <=45 && pos1>=135)
            {
            step=3;
            }
          break;
        }
      case 3:   //obrót środka w lewo
        {
      pos2++;
          
          if(pos2<= 75);
            {
              step=4;
            }
       
          break;    
        }
      case 4:   // lewa noga przednia,  prawa noga tylnia  
        {
          pos1--;
          pos3++;
      
          if(pos1 <=45 && pos3>=135)
            {
              step = 5;
            }
          break;
        }

 case 5:   //obrót środka w prawo
        {
          pos2++;
          if(pos2>=105)
            {
              step=6;
            }
           break;

        }
      case 6:   // prawa noga przednia, lewa noga tylnia 
        {
           pos1++;
           pos3--;
        
          if(pos3 <=45 && pos1>=135)
            {
            step=7;
            }
          break;
        }
  case 7:   //zerowanie
        {
  pos2--;
if(pos2<=90)
            step=0;
            break;
        }     
  case 8:   //zerowanie nóg
        {
            pos1--;
            pos3++;
            if(pos1<=90&&pos3<=90)     
            step=0;
  break;
 }


  pos (sPin1, &pos1);  
  pos (sPin2, &pos2);  
  pos (sPin3, &pos3);
    }
}

void rechts( )
{
  switch(step)
    {
      case 0:   //zerowanie
        {
            pos1=90;
            pos2=90;
            pos3=90;
            
            step=1;
            break;        
         }
      case 1:   //obrót środka wlewo
        {
          pos2--;
          if(pos2<=75)
            {
              step=2;
            }
           break;

        }
      case 2:   // prawa tylnia noga, lewa przednia noga 
        {
           pos1--;
           pos3++;
        
          if(pos3 >=135 && pos1<=45)
            {
            step=3;
            }
          break;
        }
      case 3:   //obrót środka w prawo
        {
      pos2++;
          if(pos2>= 105);
            {
              step=4;
            }
       
          break;    
        }
      case 4:   // lewa noga tylna,  prawa noga przednia 
        {
          pos1++;
          pos3--;
      
          if(pos3 <=45 && pos1>=135)
            {
              step = 5;
            }
          break;
        }

 case 5:   //obrót środka w lewo
        {
          pos2--;
          if(pos2<=75)
            {
              step=6;
            }
           break;

        }
      case 6:   // lewa noga przednia, prawa noga tylna 
        {
           pos1--;
           pos3++;
        
          if(pos1 <=45 && pos3>=135)
            {
            step=7;
            }
          break;
        }
  case 7:   //zerowanie
        {
            pos2++;
            if(pos2>=90)
            step=0;
            break;     
        }
  case 8:   //zerowanie nóg
        {
            pos1++;
            pos3--;
            if(pos1>=90&&pos3<=90)     
            step=0;
  break;
 }


  pos (sPin1, &pos1);  
  pos (sPin2, &pos2);  
  pos (sPin3, &pos3);

}
}
