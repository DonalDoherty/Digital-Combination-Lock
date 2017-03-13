/*
 * main.c
 *
 *  Created on: 2017 Mar 08 14:29:44
 *  Author: Donal Doherty
 *  Student Number: 20071040
 *  Module: Embedded Systems
 *  Assignment: Digital Combination Lock
 */




#include <DAVE.h>                 //Declarations from DAVE Code Generation (includes SFR declaration)

int32_t comb[4] = {1, 2, 3, 4}; //initializing combination as 1234
int32_t input[4] = {0, 0, 0, 0}; //initializing the input as 0000
int32_t inputcounter = 0; //input counter corresponds to array index of input array
int32_t button1; //Status of Push Button 1
int32_t button2; //Status of Push Button 2
int32_t button3; //Status of Push Button 3
int32_t button4; //Status of Push Button 4
int32_t buttonreset; //Status of reset button
int32_t resetflag = 0; //condition set to 1 when reset button is pushed, set back to 0 after lock reset
int32_t lock;	 //Status of the lock
int32_t error; //Status of the error led.
int32_t cooldowncounter = 0; //How many periods the cooldown timer has gone through
int32_t errorcounter = 0; //Indicates number of errors in a single 1 minute grace period before the 5 minute cooldown

//This method clears the timer for inputting a full four didgit combo within 10 seconds
//It also resets the input counter and the input array.
void clear()
{
	 TIMER_Stop ( &INPUT );
	 TIMER_Clear( &INPUT );
	 inputcounter = 0;
	 input[0] = 0;
	 input[1] = 0;
	 input[2] = 0;
	 input[3] = 0;
}
//Interrupt handler for the determining the cooldown given when 3 incorrect attempts are made within 1 minute of each other
//hardware constraints mean my time interval cannot be 1 minute, so i made it 30 seconds and the desired effect only happens when the interrupt has occured twice
void CooldownHandler(void)
{
		//The cooldown counter is initialized at 0, when at 0 it counts the first 30 seconds of the time period after an incorrect input, where 2 more incorrect inputs can be made
		//this then increments the cooldown counter, and starts the cooldown timer again, this time with a value of 1 as the counter
		//when the value is 1 it means the 1 minute period has come to an end, assuming that the cooldown(); method wasnt triggered by the error counter reaching 3, then the error counter resets to 0
		//when the cooldown method is invoked, the cooldown counter is immediately set to 2, triggering the part of the interrupt that controls the 5 minute delay, before this happens 
		if( cooldowncounter == 0)
		{
			cooldowncounter++;
			return;
		}
		if( cooldowncounter == 1)
		{
			TIMER_Stop( &COOLDOWN );
			TIMER_ClearEvent( &COOLDOWN );
			TIMER_Clear( &COOLDOWN );
			errorcounter = 0;
			cooldowncounter = 0;
			return;
		}
		if( cooldowncounter >= 2 && cooldowncounter < 12 )
		{
			errorcounter = 0;
			cooldowncounter++;
		}


}

//Interrupt handler for the timer that flashes the error LED for a second if the wrong input 
void ErrorLedHandler(void)
{
	TIMER_Stop( &ERROR_TIMER );
	DIGITAL_IO_ToggleOutput( &ERROR );
	error = DIGITAL_IO_GetInput( &ERROR );
	TIMER_ClearEvent( &ERROR_TIMER );
	TIMER_Clear( &ERROR_TIMER );
}
//Interrupt handler for the debounce timer, clears timer and resets status of interrupt
void DebounceHandler(void)
{
	TIMER_ClearEvent( &DEBOUNCE );
	TIMER_Clear( &DEBOUNCE );
}

//Interrupt handler for the input timer, clears event and runs the clear method
void InputHandler(void)
{
	TIMER_ClearEvent( &INPUT );
	clear();
}


//checks to see if the input is the same as the combination, if yes, toggles the lock and clears the input, if no, error output is flashed and input is cleared.
void check(void)
{
	if(resetflag == 0)
	{
		if(input[0] == comb[0] && input[1] == comb[1] && input[2] == comb[2] && input[3] == comb[3])
		{
			DIGITAL_IO_ToggleOutput( &LOCK ); //set pin from low to high or vice versa (Default is low)
			lock = DIGITAL_IO_GetInput( &LOCK ); //sets global variable to high or low, for LED indicator in MicroProbe
		}
		else
		{
			errorcounter++;
			TIMER_Start( &COOLDOWN );
			DIGITAL_IO_ToggleOutput( &ERROR );
			error = DIGITAL_IO_GetInput( &ERROR );
			TIMER_Start( &ERROR_TIMER );				
		}
		clear();
	}
	else
	{
		comb[0] = input[0];
		comb[1] = input[1];
		comb[2] = input[2];
		comb[3] = input[3];
		clear();
		resetflag = 0;
	}
}

//Stalls program for 20 miliseconds
void debounce(void)
{
	TIMER_Start( &DEBOUNCE );
	while(!TIMER_GetInterruptStatus( &DEBOUNCE ));
}	

void cooldown(void)
{
	cooldowncounter = 2;
	TIMER_ClearEvent( &COOLDOWN );//stops the counter of the 1 minute grace period, and clears it
	TIMER_Clear( &COOLDOWN );
	TIMER_Start( &COOLDOWN );//starts the timer with a cooldown of 2
	
	//flashes light 3 times
	int i;
	for( i = 0; i < 3; i++)
	{
		TIMER_Start( &ERROR_TIMER );	
		while(!TIMER_GetInterruptStatus( &ERROR_TIMER ));
	}
	
	while(cooldowncounter !=12)
	{
	}
	DIGITAL_IO_SetOutputLow( &ERROR );
	error = DIGITAL_IO_GetInput( &ERROR );
	TIMER_ClearEvent( &COOLDOWN );
	TIMER_Clear( &COOLDOWN );
	TIMER_Stop( &COOLDOWN );
	cooldowncounter = 0;
}

//If the reset button is pressed
void reset(void)
{
	debounce();
	clear();
	DIGITAL_IO_SetOutputHigh( &RESET );
	debounce();
	resetflag = 1;
	DIGITAL_IO_SetOutputLow( &RESET );
}

//takes number of button pressed as argument
void buttonPress(int32_t buttonNum)
{
		TIMER_Start( &INPUT ); //starts 10 seconds timer to finish 4 didgit combo
		debounce();//delay of 20 ms
		
	//all if statements here require the button to still be pressed after the delay of 20ms or else nothing happens
	
		if ( buttonNum ==  1 && button1 == 1 )
		{
			DIGITAL_IO_SetOutputHigh( &DIGITAL_IO_0 );
		}
		else if( buttonNum == 2 && button2 == 1 )
		{
			DIGITAL_IO_SetOutputHigh( &DIGITAL_IO_1 );
		}
		else if( buttonNum == 3 && button3 == 1 )
		{
			DIGITAL_IO_SetOutputHigh( &DIGITAL_IO_2 );
		}
		else if( buttonNum == 4 && button4 == 1 )
		{
			DIGITAL_IO_SetOutputHigh( &DIGITAL_IO_3 );
		}
		debounce(); //delay of 20 ms
		input[inputcounter] = buttonNum; //sets an index of the input array to the number of the button pressed, the index is decided by the input counter
		inputcounter++;
		

	  //Resetting pins
		DIGITAL_IO_SetOutputLow( &DIGITAL_IO_0 );
		DIGITAL_IO_SetOutputLow( &DIGITAL_IO_1 );
		DIGITAL_IO_SetOutputLow( &DIGITAL_IO_2 );
		DIGITAL_IO_SetOutputLow( &DIGITAL_IO_3 );
	
}


int32_t main(void)
{

  DAVE_STATUS_t status;
  status = DAVE_Init();           /* Initialization of DAVE APPs  */

  if(status != DAVE_STATUS_SUCCESS)
  {

    /* Placeholder for error handler code. The while loop below can be replaced with an user error handler. */
    XMC_DEBUG("DAVE APPs initialization failed\n");
    while(1U)
    {

    }
  }

  /* Placeholder for user application code. The while loop below can be replaced with user application code. */
  while(1U)
  {
	  if( button1 == 1 ) //if button1 is pressed, set output to high, add 1 to input array, increment input counter
	  {
			buttonPress(1);
		}
	  if( button2 == 1 ) //if button2 is pressed, set output to high, add 2 to input array, increment input counter
	  {
		  buttonPress(2);
	  }
	  if( button3 == 1 ) //if button3 is pressed, set output to high, add 3 to input array, increment input counter
	  {
		  buttonPress(3);
	  }
	  if( button4 == 1 ) //if button4 is pressed, set output to high, add 4 to input array, increment input counter
	  {
		 	buttonPress(4);
	  }
	  if( buttonreset == 1 )
	  {
		  reset();
	  }
	  if( inputcounter == 4 ) //if 4 buttons have been pushed, reset inputcounter and check input
	  {
			check();
	  }
		if( errorcounter == 3 )
		{
			cooldown();
		}
  }
}
