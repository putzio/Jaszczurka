#include <avr/io.h>
#include<util/delay.h>

#define sPin1[3] PB1 //przód
#define sPin2 PB2 //środek
#define sPin3 PB3 //tył

#define led1 PD2
#define led2 PD3

int step=0;
int pos1=0,pos2=90,pos3=180; 

void pos (int pin, int *angle)
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
    
  //  sei();
  for(;;)
  {
    forward();
    if(pos2>90)
      {
        PORTD |=(1<< led1);
      }
      else
      {
        PORTD &=~(1<< led1);       
      }
    
    _delay_ms(20);
  }
}
