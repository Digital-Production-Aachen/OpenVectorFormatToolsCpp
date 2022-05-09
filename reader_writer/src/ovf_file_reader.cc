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

#include <optional>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <shared_mutex>

#include "google/protobuf/util/delimited_message_util.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

#include "ovf_file_reader.h"
#include "open_vector_format.pb.h"
#include "consts.h"
#include "util.h"

#ifdef WIN32
#  include "memory_mapping_win32.h"
#else
#  error POSIX
#endif

namespace open_vector_format::reader_writer {


OvfFileReader::OvfFileReader(size_t auto_cache_threshold)
    : auto_cache_threshold_{auto_cache_threshold}
{
}

OvfFileReader::~OvfFileReader()
{
}

void OvfFileReader::OpenFile(const std::string path, Job& job)
{
    CloseFile();

    std::unique_lock lock{rwlock_};

    path_ = path;
    mapping_.emplace(path);

    if (mapping_->file_size() < 12)
    {
        CloseFile();
        throw std::runtime_error("File \"" + path + "\" is empty");
    }

    int64_t job_lut_offset_raw;
    {
        auto header_view = mapping_->CreateView(0, kMagicBytes.size() + 8);
        if (!std::equal(kMagicBytes.begin(), kMagicBytes.end(), header_view.data()))
        {
            CloseFile();
            throw std::runtime_error("File does not appear to be an ovf file");
        }

        // read job lut offset
        util::ReadFromLittleEndian(job_lut_offset_raw, header_view.data() + kMagicBytes.size());
    }
    
    if (job_lut_offset_raw < 0 || job_lut_offset_raw == kDefaultLutOffset)
    {
        throw std::runtime_error("binary file is not an ovf file or corrupted");
    }

    size_t job_lut_offset = (size_t)job_lut_offset_raw;

    // read job lut
    job_lut_ = JobLUT{};
    {
        auto job_lut_view = mapping_->CreateView(job_lut_offset, 0); // offset up until EOF
        google::protobuf::io::ArrayInputStream zcs{job_lut_view.data(), (int)job_lut_view.size()};
        google::protobuf::util::ParseDelimitedFromZeroCopyStream(
            &*job_lut_,
            &zcs,
            nullptr
        );
    }

    // read work plane luts
    wp_luts_.emplace(job_lut_->workplanepositions_size());
    for (int i = 0; i < job_lut_->workplanepositions_size(); i++)
    {
        size_t wp_offset_abs;
        auto wp_view = GetWorkPlaneFileView(i, &wp_offset_abs);
        
        int64_t wp_lut_offset_raw;
        util::ReadFromLittleEndian(wp_lut_offset_raw, wp_view.data());
        size_t wp_lut_offset_abs = (size_t)wp_lut_offset_raw;
        size_t wp_lut_offset_local = wp_lut_offset_abs - wp_offset_abs;

        google::protobuf::io::ArrayInputStream zcs{
            wp_view.data() + wp_lut_offset_local,
            (int)(wp_view.size() - wp_lut_offset_local)
        };
        google::protobuf::util::ParseDelimitedFromZeroCopyStream(
            &wp_luts_.value()[i],
            &zcs,
            nullptr
        );
    }

    // read job shell
    job.Clear();
    {
        auto job_shell_view = mapping_->CreateView((uint64_t)job_lut_->jobshellposition(), 0);
        google::protobuf::io::ArrayInputStream zcs{job_shell_view.data(), (int)job_shell_view.size()};
        google::protobuf::util::ParseDelimitedFromZeroCopyStream(
            &job,
            &zcs,
            nullptr
        );
    }

    job_shell_.emplace(job);
    are_vector_blocks_cached_ = false;

    lock.unlock();

    if (mapping_->file_size() > auto_cache_threshold_)
    {
        CacheFullJob();
    }
}

void OvfFileReader::CloseFile()
{
    std::unique_lock lock{rwlock_};

    path_.reset();
    mapping_.reset();
    
    job_shell_.reset();
    job_lut_.reset();
    wp_luts_.reset();
    
    cache_.reset();
    are_vector_blocks_cached_.reset();
}

bool OvfFileReader::IsFileOpen() const
{
    return mapping_.has_value();
}



void OvfFileReader::GetWorkPlane(const int i_work_plane, WorkPlane& wp) const
{
    std::shared_lock lock{rwlock_};
    CheckIsFileOpened();

    GetWorkPlaneImpl(i_work_plane, wp, true);
}

void OvfFileReader::GetWorkPlaneShell(const int i_work_plane, WorkPlane& wp) const
{
    std::shared_lock lock{rwlock_};
    CheckIsFileOpened();
    
    GetWorkPlaneImpl(i_work_plane, wp, false);
}

void OvfFileReader::GetVectorBlock(const int i_work_plane, const int i_vector_block, VectorBlock& vb) const
{
    std::shared_lock lock{rwlock_};
    CheckIsFileOpened();

    GetVectorBlockImpl(i_work_plane, i_vector_block, vb);
}



void OvfFileReader::CacheWorkPlaneShells()
{
    std::unique_lock lock{rwlock_};

    if (cache_.has_value() && *are_vector_blocks_cached_)
    {
        // remove vector blocks from cache
        for (int i = 0; i < cache_->work_planes_size(); i++)
        {
            cache_->mutable_work_planes(i)->clear_vector_blocks();
        }
    }
    else if (!cache_.has_value())
    {
        // create new cache
        cache_.emplace(*job_shell_);
        for (int i = 0; i < job_shell_->num_work_planes(); i++)
        {
            auto wp = cache_->add_work_planes();
            GetWorkPlaneImpl(i, *wp, false, false);
        }
    }

    are_vector_blocks_cached_ = false;
}

void OvfFileReader::CacheFullJob()
{
    std::unique_lock lock{rwlock_};

    if (cache_.has_value() && !*are_vector_blocks_cached_)
    {
        // add vector blocks to cache
        for (int i = 0; i < cache_->work_planes_size(); i++)
        {
            size_t wp_offset_abs;
            auto work_plane_view = GetWorkPlaneFileView(i, &wp_offset_abs);

            GetVectorBlocksImpl(i, *cache_->mutable_work_planes(i), work_plane_view, wp_offset_abs);
        }
    }
    else if (!cache_.has_value())
    {
        // create new cache
        cache_.emplace(*job_shell_);
        for (int i = 0; i < job_shell_->num_work_planes(); i++)
        {
            auto wp = cache_->add_work_planes();
            GetWorkPlaneImpl(i, *wp, true, false);
        }
    }

    are_vector_blocks_cached_ = true;
}

void OvfFileReader::ClearCache()
{
    std::unique_lock lock{rwlock_};

    cache_.reset();
    are_vector_blocks_cached_ = false;
}

bool OvfFileReader::IsWorkPlaneShellsCached() const
{
    return cache_.has_value();
}

bool OvfFileReader::IsFullJobCached() const
{
    return cache_.has_value() && *are_vector_blocks_cached_;
}



void OvfFileReader::GetVectorBlockImpl(const int i_work_plane, const int i_vector_block, VectorBlock& vb, bool try_cache) const
{
    if (try_cache && cache_.has_value() && *are_vector_blocks_cached_)
    {
        vb.MergeFrom(cache_->work_planes(i_work_plane).vector_blocks(i_vector_block));
        return;
    }

    size_t start_offset;
    auto work_plane_view = GetWorkPlaneFileView(i_work_plane, &start_offset);
    
    const auto wpl = wp_luts_.value()[i_work_plane];

    auto vb_pos = (size_t)wpl.vectorblockspositions(i_vector_block);
    auto vb_offset = vb_pos - start_offset;

    // this array stream is longer than the vector block alone.
    // as the protobuf streams are all buffered, and the messages
    // are delimited, that should be fine.
    google::protobuf::io::ArrayInputStream zcs{
        work_plane_view.data() + vb_offset,
        (int)(work_plane_view.size() - vb_offset)
    };
    google::protobuf::util::ParseDelimitedFromZeroCopyStream(
        &vb,
        &zcs,
        nullptr
    );
}

void OvfFileReader::GetWorkPlaneImpl(const int i_work_plane, WorkPlane& wp, bool include_vector_blocks, bool try_cache) const
{
    wp.Clear();

    if (try_cache && cache_.has_value() && include_vector_blocks && *are_vector_blocks_cached_)
    {
        wp.MergeFrom(cache_->work_planes(i_work_plane));
        return;
    }

    if (try_cache && cache_.has_value() && !include_vector_blocks)
    {
        util::MergeExcluding(
            cache_->work_planes(i_work_plane),
            wp,
            [](const google::protobuf::FieldDescriptor& fd){return fd.name() == "work_planes";}
        );
        return;
    }
    
    // we either need to read the shell, the blocks, or both from the file
    // prepare file access
    size_t wp_offset_abs;
    auto work_plane_view = GetWorkPlaneFileView(i_work_plane, &wp_offset_abs);
    const auto wpl = wp_luts_.value()[i_work_plane];

    // offset from start of work plane
    size_t shell_position = (size_t)wpl.workplaneshellposition() - wp_offset_abs;

    // write work plane shell into output
    if (try_cache && cache_.has_value())
    {
        wp.MergeFrom(cache_->work_planes(i_work_plane));
    }
    else
    {
        google::protobuf::io::ArrayInputStream zcs{
            work_plane_view.data() + shell_position,
            (int)(work_plane_view.size() - shell_position)
        };
        google::protobuf::util::ParseDelimitedFromZeroCopyStream(
            &wp,
            &zcs,
            nullptr
        );
    }

    // write vector blocks into output
    if (include_vector_blocks)
    {
        GetVectorBlocksImpl(i_work_plane, wp, work_plane_view, wp_offset_abs);
    }
}

void OvfFileReader::GetVectorBlocksImpl(const int i_work_plane, WorkPlane& wp, MemoryMapping::FileView& work_plane_view, size_t wp_offset_abs) const
{
    const auto wpl = wp_luts_.value()[i_work_plane];

    // populate work plane with vector blocks
    for (int i = 0; i < wpl.vectorblockspositions_size(); i++)
    {
        auto vb_pos = (size_t)wpl.vectorblockspositions(i);
        auto vb_offset = vb_pos - wp_offset_abs;

        VectorBlock *vb = wp.add_vector_blocks();

        // this array stream is longer than the vector block alone.
        // as the protobuf streams are all buffered, and the messages
        // are delimited, that should be fine.
        google::protobuf::io::ArrayInputStream zcs{
            work_plane_view.data() + vb_offset,
            (int)(work_plane_view.size() - vb_offset)
        };
        google::protobuf::util::ParseDelimitedFromZeroCopyStream(
            vb,
            &zcs,
            nullptr
        );
    }
}

}