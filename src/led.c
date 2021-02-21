#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#define FLAGS	0
#endif

const struct device *gpio_dev;

int led_init()
{
    gpio_dev = device_get_binding(LED0);
    if (gpio_dev == NULL)
    {
        return -1;
    }

    int err = gpio_pin_configure(gpio_dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
    return err;
}

void led_set_state(bool state)
{
    gpio_pin_set(gpio_dev, PIN, (int)state);
}