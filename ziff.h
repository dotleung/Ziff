#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <string.h>

#define MAX_READ_SIZE            4 * 1024
#define MAX_SEG_SIZE            32 * 1024 * 1024 
#define SKIP_STEP            1
#define MIN_VALID_LEN            9
#define DIRECT_COPY_MAX_BYTES			1 
#define DIRECT_COPY_MAX_SIZE           (1 << ( DIRECT_COPY_MAX_BYTES * 8 )) - 1
#define MAX_CKSUM_BYTES            2
#define MAX_CKSUM_SIZE            (1 << (MAX_CKSUM_BYTES * 8)) - 1
#define MAGIC_CODE            "ZiffIsASimpleBinaryDiffTool"
#define MAGIC_CODE_SIZE            32

#define ERR_OK          0
#define ERR_INNER           -1
#define ERR_MALLOC_FAIL         -2
#define ERR_FD_ACCESS           -3
#define ERR_RWFILE_FAIL         -4
#define ERR_CMP_FAIL        -5
#define ERR_OTHER           -999

struct ri
{
    char* chs;
    unsigned int size;
    unsigned int pos;
};

extern inline int IsCharsValid( const char* src );
extern inline int saCmp( ri& _first, ri& _second );
extern int binsearch( const ri* ris, unsigned int ri_size, const char* key, int keySize, int min_match_size,int& sameSize );
extern unsigned int BKDRHash( const char* src );

class Ziff
{
    public:
        Ziff();
        ~Ziff();
        int Init();
        int Diff(const char* src_file, const char* dst_file, const char* patch_file);
        int Merge(const char* patch_file, const char* src_file, const char* dst_file);
    protected:
        int CalcCksum(const char* src_file, const unsigned int pos, char* out_cksum, unsigned int& out_len);
        int _ziff(const int src_off, const char* src, int src_size,const char* dst, int dst_size, char* patchMem, int& out_len);
    private:
        
};
