require "GeoBounds"

def test_coord(lat, lon)
  gb=GeoBounds.new

  distance = 0.01
  while (distance <= 1000.0) do
    bounds = gb.get_geo_bounds(lat, lon, distance)

    if (bounds != false)
      print "Bounding box for (%3.6f, %3.6f) with distance %3.6f km:\n(%3.6f, %3.6f)\n(%3.6f, %3.6f)\n\n" % [lat, lon, distance, bounds[0], bounds[1], bounds[2], bounds[3]]
    else
      print "Error for (%3.6f, %3.6f) with distance %3.6f km\n\n" % [lat, lon, distance]
    end

    distance = distance * 10
  end
end

test_coord(37.7749295, -122.4194155)
test_coord(-90, -180)
test_coord(0, -180)
