#pragma once
extern unsigned int bx_cdrom_count;

class cdrom_base_c  {
public:
	cdrom_base_c() {}
protected:
	int fd;
	char* path;
	bool using_file;
};