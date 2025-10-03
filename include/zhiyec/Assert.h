#ifndef _Zhiyec_Assert_h
#define _Zhiyec_Assert_h

/* 变量拼接后缀 */
#define concat_var_suffix(var, suffix) concat_var_suffix_inner(var, suffix)
#define concat_var_suffix_inner(var, suffix) var##_##suffix

/* 编译期断言 */
#define static_assert(condition, message) \
    typedef char concat_var_suffix(static_assert, __LINE__)[(condition) ? 1 : -1]

#endif /* _Zhiyec_Assert_h */
