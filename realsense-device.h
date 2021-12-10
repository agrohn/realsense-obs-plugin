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

#pragma once

#include <librealsense2/rs.hpp> 
#include <librealsense2/rs_advanced_mode.hpp>
#include <stdexcept>
#include <string>
#include <thread>
#include <atomic>
// some reasonable defaults for depth data limits
const uint16_t DEFAULT_DEPTH_CLAMP_MIN = 10000;
const uint16_t DEFAULT_DEPTH_CLAMP_MAX = 35500;
const uint16_t DEFAULT_DEPTH_UNITS = 100; // 100 micrometers, = 0.1mm
const size_t   DEFAULT_FRAME_QUEUE_CAPACITY = 5;

class realsense_device
{
public:
  rs2::pipeline pipe;
  rs2::pipeline_profile profile;
  rs2::align * align {nullptr};
  rs2_stream align_to;

  rs2::config cfg;
  rs2::frame_queue framequeue;
  std::thread * processing_thread;
  std::atomic_bool run_processing_thread;
  std::string serial_number;
  uint16_t depthClampMin = DEFAULT_DEPTH_CLAMP_MIN;
  uint16_t depthClampMax = DEFAULT_DEPTH_CLAMP_MAX;
  uint16_t depthUnits = DEFAULT_DEPTH_UNITS;
  
  realsense_device(const std::string & serial) : framequeue(DEFAULT_FRAME_QUEUE_CAPACITY),
                                                 serial_number(serial) {
    cfg.enable_stream(RS2_STREAM_COLOR, 848, 480, RS2_FORMAT_RGB8, 30);
    cfg.enable_stream(RS2_STREAM_DEPTH, 848, 480, RS2_FORMAT_Z16, 30);
    cfg.enable_device(serial);
  }
  virtual ~realsense_device()
  {
    
    delete align;

  }
  void start()
  {
    profile = pipe.start(cfg);
    align_to = profile.get_stream(RS2_STREAM_COLOR).stream_type();
    align = new rs2::align(align_to);
    set_limits();
    start_processing_thread();
  }
  void stop()
  {
    run_processing_thread = false;
    processing_thread->join();
    pipe.stop();
    
  }
       
  // returns true if frame was received, and sets param frameset to current set.
  bool get_frame( rs2::frameset & frameset )
  {
    return framequeue.poll_for_frame(&frameset);
  }


  void set_limits()
  {
    auto rs_dev = profile.get_device();
    auto depth_sensor = rs_dev.first<rs2::depth_sensor>();
    // this ought to set the actual depth unit value
    depth_sensor.set_option( RS2_OPTION_DEPTH_UNITS, 0.01);
    if (rs_dev.is<rs400::advanced_mode>())
    {

      auto adv_mode_dev = rs_dev.as<rs400::advanced_mode>();
      
      if (!adv_mode_dev.is_enabled())
      {
        adv_mode_dev.toggle_advanced_mode(true);
      }
      
      STDepthTableControl depth_table_control = adv_mode_dev.get_depth_table();
      depth_table_control.depthUnits = depthUnits;
      depth_table_control.depthClampMin = depthClampMin;
      depth_table_control.depthClampMax = depthClampMax;
      adv_mode_dev.set_depth_table(depth_table_control);
    }
    else
    {
      throw std::runtime_error("no advanced mode available for device!");
    }
  }
protected: 
  void start_processing_thread()
  {
    processing_thread = new std::thread(process_device, this);
  }
  
  static void process_device( realsense_device * dev )
  {
    bool hadFrames = false;
    while( dev->run_processing_thread  )
    {
      rs2::frameset frames;
      if ( dev->pipe.poll_for_frames(&frames) )
      {
        dev->framequeue.enqueue(frames);
      }
    }
  }

  
};
