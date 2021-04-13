#include <avr/io.h>
#include<util/delay.h>
#include<stdint.h>
#include<avr/interrupt.h>

//do uart
#define FOSC 16000000                //czestotliwosc zegara 16MHz, bo to ten drugi, którego nie widać
#define BAUD 9600                   //szybkosc transmisji
#define MYUBRR ((FOSC)/16/BAUD)-1       //obliczenie UBRR

#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])  //makro zwracające rozmiar tablicy

//zmienne globalne
const uint8_t servo[] = {PB1,PB2,PB3};  //przód, środek, tył
volatile uint8_t pos[] ={0,90,180};     //tablica przechowująca poozycje kolejnych serw (0,1,2)
volatile uint8_t step=0;                //zmienna krok używana do znalezienia odpowiedniej instrukcji ustawienia serwo. 

//tablice, w których zapisane są kolejności kroków dla poszczególnych rodzajów ruchu
const uint8_t forward []  = {0,1,2,3,4};
const uint8_t backward[]  = {0,3,2,1,4};
const uint8_t left    []  = {0,1,2,3,4,1,2,7,8};
const uint8_t right   []  = {0,3,4,1,2,3,4,5,6};

//tablica przechowująca kolejność kroków dla obecnie wykonywanego ruchu. 
//Ostatnia wartość w tablicy odpowiada rozmiarowi obecnie wykonywanej tablicy instrukcji (np. dla forward m[9]=5) 
uint8_t mtab    [10];

volatile char buf;               //zmienna do odbioru danych
volatile bool received = false;  //flaga do informowania o otrzymaniu nowych danych

const uint8_t leds[] = {PD2,PD3,PD4,PD5,PD6};//tablica ledów

//TIMER0
#define time 20 //ms
#define inttime FOSC*time/1000/1024 //liczba, którą normalnie wpisalibyćmy do rejestru OCR0A, jeżeli nie byłby za mały//time/1000 -> s (Hz)
#define intnumber inttime/256 //liczba wykonywanych przerwań znim zostanie urucho iona właściwa funkcja

volatile uint8_t t=0;//time

//przerwanie do odbioru danych
ISR (USART_RX_vect) {
    char rec=UDR0;//wpisanie nowych danych chara
    if(rec != buf)//sprawdzenie, czy się nie zmieniły
      {
        buf = rec;//przypisanie odebranych danych do buffera
        received=true;//Informacja do głównego programu, że przyszło coś nowego
      }
}

ISR (TIMER0_COMPA_vect) {
   t++;
}

//deklaracje funkcji:

 //funkcja inicjalizująca USART
void USART_Init( unsigned int ubrr);

//funkcja inicjalizacja PWM
void PWM_Init();

//Funkcja Inicjalizująca Timer 0, który odpowiada za aktualizowanie pozycji jaszczurki
void Timer0Init();

//funkcja ustawiająca serva w żądanej pozycji
void posit (uint8_t *angle);

//funkcja ustawiająca odpowiedni kąt i poruszająca servo za pomocą posit()
void move(uint8_t tab[],uint8_t size);

void ChangeMoveType(char c);//funkcja zmieniająca instrukcję do kolejnych kroków

void TurnOff();//funkcja wyłączająca ledy

void CurStep();//funkcja podająca wartość stp na diodach


int main()
{
  for(int i=0;i<ARRAY_SIZE(leds);i++)//ustawienie ledów jako wyjście
  {
    DDRD|=(1<<leds[i]);
  }
  PWM_Init();//wywolanie inicjalizacji PWM
  USART_Init ( MYUBRR );//wywolanie inicjalizacji UART
  Timer0Init();
  
  sei();//uruchomienie przerwań

while (1)
{
  if(t>=intnumber)
    {
      t=0;
     // CurStep();
      move (mtab,mtab[9]);//funkcja ustawiająca odpowiedni kąt i poruszająca servo za pomocą posit()
    }
  
  if(received)//jeżeli zostały odebrane dane
    {
      TurnOff();//wyłączenie diod
      ChangeMoveType(buf);
      received = false;//ustawienie flagi na false, ponieważ już zmieniliśmy dane na nowe w tabeli mtab[]
    }
  }
}

void Timer0Init()
{
  TCCR0A = (1<<WGM01);  //CTC mode
  TCCR0B = (5<<CS00);   //prescaler 1024
  OCR0A = 255;// max = 256
  TIMSK0 |= (1<<OCIE0A);
}

void USART_Init( unsigned int ubrr)
{
    /* ustawienie baud */
    UBRR0H = (unsigned char)(ubrr>>8);
    UBRR0L = (unsigned char)ubrr;
    /* odblokowanie odbiory i przerwań do jego obsługi//i transmisji */
    UCSR0B = (1<<RXEN0)|(1<<RXCIE0);//|(1<<TXEN0);
    /* Ustawienie parametrów ramki: 8data, 2stop bit */
    UCSR0C = (1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);//(3<<UCSZ0), bo 3 to w dwójkowym 11, czyli na 1 jest UCSZ0 i UCSZ1
}

//inicjalizacja PWM
void PWM_Init(){
  //piny servo jako wyjścia
  for (int i =0;i<ARRAY_SIZE(servo);i++)
    DDRB |= (1<<servo[i]);
    
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
}

//funkcja ustawiająca serva w żądanej pozycji
void posit (uint8_t *angle)
{ 
  int range [] = {8,38};//Zmierzone, żeby działały serwo od 0 do 180*

  if (angle[0]>=0&&angle[0]<=180)//pierwsze servo
    {
       OCR1A = angle[0]*(range[1]-range[0])/180+range[0];
    }

  if (angle[1]>=0&&angle[1]<=180)//drugie servo
    {
       OCR1B = angle[1]*(range[1]-range[0])/180 + range[0];
    }
  if (angle[2]>=0&&angle[2]<=180)//trzecie servo
    {
       OCR2A = angle[2]*(range[1]-range[0])/180+range[0];
    }
}

//funkcja ustawiająca odpowiedni kąt i poruszająca servo za pomocą posit()
void move(uint8_t tab[],uint8_t size)
{
  
  if(step+1>size)//jeżeli step wykroczył poza tablicę
      step = 1;

  switch(tab[step])//case w zależności od kroku w podanej tablicy
    {
      case 0:   //zerowanie
        {
            pos[0]=90;
            pos[1]=90;
            pos[2]=90;
            TurnOff();
            
            step++;            
            break;        
         }
      case 1:   //obrót środka
        {
          pos[1]++;
          if(pos[1]>=105)
            {
              step++;
            }
            PORTD |=(1<<leds[1]);
           break;
        }
      case 2:   // prawa przód, lewa tył 
        {
           pos[0]++;
           pos[2]--;
        
          if(pos[2] <=60 && pos[0]>=120)
            {
              step++;
            }
          break;
        }
      case 3:   //obrót środka w drugą
        {
          TurnOff();
          pos[1]--;
          
          if(pos[1] <= 75)
            {
              step++;
            }
          break;    
        }
      case 4:   // lewa przód,  prawa tył 
        {
          pos[0]--;
          pos[2]++;
      
          if(pos[0] <=60 && pos[2]>=120)
            {
              step++;
            }
          break;
        }
        case 5:   //zerowanie
        {
            pos[1]--;
            if(pos[1]<=90)
              step++;
            break;     
        }
      case 6:   //zerowanie nóg
        {
            pos[0]--;
            pos[2]++;
            if(pos[0]<=90&&pos[2]>=90)     
              step++;
            break;
        }
      case 7:   //zerowanie
        {
            pos[1]++;
            if(pos[1]>=90)
              step++;
            break;     
        }
      case 8:   //zerowanie nóg
        {
            pos[0]++;
            pos[2]--;
            if(pos[0]>=90&&pos[2]<=90)     
             step++;
            break;
        }
      default:
        {
          step++;
        } 
    }
      posit (&pos[0]);  
}

void ChangeMoveType(char c)
{
  step = 0;
  switch (c)//przypisanie odpowiednich wartości do tabeli mtab[] w zależności od otrzymanych danych
        {
          case 'f':
            {
              for (int i = 0;i<ARRAY_SIZE(forward);i++)
                mtab[i]=forward[i];
              mtab[9] = ARRAY_SIZE(forward);
              PORTD |=(1<<leds[1]);
              break;
            }
          case 'b':
            {
              for (int i = 0;i<ARRAY_SIZE(backward);i++)
                mtab[i]=backward[i];
              mtab[9] = ARRAY_SIZE(backward);
              PORTD |=(1<<leds[2]);
              break;
            }
          case 'l':
            {
              for (int i = 0;i<ARRAY_SIZE(left);i++)
                mtab[i]=left[i];
              mtab[9] = ARRAY_SIZE(left);
              PORTD |=(1<<leds[3]);
              break;
            }
          case 'r':
            {
              for (int i = 0;i<ARRAY_SIZE(right);i++)
                mtab[i]=right[i];
              mtab[9] = ARRAY_SIZE(right);
              PORTD |=(1<<leds[4]);
              break;
            }
          default:
            {
              mtab [9] = 1;
              PORTD |=(1<<leds[0]);
            }
        }
}

void TurnOff()//funkcja wyłączająca ledy
{
  for(int i=0;i<ARRAY_SIZE(leds)-1;i++)
    PORTD&=~(1<<leds[i]);
}

void CurStep()//funkcja podająca wartość stp na diodach
{
  TurnOff();
  PORTD |= (step<<2);
}
