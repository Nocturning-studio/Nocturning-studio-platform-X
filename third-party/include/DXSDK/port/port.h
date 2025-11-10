// visual studio 2005

#ifndef _In_reads_bytes_
#define _In_reads_bytes_(...)
#endif

#ifndef _Out_writes_bytes_
#define _Out_writes_bytes_(...)
#endif

#ifndef _Out_writes_bytes_to_
#define _Out_writes_bytes_to_(...)
#endif

#ifndef _Out_writes_all_opt_
#define _Out_writes_all_opt_(...)
#endif

#ifndef _In_reads_to_opt_
#define _In_reads_to_opt_(...)
#endif

#ifndef _Out_writes_to_opt_
#define _Out_writes_to_opt_(...)
#endif

#ifndef _In_reads_
#define _In_reads_(...)
#endif

#ifndef _Out_writes_
#define _Out_writes_(...)
#endif

#ifndef _Inptr_result_bytebuffer_
#define _Inptr_result_bytebuffer_(...)
#endif

#ifndef _Outptr_result_bytebuffer_
#define _Outptr_result_bytebuffer_(...)
#endif

#ifndef _Inout_updates_bytes_
#define _Inout_updates_bytes_(...)
#endif

#ifndef _Always_
#define _Always_(...)
#endif

#ifndef _Field_size_
#define _Field_size_(...)
#endif

#ifndef _Field_size_opt_
#define _Field_size_opt_(...)
#endif

#ifndef _Field_size_full_opt_
#define _Field_size_full_opt_(...)
#endif

#ifndef _In_reads_bytes_opt_
#define _In_reads_bytes_opt_(a)			__in_bcount_opt	(a)
#endif

#ifndef _Out_writes_bytes_opt_
#define _Out_writes_bytes_opt_(a)		__out_bcount_opt(a)
#endif

#ifndef _In_reads_opt_
#define _In_reads_opt_(a)				__in_ecount_opt(a)
#endif

#ifndef _Out_writes_opt_
#define _Out_writes_opt_(a)				__out_ecount_opt(a)
#endif

#ifndef _Outptr_result_maybenull_
#define _Outptr_result_maybenull_		__out_opt
#endif

#ifndef _Inptr_result_maybenull_
#define _Inptr_result_maybenull_		__in_opt
#endif

#ifndef _Outptr_opt_result_maybenull_
#define _Outptr_opt_result_maybenull_	__out_opt
#endif

#ifndef _Inptr_opt_result_maybenull_
#define _Inptr_opt_result_maybenull_	__in_opt
#endif

#ifndef _Intptr_
#define _Intptr_						__in
#endif

#ifndef _Outptr_
#define _Outptr_						__out
#endif

#ifndef _COM_Outptr_
#define _COM_Outptr_
#endif

#ifndef _COM_Outptr_opt_
#define _COM_Outptr_opt_
#endif

#ifndef _Outptr_opt_
#define _Outptr_opt_
#endif

#ifndef _COM_Outptr_opt_result_maybenull_
#define _COM_Outptr_opt_result_maybenull_
#endif

//
//These macros are defined in the Windows 7 SDK, however to enable development using the technical preview,
//they are included here temporarily.
//
#ifndef DEFINE_ENUM_FLAG_OPERATORS 
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
extern "C++" { \
inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) | ((int)b)); } \
inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) & ((int)b)); } \
inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((int)a)); } \
inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) ^ ((int)b)); } \
inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
}
#endif

#ifndef DXGI_ERROR_DEVICE_RESET
#define DXGI_ERROR_DEVICE_RESET                 MAKE_DXGI_HRESULT(7)
#endif

#ifndef _Outptr_opt_result_bytebuffer_
#define _Outptr_opt_result_bytebuffer_(...)	__deref_opt_out_bcount(__VA_ARGS__)
#endif

#ifndef _Check_return_
#define _Check_return_
#endif
