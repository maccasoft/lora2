
typedef unsigned long dword;

struct _reg_key {
   char  sysop[36];
   int   maxlines;
   word  version;
   int   zone;
   int   net;
   int   node;
   char  release_key[10];
   dword crc;
};

struct _supporter {
   char  name[36];
   int   zone;
   int   net;
   int   node;
   int   numregs;
   dword crc;
};

struct _reg_data {
	struct _reg_key key;
	dword  amount;
	char   address[5][60];
};

