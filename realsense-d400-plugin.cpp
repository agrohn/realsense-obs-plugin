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

/* Defines common functions (required) */
OBS_DECLARE_MODULE()

/* Implements common ini-based locale (optional) */
OBS_MODULE_USE_DEFAULT_LOCALE("realsense-d400-plugin", "en-US")

extern struct obs_source_info  realsense_d400_s;  /* Defined in my-source.c  */


bool obs_module_load(void)
{
        obs_register_source(&realsense_d400_s);
        
        return true;
}

