require "GeoBounds"

def test_coord(lat, lon)
  gb=GeoBounds.new

  distance = 0.01
  while (distance <= 1000.0) do
    bounds = gb.get_geo_bounds(lat, lon, distance)

    if (bounds != false)
      morton_sw = gb.latlon_to_morton(bounds[0], bounds[1])
      morton = gb.latlon_to_morton(lat, lon)
      morton_ne = gb.latlon_to_morton(bounds[2], bounds[3])
      print "Bounding box with distance %8.3fkm [             morton # :    distance to center]\n" % [distance]
      print "sw:       (% 11.7f,% 12.7f): [%21u : %21u]\n" % [bounds[0], bounds[1], morton_sw, gb.get_morton_distance(morton_sw, morton)]
      print "center:   (% 11.7f,% 12.7f): [%21u]\n" % [lat, lon, morton]
      print "ne:       (% 11.7f,% 12.7f): [%21u : %21u]\n\n" % [bounds[2], bounds[3], morton_ne, gb.get_morton_distance(morton, morton_ne)]
    else
      print "Error for (%3.7f, %3.7f) with distance %3.7f km\n\n" % [lat, lon, distance]
    end

    distance = distance * 10
  end
end

test_coord(37.7749295, -122.4194155)
test_coord(-90, -180)
test_coord(0, -180)
test_coord(-90, 0)
test_coord(0, 0)
test_coord(90, 0)
test_coord(0, 180)
test_coord(90, 180)

#lat = -90
#lon = -180

#distance = 0.01
#gb=GeoBounds.new
#while (distance <= 1000.0) do
#  for lat in (-90..90)
#    for lon in (-180..180)
#      bounds = gb.get_geo_bounds(lat, lon, distance)
#    end
#  end
#  distance = distance * 10
#end
