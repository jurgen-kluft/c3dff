#include "cbase/c_target.h"
#include "c3dff/c_ply.h"
#include "cunittest/cunittest.h"

using namespace ncore;

extern unsigned char   skull_txt[];
extern unsigned int    skull_txt_len;

class line_reader_inline : public ncore::nply::linereader_t
{
public:
    line_reader_inline(const char* str, u32 len) : m_str(str), m_cursor(str), m_end(str + len) {}

    virtual bool read_line(const char*& str, const char*& end)
    {
        return false;
    }

    const char* m_str;
    const char* m_cursor;
    const char* m_end;
};

class ply_allocator : public ncore::nply::allocator_t
{
public:
    virtual void* alloc(u32 size) 
    {
        return nullptr;
    }

    void reset() 
    {

    }
};

UNITTEST_SUITE_BEGIN(ply)
{
    UNITTEST_FIXTURE(main)
    {
        static ply_allocator sAllocator;

        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_TEST(test_read)
        {
            line_reader_inline reader((const char*)skull_txt, skull_txt_len);
            nply::ply_t* ply = nply::create(&sAllocator, &reader);
            nply::read(ply);
        }
    }
}
UNITTEST_SUITE_END
