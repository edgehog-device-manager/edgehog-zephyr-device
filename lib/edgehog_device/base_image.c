/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "base_image.h"

#include "edgehog_device/device.h"
#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "log.h"
#include "util.h"

#include <app_version.h>

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/interface.h>
#include <astarte_device_sdk/result.h>

EDGEHOG_LOG_MODULE_REGISTER(base_image, CONFIG_EDGEHOG_DEVICE_BASE_IMAGE_LOG_LEVEL);

/************************************************
 * Static functions declaration
 ***********************************************/

static void publish_fingerprint(edgehog_device_handle_t edgehog_device);

static void publish_name(edgehog_device_handle_t edgehog_device);

static void publish_version(edgehog_device_handle_t edgehog_device);

static void publish_build_id(edgehog_device_handle_t edgehog_device);

/************************************************
 * Constants and defines
 ***********************************************/

#define FINGERPRINT_PROP "/fingerprint"
#define NAME_PROP "/name"
#define VERSION_PROP "/version"
#define BUILD_ID_PROP "/buildId"

/************************************************
 * Global functions definition
 ***********************************************/

void publish_base_image(edgehog_device_handle_t edgehog_device)
{
    publish_fingerprint(edgehog_device);
    publish_name(edgehog_device);
    publish_version(edgehog_device);
    publish_build_id(edgehog_device);
}

/************************************************
 * Static functions definition
 ***********************************************/

static void publish_fingerprint(edgehog_device_handle_t edgehog_device)
{
#if defined(APP_BUILD_VERSION)
    const char *hash = STRINGIFY(APP_BUILD_VERSION);

    if (check_empty_string_property(&io_edgehog_devicemanager_BaseImage, FINGERPRINT_PROP, hash)) {
        return;
    }

    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_BaseImage.name, FINGERPRINT_PROP, astarte_value_from_string(hash));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish " FINGERPRINT_PROP);
        return;
    }
#endif
}

static void publish_name(edgehog_device_handle_t edgehog_device)
{
#if defined(CONFIG_KERNEL_BIN_NAME)
    if (check_empty_string_property(
            &io_edgehog_devicemanager_BaseImage, NAME_PROP, CONFIG_KERNEL_BIN_NAME)) {
        return;
    }

    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_BaseImage.name, NAME_PROP,
        astarte_value_from_string(CONFIG_KERNEL_BIN_NAME));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish " NAME_PROP);
        return;
    }
#endif
}

static void publish_version(edgehog_device_handle_t edgehog_device)
{
#if defined(APP_VERSION_STRING)
    if (check_empty_string_property(
            &io_edgehog_devicemanager_BaseImage, VERSION_PROP, APP_VERSION_STRING)) {
        return;
    }

    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_BaseImage.name, VERSION_PROP,
        astarte_value_from_string(APP_VERSION_STRING));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish " VERSION_PROP);
        return;
    }
#endif
}

static void publish_build_id(edgehog_device_handle_t edgehog_device)
{
#if defined(CMAKE_BUILD_DATE_TIME)
    if (check_empty_string_property(
            &io_edgehog_devicemanager_BaseImage, BUILD_ID_PROP, CMAKE_BUILD_DATE_TIME)) {
        return;
    }

    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_BaseImage.name, BUILD_ID_PROP,
        astarte_value_from_string(CMAKE_BUILD_DATE_TIME));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish " BUILD_ID_PROP);
        return;
    }
#endif
}
