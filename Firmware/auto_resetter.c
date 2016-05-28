//Created: 22/05/2016
//Author : bob bogdan@electrobob.com
//http://www.electrobob.com/autoresetrrr/
//no fuse changes required, the device runs with default settings

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#define TIME_BEFORE_RESET_IN_SECONDS 86400 //one day
#define TIME_WARN_IN_SECONDS 60	//1 minute is enough for everything to shutdown
#define TIME_RESET_IN_SECONDS 10
#define TIME_WAIT_IN_SHUTDOWN_IN_SECONDS 60 

typedef enum {state_init_reset, state_on, state_warn, state_reset, state_wait_for_shutdown, state_shutdown, state_wait_powerup} auto_res_state;
auto_res_state state_m=state_init_reset;

uint8_t led, sec_cntr;
#define LED_IO PB3
#define LED_PIN PINB3
#define LED_PORT PORTB
#define LED_DDR DDRB
#define GATE_PIN PB4
#define GATE_PORT PORTB

uint16_t minutes=0;
uint32_t seconds;
uint8_t seconds_t;
uint8_t mode;
uint8_t led_is_pulled_down=0;

//interrupt every 0.2 seconds
ISR(TIM0_COMPA_vect){
	sec_cntr++;
	if(sec_cntr>4) {
		sec_cntr=0;
		seconds++;
	}
	//blink some led
	if(seconds & 0x00000001){
		LED_DDR |= (1<<LED_IO); //make output
		LED_PORT &= ~(1<<LED_IO); //LED_ON
	}
	else{ //led OFF
		LED_PORT |= 1<<LED_IO; //enable pull up
		LED_DDR &= ~(1<<LED_IO); //make input
		
		//if in the proper states, check if the pin is pulled low externally
		if((state_m == state_on) || (state_m == state_wait_powerup)){
			if(!(PINB & (1<<LED_IO))) { //check if pulled down
				led_is_pulled_down=1;
			}
		}
	}
};

int main(void)
{
	// Port B setup
	DDRB=(1<<DDB5) | (1<<DDB4) | (1<<DDB3) | (1<<DDB2) | (1<<DDB1) | (1<<DDB0);
	PORTB=(0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);

	// Timer/Counter 0, system clock, 1.172Khz, period 0.20053s
	TCCR0A=(0<<COM0A1) | (0<<COM0A0) | (0<<COM0B1) | (0<<COM0B0) | (1<<WGM01) | (0<<WGM00);
	TCCR0B=(0<<WGM02) | (1<<CS02) | (0<<CS01) | (1<<CS00);
	TCNT0=0x00;
	OCR0A=0xEA;
	OCR0B=0x00;

	// Timer/Counter 0 Interrupt(s) initialization
	TIMSK0=(0<<OCIE0B) | (1<<OCIE0A) | (0<<TOIE0);

	//External int disabled
	GIMSK=(0<<INT0) | (0<<PCIE);
	MCUCR=(0<<ISC01) | (0<<ISC00);

	// AC off
	ACSR=(1<<ACD) | (0<<ACBG) | (0<<ACO) | (0<<ACI) | (0<<ACIE) | (0<<ACIS1) | (0<<ACIS0);
	ADCSRB=(0<<ACME);
	DIDR0=(0<<AIN0D) | (0<<AIN1D);

	// ADC disabled
	ADCSRA=(0<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (0<<ADPS2) | (0<<ADPS1) | (0<<ADPS0);

	sei();
	
	while (1)
	{
		//initial reset lasts TIME_RESET_IN_SECONDS minutes
		if(state_m==state_init_reset)
		{
			GATE_PORT &= ~(1<<GATE_PIN); //Device OFF
			LED_DDR |= (1<<LED_IO); //make output
			LED_PORT &= ~(1<<LED_IO); //led ON
			if(seconds>=TIME_RESET_IN_SECONDS) state_m=state_on;
		}
		if(state_m==state_on)
		{
			GATE_PORT |= 1<<GATE_PIN; //make sure device is ON
			//LED blinks because of interrupt
			if(seconds >= (TIME_BEFORE_RESET_IN_SECONDS-TIME_WARN_IN_SECONDS)) state_m=state_warn;
			if(led_is_pulled_down){
				led_is_pulled_down=0;
				state_m = state_wait_for_shutdown;
				seconds_t = seconds;
			}
		}
		if(state_m==state_warn){
			GATE_PORT |= 1<<GATE_PIN; //make sure device is ON
			LED_DDR |= (1<<LED_IO); //make output
			LED_PORT &= ~(1<<LED_IO); //led ON permanently
			if(seconds >=TIME_BEFORE_RESET_IN_SECONDS)
			{
				state_m=state_init_reset;
				GATE_PORT &= ~(1<<GATE_PIN); //Device OFF
				LED_DDR &= ~(1<<LED_IO); //make input
				LED_PORT |= 1<<LED_IO; //enable pull up
				seconds=0;
				minutes=0;
			}
		}
		if(state_m == state_wait_for_shutdown)
		{
			LED_DDR |= (1<<LED_IO); //make output
			LED_PORT &= ~(1<<LED_IO); //led ON
			//practically wait a warning time before cutting power
			if(seconds_t <= seconds-TIME_WARN_IN_SECONDS) state_m=state_shutdown;
		}
		if(state_m == state_shutdown) //power off indefinetelly
		{
			GATE_PORT &= ~(1<<GATE_PIN); //Device OFF
			//wait some time
			seconds_t = seconds;
			while(seconds_t <= seconds-TIME_WARN_IN_SECONDS);
			state_m=state_wait_powerup;
			//then be able to startup again if pulled down
			
		}
		if(state_m == state_wait_powerup) //wait for a button press for powerup
		{
			if(led_is_pulled_down) { //check if pulled down
				led_is_pulled_down=0;
				state_m = state_init_reset;
				seconds=0;
			}
		}
	}
}

