#ifndef __STATIC_ASSERT_H__
#define __STATIC_ASSERT_H__

// reference BOOST_STATIC_ASSERT

#define __concatenate_direct__(x, y) x##y
#define __concatenate__(x, y) __concatenate_direct__(x, y)
//#define __concatenate_3__(x, y, z) __concatenate_direct__(x, __concatenate_direct__(y, z))

template<bool x> struct STATIC_ASSERTION_FAILURE;
template<> struct STATIC_ASSERTION_FAILURE<true> { };
template<int x> struct static_assert_test { };

#define STATIC_ASSERT(B) typedef static_assert_test<sizeof(STATIC_ASSERTION_FAILURE<(bool)(B)>)> __concatenate__(static_assert_typedef_, __LINE__)
//#define STATIC_ASSERT_MSG(B. Msg) typedef static_assert_test<sizeof(STATIC_ASSERTION_FAILURE<(bool)(B)>)> __concatenate__(static_assert_typedef_, __LINE__)

/*

altenative  implementation 

#define STATIC_ASSERT(B)	typedef int __concatenate__(__static_assert__, __LINE__)[B ? 0 : -1]       // In GNU C  zero-size array is allowed :)

*/

#endif //  static_assert.h

