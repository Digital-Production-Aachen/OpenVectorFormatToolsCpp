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