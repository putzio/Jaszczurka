#include <avr/io.h>
#include<util/delay.h>
#include<stdint.h>
#include<avr/interrupt.h>

#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])
const uint8_t leds[] = {PD2,PD3,PD4,PD5,PD6};
volatile uint8_t t=0;
void TurnOff()
{
  for(int i=0;i<ARRAY_SIZE(leds)-1;i++)
    PORTD&=~(1<<leds[i]);
}

volatile char buf;
volatile bool receved = false;

ISR (TIMER0_COMPA_vect) {
   t++;
}
//256
//16 000 000 / 1024 / 10 = 1562
//1562/8 = 195
int main()
{
  for(int i=0;i<ARRAY_SIZE(leds);i++)
  {
    DDRD|=(1<<leds[i]);
  }

  //Timer0
  TCCR0A = (1<<WGM01);
  TCCR0B = (5<<CS00);
  OCR0A = 195;
  TIMSK0 |= (1<<OCIE0A);
  
  sei();
while (1)
{
  if(t>=8)
  {
    t=0;
    PORTD^=(1<<leds[0]);
  }
 }
}

//ISR (TIMER0_OVF_vect) {
//    t++;
//    
//}
//
//int main()
//{
//  for(int i=0;i<ARRAY_SIZE(leds);i++)
//  {
//    DDRD|=(1<<leds[i]);
//  }
//
//  //Timer0
//  TCCR0A = 0;//NORMAL OPERATION, OC0A disconnected
//  TCCR0B = (5<<CS00);////Prescaller 1024
//  TIMSK0 |= (1<<TOIE0);//INTERRUPT OVERFLOW ENABLE
//  
//  sei();
//while (1)
//{
//  if(t>=30)
//  {
//    t=0;
//    PORTD^=(1<<leds[0]);
//  }
// }
//}
