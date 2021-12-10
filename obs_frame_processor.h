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
#include <vector>
#include <librealsense2/rs.hpp> 

class realsense_device;

class obs_frame_processor 
{
public:
  std::vector<uint8_t> tmp_data;
  rs2::decimation_filter decimation;
  rs2::spatial_filter spatial;
  rs2::temporal_filter temporal;
  rs2::disparity_transform depth_to_disparity;
  rs2::disparity_transform disparity_to_depth;
  rs2::hole_filling_filter hole_filling_filter;
  realsense_device *rs_device;
  size_t video_width{ 0 };
  size_t video_height{ 0 };
	
  
  obs_frame_processor();
  bool update_context(rs2::frameset & frameset, rs2_stream * align_to);
  void init( size_t width, size_t height, realsense_device *device );
  void fill_rgba( uint8_t r, uint8_t g, uint8_t b, uint8_t a);
};
