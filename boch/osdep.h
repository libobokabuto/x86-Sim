#pragma once
#if defined(_MSC_VER)
#define read _read
#define open _open
#define close _close
#else 
#endif