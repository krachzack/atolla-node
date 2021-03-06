#ifdef UNIT_TESTING

    #ifdef __cplusplus
    extern "C" {
    #endif

    extern void mock_assert(const int result, const char* const expression,
                             const char * const file, const int line);

    #undef assert
    #define assert(expression) \
            mock_assert((int)(expression), #expression, __FILE__, __LINE__)

    #ifdef __cplusplus
    }
    #endif

#else

    #include <assert.h>

#endif
