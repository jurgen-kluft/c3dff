#include "cbase/c_target.h"
#include "cbase/c_debug.h"
#include "c3dff/c_ply.h"

namespace ncore
{
    namespace nply
    {
        enum PlyType
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

        inline static s32  type_sizeof(PlyType type) { return (type & TYPE_SIZE_MASK); }
        inline static bool type_islist(PlyType type) { return (type & TYPE_LIST) == TYPE_LIST; }
        inline static bool type_is_int(PlyType type) { return (type & (TYPE_UNSIGNED | TYPE_SIGNED)) == (TYPE_UNSIGNED | TYPE_SIGNED); }
        inline static bool type_is_f32(PlyType type) { return (type & TYPE_FLOAT32) == TYPE_FLOAT32; }
        inline static bool type_is_f64(PlyType type) { return (type & TYPE_FLOAT64) == TYPE_FLOAT64; }

        struct PlyString
        {
            PlyString(const char* str, const char* end)
                : m_str(str)
                , m_end(end)
            {
            }
            PlyString(const char* str, u32 len)
                : m_str(str)
                , m_end(str + len)
            {
            }
            const char* m_str;
            const char* m_end;

            inline bool is_empty() const { return m_str == m_end; }

            inline static s32 compare(const PlyString& as, const char* b)
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

            static s32 compare(const PlyString& as, const PlyString& bs)
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
        };

        static bool operator==(const PlyString& _this, const char* other) { return PlyString::compare(_this, other) == 0; }
        static bool operator!=(const PlyString& _this, const char* other) { return PlyString::compare(_this, other) != 0; }

        static bool operator==(const PlyString& _this, const PlyString& other) { return PlyString::compare(_this, other) == 0; }
        static bool operator!=(const PlyString& _this, const PlyString& other) { return PlyString::compare(_this, other) != 0; }

        static PlyType parse_type(PlyString const& str)
        {
            // @note: Can be optimized a bit by branching on the first character, e.g. 'i', 'u', 'f'
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

        template <typename T, typename T2> inline T2 endian_swap(const T& v) { return v; }
        template <> inline u16                       endian_swap<u16, u16>(const u16& v) { return (v << 8) | (v >> 8); }
        template <> inline u32                       endian_swap<u32, u32>(const u32& v) { return (v << 24) | ((v << 8) & 0x00ff0000) | ((v >> 8) & 0x0000ff00) | (v >> 24); }
        template <> inline u64                       endian_swap<u64, u64>(const u64& v)
        {
            return (((v & 0x00000000000000ffLL) << 56) | ((v & 0x000000000000ff00LL) << 40) | ((v & 0x0000000000ff0000LL) << 24) | ((v & 0x00000000ff000000LL) << 8) | ((v & 0x000000ff00000000LL) >> 8) | ((v & 0x0000ff0000000000LL) >> 24) |
                    ((v & 0x00ff000000000000LL) >> 40) | ((v & 0xff00000000000000LL) >> 56));
        }
        template <> inline s16 endian_swap<s16, s16>(const s16& v)
        {
            u16 r = endian_swap<u16, u16>(*(u16*)&v);
            return *(s16*)&r;
        }
        template <> inline s32 endian_swap<s32, s32>(const s32& v)
        {
            u32 r = endian_swap<u32, u32>(*(u32*)&v);
            return *(s32*)&r;
        }
        template <> inline s64 endian_swap<s64, s64>(const s64& v)
        {
            u64 r = endian_swap<u64, u64>(*(u64*)&v);
            return *(s64*)&r;
        }
        template <> inline f32 endian_swap<u32, f32>(const u32& v)
        {
            union
            {
                f32 f;
                u32 i;
            };
            i = endian_swap<u32, u32>(v);
            return f;
        }
        template <> inline f64 endian_swap<u64, f64>(const u64& v)
        {
            union
            {
                f64 d;
                u64 i;
            };
            i = endian_swap<u64, u64>(v);
            return d;
        }

        class PlyAllocator
        {
        public:
            template <typename T> T* construct()
            {
                void* ptr = v_allocate(sizeof(T));
                return (T*)ptr;
            }
            void* alloc(u32 size) { return v_allocate(size); }

        protected:
            virtual void* v_allocate(u32 size) { return nullptr; }
        };

        struct PlyProperty
        {
            PlyString m_name;
            PlyType   m_property_type{TYPE_INVALID};
            PlyType   m_list_count_type{TYPE_INVALID};
        };

        struct PlyBuffer
        {
            u32 m_size;
            u8* m_buffer;
        };

        struct PlyElement
        {
            PlyString    m_name;
            u32          m_prop_count;
            PlyProperty* m_prop_array;
            PlyBuffer*   m_data;
            PlyElement*  m_next;
        };

        struct PlyComment
        {
            PlyString   m_comment;
            PlyComment* m_next;
        };

        enum PlyFormat
        {
            FORMAT_ASCII = 0, // ASCII
            FORMAT_BLE,       // Binary Little Endian
            FORMAT_BBE,       // Binary Big Endian
        };

        struct PlyHeader
        {
            PlyFormat   m_format;
            PlyString   m_version;
            u32         m_num_elements;
            u32         m_num_comments;
            PlyComment* m_comments;
            PlyElement* m_elements;
        };

        struct PlyCtxt
        {
            PlyAllocator* m_alloc;
            PlyHeader*    m_hdr;
            PlyComment*   m_comments;
            PlyElement*   m_elements;
        };

        static void add_comment(PlyCtxt* ctxt, PlyComment* comment)
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

        static PlyString read_line(const char*& str, const char* str_end)
        {
            const char* line = str;
            while (str < str_end)
            {
                if (str[0] == '\n')
                {
                    return PlyString(line, str);
                }
                str++;
            }
            return PlyString(str_end, str_end);
        }

        static void skip_whitespace(PlyString& line)
        {
            while (line.m_str < line.m_end)
            {
                if (line.m_str[0] != ' ' && line.m_str[0] != '\t')
                    break;
                line.m_str++;
            }
        }

        static PlyString read_token(PlyString& line)
        {
            skip_whitespace(line);
            const char* token = line.m_str;
            while (line.m_str < line.m_end)
            {
                if (line.m_str[0] == ' ' || line.m_str[0] == '\t')
                    break;
                line.m_str++;
            }
            return PlyString(token, line.m_str);
        }

        static u32 parse_u32(PlyString const& token)
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

        static void        read_header_comment(PlyCtxt* ctxt, PlyString& line);
        static void        read_header_format(PlyCtxt* ctxt, PlyString& line);
        static PlyElement* read_header_element(PlyCtxt* ctxt, PlyString& line);
        static void        read_header_property(PlyCtxt* ctxt, PlyElement* elem, PlyString& line);
        static void        read_header_obj_info(PlyCtxt* ctxt, PlyString& line);

        static bool parse_header(PlyCtxt* ctxt, const char* hdr_begin, const char* hdr_end)
        {
            ctxt->m_hdr = ctxt->m_alloc->construct<PlyHeader>();

            PlyString   line       = read_line(hdr_begin, hdr_end);
            PlyElement* element    = nullptr;
            s32         prop_index = 0;
            bool        success    = true;
            while (line.m_str != line.m_end)
            {
                PlyString token = read_token(line);
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
                    read_header_property(ctxt, element, line);
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

                line = read_line(hdr_begin, hdr_end);
            }
            return success;
        }

        void read_header_comment(PlyCtxt* ctxt, PlyString& line)
        {
            PlyComment* comment = ctxt->m_alloc->construct<PlyComment>();
            comment->m_comment  = line;
            add_comment(ctxt, comment);
        }

        void read_header_format(PlyCtxt* ctxt, PlyString& line)
        {
            PlyString token = read_token(line);
            if (token == "ascii")
                ctxt->m_hdr->m_format = FORMAT_ASCII;
            else if (token == "binary_little_endian")
                ctxt->m_hdr->m_format = FORMAT_BLE;
            else if (token == "binary_big_endian")
                ctxt->m_hdr->m_format = FORMAT_BBE;
            ctxt->m_hdr->m_version = read_token(line);
        }

        PlyElement* read_header_element(PlyCtxt* ctxt, PlyString& line)
        {
            PlyElement* elem    = ctxt->m_alloc->construct<PlyElement>();
            elem->m_name        = read_token(line);
            PlyString str_count = read_token(line);
            elem->m_prop_count  = parse_u32(str_count);
            elem->m_prop_array = (PlyProperty*)ctxt->m_alloc->alloc(sizeof(PlyProperty) * elem->m_prop_count);
            return elem;
        }

        void read_header_property(PlyCtxt* ctxt, PlyElement* elem, s32 prop_index, PlyString& line)
        {
            ASSERT(elem->m_prop_array != nullptr);
            PlyProperty* prop       = &elem->m_prop_array[prop_index];
            PlyString    type_token = read_token(line);
            if (type_token == "list")
            {
                PlyString count_type_token = read_token(line);
                PlyString list_type_token  = read_token(line);
                PlyString property_name    = read_token(line);
                prop->m_list_count_type    = parse_type(count_type_token);
                prop->m_property_type      = parse_type(list_type_token) | TYPE_LIST;
                prop->m_name               = property_name;
            }
            else
            {
                PlyString property_type_token = read_token(line);
                PlyString property_name       = read_token(line);
                prop->m_property_type         = parse_type(property_type_token);
                prop->m_name                  = property_name;
            }
        }

        void read_header_obj_info(PlyCtxt* ctxt, PlyString& line)
        {
            // @todo: need information, haven't seen any .ply files with this

        }


        void read_element_data(PlyCtxt* ctxt, PlyElement* elem, PlyString& line)
        {
            // @todo: how are we going to read element data and where to store?
            // - first compute the binary size of an element (float[3]/uchar[4])
            // - allocate the buffer, count * binary size of one element

            // @todo: when the user just asks for the vertices, we could return
            //        PlyArray with the stride size of one element and the property information.
            // Or

            struct user_vertex_t
            {
                float x,y,z;
                u8 color[3]; // only r, g, b
            };

            // element name, property name, destination type, binary offset
            // the user can now also mark some properties to skip!
            set_offset("vertex", "x", TYPE_FLOAT32, 0);
            set_offset("vertex", "y", TYPE_FLOAT32, 4);
            set_offset("vertex", "z", TYPE_FLOAT32, 8);
            set_offset("vertex", "r", TYPE_UINT8, 12, 0xFF); // default = 0xFF
            set_offset("vertex", "g", TYPE_UINT8, 13, 0xFF);
            set_offset("vertex", "b", TYPE_UINT8, 14, 0xFF);
            set_offset("vertex", "a", TYPE_INVALID, -1, 0xFF); // discard 'a'

            // read ply file with the above knowledge
            // read_ply( ... )

            // then now the user can ask the element buffer attached to the 'vertices' element
            ply_array_t<user_vertex_t> vertices = get_array<user_vertex_t>("vertices");
            
        }

    } // namespace nply
} // namespace ncore