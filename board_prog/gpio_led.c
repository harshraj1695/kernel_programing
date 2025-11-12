#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define LED_GPIO 60      // P9_12
#define BTN_GPIO 48      // P9_15

static int irq_num;

scp -r /lib/modules/5.10.168 debian@192.168.7.2:/tmp/

static irqreturn_t button_isr(int irq, void *dev_id)
{
    int val = gpio_get_value(BTN_GPIO);
    gpio_set_value(LED_GPIO, val);  // mirror button state to LED
    pr_info("Button state: %d\n", val);
    return IRQ_HANDLED;
}

static int __init gpio_init(void)
{
    int ret;

    gpio_request(LED_GPIO, "LED_GPIO");
    gpio_direction_output(LED_GPIO, 0);

    gpio_request(BTN_GPIO, "BTN_GPIO");
    gpio_direction_input(BTN_GPIO);

    irq_num = gpio_to_irq(BTN_GPIO);
    request_irq(irq_num, button_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "btn_irq", NULL);

    pr_info("GPIO module loaded\n");
    return 0;
}

static void __exit gpio_exit(void)
{
    free_irq(irq_num, NULL);
    gpio_free(LED_GPIO);
    gpio_free(BTN_GPIO);
    pr_info("GPIO module unloaded\n");
}

module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");

