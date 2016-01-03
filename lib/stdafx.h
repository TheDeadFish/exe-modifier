#ifndef _STDAFX_H_
#define _STDAFX_H_
#include "stdshit.h"
#include <malloc.h>
#include <Imagehlp.h>
#include "btree\btree_set.h"

template <class T, int (*comp)(const T &a, const T &b)>
struct btree_set_compar_
    : public btree::btree_key_compare_to_tag {
  int operator()(const T &a, const T &b) const {
		return comp(a, b); }};
template <class T, int (*compar)(const T &a, const T &b)>
using btree_set_compar = btree::btree_set<T,
	btree_set_compar_<T,compar>>;
	
static bool strchk(char* str) { 
	return (str && *str); }
	
int strifnd(const char* str, const char** tab, int len);
template<int size> int strifnd(const char* str , const char*(
	&str2)[size]) { return strifnd(str, str2, size); }

#endif
