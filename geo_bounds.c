/*
geo_bounds.c

Efficiently convert coordinate and radius to a bounding box,
using the Inverse Haversine formula.

Includes utility functions for converting to and from Morton
numbers (for Z-order curves), and calculating Morton distances.

*/

/*
 * Dependencies
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

/*
 * Static function declarations
 */

static unsigned long long
_get_morton_distance(
    unsigned long long morton_a,
    unsigned long long morton_b
    );

static int
_morton_to_latlon(
    unsigned long long morton_number,
    double*            lat,
    double*            lon
    );

static int
_latlon_to_morton(
    double              lat,
    double              lon,
    unsigned long long* morton_number
    );

static int
_get_bounding_box(
    double  center_lat,
    double  center_lon,
    double  radius,
    double* lat_s_deg,
    double* lon_w_deg,
    double* lat_n_deg,
    double* lon_e_deg
    );

/*
 * Constants
 */

#define EARTH_RADIUS     6371.0 /* km */

#define BEARING_SW_COS   -0.7071067812 /* cos(225 deg) */

#define DEG_TO_RAD_COEF  0.0174532925 /* (3.14159265 / 180) */
#define RAD_TO_DEG_COEF  57.295779579 /* (180 / 3.14159265) */

#define X_MASK 0x5555555555555555LLU
#define Y_MASK 0xaaaaaaaaaaaaaaaaLLU

/* avoid 'incompatible implicit declaration' warning */
long long int llabs(long long int num);

/*
 * Static function defintions
 */

static unsigned long long
_get_morton_distance(
    unsigned long long morton_a,
    unsigned long long morton_b
    )
{
    return (llabs((morton_a & X_MASK) - (morton_b & X_MASK)) & X_MASK) |
        (llabs((morton_a & Y_MASK) - (morton_b & Y_MASK)) & Y_MASK);
}

static int
_morton_to_latlon(
    unsigned long long morton_number,
    double*            lat,
    double*            lon
    )
{
    unsigned int lat_norm = 0;
    unsigned int lon_norm = 0;
    unsigned int i        = 0;

    /* check pointers */
    if ( (lat == NULL) ||
        (lon == NULL)
        )
    {
        return -1;
    }

    /* de-interleave bits */
    for (i = 0; i < sizeof(lat_norm) * CHAR_BIT; i++)
    {
        lat_norm |= (unsigned int)(((morton_number >> (i*2)) & 0x1) << i);
        lon_norm |= (unsigned int)(((morton_number >> (i*2+1)) & 0x1) << i);
    }

    /* de-normalize to doubles */
    *lat = (lat_norm / 10000000.0) - 90;
    *lon = (lon_norm / 10000000.0) - 180;

    return 0;
}

static int
_latlon_to_morton(
    double              lat,
    double              lon,
    unsigned long long* morton_number
    )
{
    unsigned int lat_norm = 0;
    unsigned int lon_norm = 0;
    unsigned int i        = 0;

    /* check pointers */
    if (morton_number == NULL)
    {
        return -1;
    }

    /* check bounds */
    if ( (lat < -90) ||
        (lat > 90) ||
        (lon < -180) ||
        (lon > 180)
        )
    {
        return -1;
    }

    /* normalize to integers */
    lat_norm = (unsigned int)((lat + 90)*10000000);
    lon_norm = (unsigned int)((lon + 180)*10000000);

    /* interleave bits */
    for (i = 0; i < sizeof(lat_norm) * CHAR_BIT; i++)
    {
        *morton_number |= (unsigned long long)((lat_norm & (0x1 << i))) << i;
        *morton_number |= (unsigned long long)((lon_norm & (0x1 << i))) << (i+1);
    }

    return 0;
}

static int
_get_bounding_box(
    double  center_lat,
    double  center_lon,
    double  radius,
    double* lat_s_deg,
    double* lon_w_deg,
    double* lat_n_deg,
    double* lon_e_deg
    )
{
    double center_lat_rad = 0;
    double center_lon_rad = 0;

    double center_lat_rad_sin = 0;

    double earth_deg      = 0;
    double earth_rad      = 0;

    double earth_rad_cos  = 0;

    double center_lat_sin_x_earth_cos = 0;
    double center_lat_cos_x_earth_sin_x_bearing_sw_cos = 0;
    
    double coef_1 = 0;
    double coef_2 = 0;

    /* check pointers */
    if ( (lat_s_deg == NULL) ||
        (lon_w_deg == NULL) ||
        (lat_n_deg == NULL) ||
        (lon_e_deg == NULL)
        )
    {
        return -1;
    }

    /* check bounds */
    if ( (center_lat < -90) ||
        (center_lat > 90) ||
        (center_lon < -180) ||
        (center_lon > 180)
        )
    {
        return -1;
    }

    /* calculate coefficients */
    center_lat_rad = center_lat * DEG_TO_RAD_COEF;
    center_lon_rad = center_lon * DEG_TO_RAD_COEF;

    center_lat_rad_sin = sin(center_lat_rad);

    earth_rad = radius / EARTH_RADIUS;

    earth_rad_cos = cos(earth_rad);

    center_lat_sin_x_earth_cos = center_lat_rad_sin * earth_rad_cos;

    center_lat_cos_x_earth_sin_x_bearing_sw_cos = cos(center_lat_rad) * sin(earth_rad) * BEARING_SW_COS;

    coef_1 = center_lat_sin_x_earth_cos + center_lat_cos_x_earth_sin_x_bearing_sw_cos;
    coef_2 = center_lat_sin_x_earth_cos - center_lat_cos_x_earth_sin_x_bearing_sw_cos;

    /* Inverse Haversine */
    *lat_s_deg = RAD_TO_DEG_COEF * asin(coef_1);

    *lon_w_deg = RAD_TO_DEG_COEF *
                (center_lon_rad +
                 atan2(
                    center_lat_cos_x_earth_sin_x_bearing_sw_cos,
                    earth_rad_cos - center_lat_rad_sin * coef_1
                    ));

    *lat_n_deg = RAD_TO_DEG_COEF * asin(coef_2);

    *lon_e_deg = RAD_TO_DEG_COEF *
                 (center_lon_rad +
                  atan2(
                    -1.0 * center_lat_cos_x_earth_sin_x_bearing_sw_cos,
                    earth_rad_cos - center_lat_rad_sin * coef_2
                    ));
    
    /* check bounds on longitudes */
    if (*lon_w_deg > 180)
    {
        *lon_w_deg -= 360;
    }
    else if (*lon_w_deg < -180)
    {
        *lon_w_deg += 360;
    }

    if (*lon_e_deg > 180)
    {
        *lon_e_deg -= 360;
    }
    else if (*lon_e_deg < -180)
    {
        *lon_e_deg += 360;
    }

    /* round to 7 decimals */
    *lat_s_deg = floor(*lat_s_deg * 10000000 + 0.5) / 10000000;
    *lon_w_deg = floor(*lon_w_deg * 10000000 + 0.5) / 10000000;
    *lat_n_deg = floor(*lat_n_deg * 10000000 + 0.5) / 10000000;
    *lon_e_deg = floor(*lon_e_deg * 10000000 + 0.5) / 10000000;

    return 0;
}

/*
 * Testing
 */

int test_coord(double lat, double lon)
{
    int    error     = 0;
    
    double lat_s_deg = 0;
    double lon_w_deg = 0;
    double lat_n_deg = 0;
    double lon_e_deg = 0;

    double i         = 0;
    
    unsigned long long morton    = 0;
    unsigned long long morton_sw = 0;
    unsigned long long morton_ne = 0;

    for (i = .01; i <= 1000.0; i = i * 10.0)
    {
        error = _get_bounding_box(
            lat,
            lon,
            i,
            &lat_s_deg,
            &lon_w_deg,
            &lat_n_deg,
            &lon_e_deg
            );
        if (error == 0)
        {
            _latlon_to_morton(lat, lon, &morton);
            _latlon_to_morton(lat_s_deg, lon_w_deg, &morton_sw);
            _latlon_to_morton(lat_n_deg, lon_e_deg, &morton_ne);

            printf("Bounding box with distance %8.3fkm [             morton # :    distance to center]\n", i);
            printf("sw:       (% 11.7f,% 12.7f): [%21llu : %21llu]\n",
                lat_s_deg,
                lon_w_deg,
                morton_sw,
                _get_morton_distance(morton_sw, morton)
                );
            printf("center:   (% 11.7f,% 12.7f): [%21llu]\n",
                lat,
                lon,
                morton
                );
            printf("ne:       (% 11.7f,% 12.7f): [%21llu : %21llu]\n\n",
                lat_n_deg,
                lon_e_deg,
                morton_ne,
                _get_morton_distance(morton, morton_ne)
                );
        }
        else
        {
            printf("Error =  %d\n\n", error);
        }
    }

    return error;
}

int main(int argc, char** argv)
{
    int error = 0;

    /* the following code should generate output matching the reference file */
    error = test_coord(37.7749295, -122.4194155);
    error = test_coord(-90, -180);
    error = test_coord(0, -180);
    error = test_coord(-90, 0);
    error = test_coord(0, 0);
    error = test_coord(90, 0);
    error = test_coord(0, 180);
    error = test_coord(90, 180);

    return error;
}

/*
 * Ruby bindings
 */

#include "ruby.h"

static VALUE cGeoBounds;

/*
 * get_geo_bounds()
 *
 * Convert latitude, longitude, and radius to a bounding box.
 *
 * Parameters:
 *  center_lat: latitude of center coordinate, in decimal degrees
 *  center_lon: longitude of center coordinate, in decimal degrees
 *  radius:     radius of bounding box, in kilometers
 *
 * Output:
 *  4-element array of bounding box coordinates, in decimal degrees:
 *   [south, west, north, east]
 */

static VALUE
get_geo_bounds(
    self,
    center_lat,
    center_lon,
    radius
    )
    VALUE self, center_lat, center_lon, radius;
{
    int    error          = 0;

    double lat_s_deg      = 0.0;
    double lon_w_deg      = 0.0;
    double lat_n_deg      = 0.0;
    double lon_e_deg      = 0.0;

    double dbl_center_lat = 0.0;
    double dbl_center_lon = 0.0;
    double dbl_radius     = 0.0;

    dbl_center_lat = RFLOAT(rb_Float(center_lat))->value;
    dbl_center_lon = RFLOAT(rb_Float(center_lon))->value;
    dbl_radius     = RFLOAT(rb_Float(radius))->value;

    error = _get_bounding_box(
        dbl_center_lat,
        dbl_center_lon,
        dbl_radius,
        &lat_s_deg,
        &lon_w_deg,
        &lat_n_deg,
        &lon_e_deg
        );
    if (error == 0)
    {
        return rb_ary_new3(
            4,
            rb_float_new(lat_s_deg),
            rb_float_new(lon_w_deg),
            rb_float_new(lat_n_deg),
            rb_float_new(lon_e_deg)
            );
    }

    return 0;
}

/*
 * latlon_to_morton()
 *
 * Convert latitude, longitude to a Morton number.
 *
 * Parameters:
 *  lat: latitude in decimal degrees
 *  lon: longitude in decimal degrees
 *
 * Output:
 *  64-bit Morton number corresponding to the lat, lon
 */
static VALUE
latlon_to_morton(
    self,
    lat,
    lon
    )
    VALUE self, lat, lon;
{
    int    error   = 0;

    double dbl_lat = 0.0;
    double dbl_lon = 0.0;

    unsigned long long morton = 0;

    dbl_lat = RFLOAT(rb_Float(lat))->value;
    dbl_lon = RFLOAT(rb_Float(lon))->value;

    error = _latlon_to_morton(
        dbl_lat,
        dbl_lon,
        &morton
        );
    if (error == 0)
    {
        return rb_float_new(morton);
    }

    return 0;
}

/*
 * morton_to_latlon()
 *
 * Convert a Morton number to latitude and longitude.
 *
 * Parameters:
 *  morton: 64-bit Morton number
 *
 * Output:
 *  2-element array of latitude and longitude
 */
static VALUE
morton_to_latlon(
    self,
    morton
    )
    VALUE self, morton;
{
    int    error    = 0;

    double dbl_lat = 0.0;
    double dbl_lon = 0.0;

    error = _morton_to_latlon(
        morton,
        &dbl_lat,
        &dbl_lon
        );
    if (error == 0)
    {
        return rb_ary_new3(
            2,
            rb_float_new(dbl_lat),
            rb_float_new(dbl_lon)
            );
    }

    return 0;
}

/*
 * get_morton_distance()
 *
 * Calculate the distance between two Morton numbers.
 *
 * Parameters:
 *  morton_a: first 64-bit Morton number
 *  morton_b: second 64-bit Morton number
 *
 * Output:
 *  64-bit integer representing the Morton distance
 */
static VALUE
get_morton_distance(
    self,
    morton_a,
    morton_b
    )
    VALUE self, morton_a, morton_b;
{
    return rb_float_new(
        _get_morton_distance(
            RFLOAT(rb_Float(morton_a))->value,
            RFLOAT(rb_Float(morton_b))->value
            )
        );
}

void Init_GeoBounds()
{
    cGeoBounds = rb_define_class("GeoBounds", rb_cObject);
    rb_define_method(cGeoBounds, "get_geo_bounds", get_geo_bounds, 3);
    rb_define_method(cGeoBounds, "latlon_to_morton", latlon_to_morton, 2);
    rb_define_method(cGeoBounds, "morton_to_latlon", morton_to_latlon, 1);
    rb_define_method(cGeoBounds, "get_morton_distance", get_morton_distance, 2);
}
