
 #include <linux/module.h>
 #include <linux/kernel.h>
 #include <linux/init.h>
 #include <linux/gpio/consumer.h>
 #include <linux/platform_device.h>
 #include <linux/sysfs.h>
 #include <linux/of_device.h>

 struct led_demo_data {
     struct device *dev;
     struct gpio_desc *led_gpio_user;
     struct gpio_desc *led_gpio_status;
 };
 
 static ssize_t demo_led_show(struct device *dev, struct device_attribute *attr,
                     char *buf)
 {
    struct led_demo_data *led_data = dev_get_drvdata(dev);
    int val = gpiod_get_value(led_data->led_gpio_user);

    pr_info("Debug: %s Demo led state value: %d\n", __func__, val);

    return sprintf(buf, "%d\n", val);
 }
 
 static ssize_t demo_led_store(struct device *dev, struct device_attribute *attr,
                      const char *buf, size_t count)
 {
    int val;
    struct led_demo_data *led_data = dev_get_drvdata(dev);
    sscanf(buf, "%d", &val);
    gpiod_set_value(led_data->led_gpio_user, val);
    gpiod_set_value(led_data->led_gpio_status, val);
    pr_info("Debug: %s Demo led changed to %d\n", __func__, val);
    
    return count;
    return 0;
 }

 static DEVICE_ATTR(demo_led, 0664, demo_led_show, demo_led_store);
  
 static int demo_led_probe(struct platform_device *pdev)
 {
    struct led_demo_data *led_data;
    struct device *dev = &pdev->dev;
    int ret;
    struct fwnode_handle *child;
    const char *label;
    
    led_data = devm_kzalloc(dev, sizeof(*led_data), GFP_KERNEL);

    if (!led_data)
    return -ENOMEM;

    led_data->dev = dev;
    device_for_each_child_node(dev, child) {
        if (fwnode_property_read_string(child, "label", &label))
            continue;
        pr_info("Found LED '%s'\n", label);
        if (strcmp(label, "user") == 0) {
            led_data->led_gpio_user = devm_fwnode_get_gpiod_from_child(dev, 
                                                   NULL, child, 
                                                   GPIOD_OUT_LOW, NULL);
            if (IS_ERR(led_data->led_gpio_user)) {
                ret = PTR_ERR(led_data->led_gpio_user);
                pr_err("Failed to get '%s' LED GPIO: %d\n", label, ret);
                fwnode_handle_put(child);
                return ret;
            }
        }
        if (strcmp(label, "status") == 0) {
            led_data->led_gpio_status = devm_fwnode_get_gpiod_from_child(dev, 
                                                   NULL, child, 
                                                   GPIOD_OUT_LOW, NULL);
            if (IS_ERR(led_data->led_gpio_status)) {
                ret = PTR_ERR(led_data->led_gpio_status);
                pr_err("Failed to get '%s' LED GPIO: %d\n", label, ret);
                fwnode_handle_put(child);
                return ret;
            }
        }
    }
    
    platform_set_drvdata(pdev, led_data);

    ret = device_create_file(led_data->dev, &dev_attr_demo_led);
	if (ret) {
		pr_err("Failed to create sysfs file: %d\n", ret);
		return ret;
	}
    pr_info("Demo led driver probe started\n");
    return 0;
 }
 
 static int demo_led_remove(struct platform_device *pdev)
 {
    struct led_demo_data *led_data = platform_get_drvdata(pdev);

	gpiod_set_value(led_data->led_gpio_user, 0);
    gpiod_set_value(led_data->led_gpio_status, 0);
	device_remove_file(led_data->dev, &dev_attr_demo_led);
    gpiod_put(led_data->led_gpio_user);
    pr_info("Demo led driver removed\n");
    return 0;
 }
 
 static struct of_device_id demo_led_table[] = {
     { .compatible = "gpio-leds"},
     { /* NULL */ },
 };
 MODULE_DEVICE_TABLE(of, demo_led_table);
 
 static struct platform_driver demo_led_driver = {
     .probe = demo_led_probe,
     .remove = demo_led_remove,
     .driver = {
         .name = "demo_led",
         .of_match_table = demo_led_table,
     },
 };

 module_platform_driver(demo_led_driver);

 MODULE_LICENSE("GPL");

 