#ifndef __C_3DFF_PLY_H__
#define __C_3DFF_PLY_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
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

        const char* g_ReadLine(const char* text_cursor, const char* text_end, const char*& str, const char*& end);

        class reader_t
        {
        public:
            virtual bool read_line(const char*& str, const char*& end)         = 0;
            virtual bool read_data(u32 size, const u8*& begin, const u8*& end) = 0;
        };

        enum etype
        {
            TYPE_SIGNED      = 0x10,
            TYPE_UNSIGNED    = 0x20,
            TYPE_FLOAT       = 0x40,
            TYPE_LIST        = 0x80,
            TYPE_INVALID     = 0,
            TYPE_INT8        = 0x0000 | 0x1 | TYPE_SIGNED,
            TYPE_UINT8       = 0x1000 | 0x1 | TYPE_UNSIGNED,
            TYPE_INT16       = 0x2000 | 0x2 | TYPE_SIGNED,
            TYPE_UINT16      = 0x3000 | 0x2 | TYPE_UNSIGNED,
            TYPE_INT32       = 0x4000 | 0x4 | TYPE_SIGNED,
            TYPE_UINT32      = 0x5000 | 0x4 | TYPE_UNSIGNED,
            TYPE_FLOAT32     = 0x6000 | 0x4 | TYPE_FLOAT,
            TYPE_FLOAT64     = 0x7000 | 0x8 | TYPE_FLOAT,
            TYPE_INDEX_MASK  = 0xF000,
            TYPE_INDEX_SHIFT = 12,
            TYPE_SIZE_MASK   = 0x0F,
            TYPE_TYPE_MASK   = 0xF0,
        };

        inline static s32   type_sizeof(etype type) { return (type & TYPE_SIZE_MASK); }
        inline static s32   type_index(etype type) { return (type & TYPE_INDEX_MASK) >> TYPE_INDEX_SHIFT; }
        inline static etype type_type(etype type) { return (etype)(type & TYPE_TYPE_MASK); }
        inline static bool  type_is_list(etype type) { return (type & TYPE_LIST) == TYPE_LIST; }
        inline static bool  type_is_int(etype type) { return (type & TYPE_SIGNED) == TYPE_SIGNED; }
        inline static bool  type_is_int8(etype type) { return type_is_int(type) && (type_sizeof(type) == 1); }
        inline static bool  type_is_int16(etype type) { return type_is_int(type) && (type_sizeof(type) == 2); }
        inline static bool  type_is_int32(etype type) { return type_is_int(type) && (type_sizeof(type) == 4); }
        inline static bool  type_is_int64(etype type) { return type_is_int(type) && (type_sizeof(type) == 8); }
        inline static bool  type_is_uint(etype type) { return (type & TYPE_UNSIGNED) == TYPE_UNSIGNED; }
        inline static bool  type_is_uint8(etype type) { return type_is_uint(type) && (type_sizeof(type) == 1); }
        inline static bool  type_is_uint16(etype type) { return type_is_uint(type) && (type_sizeof(type) == 1); }
        inline static bool  type_is_uint32(etype type) { return type_is_uint(type) && (type_sizeof(type) == 1); }
        inline static bool  type_is_uint64(etype type) { return type_is_uint(type) && (type_sizeof(type) == 1); }
        inline static bool  type_is_f32(etype type) { return (type & TYPE_FLOAT32) == TYPE_FLOAT32; }
        inline static bool  type_is_f64(etype type) { return (type & TYPE_FLOAT64) == TYPE_FLOAT64; }
        inline static bool  type_is_float(etype type) { return type_is_f32(type) || type_is_f64(type); }

        class handler_t
        {
        public:
            virtual bool setup(s32 element_index, int_t num_items, etype* property_type_array, s32* property_index_array, s32 property_count) = 0;
            virtual void read(s32 element_index, etype* property_type, s32 property_count, void* property_data)                               = 0;

        protected:
            static u32 get_offset(s32 property_index, etype* property_type_array, s32 property_count);
            static s8  read_s8(etype property_type, u32 property_offset, void* property_data);
            static s16 read_s16(etype property_type, u32 property_offset, void* property_data);
            static s32 read_s32(etype property_type, u32 property_offset, void* property_data);
            static u8  read_u8(etype property_type, u32 property_offset, void* property_data);
            static u16 read_u16(etype property_type, u32 property_offset, void* property_data);
            static u32 read_u32(etype property_type, u32 property_offset, void* property_data);
            static f32 read_f32(etype property_type, u32 property_offset, void* property_data);
            static f64 read_f64(etype property_type, u32 property_offset, void* property_data);
        };

        struct vertex_t
        {
            float x, y, z;
        };

        struct triangle_t
        {
            u32 v1, v2, v3;
        };

        enum eindex
        {
            INDEX_VERTEX = 0,
            INDEX_FACE   = 1,
            INDEX_PROP_X = 0,
            INDEX_PROP_Y = 1,
            INDEX_PROP_Z = 2,
        };

        class vertices_handler_t : public handler_t
        {
        public:
            u32       m_vertex_max;
            u32       m_vertex_count;
            vertex_t* m_vertex_array;
            etype     m_property_type[3];
            s32       m_property_offset[3];

            vertices_handler_t(vertex_t* vertex_array, u32 vertex_max)
                : m_vertex_max(vertex_max)
                , m_vertex_count(0)
                , m_vertex_array(vertex_array)
            {
                for (int i = 0; i < 3; i++)
                {
                    m_property_type[i]   = TYPE_INVALID;
                    m_property_offset[i] = 0;
                }
            }

            virtual bool setup(s32 element_index, int_t num_items, etype* property_type_array, s32* property_index_array, s32 property_count)
            {
                if (element_index == INDEX_VERTEX)
                {
                    for (int i = 0; i < property_count; i++)
                    {
                        if (property_index_array[i] >= 0)
                        {
                            m_property_type[property_index_array[i]]   = property_type_array[i];
                            m_property_offset[property_index_array[i]] = get_offset(i, property_type_array, property_count);
                        }
                    }
                    return true;
                }
                return false;
            }

            virtual void read(s32 element_index, etype* property_type_array, s32 property_count, void* property_data)
            {
                if (element_index == INDEX_VERTEX)
                {
                    vertex_t& v = m_vertex_array[m_vertex_count];
                    v.x         = read_f32(m_property_type[0], m_property_offset[0], property_data);
                    v.y         = read_f32(m_property_type[1], m_property_offset[1], property_data);
                    v.z         = read_f32(m_property_type[2], m_property_offset[2], property_data);
                    m_vertex_count++;
                }
            }
        };

        class triangles_handler_t : public handler_t
        {
        public:
            u32         m_triangle_max;
            u32         m_triangle_count;
            triangle_t* m_triangle_array;
            etype       m_property_type;
            s32         m_property_offset[4];

            triangles_handler_t(triangle_t* triangle_array, u32 triangle_max)
                : m_triangle_max(triangle_max)
                , m_triangle_count(0)
                , m_triangle_array(triangle_array)
            {
                m_property_type = TYPE_INVALID;
                for (int i = 0; i < 4; i++)
                {
                    m_property_offset[i] = 0;
                }
            }

            virtual bool setup(s32 element_index, int_t num_items, etype* property_type_array, s32* property_index_array, s32 property_count)
            {
                if (element_index == INDEX_FACE)
                {
                    ASSERT(type_is_list(property_type_array[0]));
                    m_property_type = property_type_array[1];
                    for (int i = 0; i < 4; i++)
                    {
                        m_property_offset[i] = get_offset(i, property_type_array, property_count);
                    }
                    return true;
                }
                return false;
            }

            virtual void read(s32 element_index, etype* property_type_array, s32 property_count, void* property_data)
            {
                if (element_index == INDEX_FACE)
                {
                    triangle_t& t = m_triangle_array[m_triangle_count];
                    ASSERT(read_s8(property_type_array[0], 0, property_data) == 3); // should be a triangle
                    t.v1 = read_u32(m_property_type, m_property_offset[1], property_data);
                    t.v2 = read_u32(m_property_type, m_property_offset[2], property_data);
                    t.v3 = read_u32(m_property_type, m_property_offset[3], property_data);
                    m_triangle_count++;
                }
            }
        };

        struct ply_t;
        ply_t* create(allocator_t* allocator);

        bool read_header(ply_t* ply, reader_t* reader);
        u32  get_element_count(ply_t* ply, const char* element_name);
        void set_element_index(ply_t* ply, const char* element_name, s32 index);
        bool set_property_index(ply_t* ply, const char* element_name, const char* property_name, s32 index);

        void read_data(ply_t* ply, reader_t* reader, handler_t* handler1, handler_t* handler2);

    } // namespace nply

} // namespace ncore

#endif // __C_3DFF_PLY_H__