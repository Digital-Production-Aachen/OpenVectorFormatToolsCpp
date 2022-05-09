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

#include <optional>
#include <fstream>
#include <optional>
#include <shared_mutex>

#ifdef WIN32
#  include "memory_mapping_win32.h"
#else
#  error POSIX
#endif

#include "open_vector_format.pb.h"
#include "ovf_lut.pb.h"
#include "ovf_reader_writer_export.h"


namespace open_vector_format::reader_writer {


class OvfFileReader
{
public:
    // 64MiB
    OvfFileReader(size_t auto_cache_threshold = 67108864);
    OvfFileReader(const OvfFileReader&) = delete;
    OvfFileReader& operator=(const OvfFileReader&) = delete;
    ~OvfFileReader();

    void OpenFile(const std::string path, Job& job);
    void CloseFile();
    bool IsFileOpen() const;

    void GetWorkPlane(const int i_work_plane, WorkPlane& wp) const;
    void GetWorkPlaneShell(const int i_work_plane, WorkPlane& wp) const;
    void GetVectorBlock(const int i_work_plane, const int i_vector_block, VectorBlock& vb) const;

    void CacheFullJob();
    void CacheWorkPlaneShells();
    void ClearCache();
    bool IsWorkPlaneShellsCached() const;
    bool IsFullJobCached() const;

private:
    mutable std::shared_mutex rwlock_;

    std::optional<std::string> path_;
    std::optional<MemoryMapping> mapping_;

    std::optional<Job> job_shell_;
    std::optional<JobLUT> job_lut_;
    std::optional<std::vector<WorkPlaneLUT>> wp_luts_;
    
    size_t auto_cache_threshold_;
    std::optional<Job> cache_;
    std::optional<bool> are_vector_blocks_cached_;
    
    void GetWorkPlaneImpl(const int i_work_plane, WorkPlane& wp, bool include_vector_blocks, bool try_cache = true) const;
    void GetVectorBlockImpl(const int i_work_plane, const int i_vector_block, VectorBlock& vb, bool try_cache = true) const;
    void GetVectorBlocksImpl(const int i_work_plane, WorkPlane& wp, MemoryMapping::FileView& work_plane_view, size_t wp_offset_abs) const;

    inline void CheckIsFileOpened() const
    {
        if (!path_.has_value() || !mapping_.has_value() || !job_lut_.has_value())
            throw std::runtime_error("Can't read data without opening a file first");
    }

    inline MemoryMapping::FileView GetWorkPlaneFileView(const int i_work_plane, size_t *start_offset = nullptr, size_t *end_offset = nullptr) const
    {
        auto count_work_planes = job_lut_->workplanepositions_size();
    
        if (i_work_plane < 0 || i_work_plane > count_work_planes - 1)
            throw std::runtime_error("Invalid work plane index");
        
        size_t lower_offset = job_lut_->workplanepositions(i_work_plane);
        size_t upper_offset = i_work_plane < count_work_planes - 1
            ? job_lut_->workplanepositions(i_work_plane + 1)
            : job_lut_->jobshellposition();

        if (start_offset != nullptr) *start_offset = lower_offset;
        if (end_offset != nullptr) *end_offset = upper_offset;

        return mapping_->CreateView(lower_offset, (upper_offset - lower_offset));
    }
};

}