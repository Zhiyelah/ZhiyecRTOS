#ifndef _ZHIYEC_ASSERT_H
#define _ZHIYEC_ASSERT_H

/* 变量拼接后缀 */
#define concat_var_suffix(var, suffix) concat_var_suffix_inner(var, suffix)
#define concat_var_suffix_inner(var, suffix) var##_##suffix

/* 编译期断言 */
#define static_assert(condition, message) \
    typedef char concat_var_suffix(static_assert, __LINE__)[(condition) ? 1 : -1]

#endif /* _ZHIYEC_ASSERT_H */
