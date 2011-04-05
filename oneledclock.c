// remote.c
// for NerdKits with ATmega168
// use with Nikon (and possibly Canon) cameras
// NOTE: this file best viewed with 1 tab set to 4 spaces

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

// PIN DEFINITIONS:
// PC5 -- trigger button (pulled to ground when pressed)
// PC4 -- IR LED anode
// PC3 -- Indicator LED anode (turns on when IR LED is flashing)
// PC2 -- Mode switch
// PC0 -- Potentiometer for selecting interval
// PB1 -- Intervaolmeter indicator LED - flashes while waiting for next interval click


// the_time will store the elapsed time in milliseconds (1000 = 1 second)
//
// This variable is marked "volatile" because it is modified
// by an interrupt handler.  Without the "volatile" marking,
// the compiler might just assume that it doesn't change in
// the flow of any given function (if the compiler doesn't
// see any code in that function modifying it -- sounds
// reasonable, normally!).
//
// But with "volatile", it will always read it from memory
// instead of making that assumption.
//volatile int32_t the_time_ms = 0;
volatile int32_t the_time_ms = 0U;
volatile int32_t the_counter = 0U;

void realtimeclock_setup() {
	// setup Timer0:
	//   CTC (Clear Timer on Compare Match mode)
	//   TOP set by OCR0A register
	TCCR0A |= (1<<WGM01);
	// clocked from CLK/1024
	// which is 14745600/1024, or 14400 increments per second
	TCCR0B |= (1<<CS02) | (1<<CS00);
	// set TOP to 143 because it counts 0, 1, 2, ... 142, 143, 0, 1, 2 ...
	// so 0 through 143 equals 144 events
	OCR0A = 143;
	// enable interrupt on compare event (14400 / 144 = 100 per second)
	TIMSK0 |= (1<<OCIE0A);
}

// when Timer0 gets to its Output Compare value,
// one one-hundredth of a second has elapsed (0.01 seconds).
SIGNAL(SIG_OUTPUT_COMPARE0A) {
	//add 10 instead of 1 because the event happens every .01 seconds and we need to track every .001 seconds
	the_time_ms += 10;
	
	if (the_time_ms >= 86399000)
		the_time_ms = 0;
}

void adc_init() {
	// set analog to digital converter for external reference (5v), single ended input ADC0
	ADMUX = 5;

	// set analog to digital converter to be enabled, with a clock prescale of 1/128
	// so that the ADC clock runs at 115.2kHz.
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

	// fire a conversion just to get the ADC warmed up
	ADCSRA |= (1 << ADSC);
}

uint16_t adc_read() {
	// read from ADC, waiting for conversion to finish (assumes someone else asked for a conversion.)
	while (ADCSRA & (1 << ADSC)) {
		// wait for bit to be cleared
	}
	// bit is cleared, so we have a result.

	// read from the ADCL/ADCH registers, and combine the result
	// Note: ADCL must be read first (datasheet pp. 259)
	uint16_t result = ADCL;
	uint16_t temp = ADCH;
	result = result + (temp << 8);

	// set ADSC bit to get the *next* conversion started
	ADCSRA |= (1<<ADSC);
	return result;
}

uint16_t get_sample() {

	uint16_t sample_avg = 0;
	uint16_t sample = 0;
	uint8_t i = 0;

	// take 100 samples and average them
	for (i=0; i<100; i++) {
		sample = adc_read();
		sample_avg = sample_avg + (sample / 100);
	}
	return sample_avg;
}


void blink_for(int16_t n) {
	PORTC |= (1<<PC4);
	delay_ms(n);
	// turn off that LED
	PORTC &= ~(1<<PC4);
	delay_ms(n);
}

void blink_n_times(int8_t n) {
	int8_t i = 0;
	for (i = 0; i < n; i++) {
		blink_for(500);
	}
}

void blink_number(int8_t tens, int8_t ones) {

	if (tens == 0)
		blink_for(1000);
	else
		blink_n_times(tens);
	
	delay_ms(500);
	
	if (ones == 0)
		blink_for(1000);
	else
		blink_n_times(ones);
}

//getTime(21,30,0); //9:30 PM
int32_t getTime(int32_t hour, int32_t minute, int32_t second) {
	return ( (60U * 60U * hour) + (minute * 60U ) + second ) * 1000U;
}


// we have to write our own interrupt vector handler..
ISR(PCINT1_vect) {
// this code will be called anytime that PCINT18 switches
// (hi to lo, or lo to hi)

	lcd_init();
	int32_t x = the_time_ms;
		
	int32_t seconds = x / 1000U;
	//int8_t seconds_ten = seconds / 10;
	//int8_t seconds_one = seconds % 10;		
	
	int16_t hours = seconds / 60LU / 60U;
	int8_t hours_ten = hours  / 10U;
	int8_t hours_one = hours  % 10U;
	if (hours < 10U) {
		hours_ten = 0U;
		hours_one = hours;
	}
	
	int16_t minutes = seconds - (hours * 60U * 60U);
	minutes /= 60U;
	int8_t minutes_ten = minutes / 10U;
	int8_t minutes_one = minutes % 10U;		
	if (minutes < 10U) {
		minutes_ten = 0U;
		minutes_one = minutes;
	}

	lcd_init();
	lcd_home();
	//lcd_write_string(PSTR("ADC: "));
	lcd_line_one();
	lcd_write_int16(hours_ten);
	lcd_line_two();
	lcd_write_int16(hours_one);
	lcd_line_three();
	lcd_write_int16(minutes_ten);
	lcd_line_four();
	lcd_write_int16(minutes_one);
	
	//lcd_line_four();
	//fprintf_P(&lcd_stream, PSTR("%lu"), the_time_ms);
		
	blink_number(hours_ten, hours_one);
	delay_ms(1000);
	blink_number(minutes_ten, minutes_one);
	//blink_number(hours_ten, hours_one);
}


//#define TIMER_CLOCK_FREQ 2000000.0 //2MHz for /8 prescale from 16MHz
//stolen from: http://www.uchobby.com/index.php/2007/11/24/arduino-interrupts/
//Setup Timer2.
//Configures the ATMega168 8-Bit Timer2 to generate an interrupt
//at the specified frequency.
//Returns the timer load value which must be loaded into TCNT2
//inside your ISR routine.
//See the example usage below.
/*unsigned char SetupTimer2(float timeoutFrequency){
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
}*/

#define TIMER_CLOCK_FREQ 14745600.0 
//  1 / ( 14745600 / (1024*256) ) = 1 / 61
//	14745600/262144
//	1/56.25 ~= 1/56

/*unsigned char SetupTimer2(float timeoutFrequency){
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
  TCNT2=0;

  return(result);
}*/

//http://www.cs.mun.ca/~rod/Winter2007/4723/notes/avr-intr/avr-intr.html
unsigned char SetupTimer2(uint8_t clockCnt){
	OCR2A = 143;
	// CTC, OCR2A is top, TCNT2 cleared with OCR2A reached
	// prescaler set to 1024
	//TCCR2A = _BV(WGM21);
	TCCR2A |= (1<<WGM01);
	//TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20);
	TCCR2B |= (1<<CS02) | (1<<CS00);
	// enable compare A interrupts
	TIMSK2 = (1<<OCIE0A); //_BV( OCIE2A );
	return 'a';	
}


/*ISR(TIMER2_OVF_vect) {
   //blink_for(1);
   the_counter+=1;
   if (the_counter >= 4096) { //TODO
		the_counter = 0;
		the_time_ms += 1000;
		//blink_for(1);
	}
   
   	//the_time_ms += 10;
	
	if (the_time_ms >= 86399000)
		the_time_ms = 0;
}*/

/*
 * Invoked every OCR2A * (1024/CLOCK_FREQUENCY) seconds
 */
ISR(TIMER2_COMPA_vect)
{
	//blink_for(1);
    // if ( timer2.skip != 0 ) {
        // timer2.skip--;
        // return;
    // }
    // timer2.skip = timer2.num_skip;
    // timer2.count++;
    // PORTC = (PORTC & SWT_MASK) | (~timer2.count<<4);
	
	the_time_ms += 10;
	
	if (the_time_ms >= 86399000)
		the_time_ms = 0;
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

	/*
	// LEDs as outputs
	DDRC |= (1 << IR_LED_PIN);
	DDRC |= (1 << IR_INDICATOR_LED_PIN);
	DDRC |= (1 << TIMER_INDICATOR_LED_PIN);

	//enable internal pullup resistors
	PORTD |= (1 << TRIGGER_PIN);
	PORTC |= (1 << MODE_SWITCH_PIN);
	PORTC |= (1 << POTENTIOMETER_PIN);
	*/
	// LED as output
	DDRC |= (1<<PC4);
	
	//adc_init();

	//SetupTimer2(44100);
	SetupTimer2(255);

	//realtimeclock_setup();
	//sei(); //turn on interrupt handler
	
	//start up the serial port
	uart_init();
	FILE uart_stream = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);
	stdin = stdout = &uart_stream;
	delay_ms(333);

	printf_P(PSTR("STARTING\r\n"));
	FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, 0, _FDEV_SETUP_WRITE);

	//the_time_ms = 9000000U;
	the_time_ms = getTime(21,30,0);
	//2:30am = [(60 * 60 * 2) + (30 * 60 ) ] * 1000
	//				7200			1800		
	//9000000
	
	//11:59:59 pm = [(60 * 60 * 23) + (59 * 60 ) + 59] * 1000
	//				82800			3540			59		
	//86399000
	
	while (1) {
		
		 PORTC |= (1<<PC4);
		 delay_ms(200);
		 PORTC &= ~(1<<PC4);
		 delay_ms(200);
			 
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

		 PORTC |= (1<<PC4);
		 delay_ms(200);
		 PORTC &= ~(1<<PC4);
		 delay_ms(200);

		delay_ms(5000);
	}
	return 0;
}


