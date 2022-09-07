#include "cbase/c_target.h"
#include "cbase/c_debug.h"
#include "c3dff/c_ply.h"

namespace ncore
{
    namespace nply
    {
        enum etype
        {
            TYPE_SIGNED    = 0x100,
            TYPE_UNSIGNED  = 0x200,
            TYPE_FLOAT     = 0x400,
            TYPE_LIST      = 0x800,
            TYPE_INVALID   = 0,
            TYPE_INT8      = 0x1 | TYPE_SIGNED,
            TYPE_UINT8     = 0x1 | TYPE_UNSIGNED,
            TYPE_INT16     = 0x2 | TYPE_SIGNED,
            TYPE_UINT16    = 0x2 | TYPE_UNSIGNED,
            TYPE_INT32     = 0x4 | TYPE_SIGNED,
            TYPE_UINT32    = 0x4 | TYPE_UNSIGNED,
            TYPE_FLOAT32   = 0x4 | TYPE_FLOAT,
            TYPE_FLOAT64   = 0x8 | TYPE_FLOAT,
            TYPE_SIZE_MASK = 0xFF,
        };

        inline static s32  type_sizeof(etype type) { return (type & TYPE_SIZE_MASK); }
        inline static bool type_islist(etype type) { return (type & TYPE_LIST) == TYPE_LIST; }
        inline static bool type_is_int(etype type) { return (type & (TYPE_UNSIGNED | TYPE_SIGNED)) == (TYPE_UNSIGNED | TYPE_SIGNED); }
        inline static bool type_is_f32(etype type) { return (type & TYPE_FLOAT32) == TYPE_FLOAT32; }
        inline static bool type_is_f64(etype type) { return (type & TYPE_FLOAT64) == TYPE_FLOAT64; }

        struct string_t
        {
            string_t()
                : m_str(nullptr)
                , m_end(nullptr)
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

        inline static s32 compare(const string_t& as, const char* b)
        {
            const char* a = as.m_str;
            while (a < as.m_str && *b != 0)
            {
                if (*a != *b)
                {
                    if ((s32)(unsigned char)(*a) < (s32)(unsigned char)(*b))
                        return -1;
                    return 1;
                }
                ++a;
                ++b;
            }
            if (a == as.m_str && *b == 0)
                return 0;
            if (a < as.m_str)
                return 1;
            return -1;
        }

        static s32 compare(const string_t& as, const string_t& bs)
        {
            const char* a = as.m_str;
            const char* b = bs.m_str;
            while (a < as.m_end && b < bs.m_end)
            {
                if (*a != *b)
                {
                    if (*a < *b)
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
            etype    m_binary_type;   // how we are storing it
            s32      m_binary_offset; // in an element, what is the offset to write it at
        };

        struct buffer_t
        {
            u32 m_size; // size in bytes
            u8* m_buffer;
        };

        struct element_t
        {
            string_t    m_name;
            u32         m_binary_size; // size in bytes of one element
            u32         m_prop_count;
            property_t* m_prop_array;
            buffer_t*   m_data;
            element_t*  m_next;
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
            u32        m_num_elements;
            u32        m_num_comments;
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
        }

        static void add_comment(ply_t* ctxt, comment_t* comment)
        {
            comment->m_next = nullptr;
            if (ctxt->m_hdr->m_comments == nullptr)
            {
                ctxt->m_hdr->m_comments = comment;
            }
            else
            {
                ctxt->m_comments->m_next = comment;
            }
            ctxt->m_comments = comment;
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

        string_t make_string(ply_t* ctxt, string_t const& str)
        {
            s32 const   str_len    = ((str.length() + 1) + (4 - 1)) & ~(4 - 1);
            char*       dst_str    = (char*)ctxt->m_alloc->alloc(str_len);
            const char* dst_end    = dst_str + str_len;
            char*       dst_cursor = dst_str;
            const char* src_str    = str.m_str;
            while (src_str < str.m_end)
                *dst_cursor++ = *src_str++;
            *dst_cursor = 0;
            return string_t(dst_str, dst_cursor);
        }

        void read_header_comment(ply_t* ctxt, string_t& line)
        {
            comment_t* comment = construct<comment_t>(ctxt->m_alloc);
            comment->m_comment = make_string(ctxt, line);
            add_comment(ctxt, comment);
        }

        void read_header_format(ply_t* ctxt, string_t& line)
        {
            string_t token = read_token(line);
            if (token == "ascii")
                ctxt->m_hdr->m_format = FORMAT_ASCII;
            else if (token == "binary_little_endian")
                ctxt->m_hdr->m_format = FORMAT_BLE;
            else if (token == "binary_big_endian")
                ctxt->m_hdr->m_format = FORMAT_BBE;
            string_t version_str   = read_token(line);
            ctxt->m_hdr->m_version = make_string(ctxt, version_str);
        }

        element_t* read_header_element(ply_t* ctxt, string_t& line)
        {
            element_t* elem     = construct<element_t>(ctxt->m_alloc);
            string_t   name_str = read_token(line);
            elem->m_name        = make_string(ctxt, name_str);
            string_t str_count  = read_token(line);
            elem->m_prop_count  = parse_u32(str_count);
            elem->m_prop_array  = (property_t*)ctxt->m_alloc->alloc(sizeof(property_t) * elem->m_prop_count);
            return elem;
        }

        void read_header_property(ply_t* ctxt, element_t* elem, s32 prop_index, string_t& line)
        {
            ASSERT(elem->m_prop_array != nullptr);
            property_t* prop       = &elem->m_prop_array[prop_index];
            string_t    type_token = read_token(line);
            if (type_token == "list")
            {
                string_t count_type_token = read_token(line);
                string_t list_type_token  = read_token(line);
                string_t property_name    = read_token(line);
                prop->m_list_count_type   = parse_type(count_type_token);
                prop->m_property_type     = (etype)(parse_type(list_type_token) | TYPE_LIST);
                prop->m_name              = make_string(ctxt, property_name);
            }
            else
            {
                string_t property_type_token = read_token(line);
                string_t property_name       = read_token(line);
                prop->m_property_type        = parse_type(property_type_token);
                prop->m_name                 = make_string(ctxt, property_name);
            }
        }

        void read_header_obj_info(ply_t* ctxt, string_t& line)
        {
            // @todo: need information, haven't seen any .ply files with this
        }

        static bool read_header(ply_t* ctxt)
        {
            ctxt->m_hdr = construct<header_t>(ctxt->m_alloc);

            string_t line;
            ctxt->m_line_reader->read_line(line.m_str, line.m_end);

            element_t* element    = nullptr;
            s32        prop_index = 0;
            bool       success    = true;
            while (line.m_str != line.m_end)
            {
                string_t token = read_token(line);
                if (token == "comment")
                {
                    read_header_comment(ctxt, line);
                }
                else if (token == "format")
                {
                    read_header_format(ctxt, line);
                }
                else if (token == "element")
                {
                    element    = read_header_element(ctxt, line);
                    prop_index = 0;
                }
                else if (token == "property")
                {
                    read_header_property(ctxt, element, prop_index, line);
                    prop_index += 1;
                }
                else if (token == "obj_info")
                {
                    read_header_obj_info(ctxt, line);
                }
                else
                {
                    if (token == "ply" || token == "PLY" || token.is_empty())
                        continue;

                    if (token == "end_header")
                        break;

                    success = false;
                }

                if (!ctxt->m_line_reader->read_line(line.m_str, line.m_end))
                    break;
            }
            return success;
        }

        void read_element_data(ply_t* ctxt, element_t* elem, string_t& line)
        {
            // @todo: how are we going to read element data and where to store?
            // - first compute the binary size of an element (float[3]/uchar[4])
            // - allocate the buffer, count * binary size of one element
        }

        void read(ply_t* ply) { read_header(ply); }

    } // namespace nply
} // namespace ncore