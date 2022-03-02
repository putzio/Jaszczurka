#define F_CPU 16000000UL
#include <avr/io.h>
#include<util/delay.h>
#include<stdint.h>
#include<avr/interrupt.h>
#include<stdbool.h>

//do uart
#define FOSC 16000000                //czestotliwosc zegara 16MHz, bo to ten drugi, kt�rego nie wida�
#define BAUD 9600                   //szybkosc transmisji
#define MYUBRR ((FOSC)/16/BAUD)-1       //obliczenie UBRR

#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])  //makro zwracaj�ce rozmiar tablicy

//zmienne globalne
const uint8_t servo[] = {PB1,PB2,PB3};  //prz�d, �rodek, ty�
volatile uint8_t pos[] ={0,90,180};     //tablica przechowuj�ca poozycje kolejnych serw (0,1,2)
volatile uint8_t step=0;                //zmienna krok u�ywana do znalezienia odpowiedniej instrukcji ustawienia serwo.

//tablice, w kt�rych zapisane s� kolejno�ci krok�w dla poszczeg�lnych rodzaj�w ruchu
const uint8_t forward []  = {0,1,2,3,4};
const uint8_t backward[]  = {0,3,2,1,4};
const uint8_t left    []  = {0,1,2,3,4,1,2,7,8};
const uint8_t right   []  = {0,3,4,1,2,3,4,5,6};

//tablica przechowuj�ca kolejno�� krok�w dla obecnie wykonywanego ruchu.
//Ostatnia warto�� w tablicy odpowiada rozmiarowi obecnie wykonywanej tablicy instrukcji (np. dla forward m[9]=5)
uint8_t mtab    [10];

volatile char buf;               //zmienna do odbioru danych
volatile bool received = false;  //flaga do informowania o otrzymaniu nowych danych

const uint8_t leds[] = {PD2,PD3,PD4,PD5,PD6};//tablica led�w

//TIMER0
#define time 20 //ms
#define inttime FOSC*time/1000/1024 //liczba, kt�r� normalnie wpisaliby�my do rejestru OCR0A, je�eli nie by�by za ma�y//time/1000 -> s (Hz)
#define intnumber inttime/256 //liczba wykonywanych przerwa� znim zostanie urucho iona w�a�ciwa funkcja

volatile uint8_t t=0;//time

//przerwanie do odbioru danych
ISR (USART_RX_vect) {
	char rec=UDR0;//wpisanie nowych danych chara
	if(rec != buf)//sprawdzenie, czy si� nie zmieni�y
	{
		buf = rec;//przypisanie odebranych danych do buffera
		received=true;//Informacja do g��wnego programu, �e przysz�o co� nowego
	}
}

ISR (TIMER0_COMPA_vect) {
	t++;
}

//deklaracje funkcji:

//funkcja inicjalizuj�ca USART
void USART_Init( unsigned int ubrr);

//funkcja inicjalizacja PWM
void PWM_Init();

//Funkcja Inicjalizuj�ca Timer 0, kt�ry odpowiada za aktualizowanie pozycji jaszczurki
void Timer0Init();

//funkcja ustawiaj�ca serva w ��danej pozycji
void posit (uint8_t *angle);

//funkcja ustawiaj�ca odpowiedni k�t i poruszaj�ca servo za pomoc� posit()
void move(uint8_t tab[],uint8_t size);

void ChangeMoveType(char c);//funkcja zmieniaj�ca instrukcj� do kolejnych krok�w

void TurnOff();//funkcja wy��czaj�ca ledy

void CurStep();//funkcja podaj�ca warto�� stp na diodach


int main()
{
	for(int i=0;i<ARRAY_SIZE(leds);i++)//ustawienie led�w jako wyj�cie
	{
		DDRD|=(1<<leds[i]);
	}
	PWM_Init();//wywolanie inicjalizacji PWM
	USART_Init ( MYUBRR );//wywolanie inicjalizacji UART
	Timer0Init();
	
	sei();//uruchomienie przerwa�

	while (1)
	{
		if(t>=intnumber)
		{
			t=0;
			// CurStep();
			move (mtab,mtab[9]);//funkcja ustawiaj�ca odpowiedni k�t i poruszaj�ca servo za pomoc� posit()
		}
		
		if(received)//je�eli zosta�y odebrane dane
		{
			TurnOff();//wy��czenie diod
			ChangeMoveType(buf);
			received = false;//ustawienie flagi na false, poniewa� ju� zmienili�my dane na nowe w tabeli mtab[]
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
	/* odblokowanie odbiory i przerwa� do jego obs�ugi//i transmisji */
	UCSR0B = (1<<RXEN0)|(1<<RXCIE0);//|(1<<TXEN0);
	/* Ustawienie parametr�w ramki: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);//(3<<UCSZ0), bo 3 to w dw�jkowym 11, czyli na 1 jest UCSZ0 i UCSZ1
}

//inicjalizacja PWM
void PWM_Init(){
	//piny servo jako wyj�cia
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
	//Wybieramy Fast PWM z du�ej tabeli
	TCCR2A |= (1<<WGM20)|(1<<WGM21);
	//Preskaler /1024
	TCCR2B |= ( 1<<CS22) | ( 1<<CS21) | (1<<CS20);
}

//funkcja ustawiaj�ca serva w ��danej pozycji
void posit (uint8_t *angle)
{
	int range [] = {8,38};//Zmierzone, �eby dzia�a�y serwo od 0 do 180*

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

//funkcja ustawiaj�ca odpowiedni k�t i poruszaj�ca servo za pomoc� posit()
void move(uint8_t tab[],uint8_t size)
{
	
	if(step+1>size)//je�eli step wykroczy� poza tablic�
	step = 1;

	switch(tab[step])//case w zale�no�ci od kroku w podanej tablicy
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
		case 1:   //obr�t �rodka
		{
			pos[1]++;
			if(pos[1]>=105)
			{
				step++;
			}
			PORTD |=(1<<leds[1]);
			break;
		}
		case 2:   // prawa prz�d, lewa ty�
		{
			pos[0]++;
			pos[2]--;
			
			if(pos[2] <=60 && pos[0]>=120)
			{
				step++;
			}
			break;
		}
		case 3:   //obr�t �rodka w drug�
		{
			TurnOff();
			pos[1]--;
			
			if(pos[1] <= 75)
			{
				step++;
			}
			break;
		}
		case 4:   // lewa prz�d,  prawa ty�
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
		case 6:   //zerowanie n�g
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
		case 8:   //zerowanie n�g
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
	switch (c)//przypisanie odpowiednich warto�ci do tabeli mtab[] w zale�no�ci od otrzymanych danych
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

void TurnOff()//funkcja wy��czaj�ca ledy
{
	for(int i=0;i<ARRAY_SIZE(leds)-1;i++)
	PORTD&=~(1<<leds[i]);
}

void CurStep()//funkcja podaj�ca warto�� stp na diodach
{
	TurnOff();
	PORTD |= (step<<2);
}
