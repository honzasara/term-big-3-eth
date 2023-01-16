
#include <ezButton.h>

ezButton::ezButton(gpio_num_t pin) {
	btnPin = pin;
	debounceTime = 0;
	count = 0;
	countMode = COUNT_FALLING;

	previousSteadyState = gpio_get_level(btnPin);
	lastSteadyState = gpio_get_level(btnPin);
	lastFlickerableState = gpio_get_level(btnPin);

	lastDebounceTime = 0;
}

void ezButton::setDebounceTime(unsigned long time) {
	debounceTime = time;
}

int ezButton::getState(void) {
	return lastSteadyState;
}

int ezButton::getStateRaw(void) {
	return gpio_get_level(btnPin);
}

bool ezButton::isPressed(void) {
	if(previousSteadyState == HIGH && lastSteadyState == LOW)
		return true;
	else
		return false;
}

bool ezButton::isReleased(void) {
	if(previousSteadyState == LOW && lastSteadyState == HIGH)
		return true;
	else
		return false;
}

void ezButton::setCountMode(int mode) {
	countMode = mode;
}

unsigned long ezButton::getCount(void) {
	return count;
}

void ezButton::resetCount(void) {
	count = 0;
}

void ezButton::loop(void) {
	// read the state of the switch/button:
        currentState = gpio_get_level(btnPin);

	// check to see if you just pressed the button
	// (i.e. the input went from LOW to HIGH), and you've waited long enough
	// since the last press to ignore any noise:

	// If the switch/button changed, due to noise or pressing:
	if (currentState != lastFlickerableState) {
		// reset the debouncing timer
		lastDebounceTime = millis();
		// save the the last flickerable state
		lastFlickerableState = currentState;
	}

	if ((millis() - lastDebounceTime) >= debounceTime) {
		// whatever the reading is at, it's been there for longer than the debounce
		// delay, so take it as the actual current state:

		// save the the steady state
		previousSteadyState = lastSteadyState;
		lastSteadyState = currentState;
	}

	if(previousSteadyState != lastSteadyState){
		if(countMode == COUNT_BOTH)
			count++;
		else if(countMode == COUNT_FALLING){
			if(previousSteadyState == HIGH && lastSteadyState == LOW)
				count++;
		}
		else if(countMode == COUNT_RISING){
			if(previousSteadyState == LOW && lastSteadyState == HIGH)
				count++;
		}
	}
}

