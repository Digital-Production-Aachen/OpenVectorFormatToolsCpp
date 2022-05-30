/*
---- Copyright Start ----

MIT License

Copyright (c) 2022 Digital-Production-Aachen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

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
