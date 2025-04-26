#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/pwm.h>
#include <linux/types.h>
#include <linux/sysfs.h>


#define DEVICE_NAME "project"
#define CLASS_NAME "project"

#define GPIO_LED3 22 // LED3 is regular GPIO
#define GPIO_BUTTON1 23
#define GPIO_BUTTON2 24

static struct class*  projectClass  = NULL;
static struct device* projectDevice = NULL;
static int    majorNumber;
static struct cdev project_cdev;
static DEFINE_MUTEX(project_mutex);

// PWM control
static struct pwm_device *pwm_led1 = NULL;
static struct pwm_device *pwm_led2 = NULL;
static struct hrtimer led_timer;
static ktime_t pwm_period;

// Speed tracking
static u64 last_press_time_ns = 0;
static u64 last_speed_check_ns = 0;
static int press_count = 0;
static int press_speed = 0;

// Duty cycles
static int duty_cycle_led1 = 10;
static int duty_cycle_led2 = 0;
static int duty_cycle_led3 = 0;

// File operations
static int dev_open(struct inode *inodep, struct file *filep) { return 0; }
static int dev_release(struct inode *inodep, struct file *filep) { return 0; }


// Sysfs show/store functions
static ssize_t speed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", press_speed);
}

static ssize_t led1_duty_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", duty_cycle_led1);
}

static ssize_t led1_duty_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    if (kstrtoint(buf, 10, &value) == 0) {
        mutex_lock(&project_mutex);
        duty_cycle_led1 = clamp(value, 0, 100);
        mutex_unlock(&project_mutex);
    }
    return count;
}

static ssize_t led2_duty_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", duty_cycle_led2);
}

static ssize_t led2_duty_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    if (kstrtoint(buf, 10, &value) == 0) {
        mutex_lock(&project_mutex);
        duty_cycle_led2 = clamp(value, 0, 100);
        mutex_unlock(&project_mutex);
    }
    return count;
}

static ssize_t led3_duty_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", duty_cycle_led3);
}

static ssize_t led3_duty_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int value;
    if (kstrtoint(buf, 10, &value) == 0) {
        mutex_lock(&project_mutex);
        duty_cycle_led3 = clamp(value, 0, 100);
        mutex_unlock(&project_mutex);
    }
    return count;
}
// Define sysfs attributes
static DEVICE_ATTR_RO(speed);
static DEVICE_ATTR_RW(led1_duty);
static DEVICE_ATTR_RW(led2_duty);
static DEVICE_ATTR_RW(led3_duty);


static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    char speed_str[10];
    int speed_len;
    int ret;

    mutex_lock(&project_mutex);
    speed_len = snprintf(speed_str, sizeof(speed_str), "%d\n", press_speed);
    mutex_unlock(&project_mutex);

    if (*offset >= speed_len)
        return 0;

    if (len > speed_len - *offset)
        len = speed_len - *offset;

    ret = copy_to_user(buffer, speed_str + *offset, len);
    if (ret == 0) {
        *offset += len;
        return len;
    } else {
        return -EFAULT;
    }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    return len;
}

static struct file_operations fops =
{
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

// Timer callback
enum hrtimer_restart led_timer_callback(struct hrtimer *timer)
{
    static bool leds_on = false;
    static bool last_button1_state = true;
    static bool last_button2_state = true;
    u64 now_ns = ktime_get_ns();

    if (last_speed_check_ns == 0)
        last_speed_check_ns = now_ns;

    bool current_button1_state = gpio_get_value(GPIO_BUTTON1);
    bool current_button2_state = gpio_get_value(GPIO_BUTTON2);

    mutex_lock(&project_mutex);

    if (last_button1_state && !current_button1_state) {
        press_count++;
    }
    if (last_button2_state && !current_button2_state) {
        press_count++;
    }

    last_button1_state = current_button1_state;
    last_button2_state = current_button2_state;

    if (now_ns - last_speed_check_ns > 10000000000ULL) { // 10 seconds
        press_speed = press_count / 5;
        press_count = 0;
        last_speed_check_ns = now_ns;

        // Map speed to duty cycle
        if (press_speed >= 8) {
            duty_cycle_led1 = 100;
            duty_cycle_led2 = 100;
            duty_cycle_led3 = 100;
        } else if (press_speed >= 6) {
            duty_cycle_led1 = 80;
            duty_cycle_led2 = 70;
            duty_cycle_led3 = 50;
        } else if (press_speed >= 4) {
            duty_cycle_led1 = 60;
            duty_cycle_led2 = 50;
            duty_cycle_led3 = 30;
        } else if (press_speed >= 2) {
            duty_cycle_led1 = 40;
            duty_cycle_led2 = 30;
            duty_cycle_led3 = 10;
        } else {
            duty_cycle_led1 = 10;
            duty_cycle_led2 = 0;
            duty_cycle_led3 = 0;
        }

        printk(KERN_INFO "Project: Speed=%d alternations/sec, DutyCycle=%d%%\n", press_speed, duty_cycle_led1);
    }

    // Set PWM
    pwm_config(pwm_led1, duty_cycle_led1 * 100000, 10000000); // duty_ns, period_ns
    pwm_enable(pwm_led1);

    pwm_config(pwm_led2, duty_cycle_led2 * 100000, 10000000);
    pwm_enable(pwm_led2);

    // LED3 simple ON/OFF
    if (duty_cycle_led3 > 0)
        gpio_set_value(GPIO_LED3, 1);
    else
        gpio_set_value(GPIO_LED3, 0);

    mutex_unlock(&project_mutex);

    hrtimer_forward(timer, hrtimer_get_expires(timer), pwm_period);
    return HRTIMER_RESTART;
}

// Module init
static int __init project_init(void)
{
    

    printk(KERN_INFO "Project: Initializing...\n");

    mutex_init(&project_mutex);

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "Project: Failed to register major number\n");
        return majorNumber;
    }

    projectClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(projectClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return PTR_ERR(projectClass);
    }

    projectDevice = device_create(projectClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(projectDevice)) {
        class_destroy(projectClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        return PTR_ERR(projectDevice);
    }
    
    device_create_file(projectDevice, &dev_attr_speed);
    device_create_file(projectDevice, &dev_attr_led1_duty);
    device_create_file(projectDevice, &dev_attr_led2_duty);
    device_create_file(projectDevice, &dev_attr_led3_duty);

    // Request PWM devices
    pwm_led1 = pwm_request(0, "led1_pwm");
    pwm_led2 = pwm_request(1, "led2_pwm");

    if (IS_ERR(pwm_led1) || IS_ERR(pwm_led2)) {
        printk(KERN_ALERT "Failed to request PWM devices\n");
        return -EINVAL;
    }

    // Setup GPIO for LED3
    gpio_request(GPIO_LED3, "LED3");
    gpio_direction_output(GPIO_LED3, 0);

    gpio_request(GPIO_BUTTON1, "BUTTON1");
    gpio_direction_input(GPIO_BUTTON1);

    gpio_request(GPIO_BUTTON2, "BUTTON2");
    gpio_direction_input(GPIO_BUTTON2);

    // Start PWM timer
    pwm_period = ktime_set(0, 10000000); // 10ms period
    hrtimer_init(&led_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    led_timer.function = &led_timer_callback;
    hrtimer_start(&led_timer, pwm_period, HRTIMER_MODE_REL);

    printk(KERN_INFO "Project: Initialization complete.\n");
    return 0;
}

// Module exit
static void __exit project_exit(void)
{
    hrtimer_cancel(&led_timer);

    if (pwm_led1) {
        pwm_disable(pwm_led1);
        pwm_free(pwm_led1);
    }

    if (pwm_led2) {
        pwm_disable(pwm_led2);
        pwm_free(pwm_led2);
    }

    gpio_set_value(GPIO_LED3, 0);
    gpio_free(GPIO_LED3);

    gpio_free(GPIO_BUTTON1);
    gpio_free(GPIO_BUTTON2);
    
    device_remove_file(projectDevice, &dev_attr_speed);
    device_remove_file(projectDevice, &dev_attr_led1_duty);
    device_remove_file(projectDevice, &dev_attr_led2_duty);
    device_remove_file(projectDevice, &dev_attr_led3_duty);


    device_destroy(projectClass, MKDEV(majorNumber, 0));
    class_unregister(projectClass);
    class_destroy(projectClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);

    mutex_destroy(&project_mutex);

    printk(KERN_INFO "Project: Module unloaded.\n");
}

module_init(project_init);
module_exit(project_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniel Klevak");
MODULE_DESCRIPTION("High-End Device Development Final Project Driver");
