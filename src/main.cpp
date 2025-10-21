
#include <Arduino.h>

#include "App.hpp"

App app;

void setup() {

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(100);
	digitalWrite(LED_BUILTIN, LOW);
	delay(2000);
	digitalWrite(LED_BUILTIN, HIGH);

	utils::serialLog("INO_SETUP", xTaskGetTickCount(), "Program setup.");

	app.init();
	app.main();

}

void loop() {

}
