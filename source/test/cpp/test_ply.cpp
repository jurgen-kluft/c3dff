#include "ccore/c_target.h"
#include "cbase/c_allocator.h"
#include "c3dff/c_ply.h"
#include "cunittest/cunittest.h"

using namespace ncore;

extern unsigned char skull_ply[];
extern unsigned int  skull_ply_len;

class reader_test : public ncore::nply::reader_t
{
public:
    reader_test(const char* str, u32 len)
        : m_str(str)
        , m_cursor(str)
        , m_end(str + len)
    {
    }

    virtual bool read_line(const char*& str, const char*& end)
    {
        const char* cursor = ncore::nply::g_ReadLine(m_cursor, m_end, str, end);
        if (cursor > m_cursor)
        {
            m_cursor = cursor;
            return true;
        }
        return false;
    }

    virtual bool read_data(u32 size, const u8*& begin, const u8*& end)
    {
        if ((m_cursor + size) <= m_end)
        {
            begin    = (const u8*)m_cursor;
            m_cursor = m_cursor + size;
            end      = (const u8*)m_cursor;
            return true;
        }
        return false;
    }

    const char* m_str;
    const char* m_cursor;
    const char* m_end;
};

class ply_allocator : public ncore::nply::allocator_t
{
    UnitTest::TestAllocator* m_allocator;
    u8* m_memory;
    u8* m_ptr;
    u32 m_size;

public:
    void init(UnitTest::TestAllocator* allocator)
    {
        m_allocator = allocator;
        m_size   = 128 * 1024 * 1024;
        m_memory = (u8*)m_allocator->allocate(m_size, 8);
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
        static ply_allocator sAllocator();

        UNITTEST_FIXTURE_SETUP() { sAllocator.init(&TestAllocator); }
        UNITTEST_FIXTURE_TEARDOWN() { sAllocator.exit(); }

        UNITTEST_TEST(test_read)
        {
            sAllocator.reset();

            nply::ply_t* ply = nply::create(&sAllocator);

            reader_test reader((const char*)skull_ply, skull_ply_len);
            CHECK_TRUE(nply::read_header(ply, &reader));
            {
                u32 const                vertex_count = get_element_count(ply, "vertex");
                nply::vertex_t*          vertex_array = (nply::vertex_t*)sAllocator.alloc(sizeof(nply::vertex_t) * vertex_count);
                nply::vertices_handler_t vertices_handler(vertex_array, vertex_count);

                u32 const                 triangle_count = get_element_count(ply, "face");
                nply::triangle_t*         triangle_array = (nply::triangle_t*)sAllocator.alloc(sizeof(nply::triangle_t) * triangle_count);
                nply::triangles_handler_t triangles_handler(triangle_array, triangle_count);

                nply::read_data(ply, &reader, &vertices_handler, &triangles_handler);
            }
        }
    }
}
UNITTEST_SUITE_END
