#include <stdshit.h>
#include "dlldef.h"

struct DefParse
{
	char* linePos; char* curPos;
	int curLine; char state, state2;
	cstr token; char* nTerm;

	DefParse(char* str) { ZINIT;
		linePos = curPos = str; }
	
	bool nextLine(void);
	bool chkToken(cch* str);
	bool getToken(char*& out);
	bool getHex64(u64& val);
	bool getInt32(int& val);
	
	bool hasToken() { return state > 0; }
	bool hasError() { return state < 0; }
	
	DefFile::LineInfo_t errPos() { return 
		{curLine+1, (curPos-linePos)+1}; }
	
private:
	void setError() { state = -1; }
	bool nextToken(bool newLine);
};

bool DefParse::nextToken(bool newLine)
{
	if(hasError()) return 0;
	SCOPE_REF(char*, pos, curPos);
	state = 0;

	// skip all whitespace
	byte ch, spMax = ' ';
	while(1){ if(!(ch = *pos)){ return 1; } INCP(pos);
		if(ch > spMax){ if(ch != ';') break; spMax = -1; }
		if(ch == '\r'){ if(*pos == '\n') INCP(pos); goto L1; }
		if(ch == '\n'){ L1: curLine++; linePos = pos;
			if(!newLine) return 1; spMax = ' ';}
	}
	
	// parse the token
	char* beg = pos;
	if(ch == '"') { // quote token
		for(;*pos != '"'; pos++) { if(!*pos) { 
			setError(); return 0; } }
		token = {beg, pos++};	
	} else { // normal token
		if(ch != '=') { for(;(u8(*pos) > ' ') && 
			(*pos != '=') && (*pos != ';'); pos++); }
		token = {beg-1, pos};
	}
	
	state = 1;
	return 1;
}

bool DefParse::chkToken(cch* str)
{
	if(hasToken() && token.cmp(str))
		return false;
	return nextToken(false); 
}

bool DefParse::getToken(char*& out)
{
	if(!hasToken()) return false;
	if(nTerm) *nTerm = 0;
	nTerm = token.end(); out = token; 
	return nextToken(false);
}

bool DefParse::getHex64(u64& val)
{
	if(!hasToken()) return false; char* end;
	val = strtoull(token.data, &end, 16);
	if(end != token.end()) setError();
	return nextToken(false);
}

bool DefParse::getInt32(int& val)
{	
	if(!hasToken()) return false; char* end;
	val = strtol(token.data, &end, 10);
	if(end != token.end()) setError();
	return nextToken(false);
}

bool DefParse::nextLine(void)
{
	if(hasToken()) setError();
	nextToken(true); 
	if(nTerm) *nTerm = 0;
	return hasToken();
}

FRAMEP_KEEP
DefFile::LineInfo_t DefFile::load(char* pos)
{
	enum { STATE_BEGIN = 0,
		STATE_NORMAL = 1, 
		STATE_EXPORT = 2 };
	
	DefParse tok(pos);
	//char state = STATE_BEGIN;
	while(tok.nextLine()) {
	
		if(tok.chkToken("EXPORTS")) {
			tok.state2 = STATE_EXPORT; 
			continue; }
		
		if(tok.chkToken("LIBRARY")) {
			tok.state2 = STATE_NORMAL;
			
			// read library name
			if(!tok.hasToken()) continue;
			if(tok.chkToken("BASE")) goto BASE;
			if(!tok.getToken(name)) break;
			
			// read library base
			if(!tok.hasToken()) continue;
			if(!tok.chkToken("BASE")) break;
			BASE: if(!tok.chkToken("=")) break;
			if(!tok.getHex64(base)) break;
			continue;
		}
		
		if(tok.state2 == STATE_EXPORT) {
		
			// get name and forwarder
			auto& exp = expLst.xnxcalloc();
			if(!tok.getToken(exp.name)) break;
			if(tok.chkToken("=")) {
				if(!tok.getToken(exp.frwd)) break; }
				
			// get ordinal
			if(!tok.hasToken()) continue;
			if(tok.token[0] == '@') {
				tok.token.split(1);
				if(!tok.getInt32(exp.ord)) break;
				if(tok.chkToken("NONAME")) 
					exp.NoName = 1;
			}
			
			// get private/data
			if(tok.chkToken("PRIVATE")) exp.Private = 1;
			if(tok.chkToken("DATA")) exp.Data = 1;
			continue;
		}

		break;
	}

	if(tok.state2 == 0) return {0,0};
	if(!tok.hasError()) return {-1,0};
	return tok.errPos();
}
