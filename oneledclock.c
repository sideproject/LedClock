#define F_CPU 14745600

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

#include "../libnerdkits/delay.h"
#include "../libnerdkits/lcd.h"
#include <avr/sleep.h>

//http://www.cs.mun.ca/~rod/Winter2007/4723/notes/avr-intr/avr-intr.html
//http://www.uchobby.com/index.php/2007/11/24/arduino-interrupts/

volatile uint32_t the_time_ms = 0UL;
volatile uint32_t the_counter = 0UL;
volatile uint8_t fire = 0U;

void blink_for(uint16_t ms) {
	PORTC |= (1<<PC4);
	delay_ms(ms);

	PORTC &= ~(1<<PC4);
	delay_ms(ms);
}
void blink_n_times(uint8_t n) {
	int8_t i = 0U;
	for (i = 0U; i < n; i++) {
		blink_for(255U);
	}
}
void blink_number(uint8_t tens, uint8_t ones) {

	if (tens == 0U)
		blink_for(500U);
	else
		blink_n_times(tens);
	
	delay_ms(255U);
	
	if (ones == 0U)
		blink_for(500U);
	else
		blink_n_times(ones);
}

//get_ms(21,30,0); //9:30 PM
int32_t get_ms(uint32_t hour, uint32_t minute, uint32_t second) {
	return ( (60UL * 60UL * hour) + (minute * 60UL ) + second ) * 1000UL;
}

// this code will be called anytime that PCINT18 switches (hi to lo, or lo to hi)
ISR(PCINT1_vect) {
	fire = 1U;
}

void tell_time() {
	FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, 0, _FDEV_SETUP_WRITE);
	
	int32_t x = the_time_ms;
	uint32_t seconds = x / 1000UL;
	
	// lcd_clear_and_home();
	// lcd_line_one();
	// fprintf_P(&lcd_stream, PSTR("%lu"), x);
	// lcd_line_two();
	// fprintf_P(&lcd_stream, PSTR("%lu"), seconds);
	// delay_ms(1000);
	// return;
	
	uint8_t seconds_ten = (seconds % 60UL) / 10UL;
	uint8_t seconds_one = (seconds % 60UL) % 10UL;
	
	uint16_t hours = seconds / 60UL / 60UL;
	uint8_t hours_ten = hours  / 10U;
	uint8_t hours_one = hours  % 10U;
	if (hours < 10U) {
		hours_ten = 0U;
		hours_one = hours;
	}
	
	uint16_t minutes = seconds - (hours * 60UL * 60UL);
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
		
	delay_ms(1000U);
	blink_number(minutes_ten, minutes_one);
	
	fire = 0U;
	lcd_clear_and_home();
}

void SetupTimer2(){

	//Timer2 Settings: Timer Prescaler /1024, mode 0
	TCCR2A = 0;
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
	the_time_ms += 17UL;
	
	if (the_counter >= 9U) {  //make up remainder .777778 * 9 ~= 6.9999993
		the_time_ms += 7UL;
		the_counter = 0U;
	}
	
	if (the_time_ms >= 86399000UL) { //24 hours
		the_time_ms = 0U;
	}
	
}

int main() {
	//enable internal pullup resistors
	PORTC |= (1 << PC3); //button

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
	
	the_time_ms = get_ms(15UL,43UL,30UL);

	while (1) {

		//set_sleep_mode(SLEEP_MODE_IDLE);
		//set_sleep_mode(SLEEP_MODE_ADC);
		//set_sleep_mode(SLEEP_MODE_STANDBY);
		set_sleep_mode(SLEEP_MODE_PWR_SAVE);
		//set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		
		cli();
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
		sei();

		if (fire == 1U) {
			fire = 0U;
			tell_time();
		}

	}
	return 0;
}
