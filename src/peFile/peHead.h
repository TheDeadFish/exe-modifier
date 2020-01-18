#pragma once

struct PeOptHead
{
	IMAGE_FILE_HEADER ifh;
	
	int padding1;
	
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD AddressOfEntryPoint;
	ULONGLONG ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Win32VersionValue;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	ULONGLONG SizeOfStackReserve;
	ULONGLONG SizeOfStackCommit;
	ULONGLONG SizeOfHeapReserve;
	ULONGLONG SizeOfHeapCommit;
	DWORD LoaderFlags;
	
	struct DataDir {DWORD rva, size; };
	DataDir dataDir[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
	
	
	//u32 ioh_calcSize(void);
	
	
	
	//void ioh_save(
	
	bool PE64() { return u8(Magic>>8) == 2; }
	u32 ptrSize() { return PE64() ? 8 : 4; }
};
