ActionController::Routing::Routes.draw do |map|
  map.connect 'geo_bounds', :controller => 'geobounds', :action => 'calculate'

  map.root :controller => "geobounds", :action => 'calculate'
end
