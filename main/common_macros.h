#pragma once

#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/** Remap attribute values
 *
 * This can be used to remap attribute values to different ranges.
 * Example: To convert the brightness value (0-255) into brightness percentage (0-100) and vice-versa.
 */
#define REMAP_TO_RANGE(value, from, to) ((value * to) / from)

/** Remap attribute values with inverse dependency
 *
 * This can be used to remap attribute values with inverse dependency to different ranges.
 * Example: To convert the temperature mireds into temperature kelvin and vice-versa where the relation between them
 * is: Mireds = 1,000,000/Kelvin.
 */
#define REMAP_TO_RANGE_INVERSE(value, factor) (factor / (value ? value : 1))

#define ABORT_APP_ON_FAILURE(x, ...) do {           \
        if (!(unlikely(x))) {                       \
            __VA_ARGS__;                            \
            vTaskDelay(5000 / portTICK_PERIOD_MS);  \
            abort();                                \
        }                                           \
    } while (0)

