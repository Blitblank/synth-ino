
#include <Arduino.h>

#include "app.hpp"

App app;

void setup() {

	pinMode(2, OUTPUT);
	digitalWrite(2, HIGH);
	delay(100);
	digitalWrite(2, LOW);
	delay(2000);
	digitalWrite(2, HIGH);

	utils::serialLog("INO_SETUP", xTaskGetTickCount(), "Program setup.");

	app.init();
	app.main();

}

void loop() {

}
