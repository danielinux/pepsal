/*
 * PEPsal : A Performance Enhancing Proxy for Satellite Links
 *
 * Copyleft Daniele Lacamera 2005-2007
 * Copyleft Dan Kruchinin <dkruchinin@acm.org> 2010
 * See AUTHORS and COPYING before using this software.
 *
 *
 *
 */

#ifndef __PEPSAL_SYNTAB_H
#define __PEPSAL_SYNTAB_H

#include <pthread.h>
#include "hashtable.h"
#include "list.h"
#include "pepsal.h"

struct syn_table{
    struct hashtable  *hash;
    struct list_head   conns;
    pthread_rwlock_t   lock;
    int                num_items;
};

struct syntab_key {
    int addr;
    unsigned short port;
};

#define GET_SYNTAB() (&syntab)

#define SYNTAB_LOCK_READ()    pthread_rwlock_rdlock(&(GET_SYNTAB())->lock)
#define SYNTAB_LOCK_WRITE()   pthread_rwlock_wrlock(&(GET_SYNTAB())->lock)
#define SYNTAB_UNLOCK_READ()  pthread_rwlock_unlock(&(GET_SYNTAB())->lock)
#define SYNTAB_UNLOCK_WRITE() pthread_rwlock_unlock(&(GET_SYNTAB())->lock)

extern struct syn_table syntab;

#define syntab_foreach_connection(con)                                  \
    list_for_each_entry(&GET_SYNTAB()->conns, con, struct pep_proxy, lnode)

int syntab_init(int num_conns);
struct pep_proxy *syntab_find(int addr, unsigned short port);
int syntab_add(struct pep_proxy *proxy);
void syntab_delete(struct pep_proxy *proxy);

#endif /* __PEPSAL_SYNTAB_H */
