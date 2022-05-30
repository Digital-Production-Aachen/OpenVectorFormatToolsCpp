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

#pragma once

#include <fstream>
#include <optional>

#include "open_vector_format.pb.h"
#include "ovf_lut.pb.h"
#include "ovf_reader_writer_export.h"

namespace open_vector_format::reader_writer {

class OVF_READER_WRITER_EXPORT OvfFileWriter
{
public:
    OvfFileWriter();
    OvfFileWriter(const OvfFileWriter&) = delete;
    OvfFileWriter& operator=(const OvfFileWriter&) = delete;

    void StartWritePartial(const Job& job_shell, const std::string path);
    void AppendWorkPlane(const WorkPlane& wp);
    void AppendVectorBlock(const VectorBlock& vb);
    void FinishWrite();

    void WriteFullJob(const Job& job, const std::string path);

    Job& job_shell();

private:
    enum class FileOperationState
    {
        kNone,
        kPartialWrite,
        kCompleteWrite,
        kUndefined
    };

    FileOperationState operation_;
    
    std::optional<std::ofstream> ofs_;
    
    std::optional<WorkPlane> current_wp_;
    std::optional<Job> job_shell_;
    std::optional<JobLUT> job_lut_;
    std::optional<uint64_t> job_lut_offset_offset_;


    void WriteHeader(const Job& job);
    void WriteFullWorkPlane(const WorkPlane& wp);
    void WriteFooter();

    inline void CheckFsHealth()
    {
        if (!ofs_.has_value())
            throw std::runtime_error("Output file stream was not set");

        if (!ofs_->is_open())
            throw std::runtime_error("Output file stream closed unexpectedly");

        if (!ofs_->good())
            throw std::runtime_error("Output file stream encountered an error");
    }

    inline void CheckIsWriting()
    {
        if (operation_ != FileOperationState::kPartialWrite && operation_ != FileOperationState::kCompleteWrite)
            throw std::runtime_error("Trying to write while no write operation is active");
    }
};

}