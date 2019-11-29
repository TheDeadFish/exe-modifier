#ifndef _STDAFX_H_
#define _STDAFX_H_
#include "stdshit.h"
#include "c-parse.h"
#include <malloc.h>
#include <Imagehlp.h>
	
static bool strchk(char* str) { 
	return (str && *str); }
	
int strifnd(const char* str, const char** tab, int len);
template<int size> int strifnd(const char* str , const char*(
	&str2)[size]) { return strifnd(str, str2, size); }

// old stdshit.h compat
template<class T,class U>
struct retpair{T a;U b;
constexpr retpair(T ai) : a(ai) {}
constexpr retpair(T ai,U bi) : a(ai), b(bi){}
operator T&(){return a;}};
template<class T,class U,class V,class W>
void getpair(T&a,U&b,const retpair<V,W>&ret){
a=ret.a;b=ret.b;}

template<class T>
struct xnlist{ T*dataPtr;
size_t count;
void init(){ZINIT;}
void free(){free_ref(dataPtr);}
T&xNextAlloc(void){
return::xNextAlloc(dataPtr,count);}
T&operator[](size_t index){
return dataPtr[index];}
T*begin(void){return dataPtr;}
T*end(void){return dataPtr+count;}};

template<class T, class U, class V>
bool inRange(T value, U min, V max) {
    return (value >= min) && (value <= max); }
	
SHITCALL cstr tempName(cch* prefix);

#define RngRBL Range

#define FOR_REV(var, rng, ...) { auto && __range = rng; \
	auto __begin = __range.end(); auto __end = __range.begin(); \
	while(__begin != __end) { var = *--__begin; __VA_ARGS__; }}
	
template <class T>
T* ringList_add(T*& root, T* node) {
	if(root == NULL) { node->next = node; }
	else { node->next = root->next;
		root->next = node; } return root = node;	
}

#define RingList_enum(root, pos, ...) { if(auto* pos = root) { \
	do { pos = pos->next; __VA_ARGS__; } while(pos != root); }}
	
// c++ is the biggest pile of shit
template <int N>
struct fStr { char data[N];
	constexpr fStr(cch* in) : data{} {
		for(int i=0; in[i]; i++) data[i] = in[i]; }
	operator cch*() const { return data; }
};

struct NullTermTmp {
	cstr& str; char ch;
	NullTermTmp(cstr& s) : str(s) { 
		ch = release(*str.end()); }
	~NullTermTmp() { *str.end() = ch; }
	operator char*() { return str; }
};

	
#endif
