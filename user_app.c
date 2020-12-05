#include "rui.h"
#include "lis3dh.h"
#include "opt3001.h"
#include "shtc3.h"
#include "lps22hb.h"

RUI_I2C_ST st = {0};

/////// USER DEFINED CONSTANTS
const uint16_t INIT_TIMER_LENGTH = 5000;
const uint16_t TEMP_CHECK_TIMER_LENGTH = 5000;
const uint16_t DIAGNOSTICS_TIMER_LENGTH = 30000;
const uint16_t LOCATION_CHECK_TIMER_LENGTH = 10000;
const uint16_t BATTERY_LEVEL_CHECK_TIMER_LENGTH = 30000;
const uint16_t ALERT_TIMER_LENGTH = 5000;
const uint16_t LAST_SIGNAL_TIMER = 15000;

enum stateList {TURN_ON, WAITING_TO_STARTUP, STARTUP, DIAGNOSTICS, MALFUNCTION, IDLE, TEMP_CHECK, LOCATION_CHECK};

/////// USER DEFINED VARIABLES
RUI_TIMER_ST tempCheckTimer;
RUI_TIMER_ST startTimer;
RUI_TIMER_ST diagnosticsTimer;
RUI_TIMER_ST locationCheckTimer;
RUI_TIMER_ST batteryLevelCheckTimer;
RUI_TIMER_ST alertTimer;
RUI_TIMER_ST lastSignalTimer;
bool tempCheckTimerTriggered = false;
bool startTimerTriggered = false;
bool diagnosticsTimerTriggered = false;
bool locationCheckTimerTriggered = false;
bool batteryLevelCheckTimerTriggered = false;
bool alertTimerTriggered = false;
bool lastSignalTimerTriggered = false;
float temp = 0.0;
float humidity = 0.0;

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

void sensor_off(void)
{
    lis3dh_sleep_init();
    sensorOpt3001Enable(0);
    SHTC3_Sleep();
    lps22hb_mode(0);
}

/// USER DEFINED FUNCTIONS ///

void tempCheckTimerCallback(void) {
	tempCheckTimerTriggered = true;
}

void startTimerCallback(void) {
	startTimerTriggered = true;
}

void diagnosticsTimerCallback(void) {
	diagnosticsTimerTriggered = true;
}

void locationCheckTimerCallback(void) {
	locationCheckTimerTriggered = true;
}

void batteryLevelCheckTimerCallback(void) {
	batteryLevelCheckTimerTriggered = true;
}

void alarmTimerCallback(void) {
	alarmTimerTriggered = true;
}

void lastSignalTimerCallback(void) {
	lastSignalTimerTriggered = true;
}

void main(void)
{
	//system init 
	rui_sensor_register_callback(sensor_on,sensor_off);
	rui_init();
	
	enum stateList state = TURN_ON;
	
	RUI_LOG_PRINTF("Initilization complete. Beginning user programmed sequence.");
	rui_running();
	while (1) {
		switch(state) {
			case TURN_ON:
				// In this state we will start the timer to wait for startup
				RUI_LOG_PRINTF("Waiting to boot up");
				rui_timer_init(&startTimer, startTimerCallback);
				rui_timer_setvalue(&startTimer, INIT_TIMER_LENGTH);
				rui_timer_start(&startTimer);
				state = WAITING_TO_STARTUP;
				break;
			case WAITING_TO_STARTUP:
				// In this state we wait for the timer in order to start up
				if (startTimerTriggered) {
					state = STARTUP;
				}
				break;
			case STARTUP:
				tempCheckTimer.timer_mode = RUI_TIMER_MODE_REPEATED;
				rui_timer_init(&tempCheckTimer, tempCheckTimerCallback);
				rui_timer_setvalue(&tempCheckTimer, TEMP_CHECK_TIMER_LENGTH);
				rui_timer_start(&tempCheckTimer);
				locationCheckTimer.timer_mode = RUI_TIMER_MODE_REPEATED;
				rui_timer_init(&locationCheckTimer, locationCheckTimerCallback);
				rui_timer_setvalue(&locationCheckTimer, LOCATION_CHECK_TIMER_LENGTH);
				rui_timer_start(&locationCheckTimer);
				state = DIAGNOSTICS;
			case DIAGNOSTICS:
				// In this state we run the diagnostics
				RUI_LOG_PRINTF("Running diagnostics...");
				if (true) { // TODO: implement the actual diagnostics test
					RUI_LOG_PRINTF("Diagnostics passed.");
					state = IDLE;
				}
				else {
					RUI_LOG_PRINTF("Diagnostics failed.");
					state = MALFUNCTION;
				}
				break;
			case MALFUNCTION:
				// In this state the diagnostics have failed.
				RUI_LOG_PRINTF("Sending malfunction notification");
				state = IDLE;
				break;
			case IDLE:
				if (tempCheckTimerTriggered) {
					state = TEMP_CHECK;
				}
				else if (diagnosticsTimerTriggered) {
					state = DIAGNOSTICS;
				}
				else if (locationCheckTimerTriggered) {
					state = LOCATION_CHECK;
				}
				break;
			case TEMP_CHECK:
				RUI_LOG_PRINTF("Checking the temperature...");
				tempCheckTimerTriggered = false;
				state = IDLE;
				break;
			case LOCATION_CHECK:
				RUI_LOG_PRINTF("Checking the location for movement...");
				locationCheckTimerTriggered = false;
				state = IDLE;
				break;
			default:
				RUI_LOG_PRINTF("An error occurred!");
				break;
		}
		rui_running();
	}
	    /*
        //do your work here, then call rui_device_sleep(1) to sleep
	SHTC3_GetTempAndHumi(&temp,&humidity);
	RUI_LOG_PRINTF("temperature = %f", temp);
        RUI_LOG_PRINTF("humidity = %f", humidity);
	    if (temp > 1) {
		RUI_LOG_PRINTF("The temperature variable has been assigned correctly");
	    }
	    else {
		    RUI_LOG_PRINTF("The temperature variable was NOT assigned correctly; it is still 0");
	    }
	    if (humidity > 1) {
		    RUI_LOG_PRINTF("The humidity variable has been assigned correctly");
	    }
	    else {
		    RUI_LOG_PRINTF("The humidity variable was NOT assigned correctly; it is still 0");
	    }
	//RUI_LOG_PRINTF("temperature = "NRF_LOG_FLOAT_MARKER"",NRF_LOG_FLOAT(temp));
        //RUI_LOG_PRINTF("humidity = "NRF_LOG_FLOAT_MARKER"",NRF_LOG_FLOAT(humidity));
        //RUI_LOG_PRINTF(at_parse("at+get_config=device:status"));
        rui_device_sleep(1);
        //here run system work and do not modify
        rui_running();
	*/
}
