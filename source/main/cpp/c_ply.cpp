#include "cbase/c_target.h"
#include "cbase/c_debug.h"
#include "c3dff/c_ply.h"

namespace ncore
{
    namespace nply
    {
        const char* g_ReadLine(const char* text_cursor, const char* text_end, const char*& str, const char*& end)
        {
            str = text_cursor;
            end = text_cursor;
            while (*end != '\n' && *end != '\r' && end < text_end)
            {
                end++;
            }
            text_cursor = end;
            while ((*text_cursor == '\n' || *text_cursor == '\r') && text_cursor < text_end)
                text_cursor++;
            return text_cursor;
        }

        struct string_t
        {
            string_t()
                : m_str("")
                , m_end(m_str)
            {
            }
            string_t(const char* str, const char* end)
                : m_str(str)
                , m_end(end)
            {
            }
            string_t(const char* str, u32 len)
                : m_str(str)
                , m_end(str + len)
            {
            }
            const char* m_str;
            const char* m_end;

            inline bool is_empty() const { return m_str == m_end; }
            inline u32  length() const { return (u32)(m_end - m_str); }
        };

        inline static char to_lower(char c)
        {
            if (c >= 'A' && c <= 'Z')
                return (c - 'A') + 'a';
            return c;
        }

        inline static s32 compare(const string_t& as, const char* b)
        {
            const char* a = as.m_str;
            while (a < as.m_end && *b != 0)
            {
                if (*a != *b)
                {
                    char const ca = to_lower(*a);
                    char const cb = to_lower(*b);
                    if ((s32)(unsigned char)(ca) < (s32)(unsigned char)(cb))
                        return -1;
                    return 1;
                }
                ++a;
                ++b;
            }
            if (a == as.m_end && *b == 0)
                return 0;
            if (a < as.m_end)
                return 1;
            return -1;
        }

        static s32 compare(const string_t& as, const string_t& bs)
        {
            const char* a = as.m_str;
            const char* b = bs.m_str;
            while (a < as.m_end && b < bs.m_end)
            {
                char const ca = to_lower(*a);
                char const cb = to_lower(*b);
                if (ca != cb)
                {
                    if (ca < cb)
                        return -1;
                    return 1;
                }
                a++;
                b++;
            }
            if (a == as.m_end && b == bs.m_end)
                return 0;
            if (a < as.m_end)
                return 1;
            return -1;
        }

        static bool operator==(const string_t& _this, const char* other) { return compare(_this, other) == 0; }
        static bool operator!=(const string_t& _this, const char* other) { return compare(_this, other) != 0; }

        static bool operator==(const string_t& _this, const string_t& other) { return compare(_this, other) == 0; }
        static bool operator!=(const string_t& _this, const string_t& other) { return compare(_this, other) != 0; }

        static etype parse_type(string_t const& str)
        {
            // @note: Can be optimized a bit by branching on the first character, e.g. 'i', 'u', 'f'
            //        as well as length of the string, e.g. 3(1), 4(3), 5(6), 6(4), 7(2)
            if (str == "int8" || str == "char")
                return TYPE_INT8;
            else if (str == "uint8" || str == "uchar")
                return TYPE_UINT8;
            else if (str == "int16" || str == "short")
                return TYPE_INT16;
            else if (str == "uint16" || str == "ushort")
                return TYPE_UINT16;
            else if (str == "int32" || str == "int")
                return TYPE_INT32;
            else if (str == "uint32" || str == "uint")
                return TYPE_UINT32;
            else if (str == "float32" || str == "float")
                return TYPE_FLOAT32;
            else if (str == "float64" || str == "double")
                return TYPE_FLOAT64;
            return TYPE_INVALID;
        }

        template <typename T> T* construct(allocator_t* alloc)
        {
            void* ptr = alloc->alloc(sizeof(T));
            return (T*)ptr;
        }

        struct property_t
        {
            string_t m_name;
            etype    m_property_type;
            etype    m_list_count_type;
        };

        struct buffer_t
        {
            u32 m_size; // size in bytes
            u8* m_buffer;
        };

        struct element_t
        {
            string_t     m_name;
            u32          m_index;
            u32          m_count;
            s32          m_prop_count;
            property_t** m_prop_array;
            etype*       m_prop_type_array;
            s32*         m_prop_index_array;
            element_t*   m_next;
        };

        struct objinfo_t
        {
            string_t   m_comment;
            objinfo_t* m_next;
        };

        struct comment_t
        {
            string_t   m_comment;
            comment_t* m_next;
        };

        enum eformat
        {
            FORMAT_ASCII = 0, // ASCII
            FORMAT_BLE,       // Binary Little Endian
            FORMAT_BBE,       // Binary Big Endian
        };

        struct header_t
        {
            eformat    m_format;
            string_t   m_version;
            u32        m_num_comments;
            u32        m_num_obj_info;
            u32        m_num_elements;
            comment_t* m_comments;
            objinfo_t* m_obj_info;
            element_t* m_elements;
        };

        struct ply_t
        {
            allocator_t* m_alloc;
            header_t*    m_hdr;
            comment_t*   m_comments;
            objinfo_t*   m_obj_info;
            element_t*   m_elements;
        };

        ply_t* create(allocator_t* allocator)
        {
            ply_t* ply      = construct<ply_t>(allocator);
            ply->m_alloc    = allocator;
            ply->m_hdr      = nullptr;
            ply->m_comments = nullptr;
            ply->m_elements = nullptr;
            return ply;
        }

        static void add_comment(ply_t* ply, comment_t* comment)
        {
            comment->m_next = nullptr;
            if (ply->m_hdr->m_comments == nullptr)
            {
                ply->m_hdr->m_comments = comment;
            }
            else
            {
                ply->m_comments->m_next = comment;
            }
            ply->m_comments = comment;
        }

        static void skip_whitespace(string_t& line)
        {
            while (line.m_str < line.m_end)
            {
                if (line.m_str[0] != ' ' && line.m_str[0] != '\t')
                    break;
                line.m_str++;
            }
        }

        static string_t read_token(string_t& line)
        {
            skip_whitespace(line);
            const char* token = line.m_str;
            while (line.m_str < line.m_end)
            {
                if (line.m_str[0] == ' ' || line.m_str[0] == '\t')
                    break;
                line.m_str++;
            }
            return string_t(token, line.m_str);
        }

        static u32 parse_u32(string_t const& token)
        {
            u32         v   = 0;
            const char* str = token.m_str;
            while (str < token.m_end)
            {
                const char c = *str;
                v            = v * 10;
                v            = v + c - '0';
                str++;
            }
            return v;
        }

        string_t make_string(ply_t* ply, string_t const& str)
        {
            s32 const   str_len    = ((str.length() + 1) + (4 - 1)) & ~(4 - 1);
            char*       dst_str    = (char*)ply->m_alloc->alloc(str_len);
            const char* dst_end    = dst_str + str_len;
            char*       dst_cursor = dst_str;
            const char* src_str    = str.m_str;
            while (src_str < str.m_end)
                *dst_cursor++ = *src_str++;
            *dst_cursor = 0;
            return string_t(dst_str, dst_cursor);
        }

        void read_header_comment(ply_t* ply, string_t& line)
        {
            comment_t* comment = construct<comment_t>(ply->m_alloc);
            skip_whitespace(line);
            comment->m_comment = make_string(ply, line);
            add_comment(ply, comment);
        }

        void read_header_format(ply_t* ply, string_t& line)
        {
            string_t token = read_token(line);
            if (token == "ascii")
                ply->m_hdr->m_format = FORMAT_ASCII;
            else if (token == "binary_little_endian")
                ply->m_hdr->m_format = FORMAT_BLE;
            else if (token == "binary_big_endian")
                ply->m_hdr->m_format = FORMAT_BBE;
            string_t version_str  = read_token(line);
            ply->m_hdr->m_version = make_string(ply, version_str);
        }

        element_t* read_header_element(ply_t* ply, string_t& line)
        {
            element_t* elem     = construct<element_t>(ply->m_alloc);
            string_t   name_str = read_token(line);
            elem->m_name        = make_string(ply, name_str);
            string_t str_count  = read_token(line);
            elem->m_count       = parse_u32(str_count);
            elem->m_prop_array  = nullptr;
            elem->m_prop_count  = 0;
            return elem;
        }

        property_t* read_header_property(ply_t* ply, element_t* elem, string_t& line)
        {
            property_t* prop       = construct<property_t>(ply->m_alloc);
            string_t    type_token = read_token(line);
            if (type_token == "list")
            {
                string_t count_type_token = read_token(line);
                string_t list_type_token  = read_token(line);
                string_t property_name    = read_token(line);
                prop->m_list_count_type   = parse_type(count_type_token);
                prop->m_property_type     = (etype)(parse_type(list_type_token) | TYPE_LIST);
                prop->m_name              = make_string(ply, property_name);
            }
            else
            {
                string_t property_name  = read_token(line);
                prop->m_list_count_type = TYPE_INVALID;
                prop->m_property_type   = parse_type(type_token);
                prop->m_name            = make_string(ply, property_name);
            }
            return prop;
        }

        void read_header_obj_info(ply_t* ply, string_t& line)
        {
            // @todo: need information, haven't seen any .ply files with this
        }

        bool read_header(ply_t* ply, reader_t* reader)
        {
            ply->m_hdr                 = construct<header_t>(ply->m_alloc);
            ply->m_hdr->m_format       = FORMAT_ASCII;
            ply->m_hdr->m_version      = string_t();
            ply->m_hdr->m_num_comments = 0;
            ply->m_hdr->m_num_obj_info = 0;
            ply->m_hdr->m_num_elements = 0;
            ply->m_hdr->m_comments     = nullptr;
            ply->m_hdr->m_obj_info     = nullptr;
            ply->m_hdr->m_elements     = nullptr;

            element_t* element = nullptr;

            string_t line;
            while (reader->read_line(line.m_str, line.m_end))
            {
                string_t token = read_token(line);

                if (token == "ply" || token.is_empty())
                {
                    continue;
                }

                while (true)
                {
                    if (token == "comment")
                    {
                        read_header_comment(ply, line);
                    }
                    else if (token == "format")
                    {
                        read_header_format(ply, line);
                    }
                    else if (token == "element")
                    {
                        element = read_header_element(ply, line);
                    }
                    else if (token == "property")
                    {
                        ASSERT(element != nullptr);

                        // TODO: need to make this dynamic?
                        property_t* properties[32];
                        s32         prop_count = 0;
                        do
                        {
                            properties[prop_count++] = read_header_property(ply, element, line);
                            do
                            {
                                if (!reader->read_line(line.m_str, line.m_end))
                                    return false;
                                token = read_token(line);
                            } while (token.is_empty());
                        } while (token == "property");

                        element->m_prop_count       = prop_count;
                        element->m_prop_array       = (property_t**)ply->m_alloc->alloc(sizeof(property_t*) * prop_count);
                        element->m_prop_type_array  = (etype*)ply->m_alloc->alloc(sizeof(etype) * prop_count);
                        element->m_prop_index_array = (s32*)ply->m_alloc->alloc(sizeof(s32) * prop_count);
                        for (s32 i = 0; i < prop_count; i++)
                        {
                            property_t* prop               = properties[i];
                            element->m_prop_array[i]       = prop;
                            element->m_prop_type_array[i]  = prop->m_property_type;
                            element->m_prop_index_array[i] = i;
                        }
                        continue;
                    }
                    else if (token == "obj_info")
                    {
                        read_header_obj_info(ply, line);
                    }
                    else
                    {
                        return (token == "end_header");
                    }
                    break;
                }
            }
            return false;
        }

        u32 get_element_count(ply_t* ply, const char* element_name)
        {
            element_t* elem = ply->m_hdr->m_elements;
            while (elem != nullptr)
            {
                if (elem->m_name == element_name)
                    return elem->m_count;
                elem = elem->m_next;
            }
            return 0;
        }

        void set_element_index(ply_t* ply, const char* element_name, s32 index)
        {
            element_t* elem = ply->m_hdr->m_elements;
            while (elem != nullptr)
            {
                if (elem->m_name == element_name)
                {
                    elem->m_index = index;
                    break;
                }
                elem = elem->m_next;
            }
        }

        bool set_property_index(ply_t* ply, const char* element_name, const char* property_name, s32 index) { return false; }

        static inline void trim_whitespace(const char*& str, const char*& end)
        {
            while (str < end && (str[0] == ' ' || str[0] == '\t'))
                str++;
            while (end > str && (end[-1] == ' ' || end[-1] == '\t'))
                --end;
        }

        static u64 parse_uint(string_t const& _str)
        {
            const char* str = _str.m_str;
            const char* end = _str.m_end;
            trim_whitespace(str, end);
            u64 v = 0;
            while (str < end)
            {
                v = v * 10;
                v = v + (*str - '0');
                str++;
            }
            return v;
        }

        static s64 parse_int(string_t const& _str)
        {
            string_t   str    = _str;
            const char prefix = *str.m_str;
            if (prefix == '-' || prefix == '+')
                str.m_str++;
            s64 const v = (s64)parse_uint(str);
            if (prefix == '-')
                return -v;
            return v;
        }

        static f32 parse_float32(string_t const& str)
        {
            string_t intpart(str.m_str, str.m_str);
            string_t decpart(str.m_str, str.m_end);
            trim_whitespace(intpart.m_str, decpart.m_end);
            while (intpart.m_end < decpart.m_end)
            {
                if (*intpart.m_end == '.')
                {
                    decpart.m_str = intpart.m_end + 1;
                    break;
                }
                intpart.m_end++;
            }
            u64 const intvalue = parse_int(intpart);
            u64 const decvalue = parse_int(decpart);
            f32 const fint     = (f32)intvalue;
            f32 const fdec     = (f32)decvalue / (f32)(decpart.length() * 10);
            return fint + fdec;
        }

        static f64 parse_float64(string_t const& str)
        {
            string_t intpart(str.m_str, str.m_str);
            string_t decpart(str.m_str, str.m_end);
            trim_whitespace(intpart.m_str, decpart.m_end);
            while (intpart.m_end < decpart.m_end)
            {
                if (*intpart.m_end == '.')
                {
                    decpart.m_str = intpart.m_end + 1;
                    break;
                }
                intpart.m_end++;
            }
            u64 const intvalue = parse_int(intpart);
            u64 const decvalue = parse_int(decpart);
            f64 const fint     = (f64)intvalue;
            f64 const fdec     = (f64)decvalue / (f64)(decpart.length() * 10);
            return fint + fdec;
        }

        u32 handler_t::get_offset(s32 property_index, etype* property_type_array, s32 property_count)
        {
            ASSERT(property_index < property_count);
            u32 offset = 0;
            D_FOR_I(0, property_index) { offset += type_sizeof(property_type_array[i]); }
            return offset;
        }

        static s64 read_int(etype property_type, u32 property_offset, void* property_data)
        {
            s64 v = 0;
            switch (property_type)
            {
                case TYPE_INT8: v = *((s8*)((u8*)property_data + property_offset)); break;
                case TYPE_INT16: v = *((s16*)((u8*)property_data + property_offset)); break;
                case TYPE_INT32: v = *((s32*)((u8*)property_data + property_offset)); break;
                default: break;
            }
            return v;
        }
        static u64 read_uint(etype property_type, u32 property_offset, void* property_data)
        {
            u64 v = 0;
            switch (property_type)
            {
                case TYPE_UINT8: v = *((u8*)((u8*)property_data + property_offset)); break;
                case TYPE_UINT16: v = *((u16*)((u8*)property_data + property_offset)); break;
                case TYPE_UINT32: v = *((u32*)((u8*)property_data + property_offset)); break;
                default: break;
            }
            return v;
        }
        static f32 read_float32(f32 _default, etype property_type, u32 property_offset, void* property_data)
        {
            switch (property_type)
            {
                case TYPE_FLOAT32: return *((f32*)((u8*)property_data + property_offset));
                default: break;
            }
            return _default;
        }
        static f64 read_float64(f64 _default, etype property_type, u32 property_offset, void* property_data)
        {
            switch (property_type)
            {
                case TYPE_FLOAT64: return *((f64*)((u8*)property_data + property_offset));
                default: break;
            }
            return _default;
        }

        template <typename T> T read_integer(T _default, etype property_type, u32 property_offset, void* property_data)
        {
            switch (type_type(property_type))
            {
                case TYPE_SIGNED: return (T)read_int(property_type, property_offset, property_data);
                case TYPE_UNSIGNED: return (T)read_uint(property_type, property_offset, property_data);
                default: break;
            }
            return _default;
        }

        s8  handler_t::read_s8(etype property_type, u32 property_offset, void* property_data) { return read_integer<s8>(0, property_type, property_offset, property_data); }
        s16 handler_t::read_s16(etype property_type, u32 property_offset, void* property_data) { return read_integer<s16>(0, property_type, property_offset, property_data); }
        s32 handler_t::read_s32(etype property_type, u32 property_offset, void* property_data) { return read_integer<s32>(0, property_type, property_offset, property_data); }
        u8  handler_t::read_u8(etype property_type, u32 property_offset, void* property_data) { return read_integer<u8>(0, property_type, property_offset, property_data); }
        u16 handler_t::read_u16(etype property_type, u32 property_offset, void* property_data) { return read_integer<u16>(0, property_type, property_offset, property_data); }
        u32 handler_t::read_u32(etype property_type, u32 property_offset, void* property_data) { return read_integer<u32>(0, property_type, property_offset, property_data); }
        f32 handler_t::read_f32(etype property_type, u32 property_offset, void* property_data) { return read_float32(0.0f, property_type, property_offset, property_data); }
        f64 handler_t::read_f64(etype property_type, u32 property_offset, void* property_data) { return read_float64(0.0, property_type, property_offset, property_data); }

        // TODO: endian format
        static inline u8* write_bytes(u8* dst, u8 const* src, s32 size)
        {
            while (size > 0)
            {
                *dst++ = *src++;
                --size;
            }
            return dst;
        }

        template <typename T> u8* write_data(T v, u8* dst) { return write_bytes(dst, (u8 const*)&v, sizeof(T)); }

        bool read_element_data_ascii(ply_t* ply, reader_t* reader, element_t* elem, handler_t** handler_array, s32 handler_count)
        {
            u8 dst_buffer[128];
            for (s64 i = 0; i < elem->m_count; i++)
            {
                string_t line;
                do
                {
                    if (!reader->read_line(line.m_str, line.m_end))
                        return false;
                } while (line.is_empty());

                u8* dst = dst_buffer;
                D_FOR(s32, i, 0, elem->m_prop_count)
                {
                    property_t* prop = elem->m_prop_array[i];

                    string_t value_str = read_token(line);

                    if (prop->m_property_type == TYPE_FLOAT32)
                    {
                        f32 f = parse_float32(value_str);
                        write_data<f32>(f, dst);
                    }
                    else if (prop->m_property_type == TYPE_FLOAT64)
                    {
                        f64 f = parse_float64(value_str);
                        write_data<f64>(f, dst);
                    }
                    else if (type_is_int(prop->m_property_type))
                    {
                        s64 i = parse_int(value_str);
                        switch (prop->m_property_type)
                        {
                            case TYPE_INT8: write_data<s8>(i, dst); break;
                            case TYPE_INT16: write_data<s16>(i, dst); break;
                            case TYPE_INT32: write_data<s32>(i, dst); break;
                            default: break;
                        }
                    }
                    else if (type_is_uint(prop->m_property_type))
                    {
                        u64 i = parse_uint(value_str);
                        switch (prop->m_property_type)
                        {
                            case TYPE_UINT8: write_data<u8>(i, dst); break;
                            case TYPE_UINT16: write_data<u16>(i, dst); break;
                            case TYPE_UINT32: write_data<u32>(i, dst); break;
                            default: break;
                        }
                    }
                }

                D_FOR(s32, i, 0, handler_count)
                handler_array[i]->read(elem->m_index, elem->m_prop_type_array, elem->m_prop_count, dst);
            }
            return true;
        }

        void read_elements_ascii(ply_t* ply, reader_t* reader, handler_t** handler_array, s32 handler_count)
        {
            element_t* elem = ply->m_hdr->m_elements;
            while (elem != nullptr)
            {
                read_element_data_ascii(ply, reader, elem, handler_array, handler_count);
                elem = elem->m_next;
            }
        }

        void read_data(ply_t* ply, reader_t* reader, handler_t* handler1, handler_t* handler2)
        {
            s32 const  handler_count    = 2;
            handler_t* handler_array[2] = {handler1, handler2};

            element_t* elem = ply->m_hdr->m_elements;
            while (elem != nullptr)
            {
                for (s32 i = 0; i < handler_count; ++i)
                    handler_array[i]->setup(elem->m_index, elem->m_count, elem->m_prop_type_array, elem->m_prop_index_array, elem->m_prop_count);
                elem = elem->m_next;
            }

            // now read in the data, can be in ASCII or BINARY format
            if (ply->m_hdr->m_format == FORMAT_ASCII)
            {
                read_elements_ascii(ply, reader, handler_array, handler_count);
            }
            else
            {
                // TODO: read binary format
            }
        }

    } // namespace nply
} // namespace ncore