// stdshit.h: Single file version
// DeadFish Shitware 2013-2014
// BuildDate: 08/26/14 06:32:40

#ifndef _STDSHIT_H_
#define _STDSHIT_H_
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <type_traits>

// new functions
template<class T,class U> union _CAST_{T src;U dst;};
#define CAST(type, x) (((_CAST_<typeof(x), type>*)&(x))->dst)
#define PCST(type, x) (((_CAST_<typeof(*x), type>*)(x))->dst)
template<class T> inline T toUpper(T ch){
	return((ch>='a') &&(ch<='z'))?ch-32:ch;}
template<class T, class U, class V>
bool inRange(T value, U min, V max) {
    return (value >= min) && (value <= max); }
template<class Type, class Next>
bool is_one_of(const Type& needle, const Next& next)
{return needle==next;}
template<class Type, class Next, class ... Rest>
bool is_one_of(const Type& needle, const Next& next, const Rest&... haystack)
{return needle==next || is_one_of(needle, haystack...);}
	
template<bool T,typename V>
using enable_if_t=typename std::enable_if<T,V>::type;
struct is_cstyle_castable_impl{
template<typename _From,typename _To,typename
=decltype((_To)(std::declval<_From>()))>
static std::true_type __test(int);
template<typename,typename>
static std::false_type __test(...);};
template<typename _From,typename _To>
struct is_cstyle_castable
:public std::integral_constant<bool,(decltype(
is_cstyle_castable_impl::__test<_From,_To>(0))::value)>{};
typedef unsigned char byte;
typedef unsigned short word;
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define movb2(dest, data)({ asm ("movb %b2, %b0" \
	: "=r"(dest) : "0"(dest), "g"(data)); })
#define movw2(dest, data)({ asm ("movw %w2, %w0" \
	: "=r"(dest) : "0"(dest), "g"(data)); })
#define movl2(dest, data)({ asm ("movl %k1, %k0" \
	: "=r"(dest) : "g"(data)); })
#define movb(data) ({ byte _x86bits_;\
	movb2(_x86bits_, data); _x86bits_; })
#define movw(data) ({ word _x86bits_;\
	movw2(_x86bits_, data); _x86bits_; })
#define movl(data) ({ int _x86bits_;\
	movl2(_x86bits_, data); _x86bits_; })
#define movmb2(dest, data)({ asm ("movb %b2, %b0" \
	: "=r"(dest) : "0"(dest), "m"(data)); })
#define movmw2(dest, data)({ asm ("movw %w2, %w0" \
	: "=r"(dest) : "0"(dest), "m"(data)); })
#define movml2(dest, data)({ asm ("movl %k1, %k0" \
	: "=r"(dest) : "m"(data)); })
#define movmb(data) ({ byte _x86bits_;\
	movmb2(_x86bits_, data); _x86bits_; })
#define movmw(data) ({ word _x86bits_;\
	movmw2(_x86bits_, data); _x86bits_; })
#define movml(data) ({ int _x86bits_;\
	movml2(_x86bits_, data); _x86bits_; })
#define movrb2(reg, dest, data)({ asm ("movb %b2, %b0" \
	: "="#reg(dest) : "0"(dest), "g"(data)); })
#define movrw2(reg, dest, data)({ asm ("movw %w2, %w0" \
	: "="#reg(dest) : "0"(dest), "g"(data)); })
#define movrl2(reg, dest, data)({ asm ("movl %k1, %k0" \
	: "="#reg(dest) : "g"(data)); })
#define movrb(reg, data) ({ byte _x86bits_;\
	movrb2(reg, _x86bits_, data); _x86bits_; })
#define movrw(reg, data) ({ word _x86bits_;\
	movrw2(reg, _x86bits_, data); _x86bits_; })
#define movrl(reg, data) ({ int _x86bits_;\
	movrl2(reg, _x86bits_, data); _x86bits_; })
#define movf2(reg, dest) asm volatile ("" :: #reg(dest))
#define movfb(reg, data) ({ byte _x86bits_ = data;\
	movf2(reg, _x86bits_); _x86bits_; })
#define movfw(reg, data) ({ word _x86bits_ = data;\
	movf2(reg, _x86bits_); _x86bits_; })
#define movfl(reg, data) ({ int _x86bits_ = data;\
	movf2(reg, _x86bits_); _x86bits_; })
#define lodsb2(ptr, al) \
	asm ("lodsb" :"=a"(al), "=S"(ptr) : "a"(al), "S"(ptr));
#define lodsb(ptr) ({ byte _x86bits_;\
	asm ("lodsb" :"=a"(_x86bits_), "=S"(ptr) : "S"(ptr)); \
	_x86bits_; })
#define lodsw2(ptr, ax) \
	asm ("lodsb" :"=a"(ax), "=S"(ptr) : "a"(ax), "S"(ptr));
#define lodsw(ptr) ({ word _x86bits_;\
	asm ("lodsw" :"=a"(_x86bits_), "=S"(ptr) : "S"(ptr)); \
	_x86bits_; })
#define stosb2(ptr, data) ({  \
	asm volatile ("stosb" : "=D"(ptr) : "D"(ptr), "a"(data)); })
#define stosb(ptr, data) ({  \
	asm volatile ("stosb" : "=D"(ptr) : "D"(ptr), "a"((byte)data)); })
#define stosw2(ptr, data) ({  \
	asm volatile ("stosw" : "=D"(ptr) : "D"(ptr), "a"(data)); })
#define stosw(ptr, data) ({  \
	asm volatile ("stosw" : "=D"(ptr) : "D"(ptr), "a"((word)data)); })
#define andb(dest, cnst) ({ asm ("and %b2, %b0" \
	: "=r"(dest) : "0"(dest), "g"(cnst)); })
#define andw(dest, cnst) ({ asm ("and $w2, %w0" \
	 : "=r"(dest) : "0"(dest), "g"(cnst)); })
#define orb(dest, cnst) ({ asm ("or %b2, %b0" \
	: "=r"(dest) : "0"(dest), "g"(cnst)); })
#define orw(dest, cnst) ({ asm ("or $w2, %w0" \
	 : "=r"(dest) : "0"(dest), "g"(cnst)); })
#define xorb(dest, cnst) ({ asm ("xor %b2, %b0" \
	: "=r"(dest) : "0"(dest), "g"(cnst)); })
#define xorw(dest, cnst) ({ asm ("xor $w2, %w0" \
	 : "=r"(dest) : "0"(dest), "g"(cnst)); })
#define DEF_EAX(arg) register arg asm ("eax")
#define DEF_EBX(arg) register arg asm ("ebx")
#define DEF_ECX(arg) register arg asm ("ecx")
#define DEF_EDX(arg) register arg asm ("edx")
#define DEF_ESI(arg) register arg asm ("esi")
#define DEF_EDI(arg) register arg asm ("edi")
#define movzwl(data) ({ int _x86bits_; \
	asm ("movzwl\t%1, %0" :"=r"(_x86bits_) : "r"((word)data));  \
	_x86bits_; })
#define incl(ptr) ({ \
	asm ("incl %0" :"=r"(ptr) : "0"(ptr)); })
#define incml(ptr) ({ \
	asm ("incl %0" :"=m"(ptr) : "m"(ptr)); })
#define clrl(dest) ({ \
	asm ("xor %k0,%k0" : "=r"(dest) ); })
#define nothing() ({ asm(" "); })
#define clobber(reg) asm("" ::: "%"#reg);
#include <type_traits>
#include <tuple>
#define MakeDelegate(obj, func) \
	decltype(Delegate_(obj, func))::Bind<func>(obj)
template<class R,class... P>
struct Delegate{
Delegate(){}
Delegate(void*object,void*stub)
{object_ptr=object;
stub_ptr=(stub_type)stub;}
template<class T>
void set(T object,R(__thiscall*stub)(T,P... params))
{object_ptr=(void*)object;
stub_ptr=(stub_type)stub;}
template<class T>
void set(T*object,R(T::*stub)(P... params))
{object_ptr=(void*)object;
stub_ptr=(void*)(object_ptr->*stub);}
R operator()(P... params)const
{return(*stub_ptr)(object_ptr,params...);}
bool isValid(void)
{return(stub_ptr!=0);}
typedef R(__thiscall*stub_type)
(void*object_ptr,P... params);
void*object_ptr;
stub_type stub_ptr;};
template<class R,typename... P>
struct Delegate_noctx{
template<R(*TMethod)(P...)>
struct Bind{
Bind(int dummy){}
template<class RO,class... PO>
static RO __thiscall stub(void*ctx,PO... params)
{return TMethod(params...);}
template<class RO,class... PO>
operator Delegate<RO,PO...>(){
return Delegate<RO,PO...>(
(void*)0,(void*)&stub<RO,PO...>);}};};
template<class R,typename... P>
Delegate_noctx<R,P...>Delegate_(int dummy,R(*TMethod)(P... params))
{return Delegate_noctx<R,P...>();}
template<class T,class R,typename... P>
struct Delegate_cdecl{
template<R(*TMethod)(T*,P...)>
struct Bind{
Bind(T*ctx){object_ptr=(void*)ctx;}
void*object_ptr;
template<class RO,class... PO>
static RO __thiscall stub(void*ctx,PO... params)
{return TMethod((T*)ctx,params...);}
template<class RO,class... PO>
operator Delegate<RO,PO...>(){
return Delegate<RO,PO...>(
object_ptr,(void*)&stub<RO,PO...>);}};};
template<class T,class R,typename... P>
Delegate_cdecl<T,R,P...>Delegate_(T*obj,R(*TMethod)(T*,P... params))
{return Delegate_cdecl<T,R,P...>();}
template<class T,class R,typename... P>
struct Delegate_member{
template<R(T::*TMethod)(P...)>
struct Bind{
Bind(T*ctx){object_ptr=(void*)ctx;}
void*object_ptr;
template<class RO,class... PO>
static RO __thiscall stub(void*ctx,PO... params)
{return(((T*)ctx)->*TMethod)(params...);}
template<class RO,class... PO>
operator Delegate<RO,PO...>(){
if(std::is_same<std::tuple<P...>,std::tuple<PO...>>::value)
return Delegate<RO,PO...>(object_ptr,
(void*)(((T*)object_ptr)->*TMethod));
return Delegate<RO,PO...>(
object_ptr,(void*)&stub<RO,PO...>);}};};
template<class T,class R,typename... P>
Delegate_member<T,R,P...>Delegate_(T*obj,R(T::*TMethod)(P... params))
{return Delegate_member<T,R,P...>();}
#define Delegate0 Delegate
#define MakeDelegate0(obj, func) MakeDelegate(obj, func)
#define Delegate1 Delegate
#define MakeDelegate1(obj, func) MakeDelegate(obj, func)
#define Delegate2 Delegate
#define MakeDelegate2(obj, func) MakeDelegate(obj, func)
#define DF_INTERFACE(type)			\
class type{							\
public:								\
	enum { val = __COUNTER__};		\
	void* object_ptr;				\
	void reset(void){				\
		for(int i = 1; i < sizeof(*this)/4; i++) 	\
			(&object_ptr)[i] = 0;}
#define DF_METHOD0(R)		\
	DfMethod<__COUNTER__-val, R>
#define DF_METHOD1(R, P1)		\
	DfMethod<__COUNTER__-val, R, P1>
#define DF_METHOD2(R, P1, P2)		\
	DfMethod<__COUNTER__-val, R, P1, P2>
#define DF_INTERFACE_END() };
template<int i,class R,class... P>
class DfMethod{
public:
void operator=(int j)
{stub_ptr=(stub_type*)j;}
void operator=(Delegate<R,P...>delegate)
{stub_ptr=delegate.stub_ptr;}
R operator()(P... params)const
{void*object_ptr=*((void**)this-i);
return(*stub_ptr)(object_ptr,params...);}
bool isValid(void)
{return(stub_ptr!=0);}
typedef R(__thiscall*stub_type)(
void*object_ptr,P... params);
stub_type stub_ptr;};
#include <type_traits>
template<class T,class U>
typename std::common_type<T&&,U&&>::type
min(T&&a,U&&b){
if(b<a)
return std::forward<U>(b);
return std::forward<T>(a);}
template<class T,class U>
typename std::common_type<T&&,U&&>::type
max(T&&a,U&&b){
if(b>a)
return std::forward<U>(b);
return std::forward<T>(a);}
template<class T,class U,class V>
typename std::common_type<T&&,U&&,V&&>::type
min_max(T&&val,U&&low,V&&high){
if(val<low)return std::forward<U>(low);
if(val>high)return std::forward<V>(high);
return std::forward<T>(val);}
#include <algorithm>
#define DEF_HAS_METHOD(func) \
	template<typename, typename T> struct has_##func {}; \
	template<typename C, typename Ret, typename... Args> \
	struct has_##func<C, Ret(Args...)> { private: \
	template<typename T> static constexpr auto check(T*) -> typename \
	std::is_same<decltype( std::declval<T>().func(	\
		std::declval<Args>()... ) ),Ret>::type; \
    template<typename> static constexpr std::false_type check(...); \
    typedef decltype(check<C>(0)) type; public: \
	static constexpr bool value = type::value; };
DEF_HAS_METHOD(compar);
template<bool>
struct _DFCompar_;
template<>
struct _DFCompar_<true>{
template<class T,class U>
static int comp(const T&a,const U&b){
return a.compar(b);}};
template<>
struct _DFCompar_<false>{
template<class T,class U>
static int comp(const T&a,const U&b){
if(b<a)return 1;if(a<b)return-1;
return 0;}};
template<class T,class U>
int comp_fwd(const T&a,const U&b){
return _DFCompar_<has_compar<T,
int(const T&)>::value>::comp(a,b);}
static int comp_fwd(const int&a,const int&b)
{return a-b;}
template<class T,class U>
int comp_rev(const T&a,const U&b)
{return comp_fwd(b,a);}
#define comp_ltn(type1, type2, comp)\
		[&](type1 a, type2 b) {	return comp(a, b) < 0; }
template<typename _ForwardIterator,typename _Tp,typename _Compare>
_ForwardIterator lower_sect(_ForwardIterator __first,_ForwardIterator __last,
const _Tp&__val,_Compare __comp){
typedef typename std::iterator_traits<_ForwardIterator>::value_type
_ValueType;
typedef typename std::iterator_traits<_ForwardIterator>::difference_type
_DistanceType;
_DistanceType __len=std::distance(__first,__last);
_ForwardIterator __middle=__first;
while(__len>0){
_DistanceType __half=__len>>1;
std::advance(__middle,__half);
if(__comp(*__middle,__val)){
__first=__middle;
__len=__len-__half-1;}
else
__len=__half;
__middle=__first+1;}
return __first;}
template<typename _ForwardIterator,typename _Tp,typename _Compare>
_ForwardIterator
upper_sect(_ForwardIterator __first,_ForwardIterator __last,
const _Tp&__val,_Compare __comp){
typedef typename std::iterator_traits<_ForwardIterator>::value_type
_ValueType;
typedef typename std::iterator_traits<_ForwardIterator>::difference_type
_DistanceType;
_DistanceType __len=std::distance(__first,__last);
_ForwardIterator __middle=__first;
while(__len>0){
_DistanceType __half=__len>>1;
std::advance(__middle,__half);
if(__comp(__val,*__middle))
__len=__half;
else{
__first=__middle;
__len=__len-__half-1;}
__middle=__first+1;}
return __first;}
template<class T,class U>
bool lower_sect_compare(const T&a,const U&b)
{return a<b;}
template<typename _ForwardIterator,typename _Tp>
_ForwardIterator lower_sect(_ForwardIterator __first,
_ForwardIterator __last,const _Tp&__val)
{return lower_sect(__first,__last,__val,
lower_sect_compare<typename std::iterator_traits
<_ForwardIterator>::value_type,_Tp>);}
template<typename _ForwardIterator,typename _Tp>
size_t lower_sect(_ForwardIterator __first,size_t length,const _Tp&__val)
{return lower_sect(__first,__first+length,__val)-__first;}
template<typename _ForwardIterator,typename _Tp,typename _Compare>
size_t lower_sect(_ForwardIterator __first,size_t length,const _Tp&__val,_Compare __comp)
{return lower_sect(__first,__first+length,__val,__comp)-__first;}
template<typename _ForwardIterator,typename _Tp>
_ForwardIterator upper_sect(_ForwardIterator __first,
_ForwardIterator __last,const _Tp&__val)
{return upper_sect(__first,__last,__val,
lower_sect_compare<typename std::iterator_traits
<_ForwardIterator>::value_type,_Tp>);}
template<typename _ForwardIterator,typename _Tp>
size_t upper_sect(_ForwardIterator __first,size_t length,const _Tp&__val)
{return upper_sect(__first,__first+length,__val)-__first;}
template<typename _ForwardIterator,typename _Tp,typename _Compare>
size_t upper_sect(_ForwardIterator __first,size_t length,const _Tp&__val,_Compare __comp)
{return upper_sect(__first,__first+length,__val,__comp)-__first;}
template<typename _ForwardIterator,typename _Tp>
size_t lower_bound(_ForwardIterator __first,size_t length,const _Tp&__val)
{return std::lower_bound(__first,__first+length,__val)-__first;}
template<typename _ForwardIterator,typename _Tp,typename _Compare>
size_t lower_bound(_ForwardIterator __first,size_t length,const _Tp&__val,_Compare __comp)
{return std::lower_bound(__first,__first+length,__val,__comp)-__first;}
template<typename _ForwardIterator,typename _Tp>
size_t upper_bound(_ForwardIterator __first,size_t length,const _Tp&__val)
{return std::upper_bound(__first,__first+length,__val)-__first;}
template<typename _ForwardIterator,typename _Tp,typename _Compare>
size_t upper_bound(_ForwardIterator __first,size_t length,const _Tp&__val,_Compare __comp)
{return std::upper_bound(__first,__first+length,__val,__comp)-__first;}
template<typename _ForwardIterator,typename _Tp>
_ForwardIterator binary_find(_ForwardIterator __first,_ForwardIterator __last,const _Tp&__val)
{typedef typename std::iterator_traits<_ForwardIterator>::value_type	_ValueType;
__first=std::lower_bound(__first,__last,__val,comp_ltn(_ValueType&,const _Tp&,
(comp_fwd<typeof(*__first),_Tp>)));
return(__first!=__last&&!comp_fwd(*__first,__val))?__first:__last;}
template<typename _ForwardIterator,typename _Tp,typename _Compare>
_ForwardIterator binary_find(_ForwardIterator __first,_ForwardIterator __last,
const _Tp&__val,_Compare __comp){
typedef typename std::iterator_traits<_ForwardIterator>::value_type	_ValueType;
__first=std::lower_bound(__first,__last,__val,comp_ltn(_ValueType&,const _Tp&,__comp));
return(__first!=__last&&!comp_fwd<typeof(*__first),_Tp>(*__first,__val))?__first:__last;}
template<typename _ForwardIterator,typename _Tp>
size_t binary_find(_ForwardIterator __first,size_t length,const _Tp&__val)
{return binary_find(__first,__first+length,__val)-__first;}
template<typename _ForwardIterator,typename _Tp,typename _Compare>
size_t binary_find(_ForwardIterator __first,size_t length,const _Tp&__val,_Compare __comp){
return binary_find(__first,__first+length,__val,__comp)-__first;}
#include <stdio.h>
struct Void{
size_t data;
Void(){}
template<class T>
Void(T in)
{data=(size_t)in;}
template<class T>
Void operator=(T in)
{data=(size_t)in;
return*this;}
template<class T>
operator T()
{return(T)data;}
template<class T>
operator T()const
{return(T)data;}
Void operator++(int)
{return this->data++;}
Void operator--(int)
{return this->data--;}
Void operator+=(Void offset)
{return data+=offset.data;}
Void operator-=(Void offset)
{return data-=offset.data;}
Void operator+(size_t offset)
{return data+offset;}
unsigned char&operator[](size_t n)
{return*(unsigned char*)(data+n);}
unsigned char&operator*()
{return*(unsigned char*)(data+0);}
template<class T>
T&ref(size_t n=0)
{return((T*)(data))[n];}
template<class T>
T*ptr(size_t n)
{return&((T*)(data))[n];}
template<class T>
T*&ptr(void)
{return*(T**)&data;}
BYTE&byte(size_t n=0)
{return((BYTE*)(data))[n];}
WORD&word(size_t n=0)
{return((WORD*)(data))[n];}
DWORD&dword(size_t n=0)
{return((DWORD*)(data))[n];}
int offset(Void ptr)
{return ptr.data-data;}
void align(int size){
data=(data+(size-1))&~(size-1);}};
#define VOID_OPERATOR(retType) \
	template <class T, class U> enable_if_t< \
	is_cstyle_castable<T, size_t>::value \
	&& is_cstyle_castable<U, size_t>::value \
	&& (std::is_same<T, Void>::value \
	|| std::is_same<U, Void>::value), retType>
VOID_OPERATOR(Void)operator-(const T&a,const U&b){
return size_t(a)-size_t(b);}
VOID_OPERATOR(bool)operator==(const T&a,const U&b){
return size_t(a)==size_t(b);}
VOID_OPERATOR(bool)operator!=(const T&a,const U&b){
return size_t(a)!=size_t(b);}
VOID_OPERATOR(bool)operator<(const T&a,const U&b){
return size_t(a)<size_t(b);}
VOID_OPERATOR(bool)operator>(const T&a,const U&b){
return size_t(a)>size_t(b);}
VOID_OPERATOR(bool)operator<=(const T&a,const U&b){
return size_t(a)<=size_t(b);}
VOID_OPERATOR(bool)operator>=(const T&a,const U&b){
return size_t(a)>=size_t(b);}
#define NORETURN  __attribute__((noreturn))
#define UNREACH __builtin_unreachable()
#define FATALFUNC __attribute__((noreturn,cold))
#define NOTHROW __attribute__((__nothrow__))
#define getReturn() __builtin_return_address(0)
#define TLS_VAR __thread
#define TLS_EXTERN extern __thread
#define INITIALIZER(f) \
 __attribute__((constructor)) void f(void)
#define SHITCALL2 __fastcall
#define SHITCALL __stdcall
#define SHITSTATIC __stdcall static
#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
#define ZINIT memset(this, 0, sizeof(*this))
#define ei else if
#define THIS_NULL_CHECK() if(this == NULL) return 0;
#define ALIGN4(arg) ALIGN(arg, 3)
#define ALIGN_PAGE(arg) ALIGN(arg, 4095)
#undef offsetof
#define offsetof(st, m) ((size_t)(&((st *)0)->m))
#define MACRO_CAT2(name, line) name##line
#define MACRO_CAT(name, line) MACRO_CAT2(name, line)
#define PTRADD(ptr, offset) (ptr = Void(ptr)+(offset))
#define PTRDIFF(ptr1, ptr2) (size_t(ptr1)-size_t(ptr2))
static inline size_t ALIGN(size_t arg,size_t bound)
{return((arg+bound)&~bound);}
#define DEF_ERRENUM(x) x
#define DEF_ERRTEXT(x) #x
#define DEF_ERRCODE(codes, func) \
  enum codes(DEF_ERRENUM); \
  static const char* func(int errCode) { \
	const char* const errText[] = codes(DEF_ERRTEXT); \
	return errText[errCode]; }
template<class T,class U>
struct retpair{T a;U b;
constexpr retpair(T ai) : a(ai) {}
constexpr retpair(T ai,U bi) : a(ai), b(bi){}
operator T&(){return a;}};
template<class T,class U,class V,class W>
void getpair(T&a,U&b,const retpair<V,W>&ret){
a=ret.a;b=ret.b;}
#define DEF_RETPAIR(name, T, a, U, b) \
struct name { T a; U b; name() {} \
	name(T ai) { a = ai; } \
	name(T ai, U bi) { a = ai; b = bi; } \
	operator T&() { return a; } };
#define VA_NARGS_IMPL(_1, _2, _3, _4, _5, N, ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(X,##__VA_ARGS__, 4, 3, 2, 1, 0)
#define VARARG_IMPL2(base, count, ...) base##count(__VA_ARGS__)
#define VARARG_IMPL(base, count, ...) VARARG_IMPL2(base, count, __VA_ARGS__)
#define VARARG(base, ...) VARARG_IMPL(base, VA_NARGS(__VA_ARGS__), __VA_ARGS__)
template<class F>
class finally_type{F function;
public:finally_type(F f):function(f){}
~finally_type(){function();}};
template<class F>finally_type<F>
finally(F f){return finally_type<F>(f);}
#define SCOPE_EXIT(f) auto MACRO_CAT(sExit, __LINE__) = finally([&]{f})
template<typename T>struct Range1_type2
{T value;
Range1_type2(T in):value(in){}
T operator*(){return value;}
bool operator!=(const Range1_type2<T>&that)const
{return that.value!=value;}
void operator++(){value++;}};
template<typename T>class Range1_type{
public:Range1_type(T start,T end):
begin_(start),end_(end){}
Range1_type2<T>begin(){return begin_;}
Range1_type2<T>end(){return end_;}
Range1_type2<T>begin_,end_;};
template<typename T>class Range2_type{
public:Range2_type(T*collection,size_t size):
mCollection(collection),mSize(size){}
T*begin(){return&mCollection[0];}
T*end(){return&mCollection[mSize];}
private:T*mCollection;size_t mSize;};
template<typename T>
Range1_type<T>Range(T start,T end)
{return Range1_type<T>(start,end);}
template<typename T>
Range2_type<T>Range(T*array,size_t size)
{return Range2_type<T>(array,size);}
typedef unsigned char 	byte;
typedef unsigned int 	uint;
typedef unsigned int 	u32;
typedef signed int 		s32;
typedef unsigned char 	u8;
typedef signed char 	s8;
typedef unsigned short 	u16;
typedef signed short 	s16;
static uint bswap32(uint val)
{return __builtin_bswap32(val);}
static uint swap_endian32(uint val)
{return __builtin_bswap32(val);}
static u16 swap_endian16(u16 in)
{return(in>>8)|(in<<8);}
struct big_u32{
big_u32(const big_u32&in){data=in.data;}
big_u32(u32 in){data=swap_endian32(in);}
operator u32(){return swap_endian32(data);}
template<class screb>
operator screb(){return swap_endian32(data);}
private:u32 data;};
struct big_u16{
big_u16(const big_u16&in){data=in.data;}
big_u16(u16 in){data=swap_endian32(in);}
operator u16(){return swap_endian32(data);}
template<class screb>
operator screb(){return swap_endian32(data);}
private:u16 data;};
static uint snapUpSize(uint val)
{return 2<<(__builtin_clz(val-1)^31);}
extern const char progName[];
extern HWND errWindow;
void contError(HWND hwnd,const char*fmt,...);
FATALFUNC void fatalError(const char*fmt,...);
FATALFUNC void fatalError(HWND hwnd,const char*fmt,...);
FATALFUNC void errorAlloc();
FATALFUNC void errorMaxPath();
FATALFUNC void errorDiskSpace();
FATALFUNC void errorDiskWrite();
FATALFUNC void errorDiskFail();
FATALFUNC void errorBadFile();
static inline
Void errorAlloc(Void ptr)
{if(!ptr)errorAlloc();return ptr;}
static inline
Void malloc_(size_t size)
{return malloc(size);}
static inline
Void realloc_(void*ptr,size_t size)
{return realloc(ptr,size);}
#define malloc malloc_
#define realloc realloc_
SHITCALL void free_ref(Void&ptr);
SHITCALL uint snapNext(uint val);
SHITCALL2 Void xmalloc(size_t size);
SHITCALL2 Void xrealloc(Void&ptr,size_t size);
SHITCALL2 Void xnxalloc(Void&ptr,size_t&count,size_t size);
template<class T>
void free_ref(T*&ptr)
{free_ref(*(Void*)&ptr);}
template<class T>
Void xrealloc(T*&ptr,size_t size)
{return xrealloc(*(Void*)&ptr,size);}
template<class T>
Void xnxalloc(T*&ptr,size_t&count,size_t size)
{return xnxalloc(*(Void*)&ptr,count,size);}
struct xMalloc{
xMalloc(size_t size){this->size=size;}
operator void*(){return xmalloc(size);}
template<class T>
operator T*(){return(T*)xmalloc(sizeof(T)*size);}
private:size_t size;};
template<class T>
Void xRealloc(T*&ptr,size_t size)
{return xrealloc(ptr,size*sizeof(T));}
template<class T,class U>
T&xNextAlloc(T*&ptr,U&size)
{return*(T*)xnxalloc(ptr,*(size_t*)&size,sizeof(T));}
struct xvector{
Void dataPtr;
size_t dataSize;
size_t allocSize;
void init(){ZINIT;}
void free(){free_ref(dataPtr);}
Void xnxalloc(size_t size);};
template<class T>
struct xnlist{
T*dataPtr;
size_t count;
void init(){ZINIT;}
void free(){free_ref(dataPtr);}
T&xNextAlloc(void){
return::xNextAlloc(dataPtr,count);}
T&operator[](size_t index){
return dataPtr[index];}
T*begin(void){return dataPtr;}
T*end(void){return dataPtr+count;}};
#define fclose fclose_
SHITCALL int fclose_(FILE*stream);
SHITCALL void fclose_ref(FILE*&stream);
SHITCALL FILE*xfopen(const char*,const char*);
SHITCALL FILE*xfopen(const wchar_t*,const wchar_t*);
SHITCALL char*xfgets(char*,int,FILE*);
SHITCALL wchar_t*xfgets(wchar_t*,int,FILE*);
SHITCALL void xfread(void*,size_t,size_t,FILE*);
SHITCALL void xfwrite(const void*,size_t,size_t,FILE*);
SHITCALL void xchsize(FILE*fp,long size);
SHITCALL int fsize(FILE*fp);
SHITCALL char**loadText(const char*fileName,int&LineCount);
SHITCALL char**loadText(const wchar_t*fileName,int&LineCount);
SHITCALL char**loadText(FILE*fp,int&LineCount);
template<class T>void xfread(T*ptr,size_t size,FILE*fp){xfread(ptr,sizeof(T),size,fp);}
template<class T>void xfwrite(const T*ptr,size_t size,FILE*fp){xfwrite(ptr,sizeof(T),size,fp);}
template<class T>void xfread(T*ptr,FILE*fp){xfread(ptr,1,fp);}
template<class T>void xfwrite(T*ptr,FILE*fp){xfwrite(ptr,1,fp);}
template<typename T,int size>void xfread(T(&ptr)[size],FILE*fp){xfread(ptr,size,fp);}
template<typename T,int size>void xfwrite(T(&ptr)[size],FILE*fp){xfwrite(ptr,size,fp);}
inline FILE*fopen(const wchar_t*fName,const wchar_t*mode)
{return _wfopen(fName,mode);}
inline wchar_t*fgets(wchar_t*str,int num,FILE*fp)
{return fgetws(str,num,fp);}
inline size_t strlen(const wchar_t*str)
{return wcslen(str);}
inline wchar_t*strcpy(wchar_t*d,const wchar_t*s)
{return wcscpy(d,s);}
inline int strcmp(const wchar_t*str1,const wchar_t*str2)
{return wcscmp(str1,str2);}
inline int stricmp(const wchar_t*str1,const wchar_t*str2)
{return wcsicmp(str1,str2);}
inline int strncmp(const wchar_t*str1,const wchar_t*str2,size_t num)
{return wcsncmp(str1,str2,num);}
inline int strnicmp(const wchar_t*str1,const wchar_t*str2,size_t num)
{return wcsnicmp(str1,str2,num);}
SHITCALL char*xstrdup(const char*);
SHITCALL wchar_t*xstrdup(const wchar_t*);
SHITCALL char*xstrcat(const char*,const char*);
SHITCALL wchar_t*xstrcat(const wchar_t*,const wchar_t*);
template<typename T,int size>
int strncmp(const T*str1,const T(&str2)[size])
{return strncmp(str1,str2,size-1);}
template<typename T,int size>
int strnicmp(const T*str1,const T(&str2)[size])
{return strnicmp(str1,str2,size-1);}
SHITCALL char*strScmp(const char*,const char*);
SHITCALL wchar_t*strScmp(const wchar_t*,const wchar_t*);
SHITCALL char*strSicmp(const char*,const char*);
SHITCALL wchar_t*strSicmp(const wchar_t*,const wchar_t*);
SHITCALL int strEcmp(const char*,const char*);
SHITCALL int strEcmp(const wchar_t*,const wchar_t*);
SHITCALL int strEicmp(const char*,const char*);
SHITCALL int strEicmp(const wchar_t*,const wchar_t*);
SHITCALL int strNcpy(char*,const char*,int);
SHITCALL int strNcpy(wchar_t*,const wchar_t*,int);
SHITCALL int removeCrap(char*);
SHITCALL int removeCrap(wchar_t*);
SHITCALL int strmove(char*,const char*);
SHITCALL int strmove(wchar_t*,const wchar_t*);
SHITCALL void strcpyn(char*,const char*,int);
SHITCALL void strcpyn(wchar_t*,const wchar_t*,int);
SHITCALL bool strcmpn(const char*,const char*,int);
SHITCALL bool strcmpn(const wchar_t*,const wchar_t*,int);
SHITCALL int getPathLen(const char*);
SHITCALL int getPathLen(const wchar_t*);
SHITCALL int getPath(char*);
SHITCALL int getPath(wchar_t);
SHITCALL char*getName(const char*);
SHITCALL wchar_t*getName(const wchar_t*);
SHITCALL int getName(char*dst,const char*src,size_t max);
SHITCALL int getName(wchar_t*dst,const wchar_t*src,size_t max);
SHITCALL bool isFullPath(const char*);
SHITCALL bool isFullPath(const wchar_t*);
inline bool isPathSep(int ch)
{return(ch=='\\')||(ch=='/');}
Void memmem(const void*b1,const void*b2,
size_t len1,size_t len2);
template<class T,class F>
void qsort(T*base,size_t num,F compar)
{typedef int(*qcomp)(const T&,const T&);
qsort(base,num,sizeof(*base),(Void)(qcomp)compar);}
template<class T>
void qsort(T*base,size_t num)
{qsort(base,num,comp_fwd<T,T>);}
template<class T>
void qsort_rev(T*base,size_t num)
{qsort(base,num,comp_rev<T,T>);}
template<class T,class U>
void memcpy8_ref(T*&dst,U*&src,int count){
asm volatile("rep movsb":
"=S"(src),"=D"(dst),"=c"(count)
:"S"(src),"D"(dst),"c"(count));}
template<class T,class U>
void memcpy16_ref(T*&dst,U*&src,int count){
asm volatile("rep movsw":
"=S"(src),"=D"(dst),"=c"(count)
:"S"(src),"D"(dst),"c"(count));}
template<class T,class U>
void memcpy32_ref(T*&dst,U*&src,int count){
asm volatile("rep movsd":
"=S"(src),"=D"(dst),"=c"(count)
:"S"(src),"D"(dst),"c"(count));}
template<class T,class U>
void memcpy_ref(T*&dst,U*&src,int count){
if((sizeof(T)%4)==0)memcpy32_ref(dst,src,count*(sizeof(T)/4));
ei((sizeof(T)%2)==0)memcpy16_ref(dst,src,count*(sizeof(T)/2));
else			  		 memcpy8_ref(dst,src,count*sizeof(T));}
template<class T>
T*memcpy8(T*dst,const void*src,int count){
memcpy8_ref(dst,src,count);return dst;}
template<class T>
T*memcpy16(T*dst,const void*src,int count){
memcpy16_ref(dst,src,count);return dst;}
template<class T>
T*memcpy32(T*dst,const void*src,int count){
memcpy32_ref(dst,src,count);return dst;}
template<class T>
T*memcpyX(T*dst,const void*src,int count){
memcpy_ref(dst,src,count);return dst;}
#endif