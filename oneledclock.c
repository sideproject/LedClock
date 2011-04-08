#define F_CPU 14745600

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

#include "../libnerdkits/delay.h"
#include "../libnerdkits/uart.h"
#include "../libnerdkits/lcd.h"
#include <avr/sleep.h>


//http://www.cs.mun.ca/~rod/Winter2007/4723/notes/avr-intr/avr-intr.html

volatile uint32_t the_time_ms = 0U;
volatile uint32_t the_counter = 0U;
volatile uint8_t fire = 0U;

void blink_for(int16_t ms) {
	PORTC |= (1<<PC4);
	delay_ms(ms);

	PORTC &= ~(1<<PC4);
	delay_ms(ms);
}
void blink_n_times(int8_t n) {
	int8_t i = 0;
	for (i = 0; i < n; i++) {
		blink_for(255);
	}
}
void blink_number(int8_t tens, int8_t ones) {

	if (tens == 0)
		blink_for(500);
	else
		blink_n_times(tens);
	
	delay_ms(255);
	
	if (ones == 0)
		blink_for(500);
	else
		blink_n_times(ones);
}

//get_ms(21,30,0); //9:30 PM
int32_t get_ms(int32_t hour, int32_t minute, int32_t second) {
	return ( (60U * 60U * hour) + (minute * 60U ) + second ) * 1000U;
}

// this code will be called anytime that PCINT18 switches (hi to lo, or lo to hi)
ISR(PCINT1_vect) {
	fire = 1U;
}

void tell_time() {
	FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, 0, _FDEV_SETUP_WRITE);
	
	int32_t x = the_time_ms;
	uint32_t seconds = x / 1000U;
	
	// lcd_clear_and_home();
	// lcd_line_one();
	// fprintf_P(&lcd_stream, PSTR("%lu"), x);
	// lcd_line_two();
	// fprintf_P(&lcd_stream, PSTR("%lu"), seconds);
	// delay_ms(1000);
	// return;
	
	uint8_t seconds_ten = (seconds % 60) / 10;
	uint8_t seconds_one = (seconds % 60) % 10;		
	
	uint16_t hours = seconds / 60U / 60U;
	uint8_t hours_ten = hours  / 10U;
	uint8_t hours_one = hours  % 10U;
	if (hours < 10U) {
		hours_ten = 0U;
		hours_one = hours;
	}
	
	uint16_t minutes = seconds - (hours * 60U * 60U);
	minutes /= 60U;
	uint8_t minutes_ten = minutes / 10U;
	uint8_t minutes_one = minutes % 10U;		
	if (minutes < 10U) {
		minutes_ten = 0U;
		minutes_one = minutes;
	}

	lcd_clear_and_home();
	
	lcd_line_one();
	fprintf_P(&lcd_stream, PSTR("%u%u:%u%u:%u%u"), hours_ten, hours_one, minutes_ten, minutes_one, seconds_ten, seconds_one);
	//fprintf_P(&lcd_stream, PSTR("%lu"), the_time_ms);
		
	blink_number(hours_ten, hours_one);
	delay_ms(1000);
	blink_number(minutes_ten, minutes_one);
	
	fire = 0U;
	lcd_clear_and_home();
}

/*
//stolen from: http://www.uchobby.com/index.php/2007/11/24/arduino-interrupts/
#define TIMER_CLOCK_FREQ 2000000.0 //2MHz for /8 prescale from 16MHz

//Setup Timer2.
//Configures the ATMega168 8-Bit Timer2 to generate an interrupt
//at the specified frequency.
//Returns the timer load value which must be loaded into TCNT2
//inside your ISR routine.
//See the example usage below.
unsigned char SetupTimer2(float timeoutFrequency){
  unsigned char result; //The timer load value.

  //Calculate the timer load value
  result=(int)((257.0-(TIMER_CLOCK_FREQ/timeoutFrequency))+0.5);
  //The 257 really should be 256 but I get better results with 257.

  //Timer2 Settings: Timer Prescaler /8, mode 0
  //Timer clock = 16MHz/8 = 2Mhz or 0.5us
  //The /8 prescale gives us a good range to work with
  //so we just hard code this for now.
  TCCR2A = 0;
  TCCR2B = 0<<CS22 | 1<<CS21 | 0<<CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 = 1<<TOIE2;

  //load the timer for its first cycle
  TCNT2=result;

  return(result);
}

//Timer2 overflow interrupt vector handler
ISR(TIMER2_OVF_vect) {
  //Toggle the IO pin to the other state.
  digitalWrite(TOGGLE_IO,!digitalRead(TOGGLE_IO));

  //Capture the current timer value. This is how much error we
  //have due to interrupt latency and the work in this function
  latency=TCNT2;

  //Reload the timer and correct for latency.
  TCNT2=latency+timerLoadValue;
}
*/


void SetupTimer2(){

	//Timer2 Settings: Timer Prescaler /1024, mode 0
	TCCR2A = 0;
	
	//@14.7456MHz: 9Hz = 1us
	//@14.7456MHz: 1Hz = 1/9 = 0.11111111us
	
	//14,745,600Hz / 1024 = 14,400Hz
	//@14,400Hz: 1Hz = 0.0001085069444444444443359375us
	//@14,400Hz: 1us = 1/0.0001085069444444444443359375us ~= 9216Hz
	//9216Hz / 255 = 36 (255 because Timer2 is an 8-bit counter)
	
	TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20; //set 1024 prescaler
	//TCCR2B = 1<<CS22 | 1<<CS20 | 0<<CS20; //set 256 prescaler
	//TCCR2B = 1<<CS22 | 1<<CS20 | 1<<CS20; //set 128 prescaler
	//TCCR2B = 1<<CS22 | 0<<CS21 | 0<<CS20; //set 64 prescaler
	//TCCR2B = 0<<CS22 | 1<<CS21 | 1<<CS20; //set 32 prescaler
	//TCCR2B = 0<<CS22 | 1<<CS21 | 0<<CS20; //set 8 prescaler

	//Timer2 Overflow Interrupt Enable
	TIMSK2 = 1<<TOIE2;

	//load the timer for its first cycle
	TCNT2=0;
}


//Timer2 overflow interrupt vector handler
ISR(TIMER2_OVF_vect) {
	//prescaler math: http://www.atmel.com/dyn/resources/prod_documents/doc2505.pdf
	
	//(14,745,600 / 1024) / 256 = 56.25 (divide by 256 because timer2 is an 8bit timer)
	//there will be 56.25 overflows each second
	//1,000ms / 56.25 overflows = 17.7778ms each overflow
	
	the_counter++;	
	the_time_ms += 17U;
	
	if (the_counter >= 9U) {  //make up remainder .777778 * 9 ~= 6.9999993
		the_time_ms += 7U;
		the_counter = 0U;
	}
	
	if (the_time_ms >= 86399000U) {
		the_time_ms = 0U;
	}
	
}

int main() {
	//enable internal pullup resistors
	PORTC |= (1 << PC3); //buttons

	// Pin change interrupt control register - enables interrupt vectors
	// Bit 2 = enable PC vector 2 (PCINT23..16)
	// Bit 1 = enable PC vector 1 (PCINT14..8)
	// Bit 0 = enable PC vector 0 (PCINT7..0)
	PCICR |= (1 << PCIE1);

	// Pin change mask registers decide which pins are enabled as triggers
	PCMSK1 |= (1 << PCINT11);

	// LED as output
	DDRC |= (1<<PC4);

	SetupTimer2();
	
	lcd_init();

	//~ //start up the serial port
	//~ uart_init();
	//~ FILE uart_stream = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
	//~ stdin = stdout = &uart_stream;
	//~ delay_ms(333);

	printf_P(PSTR("STARTING\r\n"));
	//~ FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, 0, _FDEV_SETUP_WRITE);

	//the_time_ms = 9000000U;
	//the_time_ms = get_ms(0,34,0);
	the_time_ms = get_ms(1,25,15);

	while (1) {

		//set_sleep_mode(SLEEP_MODE_IDLE);
		set_sleep_mode(SLEEP_MODE_PWR_SAVE);
		//set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		//set_sleep_mode(SLEEP_MODE_STANDBY);

		cli();
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
		sei();

		if (fire == 1U) {
			tell_time();
		}

	}
	return 0;
}
