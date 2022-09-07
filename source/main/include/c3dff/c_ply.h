#ifndef __C_3DFF_PLY_H__
#define __C_3DFF_PLY_H__
#include "cbase/c_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace ncore
{
    namespace nply
    {
        class allocator_t
        {
        public:
            virtual void* alloc(u32 size) = 0;
        };

        class linereader_t
        {
        public:
            virtual bool read_line(const char*& str, const char*& end) = 0;
        };

        struct ply_t;
        static ply_t* create(allocator_t* allocator, linereader_t* line_reader);
        static void set_ignore_property(ply_t* ply, const char* element_name, const char* property_name);
        static void set_read_property(ply_t* ply, const char* element_name, const char* property_name, etype destination_type, s32 offset);

        static void read(ply_t* ply);

        template <typename T> class array_t
        {
        public:
            u32* m_count;
            T*   m_array;
        };

        template <typename T> static array_t<T> get_array(ply_t* ply, const char* element_name);

        static array_t<const char*> get_comments(ply_t* ply);
        static array_t<const char*> get_obj_infos(ply_t* ply);

    } // namespace nply

} // namespace ncore

#endif // __C_3DFF_PLY_H__