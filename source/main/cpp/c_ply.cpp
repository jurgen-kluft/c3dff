#include "cbase/c_target.h"
#include "cbase/c_debug.h"
#include "c3dff/c_ply.h"

namespace ncore
{
    namespace nply
    {
        inline static s32  type_sizeof(etype type) { return (type & TYPE_SIZE_MASK); }
        inline static s32  type_index(etype type) { return (type & TYPE_INDEX_MASK) >> TYPE_INDEX_SHIFT; }
        inline static bool type_is_list(etype type) { return (type & TYPE_LIST) == TYPE_LIST; }
        inline static bool type_is_int(etype type) { return (type & TYPE_SIGNED) == TYPE_SIGNED; }
        inline static bool type_is_int8(etype type) { return type_is_int(type) && (type_sizeof(type) == 1); }
        inline static bool type_is_int16(etype type) { return type_is_int(type) && (type_sizeof(type) == 2); }
        inline static bool type_is_int32(etype type) { return type_is_int(type) && (type_sizeof(type) == 4); }
        inline static bool type_is_int64(etype type) { return type_is_int(type) && (type_sizeof(type) == 8); }
        inline static bool type_is_uint(etype type) { return (type & TYPE_UNSIGNED) == TYPE_UNSIGNED; }
        inline static bool type_is_uint8(etype type) { return type_is_uint(type) && (type_sizeof(type) == 1); }
        inline static bool type_is_uint16(etype type) { return type_is_uint(type) && (type_sizeof(type) == 1); }
        inline static bool type_is_uint32(etype type) { return type_is_uint(type) && (type_sizeof(type) == 1); }
        inline static bool type_is_uint64(etype type) { return type_is_uint(type) && (type_sizeof(type) == 1); }
        inline static bool type_is_f32(etype type) { return (type & TYPE_FLOAT32) == TYPE_FLOAT32; }
        inline static bool type_is_f64(etype type) { return (type & TYPE_FLOAT64) == TYPE_FLOAT64; }
        inline static bool type_is_float(etype type) { return type_is_f32(type) || type_is_f64(type); }

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
            etype    m_binary_property_type;
            etype    m_binary_list_count_type;
            s32      m_binary_offset;
        };

        struct buffer_t
        {
            u32 m_size; // size in bytes
            u8* m_buffer;
        };

        struct element_t
        {
            string_t     m_name;
            u32          m_count;
            u32          m_binary_size; // size in bytes of one element
            s32          m_prop_count;
            property_t** m_prop_array;
            buffer_t     m_data;    // the array of elements
            buffer_t     m_content; // the data/content for each element
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
            allocator_t*  m_alloc;
            linereader_t* m_line_reader;
            header_t*     m_hdr;
            comment_t*    m_comments;
            objinfo_t*    m_obj_info;
            element_t*    m_elements;
        };

        ply_t* create(allocator_t* allocator, linereader_t* line_reader)
        {
            ply_t* ply         = construct<ply_t>(allocator);
            ply->m_alloc       = allocator;
            ply->m_line_reader = line_reader;
            ply->m_hdr         = nullptr;
            ply->m_comments    = nullptr;
            ply->m_elements    = nullptr;
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
            prop->m_binary_property_type   = prop->m_property_type;
            prop->m_binary_list_count_type = prop->m_list_count_type;
            prop->m_binary_offset          = 0;
            return prop;
        }

        void read_header_obj_info(ply_t* ply, string_t& line)
        {
            // @todo: need information, haven't seen any .ply files with this
        }

        bool read_header(ply_t* ply)
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
            while (ply->m_line_reader->read_line(line.m_str, line.m_end))
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
                                if (!ply->m_line_reader->read_line(line.m_str, line.m_end))
                                    return false;
                                token = read_token(line);
                            } while (token.is_empty());
                        } while (token == "property");

                        element->m_prop_count = prop_count;
                        element->m_prop_array = (property_t**)ply->m_alloc->alloc(sizeof(property_t*) * prop_count);
                        for (s32 i = 0; i < prop_count; i++)
                        {
                            element->m_prop_array[i] = properties[i];
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

        bool set_ignore_property(ply_t* ply, const char* element_name, const char* property_name)
        {
            element_t* elem = ply->m_hdr->m_elements;
            while (elem != nullptr)
            {
                if (elem->m_name == element_name)
                {
                    for (s32 i = 0; i < elem->m_prop_count; i++)
                    {
                        property_t* prop = elem->m_prop_array[i];
                        if (prop->m_name == property_name)
                        {
                            prop->m_binary_offset        = -1;
                            prop->m_binary_property_type = TYPE_INVALID;
                            return true;
                        }
                    }
                }
                elem = elem->m_next;
            }
            return false;
        }

        bool set_read_property(ply_t* ply, const char* element_name, const char* property_name, etype destination_type, s32 offset)
        {
            element_t* elem = ply->m_hdr->m_elements;
            while (elem != nullptr)
            {
                if (elem->m_name == element_name)
                {
                    for (s32 i = 0; i < elem->m_prop_count; i++)
                    {
                        property_t* prop = elem->m_prop_array[i];
                        if (prop->m_name == property_name)
                        {
                            prop->m_binary_offset        = offset;
                            prop->m_binary_property_type = destination_type;
                            return true;
                        }
                    }
                }
                elem = elem->m_next;
            }
            return false;
        }

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

        static inline u8* write_bytes(u8* dst, u8 const* src, s32 size)
        {
            while (size > 0)
            {
                *dst++ = *src++;
                --size;
            }
            return dst;
        }

        // TODO: endian format

        template <typename T> u8* write_data(T v, etype dst_type, u8* dst) { return write_bytes(dst, (u8 const*)&v, type_sizeof(dst_type)); }

        bool read_element_asciidata(ply_t* ply, element_t* elem)
        {
            elem->m_binary_size = 0;
            for (s32 i = 0; i < elem->m_prop_count; i++)
            {
                property_t* prop = elem->m_prop_array[i];
                elem->m_binary_size += prop->m_list_count_type * (prop->m_binary_property_type & TYPE_SIZE_MASK);
            }

            elem->m_data.m_buffer = (u8*)ply->m_alloc->alloc(elem->m_binary_size * elem->m_count);

            u8* dst = elem->m_data.m_buffer;
            for (s64 i = 0; i < elem->m_count; i++)
            {
                string_t line;
                do
                {
                    if (!ply->m_line_reader->read_line(line.m_str, line.m_end))
                        return false;
                } while (line.is_empty());

                for (s32 i = 0; i < elem->m_prop_count; ++i)
                {
                    property_t* prop = elem->m_prop_array[i];

                    string_t value_str = read_token(line);

                    if (prop->m_property_type == TYPE_FLOAT32)
                    {
                        f32 f = parse_float32(value_str);
                        write_data<f32>(f, prop->m_binary_property_type, dst);
                    }
                    else if (prop->m_property_type == TYPE_FLOAT64)
                    {
                        f64 f = parse_float64(value_str);
                        write_data<f64>(f, prop->m_binary_property_type, dst);
                    }
                    else if (type_is_int(prop->m_property_type))
                    {
                        s64 i = parse_int(value_str);
                        write_data<s64>(i, prop->m_binary_property_type, dst);
                    }
                    else if (type_is_uint(prop->m_property_type))
                    {
                        u64 i = parse_uint(value_str);
                        write_data<u64>(i, prop->m_binary_property_type, dst);
                    }
                }
            }
            return true;
        }

        void read_elements_ascii(ply_t* ply)
        {
            element_t* elem = ply->m_hdr->m_elements;
            while (elem != nullptr)
            {
                read_element_asciidata(ply, elem);
                elem = elem->m_next;
            }
        }

        void read_data(ply_t* ply)
        {
            // for each element pre-compute the binary size and also pre-compute the binary offset of each property
            element_t* elem = ply->m_hdr->m_elements;
            while (elem != nullptr)
            {
                u32 binary_offset = 0;
                for (s32 i = 0; i < elem->m_prop_count; i++)
                {
                    if (elem->m_prop_array[i]->m_binary_property_type != TYPE_INVALID)
                    {
                        elem->m_prop_array[i]->m_binary_offset = binary_offset;
                        if (type_is_list(elem->m_prop_array[i]->m_binary_property_type))
                        {
                            binary_offset += type_sizeof(elem->m_prop_array[i]->m_binary_list_count_type);
                            binary_offset += sizeof(void*); // the pointer to the data
                        }
                        binary_offset += type_sizeof(elem->m_prop_array[i]->m_binary_property_type);
                    }
                }
                elem->m_binary_size = binary_offset;
                elem                = elem->m_next;
            }

            // TODO: List items will get a pointer to their actual data, so we can pre-allocate the objects but
            //       their data needs to be a 'checkout'/'commit' mechanic.

            // now read in the data, can be in ASCII or BINARY format
            if (ply->m_hdr->m_format == FORMAT_ASCII)
            {
                read_elements_ascii(ply);
            }
            else
            {
                // TODO: read binary format
            }
        }

    } // namespace nply
} // namespace ncore