require "GeoBounds"

# API for retrieving twitter profile images
#
# http://localhost/geo_bounds?lat=37.7749295&lon=-122.4194155&radius=100

class GeoboundsController < ApplicationController
  def calculate

    lat_s = 0
    lon_w = 0
    lat_n = 0
    lon_e = 0

    if params[:lat] != nil && params[:lon] != nil && params[:radius] != nil
      gb=GeoBounds.new
      bounds = gb.get_geo_bounds(params[:lat], params[:lon], params[:radius])

      lat_s = bounds[0]
      lon_w = bounds[1]
      lat_n = bounds[2]
      lon_e = bounds[3]
    end

    render :json =>
    {
      :type => "Feature",
      :bbox => [lon_w, lat_s, lon_e, lat_n],
      :geometry =>
      {
        :type => "Polygon",
        :coordinates =>
        [[
          [lon_w, lat_s], [lon_w, lat_n], [lon_e, lat_n], [lon_e, lat_s], [lon_w, lat_s]
        ]]
      }
    }.to_json
  end
end
