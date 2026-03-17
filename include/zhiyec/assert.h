/**
 * @file assert.h
 * @author Zhiyelah
 * @brief 断言实现
 */

#ifndef _ZHIYEC_ASSERT_H
#define _ZHIYEC_ASSERT_H

#ifndef NODEBUG
/* 运行时断言 */
#define assert(condition) ((void)0)

#else
#define assert(condition)
#endif /* NODEBUG */

/* 变量拼接后缀 */
#define concat_var_suffix(var, suffix) concat_var_suffix_inner(var, suffix)
#define concat_var_suffix_inner(var, suffix) var##_##suffix

/* 编译期断言 */
#define static_assert(condition, message) \
    typedef char concat_var_suffix(static_assert, __LINE__)[(condition) ? 1 : -1]

#endif /* _ZHIYEC_ASSERT_H */
