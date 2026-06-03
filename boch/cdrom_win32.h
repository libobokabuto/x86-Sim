#pragma once
class cdrom_win32_c : public cdrom_base_c {
public:
	cdrom_win32_c(const char* dev);
	virtual ~cdrom_win32_c(void);
private:
#ifdef WIN32
	HANDLE hFile;
#endif
};