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

/**
 * @brief Implements an incremental file reader for the open vector format.
 */
class OvfFileReader
{
public:
    /**
     * @brief Construct a new OvfFileReader object
     * 
     * @param auto_cache_threshold The file size threshold in bytes up to which the
     * full job will be cached to memory. Defaults to 64MiB. 
     */
    OvfFileReader(size_t auto_cache_threshold = 67108864);
    
    // Deleting copy and copy assignment because we are handling file streams.
    OvfFileReader(const OvfFileReader&) = delete;
    OvfFileReader& operator=(const OvfFileReader&) = delete;

    /**
     * @brief Opens an existing ovf file.
     * 
     * @param path The path from which the file should be read.
     * @param job A reference to the job object into which the job shell should be read.
     */
    void OpenFile(const std::string path, Job& job);

    /**
     * @brief Closes the file and file stream.
     */
    void CloseFile();
    
    /**
     * @brief Reports whether currently a file is open.
     */
    bool IsFileOpen() const;

    /**
     * @brief Gets a specific work plane from the currently open file.
     * 
     * Gets the full work plane, including all vector blocks.
     * 
     * @param i_work_plane The index of the work plane to get.
     * @param wp A reference to the object into which the work plane should be read.
     */
    void GetWorkPlane(const int i_work_plane, WorkPlane& wp) const;
    
    /**
     * @brief Gets a specific work plane shell from the currently open file.
     * 
     * Only gets the work plane shell, without any vector blocks.
     * 
     * @param i_work_plane The index of the work plane to get.
     * @param wp A reference to the object into which the work plane shell should be read.
     */
    void GetWorkPlaneShell(const int i_work_plane, WorkPlane& wp) const;

    /**
     * @brief Gets a specific vector block on a specific work plane from the currently open file.
     * 
     * @param i_work_plane The index of the work plane the vector block is located on.
     * @param i_vector_block The index of the vector block to get.
     * @param vb A reference to the object into which the vector block should be read.
     */
    void GetVectorBlock(const int i_work_plane, const int i_vector_block, VectorBlock& vb) const;

    
    /**
     * @brief Caches the full job into memory.
     * 
     * Overrides any previous calls regarding caching strategy.
     */
    void CacheFullJob();

    /**
     * @brief Only caches the work plane shells into memory.
     * 
     * Overrides any previous calls regarding caching strategy.
     */
    void CacheWorkPlaneShells();

    /**
     * @brief Clears all caches.
     * 
     * Overrides any previous calls regarding caching strategy.
     */
    void ClearCache();

    /**
     * @brief Reports whether the work plane shells are cached.
     * 
     * If the full job is cached, that implies the work plane shells are always
     * cached as well.
     */
    bool IsWorkPlaneShellsCached() const;
    
    /**
     * @brief Reports whether the full job is cached.
     */
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