/*
geo_bounds.c

Efficiently convert coordinate and radius to a bounding box,
using the Inverse Haversine formula.
*/

/*
 * Dependencies
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/*
 * Static function declarations
 */

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

/*
 * Static function defintions
 */

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
            printf("Bounding box for (%3.7f, %3.7f) with distance %3.7f km:\n(%3.7f, %3.7f)\n(%3.7f, %3.7f)\n\n",
                lat,
                lon,
                i,
                lat_s_deg,
                lon_w_deg,
                lat_n_deg,
                lon_e_deg
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
    int    error     = 0;

    /* the following code should generate output matching the reference file */
    error = test_coord(37.7749295, -122.4194155);
    error = test_coord(-90, -180);
    error = test_coord(0, -180);

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
    int       error          = 0;

    double    lat_s_deg      = 0.0;
    double    lon_w_deg      = 0.0;
    double    lat_n_deg      = 0.0;
    double    lon_e_deg      = 0.0;

    double    dbl_center_lat = 0.0;
    double    dbl_center_lon = 0.0;
    double    dbl_radius     = 0.0;

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

void Init_GeoBounds()
{
    cGeoBounds = rb_define_class("GeoBounds", rb_cObject);
    rb_define_method(cGeoBounds, "get_geo_bounds", get_geo_bounds, 3);
}
