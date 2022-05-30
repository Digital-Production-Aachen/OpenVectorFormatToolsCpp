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

#include "util.h"
#include "google/protobuf/message.h"

namespace open_vector_format::util {

/**
 * @brief Macro to create a case statement for each primitive type to copy from source to target message.
 * 
 * Expands to a scoped case statement, matching on a FieldDescriptor::CPPTYPE_$TYPE. Then uses reflection
 * to set the the value of the target at the same tag number as the given descriptor to the value found in
 * the source message field.
 */
#define CASE_SET_FIELD(___source___, ___target___, ___field___, ___cpptype___, ___function_suffix___) \
        case google::protobuf::FieldDescriptor::___cpptype___: \
            { \
                target.GetReflection()->Set##___function_suffix___( \
                    &(___target___), \
                    (___target___).GetDescriptor()->FindFieldByNumber((___field___)->number()), \
                    (___source___).GetReflection()->Get##___function_suffix___((___target___), (___field___)) \
                ); \
                break; \
            }

void CopyField(const google::protobuf::Message& source,
               google::protobuf::Message& target,
               const google::protobuf::FieldDescriptor* const& field)
{
    auto temp = target.GetReflection();

    switch (field->cpp_type())
    {
        CASE_SET_FIELD(source, target, field, CPPTYPE_INT32, Int32)
        CASE_SET_FIELD(source, target, field, CPPTYPE_INT64, Int64)
        CASE_SET_FIELD(source, target, field, CPPTYPE_UINT32, UInt32)
        CASE_SET_FIELD(source, target, field, CPPTYPE_UINT64, UInt64)
        CASE_SET_FIELD(source, target, field, CPPTYPE_FLOAT, Float)
        CASE_SET_FIELD(source, target, field, CPPTYPE_DOUBLE, Double)
        CASE_SET_FIELD(source, target, field, CPPTYPE_BOOL, Bool)
        CASE_SET_FIELD(source, target, field, CPPTYPE_ENUM, Enum)
        CASE_SET_FIELD(source, target, field, CPPTYPE_STRING, String)

        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            {
                target.GetReflection()->MutableMessage(
                    &target,
                    target.GetDescriptor()->FindFieldByNumber(field->number())
                )->MergeFrom(
                    source.GetReflection()->GetMessage(source, field)
                );
                break;
            }
        
        default:
            throw std::runtime_error("Unknown cpp_type \"" + std::to_string(field->cpp_type()) + "\" in field \"" + field->name() + "\"");
    }
}

#undef CASE_SET_FIELD

/**
 * @brief Macro to create a case statement for each repeated field type to copy from source to target message.
 * 
 * Expands to a scoped case statement, matching on a FieldDescriptor::CPPTYPE_$TYPE. Then uses reflection
 * to add all primitive values in the repeated filed of the source message into the target message's repeated
 * field.
 */
#define CASE_ADD_TO_REPEATED_FIELD(___source___, ___target___, ___field___, ___cpptype___, ___function_suffix___) \
        case google::protobuf::FieldDescriptor::___cpptype___: \
            { \
                const auto target_desc__ = (___target___).GetDescriptor(); \
                const auto target_refl__ = (___target___).GetReflection(); \
                const auto source_refl__ = (___source___).GetReflection(); \
                const auto element_count__ = source_refl__->FieldSize((___source___), (___field___)); \
                for (int i__ = 0; i__ < element_count__; i__++) \
                { \
                    target_refl__->Add##___function_suffix___( \
                        &(___target___), \
                        target_desc__->FindFieldByNumber((___field___)->number()), \
                        source_refl__->GetRepeated##___function_suffix___((___target___), (___field___), i__) \
                    ); \
                } \
                break; \
            }

void CopyRepeatedField(const google::protobuf::Message& source,
                       google::protobuf::Message& target,
                       const google::protobuf::FieldDescriptor* const& field)
{
    switch (field->cpp_type())
    {
        CASE_ADD_TO_REPEATED_FIELD(source, target, field, CPPTYPE_INT32, Int32)
        CASE_ADD_TO_REPEATED_FIELD(source, target, field, CPPTYPE_INT64, Int64)
        CASE_ADD_TO_REPEATED_FIELD(source, target, field, CPPTYPE_UINT32, UInt32)
        CASE_ADD_TO_REPEATED_FIELD(source, target, field, CPPTYPE_UINT64, UInt64)
        CASE_ADD_TO_REPEATED_FIELD(source, target, field, CPPTYPE_FLOAT, Float)
        CASE_ADD_TO_REPEATED_FIELD(source, target, field, CPPTYPE_DOUBLE, Double)
        CASE_ADD_TO_REPEATED_FIELD(source, target, field, CPPTYPE_BOOL, Bool)
        CASE_ADD_TO_REPEATED_FIELD(source, target, field, CPPTYPE_ENUM, Enum)
        CASE_ADD_TO_REPEATED_FIELD(source, target, field, CPPTYPE_STRING, String)
        
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            {
                const auto source_refl = source.GetReflection();
                const auto target_desc = target.GetDescriptor();
                const auto target_refl = target.GetReflection();
                const auto element_count = source_refl->FieldSize(source, field);
                for (int i = 0; i < element_count; i++)
                {
                    target_refl->AddMessage(
                        &target,
                        target_desc->FindFieldByNumber(field->number())
                    )->MergeFrom(
                        source_refl->GetRepeatedMessage(source, field, i)
                    );
                }
                break;
            }

        default:
            throw std::runtime_error("Unknown cpp_type \"" + std::to_string(field->cpp_type()) + "\" in field \"" + field->name() + "\"");
    }
}

#undef CASE_ADD_TO_REPEATED_FIELD

}