/*
  Copyright (C) 2021 anssi.grohn@karelia.fi
  
  This file is part of realsense-obs-plugin.
  
  realsense-obs-plugin is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  realsense-obs-plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with realsense-obs-plugin.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <obs-module.h>
#include <librealsense2/rs.hpp> 
#include <librealsense2/rs_advanced_mode.hpp>
#include <librealsense2/rsutil.h>
#include "realsense-device.h"
#include "obs_frame_processor.h"
#include <string>
#include <sstream>
#include <list>
#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>

using namespace std;
const char * DEVICE_LIST_NAME = "devices";

struct realsense_d400_source
{
  obs_source_t *source;
  realsense_device *rs2dev = { nullptr };
  rs2::config c;
  list<pair<string,string>> devices;
  rs2::context ctx;
  obs_frame_processor frame_processor;
  gs_texture_t *texture;

  realsense_d400_source()
  {
    source = nullptr;
    texture = nullptr;
  }
  virtual ~realsense_d400_source()
  {
    obs_enter_graphics();
    gs_texture_destroy(texture);
    obs_leave_graphics();
    delete rs2dev;
  }
};

static const char *realsense_d400_source_get_name(void *unused)
{
  UNUSED_PARAMETER(unused);
  return obs_module_text("Realsense D400 Input");
}

static list<pair<string,string>> get_camera_names_serials(realsense_d400_source & s)
{
  list<pair<string,string>> serials;
  //Add desired streams to configuration
  for( auto && dev : s.ctx.query_devices())
  {
    if ( dev.get_info(RS2_CAMERA_INFO_NAME) != std::string("Platform Camera") )
    {
      string serialnum = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
      string name = dev.get_info(RS2_CAMERA_INFO_NAME);
      serials.push_back(make_pair(name,serialnum));
    }
  }

  return serials;
}

static void realsense_d400_source_update(void *data, obs_data_t *settings)
{
  struct realsense_d400_source *context = reinterpret_cast<realsense_d400_source*>(data);
  const char *file = obs_data_get_string(settings, "file");
  const bool unload = obs_data_get_bool(settings, "unload");
  
  const char * serial = obs_data_get_string( settings, DEVICE_LIST_NAME );
  // if selected camera has changed, stop and delete previous one
  if (context->rs2dev != nullptr &&
      (context->rs2dev->serial_number != std::string(serial)) )
  {
    context->rs2dev->stop();
    delete context->rs2dev;
    context->rs2dev = nullptr;
  }

  // read values from settings
  uint16_t depthClampMin = (uint16_t) obs_data_get_int(settings, "depth_clamp_min");
  uint16_t depthClampMax = (uint16_t) obs_data_get_int(settings, "depth_clamp_max");
  uint16_t depthUnits    = (uint16_t) obs_data_get_int(settings, "depth_units");
  
  // set default values if zero
  if ( depthUnits == 0 )    depthUnits    = DEFAULT_DEPTH_UNITS;
  if ( depthClampMin == 0 ) depthClampMin = DEFAULT_DEPTH_CLAMP_MIN;
  if ( depthClampMax == 0 ) depthClampMax = DEFAULT_DEPTH_CLAMP_MAX;
  
  if ( std::string(serial) == "" ) return;
  
  if ( context->rs2dev != nullptr )
  {
    //cerr << "setting depth table values\n";
    context->rs2dev->depthClampMin = depthClampMin;
    context->rs2dev->depthClampMax = depthClampMax;
    context->rs2dev->depthUnits = depthUnits;
    context->rs2dev->set_limits();
    return;
  }
  try {
    context->rs2dev = new realsense_device(serial);

    // configure limits for depth clamp min and max
    context->rs2dev->depthClampMin = depthClampMin;
    context->rs2dev->depthClampMax = depthClampMax;
    context->rs2dev->depthUnits = depthUnits;
    context->rs2dev->start();
  
    obs_enter_graphics();
    
    if ( context->texture == nullptr )
    {
      context->frame_processor.init ( 848*2, 480, context->rs2dev);
      context->texture = gs_texture_create( 848*2, 480, GS_RGBA, 1,
        nullptr, GS_DYNAMIC);
    }
    obs_leave_graphics();


  }
  catch ( rs2::error & e )
  {
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    // make sure we don't have a device 
    delete context->rs2dev;
    context->rs2dev = nullptr;
  }
  catch ( std::exception & ex )
  {
    std::cerr << "Realsense exception " << ex.what() << "\n";
  }
 

}

static void *realsense_d400_source_create(obs_data_t *settings,
                                          obs_source_t *source)
{
  
  struct realsense_d400_source *context = new realsense_d400_source();
  context->source = source;
  realsense_d400_source_update(context, settings);
  return context;
}

static void realsense_d400_source_destroy(void *data)
{
  struct realsense_d400_source *context = reinterpret_cast<realsense_d400_source*>(data);
  
  context->rs2dev->stop();
  delete context;
}

static void realsense_d400_source_defaults(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "unload", false);
}

static void realsense_d400_source_show(void *data)
{
  struct realsense_d400_source *context = reinterpret_cast<realsense_d400_source*>(data);
}

static void realsense_d400_source_hide(void *data)
{
  struct realsense_d400_source *context = reinterpret_cast<realsense_d400_source*>(data);
}

static uint32_t realsense_d400_source_getwidth(void *data)
{
  struct realsense_d400_source *context = reinterpret_cast<realsense_d400_source*>(data);
  return 848*2;//context->if2.image.cx;
}

static uint32_t realsense_d400_source_getheight(void *data)
{
  struct realsense_d400_source *context = reinterpret_cast<realsense_d400_source*>(data);
  return 480;//context->if2.image.cy;
}

static void realsense_d400_source_tick(void *data, float seconds)
{
  struct realsense_d400_source *context = reinterpret_cast<realsense_d400_source*>(data);
  rs2::frameset frames;
  if ( context->rs2dev == nullptr ) return;
  
  if ( context->rs2dev->get_frame( frames))
  {
    if ( context->frame_processor.update_context( frames,
                                                  &context->rs2dev->align_to ))
    {

      const uint8_t *tmp = context->frame_processor.tmp_data.data();

      obs_enter_graphics();
      gs_texture_set_image( context->texture,
                            context->frame_processor.tmp_data.data(),
                            848*2*4, false);

      obs_leave_graphics();

    }
  }
}


static obs_properties_t *realsense_d400_source_properties(void *data)
{
  struct realsense_d400_source *context = reinterpret_cast<realsense_d400_source*>(data);
  // get device list
  if ( context->devices.empty())
  {
    context->devices = get_camera_names_serials(*context);
  }
  
  obs_properties_t *props = obs_properties_create();
  obs_property_t * device_list = obs_properties_add_list( props,
                                                          DEVICE_LIST_NAME,
                                                          "Available Realsense cameras",
                                                          OBS_COMBO_TYPE_LIST,
                                                          OBS_COMBO_FORMAT_STRING);
  // add devices to list
  for ( auto && tmp : context->devices )
  {
    stringstream ss;
    ss << tmp.first << " " << tmp.second;
    obs_property_list_add_string( device_list, ss.str().c_str(), tmp.second.c_str());
  }



  // define adjustable values
	obs_properties_add_int(props, "depth_clamp_min",
                         obs_module_text("Min recorded depth value"), 1, 65000,
                         1);
  
	obs_properties_add_int(props, "depth_clamp_max",
                         obs_module_text("Max recorded depth value"), 1, 65000,
                         1);
	obs_properties_add_int(props, "depth_units",
                         obs_module_text("Depth units in micrometers"), 100, 65000,
                         1);

  return props;
}
static void realsense_d400_source_render(void *data, gs_effect_t *effect)
{
  struct realsense_d400_source *context = reinterpret_cast<realsense_d400_source*>(data);

  if ( !context->texture ) return;

  
  obs_enter_graphics();
  gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
  gs_effect_set_texture(image, context->texture);
  gs_draw_sprite(context->texture, 0, context->frame_processor.video_width,
                 context->frame_processor.video_height );
  obs_leave_graphics();
  


}

// visual studio does not support C99 designated initializers.

/*struct obs_source_info realsense_d400_s = {
  .id           = "realsense-d400",
  .type         = OBS_SOURCE_TYPE_INPUT,
  .output_flags = OBS_SOURCE_VIDEO,
  .get_name     = realsense_d400_source_get_name,
  .create       = realsense_d400_source_create,
  .destroy      = realsense_d400_source_destroy,
  .get_width    = realsense_d400_source_getwidth,
  .get_height   = realsense_d400_source_getheight,
  .get_defaults = realsense_d400_source_defaults,
  .get_properties   = realsense_d400_source_properties,
  .update       = realsense_d400_source_update,
  .show         = realsense_d400_source_show,
  .hide         = realsense_d400_source_hide,
  .video_tick   = realsense_d400_source_tick,
  .video_render = realsense_d400_source_render
  };*/


struct obs_source_info realsense_d400_s = {
                                           "realsense-d400",
                                           OBS_SOURCE_TYPE_INPUT,
                                           OBS_SOURCE_VIDEO,
                                           realsense_d400_source_get_name,
                                           realsense_d400_source_create,
                                           realsense_d400_source_destroy,
                                           realsense_d400_source_getwidth,
                                           realsense_d400_source_getheight,
                                           realsense_d400_source_defaults,
                                           realsense_d400_source_properties,
                                           realsense_d400_source_update,
                                           nullptr,
                                           nullptr,
                                           realsense_d400_source_show,
                                           realsense_d400_source_hide,
                                           realsense_d400_source_tick,
                                           realsense_d400_source_render
};
