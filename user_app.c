// Do not edit these include statements
#include "rui.h"
#include "lis3dh.h"
#include "opt3001.h"
#include "shtc3.h"
#include "lps22hb.h"
#include "inner.h"

// Don't know what this is, don't touch it
RUI_I2C_ST st = {0};

////// FOR TESTING PURPOSES ONLY
bool alertOnPlease = false;
bool diagnosticsPassPlease = true;
bool batteryLowPlease = false;

/////// USER DEFINED CONSTANTS: These change the lengths of the timers that the RAK chip uses. Measured in milliseconds. 
// you can change these according to how often you think the tracker should check the temperature, etc. 
const uint16_t TEMP_STATUS_TIMER_LENGTH = 5000;
const uint16_t DIAGNOSTICS_BATTERY_TIMER_LENGTH = 30000;
const uint16_t START_LOCATION_CHECK_TIMER_LENGTH = 10000;
const uint16_t ALERT_TIMER_LENGTH = 5000;
const uint16_t LAST_SIGNAL_TIMER = 15000;

// This is a list of all the sates that the tracker can be in. Just add to this enum in order to add another state.
enum stateList {TURN_ON, WAITING_TO_STARTUP, STARTUP, DIAGNOSTICS, MALFUNCTION, IDLE, TEMP_CHECK, LOCATION_CHECK, BATTERY_LEVEL_CHECK_IDLE, ALERT_INIT, ALERT, BROADCAST_ALERT, STATUS_CHECK_IDLE, STATUS_CHECK_ALERT, LAST_BROADCAST, BATTERY_LEVEL_CHECK_ALERT};

/////// USER DEFINED VARIABLES
// These are the timers that the RUI API uses. They are of the struct type "RUI_TIMER_ST". For some reason, we have found that 
// the hardware or underlying firmware limits the amount of timers that can be used to 5.
RUI_TIMER_ST tempStatusTimer;
RUI_TIMER_ST diagnosticsBatteryTimer;
RUI_TIMER_ST startLocationCheckTimer;
RUI_TIMER_ST alertTimer;
RUI_TIMER_ST lastSignalTimer;
// These variables are changed to true whevener one of the timers reaches it's end and restarts.
bool tempCheckTimerTriggered = false;
bool startTimerTriggered = false;
bool diagnosticsTimerTriggered = false;
bool locationCheckTimerTriggered = false;
bool batteryLevelCheckTimerTriggered = false;
bool alertTimerTriggered = false;
bool lastSignalTimerTriggered = false;
bool checkStatusTimerTriggered = false;
bool workModeEnabled = false;\
// Used to store the temperature and humidity values.
float temp = 0.0;
float humidity = 0.0;

// Don't know what this is, don't touch it.
void sensor_on(void)
{

    st.PIN_SDA = 14;
    st.PIN_SCL = 13;
    st.FREQUENCY = RUI_I2C_FREQ_400K;
    rui_i2c_init(&st);

    //lis3dh init
    lis3dh_init();
    //opt3001 init
    opt3001_init();
	//shtc3 init
    SHTC3_Wakeup();
    //lps22hb init 1 wake up
    lps22hb_mode(1);

}

// Don't know what this is, don't touch it.
void sensor_off(void)
{
    lis3dh_sleep_init();
    sensorOpt3001Enable(0);
    SHTC3_Sleep();
    lps22hb_mode(0);
}

/// USER DEFINED FUNCTIONS ///

// These timer "callbacks" simply change the timerTriggered variables to true when their respective timers reach their times.
void tempStatusTimerCallback(void) {
	tempCheckTimerTriggered = true;
	checkStatusTimerTriggered = true;
}

void diagnosticsBatteryTimerCallback(void) {
	diagnosticsTimerTriggered = true;
	batteryLevelCheckTimerTriggered = true;
}

void startLocationCheckTimerCallback(void) {
	locationCheckTimerTriggered = true;
	startTimerTriggered = true;
}

void alertTimerCallback(void) {
	alertTimerTriggered = true;
}

void lastSignalTimerCallback(void) {
	lastSignalTimerTriggered = true;
}

void main(void)
{
	//system init: don't touch it, don't know what it does.
	rui_sensor_register_callback(sensor_on,sensor_off);
	rui_init();
	
	// Set up the "state" variable to work with the states that we have defined.
	enum stateList state = TURN_ON;
	
	RUI_LOG_PRINTF("Initilization complete. Beginning user programmed sequence.");
	// For some reason, we HAVE to run this command before the main while loop. Don't know why, it works though so don't touch it.
	rui_running();
	while (1) {
		switch(state) {
			case TURN_ON:
				// In this state we will start the timer to wait for startup
				RUI_LOG_PRINTF("Waiting to boot up");
				// This is how you properly initialize a timer: first, change its mode to "repeated" so it keeps cycling
				startLocationCheckTimer.timer_mode = RUI_TIMER_MODE_REPEATED;
				// Next, attach a function to it that will run when the timer is up
				rui_timer_init(&startLocationCheckTimer, startLocationCheckTimerCallback);
				// Third, set the interval
				rui_timer_setvalue(&startLocationCheckTimer, START_LOCATION_CHECK_TIMER_LENGTH * 2.0);
				// Fourth, start the timer
				rui_timer_start(&startLocationCheckTimer);
				state = WAITING_TO_STARTUP;
				break;
			case WAITING_TO_STARTUP:
				// In this state we wait for the timer in order to start up
				if (startTimerTriggered) {
					state = STARTUP;
				}
				break;
			case STARTUP:
				// In this state we will begin all of the timers for the sensors
				tempStatusTimer.timer_mode = RUI_TIMER_MODE_REPEATED;
				rui_timer_init(&tempStatusTimer, tempStatusTimerCallback);
				rui_timer_setvalue(&tempStatusTimer, TEMP_STATUS_TIMER_LENGTH * 2.0);
				rui_timer_start(&tempStatusTimer);
				
				diagnosticsBatteryTimer.timer_mode = RUI_TIMER_MODE_REPEATED;
				rui_timer_init(&diagnosticsBatteryTimer, diagnosticsBatteryTimerCallback);
				rui_timer_setvalue(&diagnosticsBatteryTimer, DIAGNOSTICS_BATTERY_TIMER_LENGTH * 2.0);
				rui_timer_start(&diagnosticsBatteryTimer);
				
				state = DIAGNOSTICS;
			case DIAGNOSTICS:
				// In this state we run the diagnostics
				RUI_LOG_PRINTF("Running diagnostics...");
				if (diagnosticsPassPlease) { // TODO: implement the actual diagnostics test
					RUI_LOG_PRINTF("Diagnostics passed.");
					state = IDLE;
				}
				else {
					RUI_LOG_PRINTF("Diagnostics failed.");
					state = MALFUNCTION;
				}
				diagnosticsTimerTriggered = false;
				break;
			case MALFUNCTION:
				// In this state the diagnostics have failed.
				RUI_LOG_PRINTF("Sending malfunction notification");
				rui_cellular_send("ATI");
				// RUI_LOG_PRINTF("Cellular response: %s", at_rsp);
				state = IDLE;
				break;
			case IDLE:
				// The tracker will be in this state most of the time.
				// TODO: make sure that the tracker is conserving power in this state.
				if (tempCheckTimerTriggered) {
					state = TEMP_CHECK;
				}
				else if (diagnosticsTimerTriggered) {
					state = DIAGNOSTICS;
				}
				else if (locationCheckTimerTriggered) {
					state = LOCATION_CHECK;
				}
				else if (batteryLevelCheckTimerTriggered) {
					state = BATTERY_LEVEL_CHECK_IDLE;
				}
				else if (checkStatusTimerTriggered) {
					state = STATUS_CHECK_IDLE;
				}
				break;
			case TEMP_CHECK:
				// In this state we check the temperature and humidity, then print the results.
				RUI_LOG_PRINTF("Checking the temperature...");
				int responseCode = SHTC3_GetTempAndHumi(&temp, &humidity);
				int tempInt = (int)temp;
				int humidityInt = (int)humidity;
				RUI_LOG_PRINTF("Temp data: %d, Humidity data: %d", tempInt, humidityInt);
				RUI_LOG_PRINTF("Response code: %d", responseCode);
				tempCheckTimerTriggered = false;
				state = IDLE;
				break;
			case LOCATION_CHECK:
				// In this state we check the location of the device to see if it has moved
				RUI_LOG_PRINTF("Checking the location for movement...");
				locationCheckTimerTriggered = false;
				if (alertOnPlease) { // TODO: have this changed for the correct update of the alarm
					state = ALERT_INIT;
				}
				else {
					state = IDLE;
				}
				break;
			case BATTERY_LEVEL_CHECK_IDLE:
				// In this state we check the battery level
				RUI_LOG_PRINTF("Checking the battery level...");
				batteryLevelCheckTimerTriggered = false;
				state = IDLE;
				break;
			case STATUS_CHECK_IDLE:
				// In this state we check with the server to see if the user has changed the alert or work status of the device
				RUI_LOG_PRINTF("Checking the alert / work mode status...");
				checkStatusTimerTriggered = false;
				state = IDLE;
				break;
			case ALERT_INIT:
				// In this state we start up alert mode 
				RUI_LOG_PRINTF("Entering alert mode...");
				alertTimer.timer_mode = RUI_TIMER_MODE_REPEATED;
				rui_timer_init(&alertTimer, alertTimerCallback);
				rui_timer_setvalue(&alertTimer, ALERT_TIMER_LENGTH * 2.0);
				rui_timer_start(&alertTimer);
				state = BROADCAST_ALERT;
				break;
			case BROADCAST_ALERT:
				// In this state we send an alert text to the user.
				RUI_LOG_PRINTF("Broadcasting alert...");
				alertTimerTriggered = false;
				state = ALERT;
				break;
			case ALERT:
				// We are in this state most of the time if we are in alert mode.
				// TODO: have the device conserve power during this mode.
				if (alertTimerTriggered) {
					state = BROADCAST_ALERT;
				}
				else if (checkStatusTimerTriggered) {
					state = STATUS_CHECK_ALERT;
				}
				else if (batteryLevelCheckTimerTriggered) {
					state = BATTERY_LEVEL_CHECK_ALERT;
				}
				break;
			case STATUS_CHECK_ALERT:
				// In this state, check to see if the user has turned the alarm off
				RUI_LOG_PRINTF("Checking the alert / work mode status...");
				checkStatusTimerTriggered = false;
				if (!alertOnPlease) { // TODO: change this to reflect the actual status of the alarm
					state = IDLE;
				}
				else {
					state = ALERT;
				}
				break;
			case BATTERY_LEVEL_CHECK_ALERT:
				// In this state we check the battery level
				RUI_LOG_PRINTF("Checking the battery level...");
				batteryLevelCheckTimerTriggered = false;
				if (batteryLowPlease) { // TODO: actually check if the battery level is low
					state = LAST_BROADCAST;
				}
				else {
					state = ALERT;
				}
				break;
			case LAST_BROADCAST:
				// In this state, we conserve power for 24 hours before sending one final alert text with our location.
				RUI_LOG_PRINTF("Final broadcast mode. Standing by for 24 hours.");
				RUI_LOG_PRINTF("Final broadcast sent. I'm dying...");
				break;
			default:
				RUI_LOG_PRINTF("An error occurred!");
				break;
		}
		// This must be called at the end of the while loop so the device works properly.
		rui_running();
	}
	  
}
