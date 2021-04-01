

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <inttypes.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>

/*
 * Get button configuration from the devicetree sw0 alias.
 *
 * At least a GPIO device and pin number must be provided. The 'flags'
 * cell is optional.
 */

#define SW0_NODE	DT_ALIAS(sw1)
#define SW0_GPIO_LABEL	DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))

#define SW1_NODE	DT_ALIAS(sw2)
#define SW1_GPIO_LABEL	DT_GPIO_LABEL(SW1_NODE, gpios)
#define SW1_GPIO_PIN	DT_GPIO_PIN(SW1_NODE, gpios)
#define SW1_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))

#define SW2_NODE	DT_ALIAS(sw3)
#define SW2_GPIO_LABEL	DT_GPIO_LABEL(SW2_NODE, gpios)
#define SW2_GPIO_PIN	DT_GPIO_PIN(SW2_NODE, gpios)
#define SW2_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW2_NODE, gpios))

static struct gpio_callback button1_cb_data;
static struct gpio_callback button2_cb_data;
static struct gpio_callback button3_cb_data;
static bool is_button1 = false;
static bool is_button2 = false;
static bool is_button3 = false;
void button1_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button1 pressed at %" PRIu32 "\n", k_cycle_get_32());
	is_button3 = true;
}

void button2_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button2 pressed at %" PRIu32 "\n", k_cycle_get_32());
	is_button2 = true;
}

void button3_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button3 pressed at %" PRIu32 "\n", k_cycle_get_32());
	is_button1 = true;
}


void btn_init(){
	const struct device *button1;
	const struct device *button2;
	const struct device *button3;
	int ret;
	button1 = device_get_binding(SW0_GPIO_LABEL);
	if (button1 == NULL) {
		printk("Error: didn't find %s device\n", SW0_GPIO_LABEL);
		return;
	}
	button2 = device_get_binding(SW1_GPIO_LABEL);
	if (button2 == NULL) {
		printk("Error: didn't find %s device\n", SW1_GPIO_LABEL);
		return;
	}
	button3 = device_get_binding(SW2_GPIO_LABEL);
	if (button3 == NULL) {
		printk("Error: didn't find %s device\n", SW2_GPIO_LABEL);
		return;
	}



	ret = gpio_pin_configure(button1, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
	ret = gpio_pin_configure(button2, SW1_GPIO_PIN, SW1_GPIO_FLAGS);
	ret = gpio_pin_configure(button3, SW2_GPIO_PIN, SW2_GPIO_FLAGS);

	ret = gpio_pin_interrupt_configure(button1,
					   SW0_GPIO_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);

	ret = gpio_pin_interrupt_configure(button2,
					   SW1_GPIO_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);

	ret = gpio_pin_interrupt_configure(button3,
					   SW2_GPIO_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button1_cb_data, button1_pressed, BIT(SW0_GPIO_PIN));
	gpio_init_callback(&button2_cb_data, button2_pressed, BIT(SW1_GPIO_PIN));
	gpio_init_callback(&button3_cb_data, button3_pressed, BIT(SW2_GPIO_PIN));
	gpio_add_callback(button1, &button1_cb_data);
	gpio_add_callback(button2, &button2_cb_data);
	gpio_add_callback(button3, &button3_cb_data);
}
void btn_loop(const struct shell *sh){
 	while (1) {
		 if (is_button1)
		 {
			 shell_execute_cmd(sh, "net ping -c 10 "CONFIG_NET_CONFIG_PEER_IPV6_ADDR);
			 is_button1 = false;
		 }
		 if (is_button2)
		 {
			shell_execute_cmd(sh, "zperf udp upload 2001:db8::1 5001 10 16 4K");
			//shell_execute_cmd(sh, "zperf udp download 5001");
			 is_button2 = false;
		 }
		 if (is_button3)
		 {
			 shell_execute_cmd(sh, "ieee802154 set_tx_power 20");
			 is_button3 = false;
		 }
		 		 		 
		// sprintf(count_str,"hello world .... %d\n",count);
		// printString(display_dev,count_str);
		k_sleep(K_MSEC(100));
		// shell_execute_cmd(sh, "net ping -c 10 2001:db8::1");
	 }
}