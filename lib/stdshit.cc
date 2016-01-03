// stdshit.cc: Single file version
// DeadFish Shitware 2013-2014
// BuildDate: 08/26/14 06:32:40

#ifndef _STDSHIT_CC_
#define _STDSHIT_CC_
#include "stdshit.h"
namespace std{
const nothrow_t nothrow=nothrow_t();
__attribute__((section(".text$length_error")))
const char length_error[]="length_error: %s";
void __attribute__((__noreturn__))
__throw_length_error(const char*str){
fatalError(length_error,str);}}
extern"C" void*emulate_nt_new(unsigned len,const std::nothrow_t&junk){
return malloc(len);}
extern"C" void*emulate_cc_new(unsigned len){
return xmalloc(len);}
extern"C" void*emulate_delete(void*p){
free(p);}
void*operator new(std::size_t,const std::nothrow_t&)__attribute__((alias("emulate_nt_new")));
void*operator new[](std::size_t,const std::nothrow_t&)__attribute__((alias("emulate_nt_new")));
void*operator new (unsigned len)__attribute__((alias("emulate_cc_new")));
void*operator new[](unsigned len)__attribute__((alias("emulate_cc_new")));
void  operator delete (void*p)__attribute__((alias("emulate_delete")));
void  operator delete[](void*p)__attribute__((alias("emulate_delete")));
void*__cxa_pure_virtual=0;
SHITCALL void free_ref(Void&ptr){
if(ptr!=NULL){
free(ptr);
ptr=NULL;}}
SHITCALL void fclose_ref(FILE*&stream){
if(stream!=NULL){
fclose(stream);
stream=NULL;}}
#undef fclose
SHITCALL int fclose_(FILE*stream){
if(stream==NULL)
return 0;
return fclose(stream);}
void comnError(HWND hwnd,bool fatal,
const char*fmt,va_list args){
char caption[64];
char text[2048];
sprintf(caption,"%s: %s",progName,
fatal?"Fatal Error":"Error");
vsprintf(text,fmt,args);
MessageBoxA(hwnd,text,caption,MB_OK);
if(fatal)ExitProcess(1);}
#define ERRORM(hwnd, fatal, x) {				\
	va_list args; va_start (args, fmt);		\
	comnError(hwnd, fatal, fmt, args);		\
	va_end (args); x; }
HWND errWindow=NULL;
void fatalError(const char*fmt,...)ERRORM(errWindow,true,UNREACH)
void fatalError(HWND hwnd,const char*fmt,...)ERRORM(hwnd,true,UNREACH)
void contError(HWND hwnd,const char*fmt,...)ERRORM(hwnd,false,)
void errorAlloc(){fatalError("Out of memory/resources");}
void errorMaxPath(){fatalError("MAX_PATH exceeded");}
void errorDiskSpace(){fatalError("Out of disk space");}
void errorDiskFail(){fatalError("file IO failure");}
void errorBadFile(){fatalError("invalid file format");}
void errorDiskWrite(){(errno==ENOSPC)?errorDiskSpace():errorDiskFail();}
SHITCALL uint snapNext(uint val){
if(val&(val-1))
return 0;
if(val>=2)
return val<<1;
return val+1;}
SHITCALL char*xstrdup(const char*str){
if(str==NULL)return NULL;
return errorAlloc(strdup(str));}
SHITCALL wchar_t*xstrdup(const wchar_t*str){
if(str==NULL)return NULL;
return errorAlloc(wcsdup(str));}
SHITCALL2 Void xmalloc(size_t size){
return errorAlloc(malloc(size));}
SHITCALL2 Void xrealloc(Void&ptr,size_t size){
return ptr=errorAlloc(realloc(ptr,size));}
SHITCALL2
Void xnxalloc(Void&ptr,size_t&count_,size_t size){
size_t result=movrl(b,count_);
incml(count_);
size_t count=movrl(d,result);
int count_1=movfl(a,result-1);
result*=size;
Void ptr2;
if(likely(count&count_1)){
ptr2=ptr;
goto PISSIN_RETURN;}
count<<=1;
if(count==0)
count++;
ptr2=xrealloc(ptr,size*count);
PISSIN_RETURN:
return ptr2+result;}
Void xvector::xnxalloc(size_t size){
size_t curSize=this->dataSize;
size_t memSize=this->allocSize;
size_t reqSize=movfl(a,curSize+size);
this->dataSize=reqSize;
Void ptr2;
if(likely(memSize>=reqSize)){
ptr2=this->dataPtr;
goto PISSIN_RETURN;}
if(memSize==0)
memSize=1;
do{
memSize<<=1;
}while(memSize<reqSize);
this->allocSize=memSize;
ptr2=xrealloc(this->dataPtr,memSize);
PISSIN_RETURN:
return ptr2+curSize;}
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
char**loadText(FILE*fp,int&LineCount){
if(fp==NULL){
LineCount=-1;
return NULL;}
LineCount=0;
int fileSize=fsize(fp);
if(fileSize==0){
fclose(fp);
return NULL;}
Void fileData=xmalloc(fileSize+1);
xfread(fileData,1,fileSize,fp);
fclose(fp);
Void curPos=fileData;
Void endPos=curPos+fileSize;
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
#define NCHAR char
#include "stdshit.cc"
#undef NCHAR
#define NCHAR wchar_t
#define NWIDE 1
#endif
SHITCALL
FILE*xfopen(const NCHAR*fName,const NCHAR*mode){
bool chkOpen=false;
if(mode[0]=='!'){
chkOpen=true;
mode++;}
LRETRY:
FILE*fp=fopen(fName,mode);
if(fp==NULL){
switch(errno){
case ENOENT:
case EACCES:
case EISDIR:
case EINVAL:
if(chkOpen)
#ifdef NWIDE
fatalError("Cannot open file: \"%S\"",fName);
#else
fatalError("Cannot open file: \"%s\"",fName);
#endif
break;
case ENOSPC:
errorDiskSpace();
goto LRETRY;
default:
errorAlloc();}}
return fp;}
SHITCALL
NCHAR*xfgets(NCHAR*str,int num,FILE*fp){
NCHAR*tmp=fgets(str,num,fp);
if((!tmp)&&(ferror(fp)))
errorDiskFail();
return tmp;}
char**loadText(const NCHAR*fileName,int&LineCount){
#ifdef NWIDE
return loadText(xfopen(fileName,L"rb"),LineCount);}
#else
return loadText(xfopen(fileName,"rb"),LineCount);}
#endif
SHITCALL
NCHAR*xstrcat(const NCHAR*str1,const NCHAR*str2){
int len1=strlen(str1);
int len2=strlen(str2);
NCHAR*buff=xmalloc(len1+len2+1);
strcpy(buff,str1);
strcpy(buff+len1,str2);
return buff;}
SHITCALL
NCHAR*strScmp(const NCHAR*str1,const NCHAR*str2){
while(1){
#ifdef NWIDE
NCHAR ch2=lodsw(str2);
#else
NCHAR ch2=lodsb(str2);
#endif
if(ch2==0)
return(NCHAR*)str1;
if(ch2!=*str1++)
return NULL;}}
NCHAR*strSicmp(const NCHAR*str1,const NCHAR*str2){
while(1){
#ifdef NWIDE
NCHAR ch2=toUpper(lodsw(str2));
#else
NCHAR ch2=toUpper(lodsb(str2));
#endif
if(ch2==0)
return(NCHAR*)str1;
if(ch2!=toUpper(*str1++))
return NULL;}}
SHITCALL
int strEcmp(const NCHAR*str1,const NCHAR*str2){
int diff=strlen(str1)-strlen(str2);
if(diff<0)
return 1;
return strcmp(str1+diff,str2);}
SHITCALL
int strEicmp(const NCHAR*str1,const NCHAR*str2){
int diff=strlen(str1)-strlen(str2);
if(diff<0)
return 0;
return stricmp(str1+diff,str2);}
SHITCALL
int strNcpy(NCHAR*dst,const NCHAR*src,int num){
for(int i=0;i<num;i++)
if(!(dst[i]=src[i]))
return i;
if(num>=0)
dst[num]='\0';
return num;}
SHITCALL
void strcpyn(NCHAR*dst,const NCHAR*src,int len){
memcpy(dst,src,len);
dst[len]='\0';}
SHITCALL
bool strcmpn(const NCHAR*str1,const NCHAR*str2,int len){
if(strlen(str1)!=len)
return false;
return!strncmp(str1,str2,len);}
SHITCALL
int removeCrap(NCHAR*str){
int len=strlen(str);
while(len--)
if(unsigned(str[len])>' ')
break;
str[len+1]='\0';
return len+1;}
SHITCALL
int strmove(NCHAR*dst,const NCHAR*src){
int len=strlen(src)+1;
memmove(dst,src,len*sizeof(NCHAR));}
SHITCALL
int getPathLen(const NCHAR*fName){
int i=strlen(fName);
while(i--){
if((fName[i]=='\\')
||(fName[i]=='/'))
return i+1;}
return 0;}
SHITCALL
int getPath(NCHAR*fName){
int len=getPathLen(fName);
fName[len]='\0';
return len;}
SHITCALL
NCHAR*getName(const NCHAR*fName)
{return(NCHAR*)fName+getPathLen(fName);}
SHITCALL
int getName(NCHAR*dst,const NCHAR*src,size_t max){
NCHAR*name=getName(src);
return strNcpy(dst,src,max-1);}
SHITCALL
bool isFullPath(const NCHAR*path){
if(path
&&((isPathSep(path[0]))
||(path[1]==':')))
return true;
return false;}