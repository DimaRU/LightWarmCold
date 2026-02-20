//
// Matter Light driver definitions
//

/** Standard max values (used for remapping attributes) */
#define STANDARD_BRIGHTNESS 255
#define STANDARD_TEMPERATURE_FACTOR 1000000

/** Matter max values (used for remapping attributes) */
#define MATTER_BRIGHTNESS 254
#define MATTER_TEMPERATURE_FACTOR 1000000

/** Default attribute values used during initialization */
#define DEFAULT_POWER true

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
