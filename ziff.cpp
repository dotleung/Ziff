#include "ziff.h"

int IsCharsValid( const char* src )
{
    return src != NULL && *src != '\0';
}

int saCmp( ri& _first, ri& _second )
{
    char* pFirst = _first.chs;
    char* pSecond = _second.chs;
    if( pFirst == NULL || pSecond == NULL || pFirst == '\0' || pSecond == '\0' )
        return 0;
    while( *pFirst == *pSecond && *pFirst != '\0' )
    {
        pFirst ++;
        pSecond ++;
    }
    return *pFirst < *pSecond;
}

int binsearch( const ri* ris, unsigned int ri_size, const char* key, int keySize, int min_match_size ,int& sameSize )
{
    int foundPos = -1;
    sameSize = 0;
    if( ris == NULL || ri_size == 0 || key == NULL || *key == '\0' )
        return foundPos;
    int searchSize = keySize;
    if( min_match_size > 0 && min_match_size < searchSize )
        searchSize = min_match_size;
    int head = 0;
    int tail = ri_size -1;
    int mid = 0;
    while( head < tail )
    {
        mid = ( head + tail ) >> 1;
        foundPos = mid;
        char* pMidStr = ris[mid].chs;
        int cmpSize  = ris[mid].size;
        cmpSize = cmpSize < searchSize ? cmpSize : searchSize;
        int cmpRes = memcmp(pMidStr, key, cmpSize);
        if( cmpRes == 0 )
            break;
        if( cmpRes > 0 )
        {
            tail = mid - 1;
        }
        else
        {
            head = mid + 1;
        }
    }
    if( ris[foundPos].size < min_match_size || memcmp(ris[foundPos].chs, key, min_match_size ) != 0)
        foundPos = -1;
    else
    {
        sameSize = min_match_size;
        while( sameSize < keySize && sameSize < ris[foundPos].size && ris[foundPos].chs[sameSize] == key[sameSize] )
        {
            ++ sameSize;
        }
    }
    return foundPos;
}

unsigned int BKDRHash( const char* src )
{
    unsigned int hash = 0;
    if( src == NULL )
        return hash;
    char* ch = (char*)src;
    unsigned int seed = 131;
    while( *ch != '\0' )
    {
        hash = hash * seed + (unsigned int)(*ch ++);
    }
    return (hash & 0x7FFFFFFF);
}

Ziff::Ziff()
{
    
}
Ziff::~Ziff()
{
    
}
int Ziff::Init()
{
    return 0;
}
int Ziff::_ziff(const int src_off, const char* src, int src_size, const char* dst,int dst_size, char* patchMem, int& out_len)
{
    if( src == NULL || *src == '\0' || dst == NULL )
    {
        printf("invalid file path,%s,%d\n", __FILE__,__LINE__);
        return ERR_FD_ACCESS;
    }
    memset( patchMem, 0, out_len );
    char* pPatch = patchMem;
    unsigned long srcSize = src_size;
    ri* srcRank = (ri*)malloc( sizeof(ri) * srcSize );
    if( srcRank == NULL )
    {
        printf("malloc failed,%s,%d\n", __FILE__,__LINE__);
        return ERR_MALLOC_FAIL;
    }
    memset( srcRank, 0, sizeof(ri) * srcSize );
	
	char* toWriteMem = (char*)malloc( DIRECT_COPY_MAX_SIZE );
	if( toWriteMem == NULL )
	{
        printf("malloc failed,%s,%d\n", __FILE__,__LINE__);
		free( srcRank );
		return ERR_MALLOC_FAIL;
	}
    for( unsigned long i = 0; i < srcSize; i ++ )
    {
        srcRank[i].chs = (char*)(src + i);
        srcRank[i].size = srcSize - i;
        srcRank[i].pos = src_off + i;
    }
    std::sort( srcRank, srcRank + srcSize, saCmp );

    char* pDst = (char*)dst;
	char* pDirMem = toWriteMem;
    int directWriteSize = 0;
    int diffPlaceHolder = DIRECT_COPY_MAX_SIZE;
    int minValidSize = MIN_VALID_LEN;
    while((int)(pDst - dst) < dst_size )
    {
        int segSize = SKIP_STEP;
        int segPos = -1;
        int sameSize = 0;
        int idx = binsearch( (const ri*)srcRank, srcSize, pDst, dst_size - (pDst-dst),  minValidSize ,sameSize );
        if( idx >= 0 && idx < srcSize 
			&& srcRank[idx].size > minValidSize )
        {
			if( directWriteSize > 0 )
			{
				memcpy( pPatch, &directWriteSize, DIRECT_COPY_MAX_BYTES );
				pPatch += DIRECT_COPY_MAX_BYTES;
				memcpy( pPatch, toWriteMem, directWriteSize );
				pPatch += directWriteSize;
				memset( toWriteMem, 0, DIRECT_COPY_MAX_SIZE );
				pDirMem = toWriteMem;
                //printf("dir_,%d,%d\n",directWriteSize,__LINE__);
			    directWriteSize = 0;
			}
            segSize = sameSize;
            segPos = srcRank[idx].pos;
            memcpy( pPatch, &diffPlaceHolder, DIRECT_COPY_MAX_BYTES);
            pPatch += DIRECT_COPY_MAX_BYTES ;
            memcpy( pPatch, &segPos, sizeof(int) );
            pPatch += sizeof( int );
            memcpy( pPatch, &segSize, sizeof(int) );
            pPatch += sizeof( segSize );
            //printf("cp:%x,%x,%d\n",(int)(pDst - dst), segPos, segSize);
        }
        else
        {
			if( directWriteSize + segSize >= DIRECT_COPY_MAX_SIZE )
			{
				memcpy( pPatch, &directWriteSize, DIRECT_COPY_MAX_BYTES );
				pPatch += DIRECT_COPY_MAX_BYTES;
				memcpy( pPatch, toWriteMem, directWriteSize );
				pPatch += directWriteSize;
				memset( toWriteMem, 0, DIRECT_COPY_MAX_SIZE );
				pDirMem = toWriteMem;
                //printf("dir_,%d,%d\n",directWriteSize,__LINE__);
			    directWriteSize = 0;
			}
            directWriteSize += segSize;
            memcpy( pDirMem, pDst, segSize );
            pDirMem += segSize;
        }
        pDst += segSize;
    }
	if( directWriteSize > 0 )
	{
		memcpy( pPatch, &directWriteSize, DIRECT_COPY_MAX_BYTES );
		pPatch += DIRECT_COPY_MAX_BYTES;
		memcpy( pPatch, toWriteMem, directWriteSize );
		pPatch += directWriteSize;
		memset( toWriteMem, 0, DIRECT_COPY_MAX_SIZE );
		pDirMem = toWriteMem;
        //printf("dir_%d\n",__LINE__);
	}
    free( srcRank );
	free( toWriteMem );
    out_len = (int)(pPatch - patchMem);
    return 0;
}
int Ziff::Diff(const char* src_file, const char* dst_file, const char* patch_file)
{
    if( !IsCharsValid( src_file ) || !IsCharsValid( dst_file ) || !IsCharsValid( patch_file ) )
    {
        printf("malloc failed,%s,%d\n", __FILE__,__LINE__);
        return ERR_MALLOC_FAIL;
    }
    int ret = ERR_OK;
    FILE* fSrc = fopen( src_file, "rb");
    FILE* fDst = fopen( dst_file, "rb");
    FILE* fPatch = fopen( patch_file, "wb");
    if( fSrc == NULL || fDst == NULL || fPatch == NULL)
    {
        if( fSrc ) fclose(fSrc);
        if( fDst ) fclose(fDst);
        if( fPatch ) fclose(fPatch);
        printf("open file failed,%s,%d\n", __FILE__,__LINE__);
        return ERR_FD_ACCESS;
    }
    unsigned int srcReadSize = MAX_SEG_SIZE;
    unsigned int dstReadSize = srcReadSize;
    unsigned int patchSize = dstReadSize;
    unsigned int cksumSize = MAX_READ_SIZE;
    
    unsigned int srcSize = 0;
    unsigned int dstSize = 0;
    fseek( fSrc, 0, SEEK_END );
    fseek( fDst, 0, SEEK_END );
    srcSize = ftell( fSrc );
    dstSize = ftell( fDst );
    fseek( fDst, 0, SEEK_SET );
    fseek( fSrc, 0, SEEK_SET );
    
    dstReadSize = dstSize / ( (srcSize + srcReadSize - 1 ) / srcReadSize);
    patchSize = dstReadSize * 2;
    char* srcMem = (char*)malloc( srcReadSize );
    char* dstMem = (char*)malloc( dstReadSize );
    char* patchMem = (char*)malloc( patchSize );
    char* cksumMem = (char*)malloc( cksumSize );
    
    if( srcMem == NULL || dstMem == NULL || patchMem == NULL || cksumMem == NULL )
    {
        if( srcMem ) free(srcMem);
        if( dstMem ) free(dstMem);
        if( patchMem ) free(patchMem);
        if( cksumMem ) free(cksumMem);
        remove( patch_file );
        printf("malloc failed,%s,%d\n", __FILE__,__LINE__);
        return ERR_MALLOC_FAIL;
    }
    unsigned int out_cksumSize = cksumSize;
    if( CalcCksum( src_file, 0, cksumMem, out_cksumSize) != ERR_OK || out_cksumSize > MAX_CKSUM_SIZE )
    {
        printf("calc sum failed,cksumsize:%d,%s,%d\n",out_cksumSize, __FILE__,__LINE__);
        ret  = ERR_CMP_FAIL;
    }
    if( ret == ERR_OK && ( fwrite( MAGIC_CODE,1, MAGIC_CODE_SIZE, fPatch ) != MAGIC_CODE_SIZE 
        || fwrite( &out_cksumSize, 1, sizeof(int), fPatch) != sizeof(int) ))
    {
        printf("write file failed,%s,%d\n", __FILE__,__LINE__);
        ret = ERR_RWFILE_FAIL;
    }
    if( ret == ERR_OK && ( fwrite( &srcSize,1, sizeof(int), fPatch) != sizeof(int) 
        || fwrite( cksumMem, 1, out_cksumSize, fPatch) != out_cksumSize ))
    {
        printf("write file failed,%s,%d\n", __FILE__,__LINE__);
        ret = ERR_RWFILE_FAIL;
    }
    out_cksumSize = cksumSize;
    if ( CalcCksum( dst_file, 0, cksumMem, out_cksumSize ) != ERR_OK )
    {
        printf("calcsum failed\n");
    }
    if( ret == ERR_OK && ( fwrite( &dstSize,1, sizeof(int), fPatch) != sizeof(int) 
        || fwrite( cksumMem, 1, out_cksumSize, fPatch) != out_cksumSize))
    {
        printf("write file failed,%s,%d\n", __FILE__,__LINE__);
        ret = ERR_RWFILE_FAIL;
    }
    int realSrcReadSize = 0;
    int realDstReadSize = 0;
    int srcOff = 0;
    while( ret == ERR_OK && ( realSrcReadSize = fread( srcMem,1, srcReadSize, fSrc) ) > 0 
        && (realDstReadSize = fread( dstMem,1, dstReadSize, fDst )) > 0 )
    {
        int outSize = patchSize;
        if( _ziff( srcOff, srcMem, realSrcReadSize, dstMem, realDstReadSize, patchMem, outSize ) != ERR_OK )
        {
                printf("_ziff failed,%s,%d\n", __FILE__,__LINE__);
                ret = ERR_INNER;
                break;
        }
        //printf("outsize == %d\n",outSize);
        if( fwrite(patchMem,1, outSize, fPatch) != outSize )
        {
            printf("write file failed,%s,%d\n", __FILE__,__LINE__);
            ret = ERR_RWFILE_FAIL;
            break;
        }
        srcOff += realSrcReadSize;
    }
    free( srcMem );
    free( dstMem );
    free( patchMem );
    free( cksumMem );
    fclose( fSrc );
    fclose( fDst );
    fclose( fPatch );
    if( ret != ERR_OK )
    {
        remove( patch_file );
        return ret;
    }
    return 0;
}
int Ziff::Merge(const char* patch_file, const char* src_file, const char* dst_file)
{
    if( !IsCharsValid( src_file ) || !IsCharsValid( dst_file ) || !IsCharsValid( patch_file ) )
    {
        printf("invalid file path,%s,%d\n",__FILE__,__LINE__);
        return ERR_FD_ACCESS;
    }
    int ret = ERR_OK;
    FILE* fSrc = fopen( src_file, "rb");
    FILE* fPatch = fopen( patch_file, "rb");
    FILE* fDst = fopen( dst_file, "wb");
    if( fSrc == NULL || fDst == NULL || fPatch == NULL)
    {
        if( fSrc ) fclose(fSrc);
        if( fDst ) fclose(fDst);
        if( fPatch ) fclose(fPatch);
        printf("open file failed,%s,%d\n",__FILE__,__LINE__);
        return ERR_FD_ACCESS;
    }
    unsigned int cksumSize = 0;
    unsigned int srcSize = 0;
    unsigned int dstSize = 0;
    unsigned int patchSize = 0;
    
    if( fseek( fPatch, 0,SEEK_END) != 0 || (patchSize = (unsigned int)ftell(fPatch)) == 0
        || fseek( fPatch, 0, SEEK_SET) != 0 )
    {
        ret = ERR_FD_ACCESS;
    }
	
	if( ret == ERR_OK && (fseek( fPatch, MAGIC_CODE_SIZE, SEEK_SET) != 0 
		|| fread( &cksumSize, 1,  sizeof(int), fPatch ) != sizeof(int)
		|| fread( &srcSize, 1, sizeof(srcSize), fPatch ) != sizeof(srcSize)) )
	{
        printf("read patch failed,%s,%d\n", __FILE__,__LINE__);
		ret = ERR_FD_ACCESS;
	}

    unsigned int readMemSize = DIRECT_COPY_MAX_SIZE + DIRECT_COPY_MAX_BYTES;
	unsigned int out_len = cksumSize;
	char* cksumMem = (char*)malloc( cksumSize + 1 );
	char* readMem = (char*)malloc( readMemSize );
	if( ret == ERR_OK && ( cksumMem == NULL || readMem == NULL) )
	{
        printf("malloc failed,%s,%d\n",__FILE__,__LINE__);
		ret = ERR_FD_ACCESS;
	}
	else
	{
		out_len = cksumSize;
		memset( cksumMem, 0, cksumSize + 1 );
		memset( readMem, 0, DIRECT_COPY_MAX_SIZE + DIRECT_COPY_MAX_BYTES );
		if( ret == ERR_OK && ( CalcCksum( src_file, 0, cksumMem, out_len ) != 0 
			|| fread( readMem, 1,  out_len, fPatch ) != out_len
			|| memcmp( readMem, cksumMem, out_len ) != 0 ) )
		{
            printf("check src cksum failed,%s,%d\n",__FILE__,__LINE__);
			ret = ERR_CMP_FAIL;
		}
	}
    if( ret == ERR_OK && (fseek( fSrc, 0, SEEK_END ) != 0 
		|| srcSize != ftell( fSrc ) || fseek( fSrc, 0, SEEK_SET ) != 0 ) )
	{
        printf("check srcinfo failed,%s,%d\n",__FILE__,__LINE__);
		ret = ERR_CMP_FAIL;
	}
	memset( cksumMem, 0, cksumSize );
	if( ret == ERR_OK && (fread(&dstSize, 1, sizeof(dstSize), fPatch) != sizeof( dstSize )
		|| fread( cksumMem, 1, cksumSize, fPatch ) != cksumSize ) )
	{
        printf("check dst cksum failed,%s,%d\n",__FILE__,__LINE__);
		ret = ERR_CMP_FAIL;
	}
	int readSize = 0;
    int offset = 0;
    int copySize = 0;
	while( ret == ERR_OK && ftell(fPatch) < patchSize )
	{
        memset( readMem, 0, readMemSize );
		if( fread( &readSize, 1,  DIRECT_COPY_MAX_BYTES, fPatch ) != DIRECT_COPY_MAX_BYTES )
		{
            printf("read file failed,%s,%d\n",__FILE__,__LINE__);
			ret = ERR_RWFILE_FAIL;
			break;
		}
		if( readSize == DIRECT_COPY_MAX_SIZE ) // diffed
		{
	        if( fread( &offset, 1, sizeof(int), fPatch) != sizeof(int)
                || fread( &copySize, 1, sizeof(int), fPatch) != sizeof(int)
                || fseek( fSrc, offset, SEEK_SET) != 0 )
            {
                printf("read/write file failed,%s,%d\n",__FILE__,__LINE__);
                ret = ERR_RWFILE_FAIL;
                break;
            }
            else
            {
                if( copySize > readMemSize )
                {
                    free( readMem );
                    readMemSize = copySize + 1;
                    readMem = (char*)malloc( readMemSize );
                    if( readMem == NULL )
                    {
                        printf("realloc failed,%s,%d\n",__FILE__,__LINE__);
                        ret = ERR_MALLOC_FAIL;
                        break;
                    }
                    memset( readMem, 0, readMemSize );
                }
                if( fread( readMem, 1, copySize, fSrc) != copySize 
                    || fwrite( readMem, 1, copySize, fDst) != copySize )
                {
                    printf("read/write file failed,%s,%d\n",__FILE__,__LINE__);
                    ret = ERR_RWFILE_FAIL;
                    break;
                }
            }
		}
		else
		{
			if( fread( readMem, 1, readSize, fPatch) != readSize
                || fwrite( readMem,1,  readSize, fDst) != readSize )
            {
                printf("read/write file failed,%s,%d\n",__FILE__,__LINE__);
                ret = ERR_RWFILE_FAIL;
                break;
            }
		}
		readSize = 0;
	}
    if( ret == ERR_OK && dstSize != (int)ftell( fDst ))
    {
        printf("Merge failed,size not equal,%s,%d\n",__FILE__,__LINE__);
        ret = ERR_INNER;
    }
	fclose( fDst );
	fclose( fSrc );
	fclose( fPatch );
    out_len = readSize;
    if( out_len < cksumSize )
    {
        if(readMem) free( readMem );
        readMem = (char*)malloc( cksumSize );
        if(readMem == NULL)
        {
            printf("malloc failed,%s,%d\n",__FILE__,__LINE__);
            ret = ERR_MALLOC_FAIL;
        }
        out_len = cksumSize;
    }
    if( ret == ERR_OK && (CalcCksum( dst_file, 0, readMem , out_len) != ERR_OK
        || memcmp( cksumMem, readMem, out_len) != 0))
    {
        printf("Check dstfile cksum failed,%s,%d\n",__FILE__,__LINE__);
        ret = ERR_CMP_FAIL;

    }

    if( cksumMem) free( cksumMem );
	if( readMem ) free( readMem );
    if( ret != ERR_OK )
    {
        remove( dst_file );
    }
	return ret;
}
int Ziff::CalcCksum(const char* src_file, const unsigned int pos, char* out_cksum, unsigned int& out_len)
{
    if( !IsCharsValid( src_file ) )
    {
        printf("invalid file path,%s,%d\n",__FILE__,__LINE__);
        return ERR_FD_ACCESS;
    }
    if( out_cksum == NULL || out_len == 0)
    {
        printf("cksum mem null,len:%d,%s,%d\n",out_len, __FILE__,__LINE__);
        return ERR_OTHER;
    }
    memset( out_cksum, 0, out_len );
    unsigned int readSize = MAX_READ_SIZE;
    char* readMem = (char*)malloc( readSize );
    if( readMem == NULL )
    {
        printf("malloc failed,%s,%d\n",__FILE__,__LINE__);
        return ERR_MALLOC_FAIL;
    }
    FILE* fSrc = fopen(src_file, "rb");
    if( fSrc == NULL )
    {
        printf("open file failed,%s,%d\n",__FILE__,__LINE__);
        free( readMem );
        return ERR_FD_ACCESS;
    }
    fseek( fSrc, 0, SEEK_END );
    unsigned int srcfSize= (int)ftell( fSrc );
    fseek( fSrc, 0, SEEK_SET );
    
    if( pos >= srcfSize )
    {
        printf("seek file failed,pos:%d,size:%d,%s,%d\n",pos,srcfSize,__FILE__,__LINE__);
        fclose( fSrc );
        free( readMem );
        return ERR_RWFILE_FAIL;
    }
    fseek( fSrc, pos, SEEK_SET );
    unsigned int hash = 0;
    while( fread( readMem, 1, readSize, fSrc) && !feof( fSrc ))
    {
        hash ^= BKDRHash( (const char*)readMem );
        memset(readMem, 0, readSize);
    }
    fclose( fSrc );
    free( readMem );
    sprintf(out_cksum, "%u", hash);
    out_len = sizeof(hash);
    return 0;
}
