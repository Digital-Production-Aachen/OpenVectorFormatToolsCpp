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

#include "google/protobuf/message.h"
#include <fcntl.h>
#include <iostream>
#include <functional>

namespace open_vector_format::util {

/**
 * @brief Copies a specific field, as specified by its descriptor, from one message to another.
 * Does not work with repeated fields.
 * 
 * @param source The source message to copy the field from.
 * @param target The target message to copy the field to.
 * @param field The field descriptor corresponding to the field to copy.
 */
void CopyField(const google::protobuf::Message& source,
               google::protobuf::Message& target,
               const google::protobuf::FieldDescriptor* const& field);

/**
 * @brief Copies a specific repeated field, as specified by its descriptor, from one message to another.
 * 
 * @param source The source message to copy the repeated field from.
 * @param target The target message to copy the repeated field to.
 * @param field The field descriptor corresponding to the repeated field to copy.
 */
void CopyRepeatedField(const google::protobuf::Message& source,
                       google::protobuf::Message& target,
                       const google::protobuf::FieldDescriptor* const& field);


/**
 * @brief Merges one protobuf message into another, excluding fields based on a given predicate.
 * 
 * @tparam UnaryPredicate The type of predicate. Usually a function reference or lambda.
 * @param source A protobuf message to read fields from.
 * @param target A protobuf message to write fields into.
 * @param pred A predicate that decides based on a field descriptor if that field should be merged.
 * When the predicate returns true, the field will be skipped.
 * 
 * This function behaves much like the target.MergeFrom(source) call, except that it can skip over
 * some fields. Sample usage:
 * @code {.cpp}
 * MergeExcluding(
 *     source,
 *     target,
 *     [](google::protobuf::FieldDescriptor& fd){
 *         return fd.name() == "field_name_to_exclude";
 *     }
 * );
 * @endcode
 * Of course, fields can also be identified by other indications, like tags.
 */
template <class UnaryPredicate>
void MergeExcluding(const google::protobuf::Message& source,
                    google::protobuf::Message& target,
                    UnaryPredicate pred)
{
    const auto source_desc = source.GetDescriptor();
    const auto target_desc = target.GetDescriptor();

    if (source_desc->full_name() != target_desc->full_name())
        throw std::runtime_error("Can't merge message type '" + source_desc->full_name() +
                                 "' into message type '" + target_desc->full_name() + "'");
    
    const auto source_refl = source.GetReflection();
    const auto target_refl = target.GetReflection();

    std::vector<const google::protobuf::FieldDescriptor*> source_fields;
    source_refl->ListFields(source, &source_fields);

    for (auto const field : source_fields)
    {
        if (pred(*field))
            continue;

        auto field_number = field->number();

        if (!field->is_repeated() && !field->is_map())
        {
            CopyField(source, target, field);
        }
        else
        {
            CopyRepeatedField(source, target, field);
        }
    }
}

/**
 * @brief Determines the endianness of the host system at runtime.
 * 
 * @return true If the host system is big endian.
 * @return false If the host system is little endian.
 * 
 * Obscure architectures with middle-endian/mixed-endian architectures are
 * neither detected nor supported.
 */
inline bool IsSystemBigEndian()
{
    // if first byte is 0x00, least significant byte is first, i.e. BE
    uint16_t num{0x00FF};
    return (((uint8_t*)&num)[0] == 0x00);
}

/**
 * @brief Writes an integer to a stream in little-endian byte order.
 * 
 * @tparam T The type of the integer to write.
 * @param integer The integer to write.
 * @param os The output stream to write to.
 */
template <typename T>
inline void WriteAsLittleEndian(T integer, std::ostream& os)
{
    std::cout << sizeof(integer) << std::endl;

    if (IsSystemBigEndian())
    {
        uint8_t buf[sizeof(integer)];
        for (int i = 0; i < sizeof(integer); i++)
            buf[i] = ((uint8_t*)(&integer))[sizeof(integer) - i - 1];
        
        os.write((char*)buf, sizeof(integer));
    }
    else
    {
        os.write((char*)(&integer), sizeof(integer));
    }
}

/**
 * @brief Reads an integer from a stream in little-endian byte order.
 * 
 * @tparam T The type of integer to read.
 * @param integer Reference to an integer to read into.
 * @param is The input stream to read from.
 */
template <typename T>
inline void ReadFromLittleEndian(T& integer, std::istream& is)
{
    if (IsSystemBigEndian())
    {
        uint8_t buf[sizeof(integer)];
        is.read((char*)buf, sizeof(integer));

        for (int i = 0; i < sizeof(integer); i++)
            ((uint8_t*)(&integer))[i] = buf[sizeof(integer) - i - 1];
    }
    else
    {
        is.read((char*)(&integer), sizeof(integer));
    }
}

/**
 * @brief Reads an integer from an array in little-endian byte order.
 * 
 * @tparam T The type of integer to read.
 * @param integer Reference to an integer to read into.
 * @param in The input array to read from.
 */
template <typename T>
inline void ReadFromLittleEndian(T& integer, uint8_t *in)
{
    if (IsSystemBigEndian())
    {
        for (int i = 0; i < sizeof(integer); i++)
            ((uint8_t*)(&integer))[i] = in[sizeof(integer) - i - 1];
    }
    else
    {
        memcpy((uint8_t*)(&integer), in, sizeof(integer));
    }
}

}