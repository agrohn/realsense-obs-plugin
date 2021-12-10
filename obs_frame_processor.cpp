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

#include "obs_frame_processor.h"
#include "realsense-device.h"
#include <iostream>
#include <algorithm>
using namespace std;
obs_frame_processor::obs_frame_processor() :  depth_to_disparity(true),
                                                                      disparity_to_depth(false),
                                                                      rs_device(nullptr)
{
	temporal.set_option(RS2_OPTION_HOLES_FILL, 1);
	temporal.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 10);
	temporal.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 1.0f);
	decimation.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2);
	spatial.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 1.0);
}

bool obs_frame_processor::update_context(rs2::frameset & frameset, rs2_stream * align_to)
{
  if ( rs_device == nullptr ) throw runtime_error("Realsense device not set in obs_frame_processor");
	rs2::video_frame vid_frame = frameset.first(*align_to);
	rs2::depth_frame depth_frame = frameset.get_depth_frame();

	if (!vid_frame || !depth_frame)
	{
          cerr << "one of two is missing\n";
          return false;
	}

	if (!depth_frame)
	{
          cerr << "depth frame is missing\n";
          return false;
	}
	rs2::depth_frame filtered = depth_frame;
	
	// filter execution order matters!
	filtered = decimation.process(filtered);
	filtered = depth_to_disparity.process(filtered);
	filtered = spatial.process(filtered);
	filtered = temporal.process(filtered);
	filtered = disparity_to_depth.process(filtered);
  filtered = hole_filling_filter.process(filtered);
  
	const uint8_t *rgb_data = reinterpret_cast<const uint8_t *>(vid_frame.get_data());
	const uint16_t *depth_data = reinterpret_cast<const uint16_t *>(filtered.get_data());
	if (depth_data == nullptr)
	{
		cerr << "depth data is null!\n";
		return false;
	}
	for (size_t h = 0;h < video_height;h++)
	{
		for (size_t w = 0;w < video_width / 2;w++)
		{

			size_t color_data_source_index = h * (video_width / 2) + w;
			size_t depth_index = h * video_width + w + (video_width / 2);
			size_t color_index = h * video_width + w;
			uint8_t R = rgb_data[color_data_source_index * 3];
			uint8_t G = rgb_data[color_data_source_index * 3 + 1];
			uint8_t B = rgb_data[color_data_source_index * 3 + 2];

			// Since we decimate depth data to half, we need to compute proper index here.
			// video_width / 4 since it is half of the half allocated image in respect to RGB image.
			size_t depth_data_source_index = (h / 2)*(video_width / 4) + w / 2;

			tmp_data[color_index * 4    ] = R;
			tmp_data[color_index * 4 + 1] = G;
			tmp_data[color_index * 4 + 2] = B;
      tmp_data[color_index * 4 + 3] = 255; // alpha

      // enforce depth values to 8-bit range, and mark 
			const uint16_t MAX_RGB_VALUE = 255;
      uint16_t dval16 = std::max((uint16_t)((depth_data[depth_data_source_index] - rs_device->depthClampMin) / 100), (uint16_t)0);
      if ( dval16 > MAX_RGB_VALUE ) dval16 = 0;

			uint8_t dval = (uint8_t)dval16;
			tmp_data[depth_index * 4    ] = dval;
			tmp_data[depth_index * 4 + 1] = dval;
			tmp_data[depth_index * 4 + 2] = dval;
			tmp_data[depth_index * 4 + 3] = 255; // alpha 
		}
	}

	return true;
}

void obs_frame_processor::init( size_t width, size_t height, realsense_device * device )
{
  video_width = width;
  video_height = height;
  rs_device = device;
  tmp_data.resize( width * height * 4 ); // RGBA
}


void obs_frame_processor::fill_rgba( uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  for (size_t h = 0;h < video_height;h++)
	{
		for (size_t w = 0;w < video_width;w++)
		{

			size_t color_index = h * video_width + w;
      tmp_data[color_index*4] = r;
      tmp_data[color_index*4+1] = g;
      tmp_data[color_index*4+2] = b;
      tmp_data[color_index*4+3] = a;
                  
    }
  }
}
