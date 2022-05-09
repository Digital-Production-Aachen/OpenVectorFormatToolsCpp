/*
---- Copyright Start ----

This file is part of the OpenVectorFormatTools collection. This collection provides tools to facilitate the usage of the OpenVectorFormat.

Copyright (C) 2022 Digital-Production-Aachen

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

---- Copyright End ----
*/

#include <iostream>
#include "ovf_reader_writer_export.h"
#include "open_vector_format.pb.h"
#include "ovf_file_reader.h"

namespace ovf = open_vector_format;

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Please specify a path to the .ovf file to read" << std::endl;
        return -1;
    }

    ovf::reader_writer::OvfFileReader reader{};

    ovf::Job job{};

    std::string path{argv[1]};

    reader.OpenFile(path, job);

    std::cout << job.DebugString();
    std::cout << "-------------------------------------------" << std::endl;


    ovf::WorkPlane wp{};
    reader.GetWorkPlaneShell(1, wp);

    std::cout << wp.DebugString();
    std::cout << "-------------------------------------------" << std::endl;

    reader.GetWorkPlane(1, wp);

    std::cout << wp.ShortDebugString() << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    ovf::VectorBlock vb{};
    reader.GetVectorBlock(1, 1, vb);

    std::cout << vb.ShortDebugString() << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    std::cout << "Finished" << std::endl;
    return 0;
}
