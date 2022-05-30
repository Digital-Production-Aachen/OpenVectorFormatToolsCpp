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


/**
 * @brief Implements an incremental file writer for the open vector file format.
 * 
 * This class implements an incremental file writer. Therefore, work planes and vector
 * blocks can be fed incrementally, and will be committed and written to the output
 * stream after each work plane is finalized. This enables writing of large jobs that
 * would otherwise not fit into system memory.
 * 
 * Note that due to internal state tracking, this file writer does not support concurrency.
 * Multiple methods must not be called similtaneously.
 */
class OVF_READER_WRITER_EXPORT OvfFileWriter
{
public:
    /**
     * @brief Construct a new OvfFileWriter object.
     */
    OvfFileWriter();
    
    // Deleting copy and copy assignment because we are handling file streams.
    OvfFileWriter(const OvfFileWriter&) = delete;
    OvfFileWriter& operator=(const OvfFileWriter&) = delete;

    /**
     * @brief Begins a partial write operation.
     * 
     * Can only be called when no other write operation is in progress.
     * Note that any work planes and vector blocks that might be part of the provided job shell
     * will be ignored, and have to be added incrementally afterwards.
     * 
     * @param job_shell The job shell to base the file off of. Can still be edited via the
     * OvfFileWriter::job_shell accessor/mutator.
     * @param path The path to write the ovf file to. Must be valid and have write permissions.
     */
    void StartWritePartial(const Job& job_shell, const std::string path);

    /**
     * @brief Appends a work plane during a partial write.
     * 
     * Any previous work plane is committed and written to the file stream, and a new work plane
     * based on the provided object is initialized in memory. All vector blocks of the provided
     * work plane are preserved, however additional vector blocks can be added with 
     * OvfFileWriter::AppendVectorBlock.
     * 
     * @param wp The next work plane to append to the file.
     */
    void AppendWorkPlane(const WorkPlane& wp);

    /**
     * @brief Appends additional vector blocks to a work plane during a partial write.
     * 
     * Only valid if there is a work plane in memory, i.e. OvfFileWriter::AppendWorkPlane was called
     * at least once. In that case, appends a copy of the provided vector block to the last work plane
     * provided in OvfFileWriter::AppendWorkPlane.
     * 
     * @param vb The vector block to append to the work plane.
     */
    void AppendVectorBlock(const VectorBlock& vb);

    /**
     * @brief Finishes a partial write operation and closes the file stream.
     * 
     * Commits and writes the last pending work plane, if available, and finishes the file writing
     * process. Then closes the stream and reverts the file operation of this writer back to none.
     */
    void FinishWrite();


    /**
     * @brief Writes a full job, including all work planes and vector blocks, to a file.
     * 
     * This method can be used for smaller jobs that easily fit into memory.
     * Note that the job shell can not be altered before it is committed to the output file.
     * 
     * @param job The full job, including all work planes and vector blocks.
     * @param path The path to write the ovf file to. Must be valid and have write permissions.
     */
    void WriteFullJob(const Job& job, const std::string path);

    /** Accessor and mutator for the job shell. This allows editing of the job shell
     *  while doing a partial write. All edits before calling OvfFileWriter::FinishWrite
     *  will be committed and written to the file. */
    Job& job_shell();

private:
    /**
     * @brief A file operation modelling the different modes of writing of this class.
     */
    enum class FileOperationState
    {
        /** No writing operation. Can transition into any form of write. */
        kNone,
        /** Partial writing, with data for work planes and vectory blocks being fed
         *  over the course of multiple calls. */
        kPartialWrite,
        /** Comlete writing, with all data being provided as a single job object. */
        kCompleteWrite,
        /** Undefined. */
        kUndefined
    };

    /** The current file operation in progress. Used for internal state tracking. */
    FileOperationState operation_;
    
    // Using optionals because forced heap allocation makes little sense for these.
    // Protobuf objects heap allocate themselves anyway. If the rest should be heap
    // allocated as well, that can easily be forced by heap allocating this object.
    // Optionals are guaranteed to hold a value during file operations kPartialWrite
    // and kCompleteWrite.

    /** Output file stream to write to when writing operation is in progress. */
    std::optional<std::ofstream> ofs_;
    
    /** The current work plane held in memory before it is committed and written. */
    std::optional<WorkPlane> current_wp_;
    /** The job shell held in memory before it is committed and written. */
    std::optional<Job> job_shell_;
    /** The job lut held in memory to be updated with offsets before it is written. */
    std::optional<JobLUT> job_lut_;
    /** The offset in the file that is written at which the job lut offset (i.e. position)
     *  will be written. */
    std::optional<uint64_t> job_lut_offset_offset_;

    /**
     * @brief Performs the write operation of the file header.
     * 
     * Writes magic bytes and a placeholder for the job lut offset to the stream, and initializes
     * the interal state of the job shell and job lut.
     * 
     * @param job The job object to initialize the internal state with. Only the shell of this job will
     * be stored internally, without any work planes.
     */
    void WriteHeader(const Job& job);

    /**
     * @brief Performs the write opeartion of a full work plane.
     * 
     * Writes the full work plane, including vector blocks, into the output stream.
     * 
     * @param wp The work plane object to write to the stream.
     */
    void WriteFullWorkPlane(const WorkPlane& wp);

    /**
     * @brief Performs the write operation of the file footer and inserts missing offsets.
     * 
     * Commits and writes a pending work plane in memory, if available, then writes job shell
     * and job lut to the file, and inserts missing offsets into the placeholder bytes. Finally
     * flushes and closes the stream.
     */
    void WriteFooter();

    /**
     * @brief Checks for health of the output file stream.
     * 
     * Throws a corresponding exception if the stream is degraded or closed.
     */
    inline void CheckFsHealth()
    {
        if (!ofs_.has_value())
            throw std::runtime_error("Output file stream was not set");

        if (!ofs_->is_open())
            throw std::runtime_error("Output file stream closed unexpectedly");

        if (!ofs_->good())
            throw std::runtime_error("Output file stream encountered an error");
    }

    /**
     * @brief Checks for the interal state to be writing.
     * 
     * Throws a corresponding exception if the file operation is invalid for writing.
     */
    inline void CheckIsWriting()
    {
        if (operation_ != FileOperationState::kPartialWrite && operation_ != FileOperationState::kCompleteWrite)
            throw std::runtime_error("Trying to write while no write operation is active");
    }
};

}