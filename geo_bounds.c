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

#define EARTH_RADIUS     6371.0

#define BEARING_SW_COS   -0.7071067811 /* cos(225 deg) */

#define DEG_TO_RAD_COEF  0.0174532925 /* (3.14159265 / 180) */
#define RAD_TO_DEG_COEF  57.2957795785 /* (180 / 3.14159265) */

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

    if ( (lat_s_deg == NULL) ||
        (lon_w_deg == NULL) ||
        (lat_n_deg == NULL) ||
        (lon_e_deg == NULL)
        )
    {
        return -1;
    }

    center_lat_rad = center_lat * DEG_TO_RAD_COEF;
    center_lon_rad = center_lon * DEG_TO_RAD_COEF;

    center_lat_rad_sin = sin(center_lat_rad);

    earth_rad = radius / EARTH_RADIUS;

    earth_rad_cos = cos(earth_rad);

    center_lat_sin_x_earth_cos = center_lat_rad_sin * earth_rad_cos;

    center_lat_cos_x_earth_sin_x_bearing_sw_cos = cos(center_lat_rad) * sin(earth_rad) * BEARING_SW_COS;

    coef_1 = center_lat_sin_x_earth_cos + center_lat_cos_x_earth_sin_x_bearing_sw_cos;
    coef_2 = center_lat_sin_x_earth_cos - center_lat_cos_x_earth_sin_x_bearing_sw_cos;

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

    return 0;
}

/*
 * Main - for testing
 */

#define CENTER_LAT        37.7749295
#define CENTER_LON        -122.4194155

int main(int argc, char** argv)
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
            CENTER_LAT,
            CENTER_LON,
            i,
            &lat_s_deg,
            &lon_w_deg,
            &lat_n_deg,
            &lon_e_deg
            );

        printf("Bounding box for (%3.6f, %3.6f) with distance %3.6f km:\n(%3.6f, %3.6f)\n(%3.6f, %3.6f)\n\n",
            CENTER_LAT,
            CENTER_LON,
            i,
            lat_s_deg,
            lon_w_deg,
            lat_n_deg,
            lon_e_deg
            );
    }

    /* The preceding code should generate this reference output:

        Bounding box for (37.774929, -122.419416) with distance 0.010000 km:
        (37.774866, -122.419496)
        (37.774993, -122.419335)

        Bounding box for (37.774929, -122.419416) with distance 0.100000 km:
        (37.774294, -122.420220)
        (37.775565, -122.418611)

        Bounding box for (37.774929, -122.419416) with distance 1.000000 km:
        (37.768570, -122.427460)
        (37.781288, -122.411370)

        Bounding box for (37.774929, -122.419416) with distance 10.000000 km:
        (37.711311, -122.499799)
        (37.838494, -122.338894)

        Bounding box for (37.774929, -122.419416) with distance 100.000000 km:
        (37.136314, -123.217094)
        (38.408074, -121.607896)

        Bounding box for (37.774929, -122.419416) with distance 1000.000000 km:
        (31.175618, -129.842217)
        (43.820306, -113.607232)
    */

    return error;
}

/*
 * Ruby bindings
 */

#include "ruby.h"

static VALUE cGeoBounds;

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

    return rb_ary_new3(
        4,
        rb_float_new(lat_s_deg),
        rb_float_new(lon_w_deg),
        rb_float_new(lat_n_deg),
        rb_float_new(lon_e_deg)
        );
}

void Init_GeoBounds()
{
    cGeoBounds = rb_define_class("GeoBounds", rb_cObject);
    rb_define_method(cGeoBounds, "get_geo_bounds", get_geo_bounds, 3);
}
