#include "includes.h"
#include "test.h"

BOOL flush_may_occur;
long expected_bytes_to_free;

static void
flush (CACHEFILE f __attribute__((__unused__)),
       int UU(fd),
       CACHEKEY k  __attribute__((__unused__)),
       void *v,
       void *e     __attribute__((__unused__)),
       long s      __attribute__((__unused__)),
       BOOL w      __attribute__((__unused__)),
       BOOL keep,
       BOOL c      __attribute__((__unused__))
       ) {
    assert(flush_may_occur);
    if (!keep) {
        int* foo = v;
        assert(*foo == 3);
        toku_free(v);
    }
}

static int
fetch (CACHEFILE f        __attribute__((__unused__)),
       int UU(fd),
       CACHEKEY k         __attribute__((__unused__)),
       u_int32_t fullhash __attribute__((__unused__)),
       void **value       __attribute__((__unused__)),
       long *sizep        __attribute__((__unused__)),
       int  *dirtyp,
       void *extraargs    __attribute__((__unused__))
       ) {
    *dirtyp = 0;
    int* foo = toku_malloc(sizeof(int));
    *value = foo;
    *sizep = 4;
    *foo = 4;
    return 0;
}

static void
other_flush (CACHEFILE f __attribute__((__unused__)),
       int UU(fd),
       CACHEKEY k  __attribute__((__unused__)),
       void *v     __attribute__((__unused__)),
       void *e     __attribute__((__unused__)),
       long s      __attribute__((__unused__)),
       BOOL w      __attribute__((__unused__)),
       BOOL keep   __attribute__((__unused__)),
       BOOL c      __attribute__((__unused__))
       ) {
}

static int 
pe_callback (
    void *brtnode_pv, 
    long bytes_to_free, 
    long* bytes_freed, 
    void* extraargs __attribute__((__unused__))
    ) 
{
    assert(expected_bytes_to_free == bytes_to_free);
    *bytes_freed = 1;
    expected_bytes_to_free--;
    int* foo = brtnode_pv;
    int blah = *foo;
    *foo = blah-1;
    return 0;
}

static int 
other_pe_callback (
    void *brtnode_pv __attribute__((__unused__)), 
    long bytes_to_free __attribute__((__unused__)), 
    long* bytes_freed __attribute__((__unused__)), 
    void* extraargs __attribute__((__unused__))
    ) 
{
    return 0;
}
static BOOL pf_req_callback(void* UU(brtnode_pv), void* UU(read_extraargs)) {
  return FALSE;
}

static int pf_callback(void* UU(brtnode_pv), void* UU(read_extraargs), int UU(fd), long* UU(sizep)) {
  assert(FALSE);
}


static void
cachetable_test (void) {
    const int test_limit = 16;
    int r;
    CACHETABLE ct;
    r = toku_create_cachetable(&ct, test_limit, ZERO_LSN, NULL_LOGGER); assert(r == 0);
    char fname1[] = __FILE__ "test1.dat";
    unlink(fname1);
    CACHEFILE f1;
    r = toku_cachetable_openf(&f1, ct, fname1, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO); assert(r == 0);

    void* v1;
    void* v2;
    long s1, s2;
    flush_may_occur = FALSE;
    for (int i = 0; i < 100000; i++) {
      r = toku_cachetable_get_and_pin(f1, make_blocknum(1), 1, &v1, &s1, flush, fetch, pe_callback, pf_req_callback, pf_callback, NULL, NULL);
        r = toku_cachetable_unpin(f1, make_blocknum(1), 1, CACHETABLE_CLEAN, 4);
    }
    for (int i = 0; i < 8; i++) {
      r = toku_cachetable_get_and_pin(f1, make_blocknum(2), 2, &v2, &s2, flush, fetch, pe_callback, pf_req_callback, pf_callback, NULL, NULL);
        r = toku_cachetable_unpin(f1, make_blocknum(2), 2, CACHETABLE_CLEAN, 4);
    }
    for (int i = 0; i < 4; i++) {
      r = toku_cachetable_get_and_pin(f1, make_blocknum(3), 3, &v2, &s2, flush, fetch, pe_callback, pf_req_callback, pf_callback, NULL, NULL);
        r = toku_cachetable_unpin(f1, make_blocknum(3), 3, CACHETABLE_CLEAN, 4);
    }
    for (int i = 0; i < 2; i++) {
      r = toku_cachetable_get_and_pin(f1, make_blocknum(4), 4, &v2, &s2, flush, fetch, pe_callback, pf_req_callback, pf_callback, NULL, NULL);
        r = toku_cachetable_unpin(f1, make_blocknum(4), 4, CACHETABLE_CLEAN, 4);
    }
    flush_may_occur = FALSE;
    expected_bytes_to_free = 4;
    r = toku_cachetable_put(f1, make_blocknum(5), 5, NULL, 4, other_flush, other_pe_callback, NULL);
    flush_may_occur = TRUE;
    r = toku_cachetable_unpin(f1, make_blocknum(5), 5, CACHETABLE_CLEAN, 4);

    r = toku_cachefile_close(&f1, 0, FALSE, ZERO_LSN); assert(r == 0 && f1 == 0);
    r = toku_cachetable_close(&ct); assert(r == 0 && ct == 0);
}

int
test_main(int argc, const char *argv[]) {
    default_parse_args(argc, argv);
    cachetable_test();
    return 0;
}