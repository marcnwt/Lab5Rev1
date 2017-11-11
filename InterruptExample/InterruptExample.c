

#include <avr/interrupt.h>
#include <avr/io.h>


//Defines 

#define bit_get(p,m) ((p) & (m))
#define bit_set(p,m) ((p) |= (m))
#define bit_clear(p,m) ((p) &= ~(m))
#define bit_flip(p,m) ((p) ^= (m))
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define BIT(x) (0x01 << (x))
#define LONGBIT(x) ((unsigned long)0x00000001 << (x))

// Global Variable 
volatile unsigned char ADC_result_H;
volatile unsigned char ADC_result_L;
volatile unsigned int ADC_result_flag;
volatile unsigned int ADC_result;

volatile char STATE;

void adcSetup(void);
void inputOutputSetup(void);
void interruptsSetup(void);
void pwm(void);
void dcMotorDriver(void);
void motorBrake(void);


void mTimer(int count);


int main(){

adcSetup();
inputOutputSetup();
interruptsSetup();
pwm();
ADC_result = 0; 

	STATE = 0;


	goto POLLING_STAGE;

	// POLLING STATE
	POLLING_STAGE: 
		PORTC = 0x0F;	// Indicates this state is active

		int brakeSwitch;
		brakeSwitch = (PIND & 0X01);
		
		if(brakeSwitch){

			motorBrake();
		}
		else{

			dcMotorDriver();
		}

		switch(STATE){
			case (0) :
				goto POLLING_STAGE;
				break;	//not needed but syntax is correct
			case (1) :
				goto MAGNETIC_STAGE;
				break; 
			case (2) :
				goto REFLECTIVE_STAGE;
				break;
			case (4) :
				goto BUCKET_STAGE;
				break;
			case (5) :
				goto END;
			default :
				goto POLLING_STAGE;
		}//switch STATE
		

	MAGNETIC_STAGE:
		// Do whatever is necessary HERE
		PORTC = 0x01; // Just output pretty lights know you made it here
		//Reset the state variable
		STATE = 0;
		goto POLLING_STAGE;

	REFLECTIVE_STAGE: 
		
		if (ADC_result_flag)
        {
            ADC_result_flag = 0x00;
            ADCSRA |= _BV(ADSC); //Start conversion

		}
		//Display result - Create display function 
		PORTC = ADC_result_H; 
		PORTD = (ADC_result_L >> 6);

		// int tempADC_Result =  (ADC_result_L >> 6) | (ADC_result_H << 2);

		// if (tempADC_Result > ADC_result){

		// 	ADC_result = tempADC_Result;

		// 	//Display result - Create display function 
		// 	PORTC = ADC_result_H; 
		// 	PORTD = (ADC_result_L >> 6);

		// }


		//Reset the state variable
		STATE = 0;
		goto POLLING_STAGE;
	
	BUCKET_STAGE:
		// Do whatever is necessary HERE
		PORTC = 0x04;
		//Reset the state variable
		STATE = 0;
		goto POLLING_STAGE;
		
	END: 
		// The closing STATE ... how would you get here?
		PORTC = 0xF0;	// Indicates this state is active
		// Stop everything here...'MAKE SAFE'
	return(0);

}


/* Set up the External Interrupt 0 Vector */

ISR(INT2_vect){
	/* Toggle PORTA bit 0 */
	//Gets toggled by the reflective optical sensor 
	STATE = 2;
}

ISR(INT3_vect){
	/* Toggle PORTA bit 3 */
STATE = 4;
}

// If an unexpected interrupt occurs (interrupt is enabled and no handler is installed,
// which usually indicates a bug), then the default action is to reset the device by jumping 
// to the reset vector. You can override this by supplying a function named BADISR_vect which 
// should be defined with ISR() as such. (The name BADISR_vect is actually an alias for __vector_default.
// The latter must be used inside assembly code in case <avr/interrupt.h> is not included.

ISR(BADISR_vect)
{
    // user code here
}


void adcSetup(void)
{

    cli(); // disable all of the interrupt ==========================

    // config ADC =========================================================
	// ADC channel 1 ADC1 / PORTF1	

    ADCSRA |= _BV(ADEN);              // enable ADC
    ADCSRA |= _BV(ADIE);              // enable interrupt of ADC
    ADMUX |= _BV(ADLAR) | _BV(REFS0); //ADC data register Left adjust (8 bits in ADCH and 2 bits in ADCL) | Avcc with external cap on Aref pin
    ADMUX |= 0b00000001;              //Select ADC channel 1 ADC1 / PORTF1

    // sets the Global Enable for all interrupts ==========================
    sei();

    // initialize the ADC, start one conversion at the beginning ==========
    ADCSRA |= _BV(ADSC);
}

ISR(ADC_vect)
{
    ADC_result_L = ADCL;
	ADC_result_H = ADCH;
    ADC_result_flag = 1;

}

void inputOutputSetup(void)
{
    //Input for light
    //DDRD = 0x00;	//Input
	//DDRC = 0xFF;	//LED output

	// Going to set up interrupt0 on PD0
	DDRD = 0b11110110;
	DDRC = 0xFF;	// just use as a display




}

void interruptsSetup(void){

	
		cli();	// Disables all interrupts
	
		// Set up the Interrupt 0,3 options
		//External Inturrupt Control Register A - EICRA (pg 94 and under the EXT_INT tab to the right
		// Set Interrupt sense control to catch a rising edge


		EICRA |=  _BV(ISC01) | _BV(ISC00);//Rising edge trigger.

		// EICRA |=  _BV(ISC01);
		// EICRA &= ~_BV(ISC00);
	
		//	EICRA &= ~_BV(ISC01) & ~_BV(ISC00); /* These lines would undo the above two lines */
		//	EICRA &= ~_BV(ISC31) & ~_BV(ISC30); /* Nice little trick */
	
	
		// See page 96 - EIFR External Interrupt Flags...notice how they reset on their own in 'C'...not in assembly
		EIMSK |= 0x09; // Enables interrupts 0 and 3.
	
		// Enable all interrupts
		sei();	// Note this sets the Global Enable for all interrupts


}




void mTimer(int count)
{

    int i = 0;

    TCCR1B |= _BV(CS10); // Sets bit 0 of the timer counter control register to 1: enables timer with no prescaling
    TCCR1B |= _BV(WGM12); //Enables clear timer on compare mode by writing a 1 to to the WGM12 bit (bit 3) of the TCCR1B register
    OCR1A = 0x03E8; //Sets the output compare register to 0x03E8 == 1000 in decimal.
    TCNT1 = 0x0000; //Sets initial value of the timer to 0
    //TIMSK1 |= _BV(OCIE1A); //Sets the OCIE1A bit to 1 in the timer interrupt mask register; enables the timer output compare interrupt
    TIFR1 |= _BV(OCF1A); //OCF1A is automatically cleared when the output compare interrupt is triggered, writing a 1 to this bit
                         // also does this
    while (i < count)
    {
        if ((TIFR1 & 0x02) == 0x02)
        {
            TIFR1 |= _BV(OCF1A);
            i++;
        }
    }

    return;
}

void pwm(void){

	TCCR0A |= _BV(WGM01);
	TCCR0A |= _BV(WGM00);

	TCCR0A |= _BV(COM0A1);
	TCCR0B |= _BV(CS01);
	DDRB |= _BV(PB7);
	0CR0A = 0X7F;

}


void dcMotorDriver(void){
	PORTB = (PORTB & 0b111000) | 0b0000100;

}

void motorBrake(void){

	PORTB = (PORTB & 0b111000) | 0b0000000;

}