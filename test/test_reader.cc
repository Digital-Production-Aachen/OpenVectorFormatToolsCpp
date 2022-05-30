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

#include <catch2/catch_test_macros.hpp>

#include "google/protobuf/util/message_differencer.h"

#include "ovf_reader_writer_export.h"
#include "open_vector_format.pb.h"
#include "ovf_file_reader.h"

namespace ovf = open_vector_format;

TEST_CASE( "", "[reader]" ) {
    ovf::reader_writer::OvfFileReader reader{};

    SECTION( "correctly reads file with empty/default protobuf objects" ) {
        ovf::Job job{};
        ovf::Job emptyJob{};
        reader.OpenFile("empty.ovf", job);
        
        REQUIRE( reader.IsFileOpen() );
        REQUIRE( google::protobuf::util::MessageDifferencer::Equivalent(job, emptyJob) );

        ovf::WorkPlane wp{};
        REQUIRE_THROWS_AS( reader.GetWorkPlane(0, wp), std::runtime_error );
        REQUIRE_THROWS_AS( reader.GetWorkPlane(1, wp), std::runtime_error );
        REQUIRE_THROWS_AS( reader.GetWorkPlaneShell(0, wp), std::runtime_error );
        REQUIRE_THROWS_AS( reader.GetWorkPlaneShell(1, wp), std::runtime_error );

        ovf::VectorBlock vb{};
        REQUIRE_THROWS_AS( reader.GetVectorBlock(0, 0, vb), std::runtime_error );
        REQUIRE_THROWS_AS( reader.GetVectorBlock(0, 1, vb), std::runtime_error );
        REQUIRE_THROWS_AS( reader.GetVectorBlock(1, 0, vb), std::runtime_error );
        REQUIRE_THROWS_AS( reader.GetVectorBlock(1, 1, vb), std::runtime_error );
        
        reader.CloseFile();
        REQUIRE( !reader.IsFileOpen() );
    }
}