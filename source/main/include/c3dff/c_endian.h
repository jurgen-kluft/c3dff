#ifndef __C_3DFF_ENDIAN_H__
#define __C_3DFF_ENDIAN_H__
#include "cbase/c_target.h"
#ifdef USE_PRAGMA_ONCE
#pragma once
#endif

namespace ncore
{
    namespace nendian
    {
        template <typename T, typename T2> inline T2 swap(const T& v) { return v; }
        template <> inline u16                       swap<u16, u16>(const u16& v) { return (v << 8) | (v >> 8); }
        template <> inline u32                       swap<u32, u32>(const u32& v) { return (v << 24) | ((v << 8) & 0x00ff0000) | ((v >> 8) & 0x0000ff00) | (v >> 24); }
        template <> inline u64                       swap<u64, u64>(const u64& v)
        {
            return (((v & 0x00000000000000ffLL) << 56) | ((v & 0x000000000000ff00LL) << 40) | ((v & 0x0000000000ff0000LL) << 24) | ((v & 0x00000000ff000000LL) << 8) | ((v & 0x000000ff00000000LL) >> 8) | ((v & 0x0000ff0000000000LL) >> 24) |
                    ((v & 0x00ff000000000000LL) >> 40) | ((v & 0xff00000000000000LL) >> 56));
        }
        template <> inline s16 swap<s16, s16>(const s16& v)
        {
            u16 r = swap<u16, u16>(*(u16*)&v);
            return *(s16*)&r;
        }
        template <> inline s32 swap<s32, s32>(const s32& v)
        {
            u32 r = swap<u32, u32>(*(u32*)&v);
            return *(s32*)&r;
        }
        template <> inline s64 swap<s64, s64>(const s64& v)
        {
            u64 r = swap<u64, u64>(*(u64*)&v);
            return *(s64*)&r;
        }
        template <> inline f32 swap<u32, f32>(const u32& v)
        {
            union
            {
                f32 f;
                u32 i;
            };
            i = swap<u32, u32>(v);
            return f;
        }
        template <> inline f64 swap<u64, f64>(const u64& v)
        {
            union
            {
                f64 d;
                u64 i;
            };
            i = swap<u64, u64>(v);
            return d;
        }
    } // namespace nendian
} // namespace ncore

#endif // __C_3DFF_ENDIAN_H__