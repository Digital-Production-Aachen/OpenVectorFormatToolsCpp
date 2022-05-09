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

#ifndef WIN32
#  error win32 headers included for non-win32 build 
#endif

#include <string>
#include <stdexcept>
#include <windows.h>
#include <fileapi.h>
#include <sysinfoapi.h>

namespace open_vector_format::reader_writer {

/**
 * @brief WIN32 specific implementation for memory mapping files.
 * 
 * Implements memory mapping behaviour based on direct calls to the WIN32 C API.
 */
class MemoryMapping
{
public:

    /**
     * @brief WIN32 specific implementation for a memory mapped view of a file.
     * 
     * Implements the view of a memory mapped file, abstracting over the WIN32 C API
     * and page-aligning the memory map.
     */
    class FileView
    {
    public:

        /**
         * @brief Construct a new File View object
         * 
         * @param base_addr The base address in memory at which the memory mapping starts.
         * @param start_addr The address in memory at which the data actually starts. Must be greater or equal to base_addr.
         * @param size_from_base_addr The absolute size of the complete memory mapping.
         */
        FileView(uint8_t *base_addr, uint8_t *start_addr, size_t size_from_base_addr)
            : base_addr_{base_addr}, start_addr_{start_addr}, size_from_base_addr_{size_from_base_addr}
        {}

        // RAII and copy constructors are hard to get right.
        // When they are not necessary, it's better to delete them.
        FileView(const FileView&) = delete;
        FileView& operator=(const FileView&) = delete;
        
        /**
         * @brief Destroy the File View object
         * 
         * Unmaps the file view.
         */
        ~FileView()
        {
            UnmapViewOfFile(base_addr_);
        }

        /**
         * @brief Accessor to the mapped data.
         * 
         * @return uint8_t* Pointer to the memory segment representing the exact offset from the
         * beginning of the file as requested on creation. Guaranteed to be valid for the size
         * available in the size() accessor.
         */
        uint8_t *data() const
        {
            return start_addr_;
        }

        /**
         * @brief Accessor to the size of mapped data.
         * 
         * @return size_t The size in bytes of this mapping. This number may be higher than the requestes
         * minimum size.
         */
        size_t size() const
        {
            return size_from_base_addr_ - ((SIZE_T)start_addr_ - (SIZE_T)base_addr_);
        }

    private:
        /** The base address in memory at which the memory mapping starts. */
        uint8_t *base_addr_;

        /** The address in memory at which the data actually starts. Must be greater or equal to base_addr. */
        uint8_t *start_addr_;

        /** The absolute size of the complete memory mapping. */
        SIZE_T size_from_base_addr_;
    };

    /**
     * @brief Construct a new Memory Mapping object
     * 
     * @param path A valid path to a file. The contents of this file will be mapped to memory.
     * @throws std::runtime_error A handle to the file could not be obtained.
     */
    MemoryMapping(const std::string path)
    {
        GetSystemInfo(&system_info_);
        
        file_ = CreateFileA(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            nullptr
        );

        if (file_ == INVALID_HANDLE_VALUE)
        {
            throw std::runtime_error("Opening file \"" + path + "\" failed");
        }

        DWORD low, high;
        low = GetFileSize(file_, &high);
        file_size_ = (((uint64_t)high) << 32) | ((uint64_t)low);

        file_mapping_ = CreateFileMappingA(
            file_,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr
        );

        if (file_mapping_ == INVALID_HANDLE_VALUE)
        {
            throw std::runtime_error("Creating file mapping for file \"" + path + "\" failed");
            CloseHandle(file_);
        }
    }

    // RAII and copy constructors are hard to get right.
    // When they are not necessary, it's better to delete them.
    MemoryMapping(const MemoryMapping&) = delete;
    MemoryMapping& operator=(const MemoryMapping&) = delete;
    
    /**
     * @brief Destroy the Memory Mapping object
     * 
     * Handles to the file mapping and file itself are closed.
     * This does not invalidate views that are still opened, as those hold
     * handles to the file mapping themselves.
     */
    ~MemoryMapping()
    {
        CloseHandle(file_mapping_);
        CloseHandle(file_);
    }

    /**
     * @brief Create a new FileView 
     * 
     * @param offset The absolute offset in bytes from the beginning of the file.
     * @param min_size The minimum size the view has to have, in bytes. When min_size
     * is 0, the mapping extends to the end of the file.
     * @return A new file view. The first byte of its data is guaranteed to be at the 
     * offset specified, and its size is at least as long as min_size.
     */
    FileView CreateView(const size_t offset, const size_t min_size) const
    {
        DWORD granularity = system_info_.dwAllocationGranularity;
        SIZE_T granularized_offset = (offset / granularity) * granularity;

        DWORD offset_low = static_cast<DWORD>(granularized_offset & 0xFFFFFFFFul);
        DWORD offset_high = static_cast<DWORD>((granularized_offset >> 32) & 0xFFFFFFFFul);

        SIZE_T corrected_min_size = ((offset - granularized_offset) + min_size);
        SIZE_T granularized_min_size = (((corrected_min_size + 1) / granularity) * granularity);
        if (granularized_offset + granularized_min_size >= file_size_)
        {
            granularized_min_size = 0; // up to EOF
        }

        auto view = static_cast<uint8_t*>(MapViewOfFile(
            file_mapping_,
            FILE_MAP_READ,
            offset_high,
            offset_low,
            min_size == 0 ? 0 : (SIZE_T)granularized_min_size
        ));
        
        MEMORY_BASIC_INFORMATION map_info;
        VirtualQuery(view, &map_info, sizeof(map_info));

        return FileView{
            view,
            view + (offset - granularized_offset),
            map_info.RegionSize
        };
    }

    /**
     * @brief Accessor for the size of the full file.
     * 
     * @return size_t The size of the full file in bytes. 
     */
    size_t file_size() const
    {
        return file_size_;
    }

private:
    /** WIN32 handle to the file itself. */ 
    HANDLE file_;

    /** WIN32 handle to the overarching file mapping object. */
    HANDLE file_mapping_;

    /** Full file size, queried on construction. */
    SIZE_T file_size_;

    /** System info, queried on construction. Needed for memory page size, as file views must be page-aligned. */
    SYSTEM_INFO system_info_;
};


}