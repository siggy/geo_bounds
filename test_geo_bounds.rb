require "GeoBounds"

gb=GeoBounds.new

distance = 0.01
while (distance <= 1000.0) do
  bounds = gb.get_geo_bounds(37.7749295, -122.4194155, distance)
  
  output = "Bounding box for (%3.6f, %3.6f) with distance %3.6f km:\n(%3.6f, %3.6f)\n(%3.6f, %3.6f)\n" % [37.7749295, -122.4194155, distance, bounds[0], bounds[1], bounds[2], bounds[3]]
  print output, "\n"
  
  distance = distance * 10
end
