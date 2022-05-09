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