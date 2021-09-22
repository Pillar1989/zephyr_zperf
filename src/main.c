/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zperf sample.
 */


#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/sensor.h>
#include <drivers/spi.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <sys/util.h>
#include "shell/shell_uart.h"

#include "icmpv6.h"
#include "ipv6.h"
#include "connection.h"


#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(rftest);


#include "net_private.h"

#define LED0_NODE DT_ALIAS(led0)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)

#define LED1_NODE DT_ALIAS(led1)
#define LED1	DT_GPIO_LABEL(LED1_NODE, gpios)
#define PIN1	DT_GPIO_PIN(LED1_NODE, gpios)
#define FLAGS1	DT_GPIO_FLAGS(LED1_NODE, gpios)


#define SW0_NODE	DT_ALIAS(sw0)
#define SW0_GPIO_LABEL	DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))




static struct device *led0_d;
static struct device *led1_d;
static struct device *btn_d;
static bool led_is_on = true;
static struct k_delayed_work dwork;
static bool is_btn = false;

K_SEM_DEFINE(test_ping_timeout, 0, 1);
extern uint32_t z_impl_sys_rand32_get(void);

static enum net_verdict handle_ipv6_echo_reply(struct net_pkt *pkt,
					       struct net_ipv6_hdr *ip_hdr,
					       struct net_icmp_hdr *icmp_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmpv6_echo_req);
	struct net_icmpv6_echo_req *icmp_echo;
	uint32_t cycles;

	icmp_echo = (struct net_icmpv6_echo_req *)net_pkt_get_data(pkt,
								&icmp_access);
	if (icmp_echo == NULL) {
		return -NET_DROP;
	}

	net_pkt_skip(pkt, sizeof(*icmp_echo));
	if (net_pkt_read_be32(pkt, &cycles)) {
		return -NET_DROP;
	}

	cycles = k_cycle_get_32() - cycles;

	printk("%d bytes from %s to %s: icmp_seq=%d ttl=%d "
#ifdef CONFIG_IEEE802154
		 "rssi=%d "
#endif
#ifdef CONFIG_FPU
		 "time=%.2f ms\n",
#else
		 "time=%d ms\n",
#endif
		 ntohs(ip_hdr->len) - net_pkt_ipv6_ext_len(pkt) -
								NET_ICMPH_LEN,
		 net_sprint_ipv6_addr(&ip_hdr->src),
		 net_sprint_ipv6_addr(&ip_hdr->dst),
		 ntohs(icmp_echo->sequence),
		 ip_hdr->hop_limit,
#ifdef CONFIG_IEEE802154
		 net_pkt_ieee802154_rssi(pkt),
#endif
#ifdef CONFIG_FPU
		 ((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000.f));
#else
		 ((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000));
#endif
	k_sem_give(&test_ping_timeout);
	
	gpio_pin_set(led0_d, PIN, (int)led_is_on);
	led_is_on = !led_is_on;

	net_pkt_unref(pkt);
	return NET_OK;
}




static struct net_icmpv6_handler ping6_handler = {
	.type = NET_ICMPV6_ECHO_REPLY,
	.code = 0,
	.handler = handle_ipv6_echo_reply,
};


static inline void remove_ipv6_ping_handler(void)
{
	net_icmpv6_unregister_handler(&ping6_handler);
}

static int ping_ipv6(char *host,
		     unsigned int count,
		     unsigned int interval,
		     int iface_idx)
{
	struct net_if *iface = net_if_get_by_index(iface_idx);
	int ret = 0;
	struct in6_addr ipv6_target;
	struct net_nbr *nbr;


	if (net_addr_pton(AF_INET6, host, &ipv6_target) < 0) {
		return -EINVAL;
	}

	net_icmpv6_register_handler(&ping6_handler);

	if (!iface) {
		iface = net_if_ipv6_select_src_iface(&ipv6_target);
		if (!iface) {
			nbr = net_ipv6_nbr_lookup(NULL, &ipv6_target);
			if (nbr) {
				iface = nbr->iface;
			} else {
				iface = net_if_get_default();
			}
		}
	}

	printk("PING %s\n", host);

	for (int i = 0; i < count; ++i) {
		uint32_t time_stamp = htonl(k_cycle_get_32());

		ret = net_icmpv6_send_echo_request(iface,
						   &ipv6_target,
						   z_impl_sys_rand32_get(),
						   i,
						   &time_stamp,
						   sizeof(time_stamp));
		if (ret) {
			break;
		}

		k_msleep(interval);
	}

	remove_ipv6_ping_handler();

	return ret;
}


#define BLINK_MS 500


static void ping_work_handler(struct k_work *work){
	ARG_UNUSED(work);
	int r;

	char ip[] = CONFIG_NET_CONFIG_PEER_IPV6_ADDR;
	ping_ipv6(ip,1,100,0);
	if(is_btn == false){
		r = k_delayed_work_submit(&dwork, K_MSEC(BLINK_MS));
		__ASSERT(r == 0, "k_delayed_work_submit() failed for LED work: %d", r);
	}
}

static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	is_btn = true;
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());

	gpio_pin_set(led1_d, PIN1, true);
}


void main(void)
{
#if 1
	int r;
	const struct shell *sh;
	bool is_zperf_server = true;

	led0_d =  (struct device *)device_get_binding( DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	gpio_pin_configure(led0_d, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);

	led1_d =  (struct device *)device_get_binding( DT_GPIO_LABEL(DT_ALIAS(led1), gpios));
	gpio_pin_configure(led1_d, PIN1, GPIO_OUTPUT_ACTIVE | FLAGS1);
	gpio_pin_set(led1_d, PIN1, false);


	sh = (const struct shell *)shell_backend_uart_get_ptr();
	btn_d = device_get_binding(SW0_GPIO_LABEL);
	if (btn_d == NULL) {
		printk("Error: didn't find %s device\n", SW0_GPIO_LABEL);
		return;
	}

	r = gpio_pin_configure(btn_d, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
	if (r != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       r, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return;
	}

	r = gpio_pin_interrupt_configure(btn_d,
					   SW0_GPIO_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (r != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			r, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(btn_d, &button_cb_data);
	printk("Set up button at %s pin %d\n", SW0_GPIO_LABEL, SW0_GPIO_PIN);


	dwork.work.handler = ping_work_handler;
	r = k_delayed_work_submit(&dwork, K_MSEC(BLINK_MS));
	__ASSERT(r == 0, "k_delayed_work_submit() failed for LED work: %d", r);
	for (;;) {
		if (is_btn){
			if (is_zperf_server){
				is_zperf_server = false;
				shell_execute_cmd(sh, "zperf udp download 5001");
			}	
		}
		k_sleep(K_MSEC(1000));
	}
#endif 
}



// #include <usb/usb_device.h>
// #include <net/net_config.h>

// void main(void)
// {
// #if defined(CONFIG_USB)
// 	int ret;

// 	ret = usb_enable(NULL);
// 	if (ret != 0) {
// 		printk("usb enable error %d\n", ret);
// 	}

// 	(void)net_config_init_app(NULL, "Initializing network");
// #endif /* CONFIG_USB */
// }
