mode(-1);
// Copyright (C) 2008  Andreas Stewering
//
// This file is part of Etherlab.
//
// Etherlab is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Etherlab is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Etherlab; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA//
// ====================================================================
try
CURRENT_DIR=getcwd()
// specific part
libname='libetherlab_scicos_rt.so'

DIR = get_absolute_file_path('loader_src.sce');
names     = ['etl_scicos_realtime']; // ; Seperator
chdir(DIR)
if isdef('dynlinkNR_etherlab') then
        ulink(dynlinkNR_etherlab);
end


dynlinkNR_etherlab=link(DIR+libname,names,'c');
catch
  disp('error loading '+libname+' !');
end
clear names;
clear libname
clear DIR
clear CURRENT_DIR

