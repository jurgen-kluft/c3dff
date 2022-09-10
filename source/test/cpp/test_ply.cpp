#include "cbase/c_target.h"
#include "cbase/c_allocator.h"
#include "c3dff/c_ply.h"
#include "cunittest/cunittest.h"

using namespace ncore;

extern unsigned char skull_ply[];
extern unsigned int  skull_ply_len;

class line_reader_inline : public ncore::nply::linereader_t
{
public:
    line_reader_inline(const char* str, u32 len)
        : m_str(str)
        , m_cursor(str)
        , m_end(str + len)
    {
    }

    virtual bool read_line(const char*& str, const char*& end)
    {
        str = m_cursor;
        end = m_cursor;
        while (*end != '\n' && *end != '\r' && end < m_end)
        {
            end++;
        }
        m_cursor = end;
        while ((*m_cursor == '\n' || *m_cursor == '\r') && m_cursor < m_end)
            m_cursor++;
        return str < m_end;
    }

    const char* m_str;
    const char* m_cursor;
    const char* m_end;
};

extern ncore::alloc_t* gTestAllocator;

class ply_allocator : public ncore::nply::allocator_t
{
    u8* m_memory;
    u8* m_ptr;
    u32 m_size;

public:
    void init()
    {
        m_size   = 128 * 1024 * 1024;
        m_memory = (u8*)gTestAllocator->allocate(m_size);
        m_ptr    = m_memory;
    }

    void exit() { gTestAllocator->deallocate(m_memory); }

    virtual void* alloc(u32 size)
    {
        ASSERT(size < ((m_memory + m_size) - m_ptr));
        u8* ptr = m_ptr;
        m_ptr += (size + (16 - 1)) & ~(16 - 1);
        return ptr;
    }

    void reset() { m_ptr = m_memory; }
};

UNITTEST_SUITE_BEGIN(ply)
{
    UNITTEST_FIXTURE(main)
    {
        static ply_allocator sAllocator;

        UNITTEST_FIXTURE_SETUP() { sAllocator.init(); }
        UNITTEST_FIXTURE_TEARDOWN() { sAllocator.exit(); }

        UNITTEST_TEST(test_read)
        {
            sAllocator.reset();

            line_reader_inline reader((const char*)skull_ply, skull_ply_len);
            nply::ply_t*       ply = nply::create(&sAllocator, &reader);

            CHECK_TRUE(nply::read_header(ply));
            {
                u32 const       vertex_count = get_element_count(ply, "vertex");
                nply::vertex_t* vertex_array = (nply::vertex_t*)sAllocator.alloc(sizeof(nply::vertex_t) * vertex_count);
                nply::vertices_handler_t vertices_handler(vertex_array, vertex_count);

                u32 const                 triangle_count = get_element_count(ply, "face");
                nply::triangle_t*         triangle_array = (nply::triangle_t*)sAllocator.alloc(sizeof(nply::triangle_t) * triangle_count);
                nply::triangles_handler_t triangles_handler(triangle_array, triangle_count);

                nply::read_data(ply, &vertices_handler, &triangles_handler);
            }
        }
    }
}
UNITTEST_SUITE_END
