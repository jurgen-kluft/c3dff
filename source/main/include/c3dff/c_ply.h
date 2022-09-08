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

        enum etype
        {
            TYPE_SIGNED    = 0x100,
            TYPE_UNSIGNED  = 0x200,
            TYPE_FLOAT     = 0x400,
            TYPE_LIST      = 0x800,
            TYPE_INVALID   = 0,
            TYPE_INT8      = 0x00000 | 0x1 | TYPE_SIGNED,
            TYPE_UINT8     = 0x10000 | 0x1 | TYPE_UNSIGNED,
            TYPE_INT16     = 0x20000 | 0x2 | TYPE_SIGNED,
            TYPE_UINT16    = 0x30000 | 0x2 | TYPE_UNSIGNED,
            TYPE_INT32     = 0x40000 | 0x4 | TYPE_SIGNED,
            TYPE_UINT32    = 0x50000 | 0x4 | TYPE_UNSIGNED,
            TYPE_FLOAT32   = 0x60000 | 0x4 | TYPE_FLOAT,
            TYPE_FLOAT64   = 0x70000 | 0x8 | TYPE_FLOAT,
            TYPE_INDEX_MASK = 0xFF0000,
            TYPE_INDEX_SHIFT = 16,
            TYPE_SIZE_MASK = 0xFF,
        };

        struct ply_t;
        ply_t* create(allocator_t* allocator, linereader_t* line_reader);

        bool read_header(ply_t* ply);
        bool set_ignore_property(ply_t* ply, const char* element_name, const char* property_name);
        bool set_read_property(ply_t* ply, const char* element_name, const char* property_name, etype destination_type, s32 offset);

        void read_data(ply_t* ply);

        template <typename T> class array_t
        {
        public:
            u32* m_count;
            T*   m_array;
        };

        template <typename T> array_t<T> get_array(ply_t* ply, const char* element_name);

        array_t<const char*> get_comments(ply_t* ply);
        array_t<const char*> get_obj_infos(ply_t* ply);

    } // namespace nply

} // namespace ncore

#endif // __C_3DFF_PLY_H__