// stdshit.h: Single file version
// DeadFish Shitware 2013-2014
// BuildDate: 11/01/16 16:25:57

#define _STDSHIT_CC_
#ifndef _STDSHIT_X_
#include "stdshit.h"
__attribute__((section(".text$powersOf10")))
const uint powersOf10[]={1,10,100,1000,10000,
100000,1000000,10000000,100000000,1000000000};
__attribute__((section(".text$tableOfHex")))
const byte tableOfHex[2][16]={
{'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'},
{'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'}};
#ifndef __SSE4_1__
#define _DFC_(n, v) ASM_FUNC("_f"#n, "pushl $"#v"; fnstcw 2(%esp); " \
 "fldcw (%esp); frndint; fldcw 2(%esp); addl $4, %esp; ret; "); \
 ASM_FUNC("_i"#n, "pushl $"#v"; push %eax; fnstcw 6(%esp); fldcw 4(%esp);" \
 "fistpl (%esp); fldcw 6(%esp); pop %eax; addl $4, %esp; ret;");
_DFC_(ceil,0xB7F)_DFC_(floor,0x77F);
#undef _DFC_
#endif
ASM_FUNC("_sfree","push %eax; push %edx; push %ecx; push 16(%esp);"
"call _free; add $4, %esp; pop %ecx; pop %edx; pop %eax; ret $4");
ASM_FUNC("_sfreer","push %eax; movl 8(%esp), %eax; cmp $0, (%eax);"
"jz 1f; push (%eax); call _sfree; and $0, (%eax); 1: pop %eax; ret $4;");
ASM_FUNC("_smalloc","push %edx; push %ecx; push %eax;"
"call _malloc; add $4, %esp; pop %ecx; pop %edx; ret");
ASM_FUNC("_xmalloc","movl 4(%esp), %eax; test %eax,%eax; jz 1f; call "
"_smalloc; test %eax,%eax; jnz 1f; call __Z10errorAllocv; 1: ret $4;");
SHITCALL uint snapNext(uint val){if(val&(val-1))return 0;
if(val>=2)return val<<1;return val+1;}
SHITCALL int fclose_ref(FILE*&stream){if(!stream)
return(int)stream;return fclose(release(stream));}
#undef fclose
SHITCALL int fclose_(FILE*stream){
if(stream==NULL)return 0;return fclose(stream);}
SHITCALL void freeLst(Void ptr,int count){if(!ptr)return;
for(int i=0;i<count;i++)free((void*)ptr.sizet(i));}
SHITCALL void freeLst_ref(Void&ptr,int count){
freeLst(ptr,count);free_ref(ptr);}
SHITCALL Void calloc(size_t size){return calloc(1,size);}
SHITCALL2
Void nxalloc(Void&ptr,size_t&count_,size_t size){
size_t result=movrl(b,count_);incml(count_);
size_t count=movrl(d,result);
int count_1=movfx(a,result-1);
result*=size;Void ptr2;
if(likely(count&count_1)){
ptr2=ptr;goto SKIP_ALLOC;}
count<<=1;if(count==0)count++;
if(!(ptr2=realloc(ptr,size*count))){
return ptr2;count_--;}ptr=ptr2;
SKIP_ALLOC:
return ptr2+result;}
SHITCALL2 Void xcalloc(size_t size){if(size==0)return NULL;return errorAlloc(calloc(1,size));}
REGCALL(2)Void xrealloc(Void&ptr,size_t size){if(size==0){free_ref(ptr);return NULL;}
return ptr=errorAlloc(realloc(ptr,size));}
SHITCALL2 Void xrecalloc(Void&ptr,size_t size){if(ptr==NULL)return(ptr=xcalloc(size));
return xrealloc(ptr,size);}
SHITCALL2 Void xmemdup8(Void src,int count){if(count==0)return NULL;
void*dst=xmalloc(count);memcpy8(dst,src,count);return dst;}
SHITCALL2 Void xmemdup16(Void src,int count){if(count==0)return NULL;
void*dst=xmalloc(count*2);memcpy16(dst,src,count);return dst;}
SHITCALL2 Void xmemdup32(Void src,int count){if(count==0)return NULL;
void*dst=xmalloc(count*4);memcpy32(dst,src,count);return dst;}
SHITCALL2
Void xnxalloc(Void&ptr,size_t&count_,size_t size){
size_t result=movrl(b,count_);incml(count_);
size_t count=movrl(d,result);
int count_1=movfx(a,result-1);
result*=size;Void ptr2;
if(likely(count&count_1)){
ptr2=ptr;goto SKIP_ALLOC;}
count<<=1;if(count==0)count++;
ptr2=xrealloc(ptr,size*count);
SKIP_ALLOC:
return ptr2+result;}
Void xvector_::xalloc_(size_t size){
this->dataSize=size;
this->allocSize=size;
return dataPtr=xmalloc(size);}
Void xvector_::xreserve_(size_t size){
if(this->allocSize>=size)
return this->dataPtr;
this->allocSize=size;
return xrealloc(this->dataPtr,size);}
Void xvector_::xresize_(size_t size){
this->dataSize=size;
ARGFIX(size);
return this->xreserve_(size);}
VoidLen REGCALL(3)xvector_alloc(
size_t reqSize,size_t curSize,xvector_*This){
size_t memSize=This->allocSize<<1;
if(memSize<reqSize)memSize=reqSize;
memSize=ALIGN(memSize,7);This->allocSize=memSize;
return VoidLen(xrealloc(This->dataPtr,memSize),curSize);}
VoidLen xvector_::xnxalloc2_(size_t size){
size_t curSize=this->dataSize;
size_t reqSize=movfx(a,curSize+size);
this->dataSize=reqSize;
Void ptr2;
if(likely(this->allocSize>=reqSize)){
ptr2=this->dataPtr;goto SKIP_ALLOC;}
{GET_RETPAIR(ptr2,curSize,xvector_alloc(
reqSize,curSize,this))};
SKIP_ALLOC:
return VoidLen(ptr2+curSize,curSize);}
VoidLen xvector_::xrxalloc2_(size_t size){
size_t curSize=this->dataSize;
size_t reqSize=movfx(a,curSize+size);
Void ptr2;
if(likely(this->allocSize>=reqSize)){
ptr2=this->dataPtr;goto SKIP_ALLOC;}
{GET_RETPAIR(ptr2,curSize,xvector_alloc(
reqSize,curSize,this))};
SKIP_ALLOC:
return VoidLen(ptr2+curSize,curSize);}
RetEdx<int>xvector_::write8(Void src,int count){VoidLen vl=
xnxalloc2_(count);memcpy8((byte*)vl.ptr,src,count);return vl.offset;}
RetEdx<int>xvector_::write16(Void src,int count){VoidLen vl=
xnxalloc2_(count*2);memcpy16((word*)vl.ptr,src,count);return vl.offset;}
RetEdx<int>xvector_::write32(Void src,int count){VoidLen vl=
xnxalloc2_(count*4);memcpy32((uint*)vl.ptr,src,count);return vl.offset;}
RetEdx<int>xvector_::write8(byte val){VoidLen vl=xnxalloc2_(1);
*(byte*)vl.ptr=val;return vl.offset;}
RetEdx<int>xvector_::write16(word val){VoidLen vl=xnxalloc2_(2);
*(word*)vl.ptr=val;return vl.offset;}
RetEdx<int>xvector_::write32(uint val){VoidLen vl=xnxalloc2_(4);
*(uint*)vl.ptr=val;return vl.offset;}
RetEdx<int>xvector_::write64(INT64 val){VoidLen vl=xnxalloc2_(8);
*(INT64*)vl.ptr=val;return vl.offset;}
ASM_FUNC("_UTF8_LEN1","cmp $127, %eax; ja _UTF8_LEN2; mov $1, %eax; ret;"
GLOB_LAB("_UTF8_LEN2")"cmp $2047, %eax; ja 1f; mov $2, %eax; ret;"
"1: sub $65536, %eax; mov $4, %eax; sbb $0, %eax; ret");
ASM_FUNC("_UTF8_PUT1","cmp $127, %eax; "
"ja _UTF8_PUT2; stosb; ret;"GLOB_LAB("_UTF8_PUT2")
"cmpl $2048, %eax; movl %eax, %edx; jae _UTF8_PUT34;"
"sar $6, %eax; orb $192, %al; 1: stosb; andb $63, %dl;"
"orb $128, %dl; mov %dl, (%edi); inc %edi; ret;"
"_UTF8_PUT34: cmpl $65536, %eax; jae _UTF8_PUT4;"
"sar $12, %eax; orb $224, %al; 2: stosb; movl %edx, %eax;"
"sar $6, %eax; andb $63, %al; orb $128, %al; jmp 1b;"
"_UTF8_PUT4: sar $18, %eax; orb $240, %al; stosb; movl %edx, %eax;"
"sar $12, %eax; andb $63, %al; orb $128, %al; jmp 2b"
);
#define UTF8_GETB(x) "movb " #x "(%esi), %dl; sall	$6, %eax;" \
	"xorb $128, %dl; cmpb $64, %dl; jae _UTF8_GETE; orb %dl, %al;"
#define UTF8_CHK(x) "cmpl %edx, %esi; je _UTF8_GETE; andb $" #x ", %al;"
ASM_FUNC("_UTF8_GET1","orl $-1, %edx;"GLOB_LAB("_UTF8_GET2")
"cmpb $194, %al; jb _UTF8_GETE; cmpb $224, %al; jae _UTF8_GET34;"
UTF8_CHK(31)UTF8_GETB(0)"inc %esi; ret;"
"_UTF8_GET34: dec %edx;	cmpb $240, %al; jae _UTF8_GET4;"
UTF8_CHK(15)UTF8_GETB(0)UTF8_GETB(1)
"cmpw $2048, %ax; jb _UTF8_GETE; inc %esi; inc %esi; ret;"
"_UTF8_GETE: movl $63, %eax; ret;"
"_UTF8_GET4: dec %edx;"UTF8_CHK(15)
UTF8_GETB(0)"cmpl $16, %eax; jb _UTF8_GETE;"
"cmpw $272, %ax; jae _UTF8_GETE;"
UTF8_GETB(1)UTF8_GETB(2)"lea 3(%esi), %esi; ret"
);
ASM_FUNC("_UTF16_PUT1",
"cmp $65536, %eax; jb 1f; movl %eax, %edx;"
"shrl $10, %eax; andw $1023, %dx;"
"addw $55232, %ax; addw $56320, %dx;"
"stosw; movl %edx, %eax; 1: stosw; ret;");
ASM_FUNC("_UTF16_GET1","orl $-1, %edx; _UTF16_GET2: "
"movzwl %ax, %eax; cmpw $0xD800, %ax; jb 1f; cmpw $0xDC00, %ax;"
"jae 1f; cmpl %edx, %esi; je 1f; movzwl (%esi), %edx;"
"cmpw $0xDC00, %dx; jb 1f; cmpw $0xE000, %dx; jae 1f; inc %esi;"
"inc %esi; sal $10, %eax; lea 0xFCA02400(%eax,%edx), %eax; 1: ret");
#define UTF816SZ(z,n,x) int __stdcall MCAT(utf816_size,x)(cch* str MIF(n, \
	(,int len),)) { int r = 0; asm("1: inc %0;" MIF(n, "cmp %2,%1; " \
	"jz 3f;",) "movzbl (%1),%%eax; inc %1; testb %%al,%%al;" MIF(z, \
	"je 3f;",) "jns 1b;" MIF(n, "movl %2,%%edx; call _UTF8_GET2", "call " \
	"_UTF8_GET1") ";shrl $16, %%eax; je 1b; inc %0; jmp 1b; 3:" : "+b"(r) \
	: "S"(str) MIF(n,(,"c"(str+len)),) : "eax", "edx" ); return r*2; }
UTF816SZ(1,0,);UTF816SZ(1,1,);
ASM_FUNC("__Z10utf816_cpyPwPKc@8","pushl %esi; pushl %edi; movl 16(%esp), %esi;"
"movl 12(%esp), %edi; 1: movzbl (%esi), %eax; inc %esi; testb %al, %al; js 2f;"
"stosw; jne 1b; lea -2(%edi), %eax; popl %edi; popl %esi; ret $8;"
"2: call _UTF8_GET1; call _UTF16_PUT1; jmp 1b");
#define UTF816CP(n, z) WCHAR* __stdcall n(WCHAR* dst, cch* str, int len) \
	{ asm("jmp 0f; 1: stosw; 0: cmp %2,%1; jz 3f; movzbl (%1),%%eax;" \
	"inc %1; testb %%al,%%al;" MIF(z,"je 3f;",) "jns 1b; movl " \
	"%2,%%edx; call _UTF8_GET2; call _UTF16_PUT1; jmp 0b; 3:" : "+D"(dst) : \
	"S"(str) ,"c"(str+len) : "eax", "edx" ); *dst = '\0'; return dst; }
UTF816CP(utf816_cpy,1)
ASM_FUNC("_UTF16TO8_LEN1","orl $-1, %edx; _UTF16TO8_LEN2:"
"inc %ebx; cmp $0x800, %ax; jae 1f; 2: ret; 1: inc %ebx;"
"andw $0xFC00, %ax; cmpw $0xD800, %ax; jne 2b; cmpl %edx, %esi;"
"jae 2b; movw (%esi), %ax; andw $0xFC00, %ax; cmpw $0xDC00, %ax;"
"jne 2b; inc %ebx; inc %esi; inc %esi; ret");
ASM_FUNC("__Z11utf816_sizePKw@4","pushl %esi; pushl %ebx; movl 12(%esp), %esi;"
"xorl %ebx, %ebx; 1: lodsw; inc %ebx; cmpw $128, %ax; jae 2f; testb %al, %al;"
"jne 1b; movl %ebx, %eax; popl %ebx; popl %esi; ret $4;"
"2: call _UTF16TO8_LEN1; jmp 1b");
ASM_FUNC("__Z11utf816_sizePKwi@8","pushl %ebx; pushl %ebp; xorl %ebx, %ebx; pushl %esi;"
"movl 20(%esp), %ebp; movl 16(%esp), %esi; addl %esi, %ebp; 1: inc %ebx;cmpl %ebp, %esi;"
"jae 3f; lodsw; cmpw $128, %ax; jb 1b;movl %ebp, %edx; call _UTF16TO8_LEN2; jmp 1b;"
"3: popl %esi; popl %ebp; movl %ebx, %eax; popl %ebx; ret $8;");
ASM_FUNC("__Z10utf816_cpyPcPKw@8","pushl %esi; pushl %edi; movl 16(%esp), %esi;"
"movl 12(%esp), %edi; 1: lodsw; cmpw $128, %ax; jae 2f; stosb;"
"testb %al, %al; jne 1b; lea -1(%edi), %eax; popl %edi; popl %esi;"
"ret $8; 2: call _UTF16_GET1; call _UTF8_PUT2; jmp 1b");
ASM_FUNC("__Z10utf816_cpyPcPKwi@12","pushl %edi; pushl %ebp; pushl %esi; movl 24(%esp), %ebp;"
"movl 20(%esp), %esi; movl 16(%esp), %edi; lea (%esi,%ebp,2), %ebp; jmp 1f;"
"0: stosb; 1: cmpl %ebp, %esi; jae 3f; lodsw; cmpw $128, %ax; jb 0b;"
"movl %ebp, %edx; call _UTF16_GET2; call _UTF8_PUT2; jmp 1b;"
"3: popl %esi; popl %ebp; lea -1(%edi), %eax; popl %edi; ret $12;");
#define UTF816_DUP(t1, t2, t3) \
t1 __stdcall utf816_dup(const t3* src) { if(!src) return {0,0};	\
	t2* buff = (t2*) xmalloc(utf816_size(src)); ARGFIX(src); t2* end \
	= utf816_cpy( buff, src); return t1{buff, end-buff}; } \
t1 __stdcall utf816_dup(const t3* src, int len) { if(!len) return {0,0}; \
	t2* buff = (t2*) xmalloc(utf816_size(src, len)); ARGFIX(src); ARGFIX(len); \
	t2* end = utf816_cpy(buff, src, len); return t1{buff, end-buff}; }
UTF816_DUP(cstrW,wchar_t,char);UTF816_DUP(cstr,char,wchar_t);
asm(".section .text$cmd_esc_len;"
"_cmd_esc_msk: .long 0xFFFFFFFF,"
"0x58001367, 0x78000000, 0x10000000,"
"0xFFFFFFFF, 5, 0, 0;"
"cmd_esc_len1: incl %eax; cmd_esc_len2:"
"addl %edx, %eax; test %ebx,%ebx; jg 5f;"
"je 6f; incl %eax; 5: incl %eax; 6: ret;");
asm(".section .text$cmd_esc_lenA; _cmd_esc_lenA:"
".long cmd_esc_len2, cmd_esc_len1, cmd_esc_getA;"
"cmd_esc_getA: movzbl (%edi), %ebx; incl %edi; ret;");
asm(".section .text$cmd_esc_lenW; _cmd_esc_lenW:"
".long cmd_esc_len2, cmd_esc_len1, cmd_esc_getW;"
"cmd_esc_getW: movzwl (%edi), %ebx; add $2, %edi; ret;");
asm(".section .text$cmd_esc_cpyA;"
"_cmd_esc_cpyA: .long 3f, 2f, cmd_esc_getA;"
"2: movb $34, (%eax); incl %eax;"
"3: decl %esi; js 4f; movb $92, (%eax);"
"inc %eax; jmp 3b; 4: test %ebx,%ebx; jg 5f;"
"je 6f; movb $94, (%eax); incl %eax; 5:"
"movb %bl, (%eax); incl %eax; 6: ret;");
asm(".section .text$cmd_esc_cpyW;"
"_cmd_esc_cpyW: .long 3f, 2f, cmd_esc_getW;"
"2: movw $34, (%eax); addl $2, %eax;"
"3: decl %esi; js 4f; movw $92, (%eax);"
"addl $2, %eax; jmp 3b; 4: test %ebx,%ebx; jg 5f;"
"je 6f; movw $94, (%eax); addl $2, %eax; 5:"
"movw %bx, (%eax); addl $2, %eax; 6: ret;");
ASM_FUNC("__Z14cmd_escape_lenPKcji@12",
"movl $_cmd_esc_lenA, %edx; xor %eax, %eax; jmp _cmd_escape@20");
ASM_FUNC("__Z14cmd_escape_lenPKwji@12",
"movl $_cmd_esc_lenW, %edx; xor %eax, %eax; jmp _cmd_escape@20");
ASM_FUNC("__Z10cmd_escapePcPKcji@16",
"movl $_cmd_esc_cpyA, %edx; jmp _cmd_escape@20");
ASM_FUNC("__Z10cmd_escapePwPKwji@16",
"movl $_cmd_esc_cpyW, %edx; jmp _cmd_escape@20");
extern"C"
size_t REGCALL(2)cmd_escape(size_t dstLen,void*vtable,
const void*src,size_t len,byte flags){
len+=(size_t)src;
asm("movl %%ecx, %%ebp; andl $2, %%ebp; loop: orl $-1, %%esi;"
"1: incl %%esi; cmpl %3, %%edi; je end; call *8(%%edx);"
"cmpl $92, %%ebx; je 1b; cmpl $127, %%ebx; ja norm;"
"bt %%ebx, _cmd_esc_msk(,%%ebp,8); jnc norm;"
"testl %%ebx, %%ebx; je end; cmpl $34, %%ebx; je quot;"
"cmpl $37, %%ebx; je pcnt; testb %%cl, %%cl; jns chg_quot;"
"norm: call *(%%edx); jmp loop; quot: leal 1(%%esi,%%esi),"
"%%esi; testl %%ebp, %%ebp; jne norm; jmp esc_char;"
"pcnt: testb $32, %%cl; jne norm; esc_char:"
"bts $31, %%ebx; testb %%cl, %%cl; jns norm;"
"chg_quot: xorb $128, %%cl; call *4(%%edx); jmp loop;"
"end: xorl %%ebx, %%ebx; testb %%cl,%%cl; js in_quot;"
"testb $5, %%cl; je done; testb $1, %%cl; jne en_quot;"
"testl %%esi, %%esi; je done; en_quot: addl $4, %%edx;"
"in_quot: testb $1, %%cl; jne done;"
"add %%esi, %%esi; movb $34, %%bl; done: call *(%%edx);"
:"+a"(dstLen):"c"(flags),"D"(src),"m"(len),"d"(vtable):
"ebx","esi","ebp","memory");return dstLen;}
int sysfmt(const char*fmt,...){
VA_ARG_FWD(fmt);char*str=xstrfmt(va);
SCOPE_EXIT(free(str));return system(str);}
extern"C" {
FILE*freopen(cch*name,cch*mode,FILE*fp){
WCHAR wmode[32];utf816_cpy(wmode,mode);
FNWIDEN(1,name);return _wfreopen(cs1,wmode,fp);}
FILE*fopen(cch*name,cch*mode){
WCHAR wmode[32];utf816_cpy(wmode,mode);
FNWIDEN(1,name);return _wfopen(cs1,wmode);}
int rename(cch*old_name,cch*new_name){FNWIDEN(1,old_name);
FNWIDEN(2,new_name);return _wrename(cs1,cs2);}
#define _FWNDFN1(n,wn) int n(cch *s) { FNWIDEN(1,s); return wn(cs1); }
_FWNDFN1(remove,_wremove);_FWNDFN1(system,_wsystem);
_FWNDFN1(_mkdir,_wmkdir);_FWNDFN1(_rmdir,_wrmdir);}
BOOL WINAPI setWindowText(HWND h,cch*s){
return SetWindowTextW(h,widen(s));}
BOOL WINAPI setDlgItemText(HWND h,int i,cch*s){
return SetDlgItemTextW(h,i,widen(s));}
#define W32SARD_(l,g) WCHAR* ws; { int sz = l+1; ws = xMalloc \
	(sz); g; } SCOPE_EXIT(free(ws)); return utf816_dup(ws);
cstr WINAPI getWindowText(HWND h){W32SARD_(
GetWindowTextLengthW(h),GetWindowTextW(h,ws,sz))}
cstr WINAPI getDlgItemText(HWND h,int i){
return getWindowText(GetDlgItem(h,i));}
HMODULE getModuleBase(void*ptr){
MEMORY_BASIC_INFORMATION bmi;
VirtualQuery(ptr,&bmi,sizeof(bmi));
return(HMODULE)bmi.AllocationBase;}
extern"C" void __wgetmainargs(int*argc,
wchar_t***argv_,wchar_t***envp,int mode,void*si);
extern"C" void __getmainargs(int*argc,
char***argv_,char***envp,int mode,void*si){
wchar_t**wargv;__wgetmainargs(argc,
&wargv,(wchar_t***)envp,mode,si);
int size=0;for(wchar_t**wargv_=wargv;*wargv_;
wargv_++)size+=utf816_size(*wargv_);
char**argv=*argv_=xMalloc((*argc)+1);
char*curPos=xmalloc(size);
for(wchar_t**wargv_=wargv;*wargv_;
wargv_++,argv++){*argv=curPos;curPos=
utf816_cpy(curPos,*wargv_)+1;}*argv=0;}
ASM_FUNC("getFullPath_","push %ebx; movl 12(%esp), %ebx; pushl 16(%esp);"
"push %ebx; call __Z10utf816_dupPKci@8; shrb 20(%esp); movl %eax, %ecx;"
"jnc 1f; push %ebx; call _sfree; 1: cmpl $0x5C005C, (%eax); jnz 1f; cmpl "
"$0x2F003F, 4(%eax); jz 0f; 1: push %eax; add $260, %edx; pushl $0;"
"lea 12(%edx,%edx), %eax; push %eax; call _xmalloc; lea 12(%eax), %ebx;"
"push %ebx; push %edx; push %ecx; call _GetFullPathNameW@16; call _sfree;"
"movl %eax, %edx; movl %ebx, %eax; lea -12(%ebx), %ecx; 0: pop %ebx; ret;");
ASM_FUNC("__Z11getFullPathPKcii@12","call getFullPath_; push %ecx;"
"push %eax; call __Z10utf816_dupPKw@4; call _sfree; ret $12");
ASM_FUNC("__Z14getNtPathName_PKcii@12","call getFullPath_; cmp %eax, %ecx; jz 1f;"
"cmpl $0x005C005C, (%eax); jnz 2f; cmpl $0x005C002E, 4(%eax); jz 3f;"
"sub $4, %eax; movl $0x004E0055, (%eax); movb $0x43, 4(%eax);"
"2: sub $8, %eax; movl $0x005C005C, (%eax); 3: movl $0x005C003F,"
"4(%eax); 1: shrb 12(%esp); jnc 1f; movb $63, 2(%eax); 1: ret $12;");
ASM_FUNC("__Z14getNtPathName_PKc@4","push $0; push $-1; push 12(%esp);"
"call __Z14getNtPathName_PKcii@12; ret $4;");
ASM_FUNC("_isRelPath0","test %eax, %eax; jz 1f;"
"cmpb $0, (%eax); jz 1f; jmp 4f;"
GLOB_LAB("_isRelPath")"cmp $1, %edx; jbe 2f; 4:cmpb $58, 1(%eax); jnz 3f;"
"0: movb $0, %cl; ret; 2: test %edx, %edx; jz 1f; 3: cmpb $92, (%eax);"
"jz 0b; cmpb $47, (%eax); jz 0b; 1: movb $1, %cl; ret;");
ASM_FUNC("__Z7getPath4cstr@8","mov %edx, %ecx; 0: test %edx, %edx; "
"jz 1f; cmpb $92, -1(%eax,%edx); jz 1f; cmpb $47, -1(%eax,%edx);"
"jz 1f; dec %edx; jmp 0b; 1: test %edx, %edx; jz 1f; 0: ret; 1:"
"cmp $2, %ecx; jb 0b; cmpb $58, 1(%eax); jnz 0b; add $2, %edx; ret;"
GLOB_LAB("__Z7getName4cstr@8")"call __Z7getPath4cstr@8;"
"subl %edx, %ecx; add %edx, %eax; mov %ecx, %edx; ret;");
cstr getModuleFileName(HMODULE hModule){
WCHAR buff[MAX_PATH];
if(!GetModuleFileNameW(hModule,buff,MAX_PATH))
return{0,0};return utf816_dup(buff);}
cstr getProgramDir(void){
return getPath0(getModuleFileName(NULL));}
HANDLE WINAPI createFile(LPCSTR a,DWORD b,DWORD c,
LPSECURITY_ATTRIBUTES d,DWORD e,DWORD f,HANDLE g)
{return CreateFileW(widen(a),b,c,d,e,f,g);}
#define DEF_RDTEXT(name, text) \
	__attribute__((section(".text$"#name))) \
	static const char name[] = text;
DEF_RDTEXT(str_pcs_pcs,"%s: %s");
DEF_RDTEXT(str_fatal_error,"Fatal Error");
DEF_RDTEXT(str_error,"Error");
DEF_RDTEXT(str_out_of_mem,"Out of memory/resources");
DEF_RDTEXT(str_max_path,"MAX_PATH exceeded");
DEF_RDTEXT(str_out_of_space,"Out of disk space");
DEF_RDTEXT(str_io_fail,"file IO failure");
DEF_RDTEXT(str_bad_file,"invalid file format");
DEF_RDTEXT(str_open_fileA,"Cannot open file: \"%s\"");
DEF_RDTEXT(str_open_fileW,"Cannot open file: \"%S\"");
DEF_RDTEXT(str_rbA,"rb");
#define DEF_RDTEXTW(name, text) \
	__attribute__((section(".text$"#name))) \
	static const WCHAR name[] = text;
DEF_RDTEXTW(str_rbW,L"rb");
namespace std{
DEF_RDTEXT(length_error,"length_error: %s");
__attribute__((section(".text$nothrow_t")))
const nothrow_t nothrow=nothrow_t();
void __attribute__((__noreturn__))
__throw_length_error(const char*str){fatalError(length_error,str);}}
extern"C" void*emulate_nt_new(unsigned len,const std::nothrow_t&junk){
return malloc(len);}
extern"C" void*emulate_cc_new(unsigned len){return xmalloc(len);}
extern"C" void*emulate_delete(void*p){free(p);}
void*operator new(std::size_t,const std::nothrow_t&)__attribute__((alias("emulate_nt_new")));
void*operator new[](std::size_t,const std::nothrow_t&)__attribute__((alias("emulate_nt_new")));
void*operator new (unsigned len)__attribute__((alias("emulate_cc_new")));
void*operator new[](unsigned len)__attribute__((alias("emulate_cc_new")));
void  operator delete (void*p)__attribute__((alias("emulate_delete")));
void  operator delete[](void*p)__attribute__((alias("emulate_delete")));
ASM_FUNC("___cxa_pure_virtual","int $3");
void comnError(HWND hwnd,bool fatal,
const char*fmt,va_list args){
char caption[64];char text[2048];
sprintf(caption,str_pcs_pcs,progName,
fatal?str_fatal_error:str_error);
vsprintf(text,fmt,args);
MessageBoxA(hwnd,text,caption,MB_OK);
if(fatal)ExitProcess(1);}
#define ERRORM(hwnd, fatal, x) {				\
	va_list args; va_start (args, fmt);		\
	comnError(hwnd, fatal, fmt, args);		\
	va_end (args); x; }
void fatalError(const char*fmt,...)ERRORM(
GetLastActivePopup(GetActiveWindow()),true,UNREACH);
void fatalError(HWND hwnd,const char*fmt,...)ERRORM(hwnd,true,UNREACH)
void contError(HWND hwnd,const char*fmt,...)ERRORM(hwnd,false,)
void errorAlloc(){fatalError(str_out_of_mem);}
void errorMaxPath(){fatalError(str_max_path);}
void errorDiskSpace(){fatalError(str_out_of_space);}
void errorDiskFail(){fatalError(str_io_fail);}
void errorBadFile(){fatalError(str_bad_file);}
void errorDiskWrite(){(GetLastError()==ERROR_DISK_FULL)
?errorDiskSpace():errorDiskFail();}
SHITCALL
int fopen_ErrChk(void){switch(errno){case ENOENT:
case EACCES:case EISDIR:case EINVAL:return 1;
case ENOSPC:return-1;default:return 0;}}
SHITCALL void xfclose(FILE*fp){
if(fclose(fp))errorDiskFail();}
SHITCALL void xfclose_ref(FILE*&fp){
if(fclose_ref(fp))errorDiskFail();}
SHITCALL
void xfread(void*ptr,size_t size,size_t num,FILE*fp){
size_t result=fread(ptr,size,num,fp);
if(result!=num){
if(ferror(fp))
errorDiskFail();
else
errorBadFile();}}
SHITCALL
void xfwrite(const void*ptr,size_t size,size_t num,FILE*fp){
size_t result=fwrite(ptr,size,num,fp);
if(result!=num)
errorDiskWrite();}
SHITCALL
void xchsize(FILE*fp,long size){
int fd=fileno(fp);
if(_chsize(fd,size))
errorDiskWrite();}
SHITCALL
int fsize(FILE*fp){
int curPos=ftell(fp);
fseek(fp,0,SEEK_END);
int endPos=ftell(fp);
fseek(fp,curPos,SEEK_SET);
return endPos-curPos;}
loadFile_t loadFile(FILE*fp,int extra){
if(!fp)return loadFile_t(0,-1);
SCOPE_EXIT(fclose(fp));
loadFile_t result;result.size=fsize(fp);
if(result.data=malloc(result.size+extra)){
memset(result.data+result.size,0,extra);
xfread(result.data,result.size,fp);
}else{min_ref(result.size,0x7FFFFFFF);}
return result;}
char**loadText(FILE*fp,int&LineCount){
auto file=loadFile(fp,1);
if(fp==NULL){LineCount=-1;
return NULL;}LineCount=0;
Void curPos=file.data;
Void endPos=curPos+file.size;
char**lineData=NULL;
while(curPos<endPos){
xNextAlloc(lineData,LineCount)=curPos;
Void nextPos=strchr(curPos,'\n');
if(nextPos!=NULL)nextPos[0]='\0';
else nextPos=strchr(curPos,'\0');
removeCrap((char*)curPos);
curPos=nextPos+1;}
return lineData;}
Void memmem(const void*b1,const void*b2,
size_t len1,size_t len2){
char*sp=(char*)b1;
char*pp=(char*)b2;
char*eos  =sp+len1-len2;
if(!(b1&&b2&&len1&&len2))
return NULL;
while(sp<=eos){
if(*sp==*pp)
if(memcmp(sp,pp,len2)==0)
return sp;
sp++;}
return NULL;}
int xvfprintf(FILE*stream,const char*format,va_list arg){
int result=vfprintf(stream,format,arg);
if(result<0)errorDiskWrite();return result;}
int xfprintf(FILE*stream,const char*format,...){
va_list vl;va_start(vl,format);
return xvfprintf(stream,format,vl);va_end(vl);}
void xfputs(const char*str,FILE*stream){
if(fputs(str,stream)<0)errorDiskWrite();}
#endif
cstr cstr_len(const char*str){
int len=0;if(str!=NULL){
asm("push %2; call _strlen;"
"movl %%eax, %%edx; pop %%eax":"=a"(str),
"=d"(len):"g"(str):"ecx");}return{str,len};}
cstr cstr_dup(cstr str){
char*buff=xmalloc(str.slen+1);
buff[str.slen]='\0';return{(char*)memcpy(
buff,str.data,str.slen),str.slen};}
#define BSTR_ALLOC() auto* This = this; if(mlen<= len) {\
	GET_RETPAIR(This, len, alloc(len)); } char* data = \
	This->data; int slen = This->slen; data[len] = 0;
bstr::bstr(const cstr&that):bstr(ZT()){*this=that;}
bstr::alloc_t bstr::alloc(int len){
if(mlen==0)data=NULL;
mlen=ALIGN(max(mlen+(mlen>>1),
len+1),(8/sizeof(char))-1);
xRealloc(data,mlen);
return{this,len};}
char*bstr::xresize(int len){BSTR_ALLOC();
This->slen=len;return data;}
char*bstr::xnalloc(int len){len+=slen;BSTR_ALLOC();
This->slen=len;return data+slen;}
char*bstr::xreserve(int len){
BSTR_ALLOC();return data;}
char*bstr::xralloc(int len){len+=slen;
BSTR_ALLOC();return data+slen;}
bstr&bstr::strcpy(const char*str){::strcpy(xresize(
strlen(str)),str);return*this;}
bstr&bstr::strcpy(cstr str){memcpyX(xresize(
str.slen),str.data,str.slen);return*this;}
bstr&bstr::strcat(const char*str){::strcpy(xnalloc(
strlen(str)),str);return*this;}
bstr&bstr::strcat(cstr str){memcpyX(xnalloc(
str.slen),str.data,str.slen);return*this;}
bstr&bstr::fmtcpy(const char*fmt,...){
VA_ARG_FWD(fmt);setend(xstrfmt_fill(xreserve(
xstrfmt_len(va)-1),va));return*this;}
bstr&bstr::fmtcat(const char*fmt,...){
VA_ARG_FWD(fmt);setend(xstrfmt_fill(xralloc(
xstrfmt_len(va)-1),va));return*this;}
cstr bstr::nullTerm(void){int len=slen;
BSTR_ALLOC();return{This->data,len};}
cstr bstr::calcLen(void){cstr ret=cstr_len(data);
return{ret.data,slen=ret.slen};}
cstr bstr::updateLen(void){cstr ret=cstr_len(data);
return{ret.data,slen+=ret.slen};}
struct xstrfmt_fmt_:xstrfmt_fmt{
char getFillCh(){
char fillCh=' ';
if(flags&PADD_ZEROS)
movb2(fillCh,'0');
return fillCh;}
size_t ext_mode(){size_t(__thiscall*funcPtr)
(void*ctx)=va_arg(ap,Void);return funcPtr(this);}
size_t str_mode(void);
size_t dec_mode(bool sign);
size_t hex_mode();size_t cmd_mode();
size_t sep_mode();
size_t flt_mode(char*fmt);
static REGCALL(1)void slash(void);
DEF_RETPAIR(core_t,size_t,extraLen,char*,str);
core_t core(char*str);};
#define APPEND_SLASH asm("call xstrfmt_slash" : "+D"(dstPos))
asm(".section .text$_ZN12xstrfmt_fmt_8str_modeEv,\"x\";"
"xstrfmt_slash: movb -1(%edi), %al;"
"cmp $92, %al; jz 1f; cmp $47, %al; jz 1f;"
"cmp $58, %al; jz 1f; movb $92, %al; stosb; 1: ret");
size_t xstrfmt_fmt_::str_mode(void){
char*dstPos=dstPosArg;
char*str=(char*)va_arg(ap,char*);
char*str0=str;
movfx(S,str);movfx(b,flags);
if(!str)str=(char*)"";
size_t strLen;
if(flags&FLAG_DOLAR){
strLen=va_arg(ap,size_t);
}else{strLen=precision;
if(!dstPos||width)strLen
=strnlen(str,strLen);}
if((flags&FLAG_XCLMTN)&&!(strLen&&*str))
{strLen=1;str=(char*)".";}
if(!dstPos)return max(strLen,width)
+!!(flags&FLAG_SLASH)
+!!(flags&FLAG_COMMA);
if(flags&FLAG_COMMA)APPEND_SLASH;
int len=width-strLen;
if(len>0){
char fillCh=getFillCh();
do{stosx(dstPos,fillCh);
}while(--len>0);}
VARFIX(flags);if(flags&FLAG_DOLAR)
memcpy_ref(dstPos,str,strLen);
else{
char*endPos=str+strLen;asm goto(
"jmp %l1"::"r"(endPos)::LOOP_START);
do{{char ch;lodsx(str,ch);
if(ch=='\0')break;
stosx(dstPos,ch);}
LOOP_START:;
}while(str!=endPos);}
if(flags&FLAG_SLASH)APPEND_SLASH;
if(flags&FLAG_HASH)free(str0);
return(size_t)dstPos;}
size_t xstrfmt_fmt_::flt_mode(char*fmt){
char*dstPos=dstPosArg;
SCOPE_EXIT(va_arg(ap,double));
if(!dstPos){
int exp=(((int*)ap)[1]>>20)&2047;
int len=max(3,(exp*19728-19965287)>>16);
len+=max(s8(precision),6);
max_ref(len,length);return len;}
va_list va=ap;char buff[32];
char*dp=buff+32;*dp='\0';
while((*--dp=*--fmt)!='%'){if(*fmt
=='*')va-=sizeof(size_t);}
return size_t(dstPos+
vsprintf(dstPos,dp,va));}
size_t xstrfmt_fmt_::dec_mode(bool sign){
char*dstPos=dstPosArg;
size_t data=va_arg(ap,size_t);
char signCh=0;
if(sign!=0){if(int(data)<0){
signCh='-';data=-data;}
ei(flags&SPACE_POSI)signCh=' ';
ei(flags&FORCE_SIGN)signCh='+';
}int strLen=1;
for(;strLen<ARRAYSIZE(powersOf10);strLen++){
if(powersOf10[strLen]>data)break;}
strLen=max(strLen+bool(signCh),width);
if(dstPos==NULL)return strLen;
char*endPos=dstPos+strLen;
char*curPos=endPos;
do{*(--curPos)='0'+data%10;
}while(data/=10);
if(curPos>dstPos){
char fillCh;
if(flags&PADD_ZEROS){
if(signCh)stosx(dstPos,signCh);
fillCh='0';
}else{
if(signCh)*(--curPos)=signCh;
fillCh=' ';}
while(curPos-->dstPos)
*curPos=fillCh;
}return(size_t)endPos;}
size_t xstrfmt_fmt_::hex_mode(void){
INT64 data=(length>=2)?
va_arg(ap,INT64):va_arg(ap,size_t);
int maxBit=(DWORD(data>>32))?
32+(__builtin_clz(data>>32)^31):(__builtin_clz(data|1)^31);
size_t strLen=max((maxBit+4)/4,width);
if(dstPosArg==NULL)return strLen;
char*endPos=dstPosArg+strLen;
char*curPos=endPos;
const byte*hexTab=(flags&UPPER_CASE)?
tableOfHex[1]:tableOfHex[0];
do{*(--curPos)=hexTab[data&15];
}while(data>>=4);
char fillCh=getFillCh();
while(curPos-->dstPosArg)
*curPos=fillCh;
return(size_t)endPos;}
size_t xstrfmt_fmt_::cmd_mode(void){
char cmdFlag=0;
asm("sar $3, %k1; rolb $2, %b0;"
:"=Q"(cmdFlag):"0"(flags));
char*src=va_arg(ap,char*);
size_t maxLen=(cmdFlag&ESC_FIXED)?
va_arg(ap,size_t):precision;
if(dstPosArg==NULL)return cmd_escape_len(src,maxLen,cmdFlag);
else return(size_t)cmd_escape(dstPosArg,src,maxLen,cmdFlag);}
size_t xstrfmt_fmt_::sep_mode(void){
if(dstPosArg==NULL)return 1;
else{WRI(dstPosArg,'\\');
return(size_t)dstPosArg;}}
xstrfmt_fmt_::core_t xstrfmt_fmt_::core(char*str){
flags=0;char ch;char ch2;
while(lodsx(str,ch),ch2=ch-' ',
(ch2<17)&&(ch!='*')&&(ch!='.'))
flags|=1<<ch2;
int*dst=&width;
GET_INT_NEXT:{int result;
if(ch=='*'){lodsx(str,ch);
result=va_arg(ap,int);}
else{result=0;byte tmp;
while((tmp=ch-'0')<10){result*=10;
result+=tmp;lodsx(str,ch);}
}*dst=movfx(D,result);
if(dst==&width){
dst=&precision;*dst=0x7FFFFFFF;
if(ch=='.'){lodsx(str,ch);
goto GET_INT_NEXT;}}}
length=0;LENGTH_NEXT:
if(ch=='h'){length--;lodsx(str,ch);goto LENGTH_NEXT;}
if(ch=='l'){length++;lodsx(str,ch);goto LENGTH_NEXT;}
if((ch>='A')&&(ch<='Z')){asm("add $32, %0":
"=r"(ch):"r"(ch));flags|=UPPER_CASE;}
size_t extraLen;
switch(ch){
if(0){case'j':flags|=FLAG_XCLMTN;}
if(0){case'k':flags|=FLAG_COMMA;}
if(0){case'v':flags|=FLAG_DOLAR;}
case's':extraLen=str_mode();break;
case'x':extraLen=hex_mode();break;
case'd':extraLen=dec_mode(false);break;
case'u':extraLen=dec_mode(true);break;
case'q':extraLen=ext_mode();break;
case'z':extraLen=cmd_mode();break;
case':':extraLen=sep_mode();break;
case'f':extraLen=flt_mode(str);break;
default:UNREACH;}
return core_t(extraLen,str);}
SHITCALL
int xstrfmt_len(VaArgFwd<const char*>va){
xstrfmt_fmt_ ctx;ctx.ap=va.start();
ctx.dstPosArg=0;
int extraLen=1;char ch;
DEF_ESI(char*curPos)=(char*)*va.pfmt;
while(lodsx(curPos,ch),ch){
if(ch!='%'){
ESCAPE_PERCENT:extraLen++;}
ei(*curPos=='%'){curPos++;
goto ESCAPE_PERCENT;}
else{
auto result=ctx.core(curPos);
curPos=result.str;
extraLen+=result.extraLen;}
}return extraLen;}
SHITCALL
char*xstrfmt_fill(char*buffer,
VaArgFwd<const char*>va){
xstrfmt_fmt_ ctx;ctx.ap=va.start();
DEF_ESI(char*curPos)=(char*)*va.pfmt;
DEF_EDI(char*dstPos)=buffer;
while(1){
char ch;lodsx(curPos,ch);
if(ch!='%'){ESCAPE_PERCENT:
stosx(dstPos,ch);
if(ch=='\0')return dstPos-1;}
ei(*curPos=='%'){curPos++;
goto ESCAPE_PERCENT;}
else{
ctx.dstPosArg=dstPos;
auto result=ctx.core(curPos);
curPos=result.str;
dstPos=(char*)result.extraLen;}}}
SHITCALL
cstr xstrfmt(VaArgFwd<const char*>va){
va_list ap=va.start();
char*buffer=xMalloc(xstrfmt_len(va));
char*endPos=xstrfmt_fill(buffer,va);
return{buffer,endPos};}
SHITCALL NEVER_INLINE
cstr xstrfmt(const char*fmt,...){
VA_ARG_FWD(fmt);return xstrfmt(va);}
SHITCALL NEVER_INLINE int strfmt(
char*buffer,const char*fmt,...){
VA_ARG_FWD(fmt);
char*endPos=xstrfmt_fill(buffer,va);
return endPos-buffer;}
void xvector_::fmtcat(const char*fmt,...){
VA_ARG_FWD(fmt);
int strLen=xstrfmt_len(va)*sizeof(char);
char*buffer=xnxalloc_(strLen);
dataSize-=sizeof(char);
xstrfmt_fill(buffer,va);}
RetEdx<int>xvector_::strcat2(const char*str){
return this->write(str,strlen(str)+1);}
int xvector_::strcat(const char*str){
int strLen=strlen(str);
this->write(str,strLen+1);
dataSize-=sizeof(char);
return strLen;}
SHITCALL char*strstr(const char*str1,const char*str2,int maxLen){
int cmpLen=strlen(str2)-1;
if(cmpLen<0)return NULL;
const char*endPos=(maxLen<0)?(char*)size_t(-1)
:str1+(maxLen-cmpLen);
char findCh=*str2++;char ch;
while((str1<endPos)&&((lodsx(str1,ch),ch)))
if((ch==findCh)&&(!strncmp(str1,str2,cmpLen)))
return(char*)str1-1;
return NULL;}
SHITCALL char*strstri(const char*str1,const char*str2,int maxLen){
int cmpLen=strlen(str2)-1;
if(cmpLen<0)return NULL;
const char*endPos=(maxLen<0)?(char*)size_t(-1)
:str1+(maxLen-cmpLen);
char findCh=toUpper(*str2++);char ch;
while((str1<endPos)&&((lodsx(str1,ch),ch=toUpper(ch))))
if((ch==findCh)&&(!strnicmp(str1,str2,cmpLen)))
return(char*)str1-1;
return NULL;}
#ifdef _OLDMINGW_
SHITCALL size_t strnlen(
const char*str,size_t maxLen){size_t len=0;
for(;(len<maxLen)&&str[len];len++);return len;}
#endif
SHITCALL cstr xstrdup(const char*str){
if(str==NULL)return{0,0};
int len=strlen(str);
char*ret=xMalloc(len+1);
return{strcpy(ret,str),len};}
SHITCALL cstr xstrdup(const char*str,size_t maxLen){
if(str==NULL)return NULL;
int strLen=strnlen(str,maxLen);
char*buffer=xMalloc(strLen+1);
return strcpyn(buffer,str,strLen);}
SHITCALL
FILE*xfopen(const char*fName,const char*mode){
bool chkOpen=false;
if(mode[0]=='!'){
chkOpen=true;
mode++;}
LRETRY:
FILE*fp=fopen(fName,mode);
if(fp==NULL){
int err=fopen_ErrChk();
if(err>0){if(chkOpen)fatalError(
str_open_fileA,fName);
}ei(err<0){
errorDiskSpace();goto LRETRY;
}else{errorAlloc();}}
return fp;}
SHITCALL
char*xfgets(char*str,int num,FILE*fp){
char*tmp=fgets(str,num,fp);
if((!tmp)&&(ferror(fp)))
errorDiskFail();
return tmp;}
SHITCALL
loadFile_t loadFile(const char*fileName,int extra){
return loadFile(xfopen(fileName,str_rbA),extra);}
SHITCALL
char**loadText(const char*fileName,int&LineCount){
return loadText(xfopen(fileName,str_rbA),LineCount);}
SHITCALL
char*xstrdupr(char*&str1,const char*str2){
return free_repl(str1,xstrdup(str2));}
SHITCALL
char*xstrdupr(char*&str1,const char*str2,size_t sz){
return free_repl(str1,xstrdup(str2,sz));}
SHITCALL
int strcmp2(const char*str1,const char*str2){
for(const char*curPos=str2;;curPos++){
char ch1;lodsx(str1,ch1);
char ch2=*curPos;
if(ch1!=ch2)return curPos-str2;
if(ch2==0)return-1;}}
SHITCALL
int stricmp2(const char*str1,const char*str2){
for(const char*curPos=str2;;curPos++){
char ch1;lodsx(str1,ch1);ch1=toUpper(ch1);
char ch2=toUpper(*curPos);
if(ch1!=ch2)return curPos-str2;
if(ch2==0)return-1;}}
SHITCALL
char*strScmp(const char*str1,const char*str2){
while(1){
char ch2;lodsx(str2,ch2);
if(ch2==0)
return(char*)str1;
if(ch2!=*str1++)
return NULL;}}
SHITCALL
char*strSicmp(const char*str1,const char*str2){
while(1){char ch2;
lodsx(str2,ch2);ch2=toUpper(ch2);
if(ch2==0)
return(char*)str1;
if(ch2!=toUpper(*str1++))
return NULL;}}
SHITCALL
int strEcmp(const char*str1,const char*str2){
int diff=strlen(str1)-strlen(str2);
if(diff<0)
return 1;
return strcmp(str1+diff,str2);}
SHITCALL
int strEicmp(const char*str1,const char*str2){
int diff=strlen(str1)-strlen(str2);
if(diff<0)
return 0;
return stricmp(str1+diff,str2);}
SHITCALL
int strNcpy(char*dst,const char*src,int num){
for(int i=0;i<num;i++)
if(!(dst[i]=src[i]))
return i;
if(num>=0)
dst[num]='\0';
return num;}
SHITCALL cstr strcpyn(
char*dst,const char*src,int len){
memcpyX(dst,src,len);
dst[len]='\0';return{dst,len};}
SHITCALL
bool strcmpn(const char*str1,const char*str2,int len){
if(strlen(str1)!=len)
return false;
return!strncmp(str1,str2,len);}
SHITCALL
bool stricmpn(const char*str1,const char*str2,int len){
if(strlen(str1)!=len)
return false;
return!strnicmp(str1,str2,len);}
SHITCALL
int removeCrap(char*str){
int len=strlen(str);
while(len--)
if(unsigned(str[len])>' ')
break;
str[len+1]='\0';
return len+1;}
SHITCALL
int strmove(char*dst,const char*src){
int len=strlen(src)+1;
memmove(dst,src,len*sizeof(char));
return len;}
SHITCALL cstr pathCat(cstr name1,cstr name2){
if(!isRelPath(name2))return name2.xdup();
return xstrfmt("%$j%$k",name1,name2);}
SHITCALL cstr replName(cstr name1,cstr name2){
return pathCat(getPath(name1),name2);}
SHITCALL cstr fullNameRepl(cstr base,cstr name){
return getFullPath(replName(base,name),true);}
SHITCALL cstr fullNameCat(cstr base,cstr name){
return getFullPath(pathCat(base,name),true);}
CSTRTH1_(getPath)CSTRTH1_(getName)
CSTRTH2_(pathCat)CSTRTH2_(replName)
CSTRTH2_(fullNameRepl)
CSTRTH2_(fullNameCat)
SHITCALL
bool isFullPath(const char*path){
if(path
&&((isPathSep(path[0]))
||(path[1]==':')))
return true;
return false;}
int _vsnprintf_s(char*buffer,size_t sizeOfBuffer,
const char*format,va_list ap){
if(sizeOfBuffer!=0){
size_t result=_vsnprintf
(buffer,sizeOfBuffer,format,ap);
if(result<sizeOfBuffer)
return result;
buffer[sizeOfBuffer-1]='\0';}
return-1;}
int sprintf_s(char*buffer,size_t sizeOfBuffer,
const char*format,...){
int count;va_list ap;
va_start(ap,format);
count=_vsnprintf_s(buffer,sizeOfBuffer,format,ap);
va_end(ap);return count;}