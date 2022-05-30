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

#include <optional>
#include <fstream>

#include "google/protobuf/util/delimited_message_util.h"

#include "ovf_file_writer.h"
#include "util.h"
#include "consts.h"

namespace open_vector_format::reader_writer {

OvfFileWriter::OvfFileWriter()
    : operation_{FileOperationState::kNone}
{
}


void OvfFileWriter::StartWritePartial(const Job& job_shell, const std::string path)
{
    if (operation_ != FileOperationState::kNone)
        throw std::runtime_error("Trying to start new write with write operation in progress");
    
    operation_ = FileOperationState::kPartialWrite;

    ofs_ = std::ofstream{path};

    WriteHeader(job_shell);

    current_wp_ = {};
}

void OvfFileWriter::AppendWorkPlane(const WorkPlane& wp)
{
    if (operation_ != FileOperationState::kPartialWrite)
        throw std::runtime_error("Trying to append work plane without partial write operation in progress");

    // actually write current_wp_ to stream
    if (current_wp_.has_value())
    {
        WriteFullWorkPlane(*current_wp_);
        current_wp_ = {};
    }

    // use wp as new workplane
    current_wp_ = WorkPlane{wp};
}

void OvfFileWriter::AppendVectorBlock(const VectorBlock& vb)
{
    if (operation_ != FileOperationState::kPartialWrite)
        throw std::runtime_error{"Trying to append vector block without partial write operation in progress"};

    if (!current_wp_.has_value())
        throw std::runtime_error("Trying to append vector block before writing first work plane");

    current_wp_->add_vector_blocks()->MergeFrom(vb);
}

void OvfFileWriter::FinishWrite()
{
    if (operation_ != FileOperationState::kPartialWrite)
        throw std::runtime_error("Trying to finish partial write without partial write operation in progress");
    
    WriteFooter();

    ofs_ = {};

    operation_ = FileOperationState::kNone;
}

void OvfFileWriter::WriteFullJob(const Job& job, const std::string path)
{
    if (operation_ != FileOperationState::kNone)
        throw std::runtime_error("Trying to start new write with write operation in progress");
    
    operation_ = FileOperationState::kCompleteWrite;

    ofs_ = std::ofstream{path};

    WriteHeader(job);

    for (int i = 0; i < job.num_work_planes(); i++)
        WriteFullWorkPlane(job.work_planes(i));

    WriteFooter();

    ofs_ = {};

    operation_ = FileOperationState::kNone;
}


Job& OvfFileWriter::job_shell()
{
    if (!job_shell_.has_value())
    {
        throw std::runtime_error("No write operation in progress");
    }
    
    return *job_shell_;
}


void OvfFileWriter::WriteHeader(const Job& job)
{
    CheckIsWriting();
    CheckFsHealth();

    job_shell_ = Job{};
    util::MergeExcluding(
        job,
        *job_shell_,
        [](const google::protobuf::FieldDescriptor& fd){return fd.name() == "work_planes";}
    );
    job_shell_->set_num_work_planes(0);

    ofs_->write((char*)kMagicBytes.data(), kMagicBytes.size());
    
    job_lut_offset_offset_ = ofs_->tellp();
    uint8_t dummy_offset[8];
    ofs_->write((char*)dummy_offset, 8);

    job_lut_ = JobLUT{};
}


void OvfFileWriter::WriteFullWorkPlane(const WorkPlane& wp)
{
    CheckIsWriting();
    CheckFsHealth();

    ofs_->seekp(0, std::ios::end);

    // add start offset of this workplane to job lut
    uint64_t workplane_offset = ofs_->tellp();
    job_lut_->add_workplanepositions(workplane_offset);

    // write placeholder for position of workplane lut
    uint8_t dummy_offset[8];
    ofs_->write((char*)dummy_offset, 8);

    WorkPlaneLUT wp_lut{};
    for (int i = 0; i < wp.vector_blocks_size(); i++)
    {
        uint64_t vb_position = ofs_->tellp();
        wp_lut.add_vectorblockspositions(vb_position);
        google::protobuf::util::SerializeDelimitedToOstream(
            wp.vector_blocks(i),
            &*ofs_
        );
    }

    // copy everything excluding vector blocks to shell object
    WorkPlane shell_to_write{};
    util::MergeExcluding(
        wp,
        shell_to_write,
        [](const google::protobuf::FieldDescriptor& fd){return fd.name() == "vector_blocks";}
    );
    
    // write next work plane number to shell
    auto next_work_plane_num = job_shell_->num_work_planes();
    shell_to_write.set_work_plane_number(next_work_plane_num);

    uint64_t workplane_shell_offset = ofs_->tellp();
    wp_lut.set_workplaneshellposition(workplane_shell_offset);
    google::protobuf::util::SerializeDelimitedToOstream(
        shell_to_write,
        &*ofs_
    );

    uint64_t workplane_lut_offset = ofs_->tellp();
    google::protobuf::util::SerializeDelimitedToOstream(
        wp_lut,
        &*ofs_
    );

    ofs_->seekp(workplane_offset);
    util::WriteAsLittleEndian(workplane_lut_offset, *ofs_);
    ofs_->seekp(0, std::ios::end);

    job_shell_->set_num_work_planes(job_shell_->num_work_planes() + 1);
}

void OvfFileWriter::WriteFooter()
{
    CheckIsWriting();
    CheckFsHealth();

    if (current_wp_.has_value())
    {
        WriteFullWorkPlane(*current_wp_);
        current_wp_ = {};
    }

    ofs_->seekp(0, std::ios::end);

    uint64_t job_shell_offset = ofs_->tellp();
    job_lut_->set_jobshellposition(job_shell_offset);
    google::protobuf::util::SerializeDelimitedToOstream(
        *job_shell_,
        &*ofs_
    );
    job_shell_ = {};

    uint64_t job_lut_offset = ofs_->tellp();
    ofs_->seekp(*job_lut_offset_offset_);
    util::WriteAsLittleEndian(job_lut_offset, *ofs_);
    job_lut_offset_offset_ = {};

    ofs_->seekp(0, std::ios::end);
    google::protobuf::util::SerializeDelimitedToOstream(
        *job_lut_,
        &*ofs_
    );
    job_lut_ = {};

    ofs_->flush();
    ofs_->close();
    ofs_ = {};
}

}