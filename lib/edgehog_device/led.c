/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "led.h"

#include "edgehog_private.h"
#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(led, CONFIG_EDGEHOG_DEVICE_LED_LOG_LEVEL);

#if DT_NODE_HAS_STATUS(EDGEHOG_LED_NODE, okay)
#include <zephyr/drivers/gpio.h>

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define BLINK_DELAY 1000
#define DOUBLE_BLINK_DELAY_ON 300
#define DOUBLE_BLINK_DELAY_OFF 200
#define SLOW_BLINK_DELAY 2000

#define THREAD_STACK_SIZE 1024
#define LED_STATE_RUN_BIT (1)

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(led_thread_stack, THREAD_STACK_SIZE);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Start a one-shot timer that expires after 1 minute.
 *
 * @param[in] led_blink_timer Handle to valid led blink timer.
 * @param[in] led_run_state Handle to valid led run state.
 */
static void set_blink_timer(struct k_timer *led_blink_timer, atomic_t *led_run_state);

/**
 * @brief Setup Edgehog LED.
 *
 * @param[in] edgehog_led_dev Handle to valid edgehog gpio_dt_spec.
 * @return EDGEHOG_RESULT_OK if setup has been successful, an error code otherwise.
 */
static edgehog_result_t edgehog_led_setup(const struct gpio_dt_spec *edgehog_led_dev);

/**
 * @brief Get a k_thread_entry_t function based on the astarte value.
 *
 * @param[in] value An astarte individual value representing the LED behaviour to select.
 * @param[out] blink_entry k_thread_entry_t struct used to store the blink entry function.
 * @return EDGEHOG_RESULT_OK if publish has been successful, an error code otherwise.
 */
static edgehog_result_t get_blink_entry(astarte_individual_t value, k_thread_entry_t *blink_entry);

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

static void blink_entry_point(
    const struct gpio_dt_spec *edgehog_led_dev, atomic_t *led_run_state, k_timeout_t period)
{
    if (!edgehog_led_dev) {
        EDGEHOG_LOG_ERR("No devices with compatible edgehog_led found");
        return;
    }

    while (atomic_test_bit(led_run_state, LED_STATE_RUN_BIT)) {
        int ret = gpio_pin_toggle_dt(edgehog_led_dev);
        if (ret < 0) {
            EDGEHOG_LOG_ERR("Unable to toggle Edgehog LED");
        }
        k_sleep(period);
    }

    gpio_pin_set_dt(edgehog_led_dev, 0);
}

static void single_blink_entry_point(void *led_run_state, void *ptr2, void *ptr3)
{
    ARG_UNUSED(ptr2);
    ARG_UNUSED(ptr3);

    if (!led_run_state) {
        return;
    }

    const struct gpio_dt_spec edgehog_led_dev = GPIO_DT_SPEC_GET(EDGEHOG_LED_NODE, gpios);
    blink_entry_point(&edgehog_led_dev, (atomic_t *) led_run_state, K_MSEC(BLINK_DELAY));
}

static void slow_blink_entry_point(void *led_run_state, void *ptr2, void *ptr3)
{
    ARG_UNUSED(ptr2);
    ARG_UNUSED(ptr3);

    if (!led_run_state) {
        return;
    }

    const struct gpio_dt_spec edgehog_led_dev = GPIO_DT_SPEC_GET(EDGEHOG_LED_NODE, gpios);
    blink_entry_point(&edgehog_led_dev, (atomic_t *) led_run_state, K_MSEC(SLOW_BLINK_DELAY));
}

static void double_blink_entry_point(void *led_run_state, void *ptr2, void *ptr3)
{
    ARG_UNUSED(ptr2);
    ARG_UNUSED(ptr3);

    if (!led_run_state) {
        return;
    }

    const struct gpio_dt_spec edgehog_led_dev = GPIO_DT_SPEC_GET(EDGEHOG_LED_NODE, gpios);

    while (atomic_test_bit((atomic_t *) led_run_state, LED_STATE_RUN_BIT)) {
        gpio_pin_set_dt(&edgehog_led_dev, 1);
        k_sleep(K_MSEC(DOUBLE_BLINK_DELAY_ON));
        gpio_pin_set_dt(&edgehog_led_dev, 0);
        k_sleep(K_MSEC(DOUBLE_BLINK_DELAY_OFF));
        gpio_pin_set_dt(&edgehog_led_dev, 1);
        k_sleep(K_MSEC(DOUBLE_BLINK_DELAY_ON));
        gpio_pin_set_dt(&edgehog_led_dev, 0);
        k_sleep(K_MSEC(BLINK_DELAY));
    }
    gpio_pin_set_dt(&edgehog_led_dev, 0);
}

extern void expiry_blink_timer(struct k_timer *timer_id)
{
    atomic_t *led_run_state = (atomic_t *) k_timer_user_data_get(timer_id);

    if (!led_run_state) {
        EDGEHOG_LOG_ERR("No led thread is running");
        return;
    }

    atomic_clear_bit(led_run_state, LED_STATE_RUN_BIT);
}
#endif

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_result_t edgehog_led_event(
    edgehog_device_handle_t edgehog_dev, astarte_device_datastream_individual_event_t *event)
{
    if (!event) {
        ARG_UNUSED(edgehog_dev);
        EDGEHOG_LOG_ERR("Unable to handle event, event undefined");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

#if !DT_NODE_HAS_STATUS(EDGEHOG_LED_NODE, okay)
    EDGEHOG_LOG_ERR("Unable to find the edgehog LED Node in the device-tree");
    return EDGEHOG_RESULT_LED_NODE_NOT_FOUND;
#else
    const struct gpio_dt_spec edgehog_led_dev = GPIO_DT_SPEC_GET(EDGEHOG_LED_NODE, gpios);

    edgehog_result_t edgehog_result = edgehog_led_setup(&edgehog_led_dev);

    if (edgehog_result != EDGEHOG_RESULT_OK) {
        return edgehog_result;
    }

    if (atomic_test_bit(&edgehog_dev->led_thread.led_run_state, LED_STATE_RUN_BIT)) {
        EDGEHOG_LOG_ERR("Unable to perform LED blink while another is still active.");
        return EDGEHOG_RESULT_LED_ALREADY_IN_PROGRESS;
    }

    if (atomic_test_and_set_bit(&edgehog_dev->led_thread.led_run_state, LED_STATE_RUN_BIT)) {
        EDGEHOG_LOG_ERR("Unable to set LED RUN BIT");
        edgehog_result = EDGEHOG_RESULT_OUT_OF_MEMORY;
        goto fail;
    }

    struct k_thread *thread_handle = &edgehog_dev->led_thread.led_thread_handle;
    memset(thread_handle, 0, sizeof(struct k_thread));

    k_thread_entry_t blink_entry;

    edgehog_result = get_blink_entry(event->individual, &blink_entry);

    if (edgehog_result != EDGEHOG_RESULT_OK) {
        goto fail;
    }

    k_tid_t thread_id = k_thread_create(thread_handle, led_thread_stack, THREAD_STACK_SIZE,
        blink_entry, &edgehog_dev->led_thread.led_run_state, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0,
        K_NO_WAIT);

    if (!thread_id) {
        EDGEHOG_LOG_ERR("Led blink thread creation failed.");
        edgehog_result = EDGEHOG_RESULT_THREAD_CREATE_ERROR;
        goto fail;
    }

    set_blink_timer(
        &edgehog_dev->led_thread.led_blink_timer, &edgehog_dev->led_thread.led_run_state);

    return EDGEHOG_RESULT_OK;

fail:
    atomic_clear_bit(&edgehog_dev->led_thread.led_run_state, LED_STATE_RUN_BIT);
    return edgehog_result;

#endif
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

#if DT_NODE_HAS_STATUS(EDGEHOG_LED_NODE, okay)

static edgehog_result_t get_blink_entry(astarte_individual_t value, k_thread_entry_t *blink_entry)
{
    edgehog_result_t edgehog_result = EDGEHOG_RESULT_OK;

    if (value.tag != ASTARTE_MAPPING_TYPE_STRING) {
        EDGEHOG_LOG_ERR("Unable to handle event, event value is not a string");
        edgehog_result = EDGEHOG_RESULT_ASTARTE_ERROR;
    } else if (strcmp(value.data.string, "Blink60Seconds") == 0) {
        *blink_entry = single_blink_entry_point;
    } else if (strcmp(value.data.string, "DoubleBlink60Seconds") == 0) {
        *blink_entry = double_blink_entry_point;
    } else if (strcmp(value.data.string, "SlowBlink60Seconds") == 0) {
        *blink_entry = slow_blink_entry_point;
    } else {
        EDGEHOG_LOG_ERR("Unable to handle event, behaivour not supported %s", value.data.string);
        edgehog_result = EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    return edgehog_result;
}

static edgehog_result_t edgehog_led_setup(const struct gpio_dt_spec *edgehog_led_dev)
{
    if (!gpio_is_ready_dt(edgehog_led_dev)) {
        EDGEHOG_LOG_ERR("GPIO of Edgehog LED is not ready");
        return EDGEHOG_RESULT_LED_DEVICE_IS_NOT_READY;
    }

    int ret = gpio_pin_configure_dt(edgehog_led_dev, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        EDGEHOG_LOG_ERR("Edgehog LED configuration error %d", ret);
        return EDGEHOG_RESULT_LED_CONFIGURE_ERROR;
    }

    return EDGEHOG_RESULT_OK;
}

static void set_blink_timer(struct k_timer *led_blink_timer, atomic_t *led_run_state)
{
    k_timer_init(led_blink_timer, expiry_blink_timer, NULL);
    k_timer_user_data_set(led_blink_timer, (void *) led_run_state);
    k_timer_start(led_blink_timer, K_MINUTES(1), K_NO_WAIT);
}

#endif
