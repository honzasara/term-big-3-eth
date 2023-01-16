
#ifndef ezButton_h
#define ezButton_h

#include "driver/gpio.h"
#include "main.h"

#define COUNT_FALLING 0
#define COUNT_RISING  1
#define COUNT_BOTH    2

#define HIGH 1
#define LOW 0

class ezButton
{
	private:
		gpio_num_t btnPin;
		unsigned long debounceTime;
		unsigned long count;
		int countMode;

		int previousSteadyState;  // the previous steady state from the input pin, used to detect pressed and released event
		int lastSteadyState;      // the last steady state from the input pin
		int lastFlickerableState; // the last flickerable state from the input pin
		int currentState;         // the current reading from the input pin

		unsigned long lastDebounceTime; // the last time the output pin was toggled

	public:
		ezButton(gpio_num_t pin);
		void setDebounceTime(unsigned long time);
		int getState(void);
		int getStateRaw(void);
		bool isPressed(void);
		bool isReleased(void);
		void setCountMode(int mode);
		unsigned long getCount(void);
		void resetCount(void);
		void loop(void);
};

#endif
