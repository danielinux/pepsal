/*
 * PEPsal : A Performance Enhancing Proxy for Satellite Links
 *
 * Copyleft Dan Kruchinin <dkruchinin@acm.org> 2010
 * See AUTHORS and COPYING before using this software.
 *
 *
 *
 */

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "pepsal.h"
#include "syntab.h"

struct syn_table syntab;

/* Bob Jenkin's MIX function */
#define BJ_MIX(a, b, c)                            \
    do {                                           \
        a -= b; a -= c; a ^= (c>>13);              \
        b -= c; b -= a; b ^= (a<<8);               \
        c -= a; c -= b; c ^= (b>>13);              \
        a -= b; a -= c; a ^= (c>>12);              \
        b -= c; b -= a; b ^= (a<<16);              \
        c -= a; c -= b; c ^= (b>>5);               \
        a -= b; a -= c; a ^= (c>>3);               \
        b -= c; b -= a; b ^= (a<<10);              \
        c -= a; c -= b; c ^= (b>>15);              \
    } while (0)



static unsigned int syntab_hashfunction(void *k)
{
    struct syntab_key *sk = k;
    unsigned int a, b, key;

    key = sk->addr;
    a = sk->port;
    b = 0x9e3779b9; /* the golden ratio */

    BJ_MIX(a, b, key);

    /* Robert Jenkins' 32 bit integer hash function */
    key = (key + 0x7ed55d16) + (key << 12);
    key = (key ^ 0xc761c23c) ^ (key >> 19);
    key = (key + 0x165667b1) + (key << 5);
    key = (key + 0xd3a2646c) ^ (key << 9);
    key = (key + 0xfd7046c5) + (key << 3);
    key = (key ^ 0xb55a4f09) ^ (key >> 16);

    return key;
}

static int __keyeqfn(void *k1, void *k2)
{
    return (memcmp(k1, k2, sizeof(struct syntab_key)) == 0);
}

int syntab_init(int num_conns)
{
    int ret, hash_size;

    memset(&syntab, 0, sizeof(syntab));
    hash_size = (num_conns * 25) / 100; /* ~25% of max number of connections */
    syntab.hash = create_hashtable(hash_size, syntab_hashfunction, __keyeqfn);
    if (!syntab.hash) {
        ret = ENOMEM;
        goto err;
    }

    ret = pthread_rwlock_init(&syntab.lock, NULL);
    if (ret) {
        ret = errno;
        goto err_destroy_hash;
    }

    list_init_head(&syntab.conns);
    syntab.num_items = 0;

    return 0;

err_destroy_hash:
    hashtable_destroy(syntab.hash, 0);
err:
    errno = ret;
    return -1;
}

static __inline void syntab_make_key(struct syntab_key *key,
                                     int addr, unsigned short port)
{
	memset(key, 0, sizeof(*key));
    key->addr = addr;
    key->port = port;
}

struct pep_proxy *syntab_find(int addr, unsigned short port)
{
    struct syntab_key key;

    syntab_make_key(&key, addr, port);
    return hashtable_search(syntab.hash, &key);
}

int syntab_add(struct pep_proxy *proxy)
{
    struct syntab_key *key;
    int ret;

    assert(proxy->status == PST_PENDING);
    key = malloc(sizeof(*key));
    if (!key) {
        errno = ENOMEM;
        return -1;
    }

	syntab_make_key(key, proxy->src.addr, proxy->src.port);
    ret = hashtable_insert(syntab.hash, key, proxy);
    if (ret == 0) {
        free(key);
        return -1;
    }

    list_add2tail(&syntab.conns, &proxy->lnode);
    syntab.num_items++;

    return 0;
}

void syntab_delete(struct pep_proxy *proxy)
{
    struct syntab_key key;

    syntab_make_key(&key, proxy->src.addr, proxy->src.port);
    hashtable_remove(syntab.hash, &key);
    list_del(&proxy->lnode);
    syntab.num_items--;
}
