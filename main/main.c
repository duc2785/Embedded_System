#define F_CPU 8000000UL
#include "main.h"
#include "stdio.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdbool.h>


unsigned long encoder = 0;
unsigned int PWM1 = 0;
unsigned int PWM2 = 0;
unsigned int CountT2 = 0, CountT0 = 0, Count_bt = 0;
unsigned int Temp1 = 1500;
unsigned int Temp2 = 2500;
char lcd[10];


// gia tri khoi diem
unsigned int V_init = 10;
unsigned int Prpm_init = 100;
float Pump_init = 5;
float P_init = 0;
unsigned int Gpm_lcd;
//
volatile float V = 0; // the tich cua bo dieu ap
unsigned int Rpm_new = 0; // toc do quay cua dong co tren giay
unsigned int  Rpm_cu = 0 ;
unsigned int  Rpm_tmp = 0 ;
unsigned int  Count_tmp = 0 ;
unsigned int  Erpm_new; // toc do quay cua dong co
unsigned int  Erpm_old;
unsigned int Prpm = 0; // toc do quay cua may bom tren phut
bool direct;
volatile float Gpm; // luu luong tieu thu ga tren phut
volatile float Pump = 0.0; // luu luong bom ga tren phut
volatile float P = 0.0; // ap suat trong bo dieu ap
// he so tang giam

unsigned int Pump_rpc = 30;
unsigned int Pump_slc = 15;


void display_label(){
	LCD_Init();
	LCD_Clear();
	// hien thi len man hinh lcd
	LCD_String_xy(0, 0, "Erpm |");
	LCD_String_xy(0, 6, "Prpm |");
	LCD_String_xy(0, 13, "Pa |");
}

int rpm2erpm(unsigned int rpm)
{
	return (rpm * 3.6 * 2);
}
void Calculate_Gpm()
{
	if((Rpm_tmp != Rpm_new))
	{
		if (Rpm_new > 1)
		{
			Erpm_new = rpm2erpm(Rpm_new);
			Erpm_old = rpm2erpm(Rpm_cu);
			Gpm = (Gpm *Erpm_new) / (Erpm_old);
		}
		
	}
	
	Rpm_tmp = Rpm_new;
	Gpm_lcd = Gpm ;
	_delay_ms(3);
}

void pressure_balance()
{
	V += Pump;
	V -= Gpm;
	P = (P_init * V) / V_init; // ap suat moi thu duoc tinh theo cong thuc


	sprintf(lcd,"%d ",Erpm_new);
	LCD_String_xy(1, 0, lcd);
	
	sprintf(lcd,"%d ",Prpm);
	LCD_String_xy(1, 6, lcd);
	
	sprintf(lcd,"%2.1f ", P);
	LCD_String_xy(1, 13, lcd);

	//dieu chinh toc do bom sao cho dat yeu cau ve ap suat
	if (P < (P_init - 0.2)){   // 0.2 la nguong/phuong sai
		direct = false;
		Prpm += Pump_rpc;
		Pump = Prpm / 20;
	}
	if (P > (P_init + 0.2)){
		direct = true;
		Prpm -= Pump_rpc;
		// Pump -= (Pump_init/Prpm_init*Pump_rpc);
		Pump = Prpm / 20;
	}
	if (P == P_init){
		//
	}
	else{
		if (direct == false){
			Prpm += Pump_slc;
			// Pump -= (Pump_init/Prpm_init*Pump_slc);
			Pump = Prpm / 20;
		}
		else{
			Prpm -= Pump_slc;
			// Pump += (Pump_init/Prpm_init*Pump_slc);
			Pump = Prpm / 20;
		}
	}
}

int main(void)
{
	// Port LCD
	DDRA  |= (1<<PA7) | (1<<PA6) | (1<<PA5) | (1<<PA4) | (1<<PA3) | (1<<PA2) | (0<<PA1) | (0<<PA0);
	PORTA |= (0<<PA7) | (0<<PA6) | (0<<PA5) | (0<<PA4) | (0<<PA3) | (0<<PA2) | (0<<PA1) | (0<<PA0);

	// Port Button
	DDRB = 0x00; // PORTB là đầu ra
	PORTB = 0x00;

	// Port Motor DC
	DDRD |= (0<<PD7) | (0<<PD6) | (1<<PD5) | (1<<PD4) | (0<<PD3) | (0<<PD2) | (1<<PD1) | (0<<PD0);
	PORTD = 0x00;

	// Khai báo timer 0 mode normal chia 64 đếm 1ms
	TCCR0 = (1<<CS02) | (0<<CS01) | (1<<CS00);
	TCNT0 = 99;

	// Khai báo timer2 mode normal chia 64 đếm 1ms
	TCCR2 = (1<<CS22) | (0<<CS21) | (0<<CS20);
	TCNT2 = 130;

	// Khai báo fast PWM của timer 1 có chu kỳ mới sử dụng ICR1, chu kỳ 2ms
	TCCR1A |= (1<<COM1A1) | (1<<WGM11) | (0<<WGM10) | (1<<COM1B1);
	TCCR1B |= (1<<WGM13) | (1<<WGM12) | (1<<CS10);
	TCNT1H = 0x00;
	TCNT1L = 0x00;
	ICR1H = 0x3E;
	ICR1L = 0x7F;
	OCR1AH = 0x00;
	OCR1AL = 0x00;
	OCR1BH = 0x00;
	OCR1BL = 0x00;

	TIMSK = (1 << TOIE2) | (1 << TOIE0);
	MCUCR = (1<<ISC01) | (1<<ISC00);

	sei();

	display_label();
	V = V_init;

	while (1)
	{
		Calculate_Gpm();
		pressure_balance();
		_delay_ms(200);
	}

	return 0;
}


ISR(INT0_vect)
{
	/*if(PORTD == (1 << PIND1)){*/
		encoder ++;
		/*}*/
		
		
	}

	ISR (TIMER0_OVF_vect ){
		// dung encoder 200 xung bo chia 20ms
		
		if(CountT0 >= 40)
		{
			CountT0=0;
			Rpm_new = (float)encoder / (334.0 * 800.0) * 1000.0;
			if(Rpm_new > 0 )
			{
				Count_tmp ++;
				if (Count_tmp == 1)
				{
					Rpm_cu = Rpm_new;
					if (PORTD == (0 << PIND1))
					{
						Count_tmp = 0;
					}
					else Count_tmp = 2;
				}
			}
			encoder=0;
		}
		CountT0++;
		TCNT0=99;
	}



	ISR (TIMER2_OVF_vect)
	{
		if(CountT2 >=96)
		{
			CountT2=0;
			
			if(((PINB & (1 << button_up)) == 0) && (PORTD == (1 << PIND1)))
			{
				Speed_up(&PWM1, &PWM2, Temp1, Temp2);
				if(Rpm_new != 0)
				{
					Rpm_cu = Rpm_new ;
				}
				
			}
			
			else if(((PINB & (1 << button_dowm)) == 0) && (PORTD == (1 << PIND1)))
			{
				Speed_down(&PWM1, &PWM2, Temp1, Temp2);
				Rpm_cu = Rpm_new ;
			}
			
			else if((PINB & (1 << button_start)) == 0)
			{
				On_Off(&PWM1, &PWM2, Temp1, Temp2);
				if(PORTD == (0 << PIND1)){
					Gpm = 0;
					Pump = 0.0;
					Prpm = 0;
					P_init = 0;
				}
				else{
					Gpm = 5;
					Pump = 5.0;
					Prpm = 100;
					P_init = 4;
				}
			}
			
			_delay_ms(1);
			OCR1AH= PWM1 >>8 ;
			OCR1AL= PWM1 && 0xFF ;
			
			OCR1BH= PWM2 >>8 ;
			OCR1BL= PWM2 && 0xFF ;
			
		}
		CountT2++;
		TCNT2=130;
		
	}
