#include <zephyr.h>
#include <logging/log.h>
#include <led.h>
#include <clap_detection.h>
#include <ble_services.h>

LOG_MODULE_REGISTER(main);

static int num_claps = 0;

// Timer that resets num_claps after 1 second
void reset_num_claps(struct k_timer *timer)
{
	num_claps = 0;
}
K_TIMER_DEFINE(reset_claps_timer, reset_num_claps, NULL);

int main(void)
{
	int err;
	
	err = led_init();
	if (err)
	{
		LOG_ERR("Failed to initalize LED (err %d)", err);
	}

	err = clap_detection_init();
	if (err)
	{
		LOG_ERR("Failed to initalize clap detection (err %d)", err);
	}

	err = ble_services_init();
	if (err)
	{
		LOG_ERR("Failed to initialize BLE button service (err %d)", err);
	}

	// Show a recognizable LED pattern on startup
	led_set_state(true);
	k_msleep(500);
	led_set_state(false);
	k_msleep(500);
	led_set_state(true);
	k_msleep(500);
	led_set_state(false);
	
	// Start listening for claps
	while (1) 
	{
		if(clap_detection_check())
		{
			LOG_INF("Clap!");
			num_claps++;
			
			k_timer_start(&reset_claps_timer, K_SECONDS(1), K_SECONDS(1));
		}

		if(num_claps == 2)
		{
			num_claps = 0;

			ble_services_set_button_state(true);
			led_set_state(true);

			k_msleep(1000);

			ble_services_set_button_state(false);
			led_set_state(false);
		}
	}
}