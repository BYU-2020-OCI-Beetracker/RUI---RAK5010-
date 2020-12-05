#include "rui.h"
#include "lis3dh.h"
#include "opt3001.h"
#include "shtc3.h"
#include "lps22hb.h"

RUI_I2C_ST st = {0};

/////// USER DEFINED VARIABLES
RUI_TIMER_ST tempCheckTimer;
RUI_TIMER_ST startTimer;
bool tempCheckTimerTriggered = false;
bool startTimerTriggered = false;
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

void main(void)
{
	//system init 
	rui_sensor_register_callback(sensor_on,sensor_off);
	rui_init();
	
	uint8_t state = 0;
	
	RUI_LOG_PRINTF("Initilization complete. Beginning user programmed sequence.");
	rui_running();
	while (1) {
		switch(state) {
			case 0:
				// In this state we will start the timer to wait for startup
				RUI_LOG_PRINTF("Starting timer...");
				rui_timer_init(&startTimer, startTimerCallback);
				rui_timer_setvalue(&startTimer, 5000);
				rui_timer_start(&startTimer);
				state = 1;
				break;
			case 1:
				// In this state we wait for the timer in order to start up
				if (startTimerTriggered) {
					state = 2;
				}
				break;
			case 2:
				// In this state we begin starting up
				RUI_LOG_PRINTF("Starting up...");
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
