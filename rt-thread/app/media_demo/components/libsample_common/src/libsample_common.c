#include "libsample_common/include/libsample_common.h"

static char *model_name = MODEL_TAG_LIBCOMMON;

FH_SINT32 openFile(FILE **fd, FH_CHAR *filename)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (filename == NULL)
    {
        SDK_PRT(model_name, "Open File failed filename == NULL!\n");
        return FH_SDK_FAILED;
    }

    *fd = fopen(filename, "rb");
    if (!*fd)
    {
        SDK_PRT(model_name, "Open File[%s] failed fd == NULL!\n", filename);
        return FH_SDK_FAILED;
    }

    // SDK_PRT(model_name, "Open File[%s] success!\n", filename);

    return ret;
}

FH_VOID *readFile(FH_CHAR *filename)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 readLen = 0;
    FH_SINT32 fileLen = 0;
    FILE *fp = NULL;
    FH_VOID *buf = NULL;

    SDK_CHECK_NULL_PTR(model_name, filename);

    ret = openFile(&fp, filename);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    fseek(fp, 0, SEEK_END);
    fileLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = malloc(fileLen);
    SDK_CHECK_NULL_PTR(model_name, buf);

    readLen = fread(buf, 1, fileLen, fp);
    if (readLen < fileLen)
    {
        SDK_ERR_PRT(model_name, "Read file failed, fileLen = %d, readLen = %d\n", fileLen, readLen);
        goto Exit;
    }

    if (fp)
    {
        fclose(fp);
    }

    return buf;

Exit:
    if (fp)
    {
        fclose(fp);
    }
    return NULL;
}

FH_VOID *readFile_with_len(FH_CHAR *filename, FH_UINT32 *len)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 readLen = 0;
    FH_SINT32 fileLen = 0;
    FILE *fp = NULL;
    FH_VOID *buf = NULL;

    SDK_CHECK_NULL_PTR(model_name, filename);

    ret = openFile(&fp, filename);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    fseek(fp, 0, SEEK_END);
    fileLen = ftell(fp);
    *len = fileLen;
    fseek(fp, 0, SEEK_SET);

    buf = malloc(fileLen);
    SDK_CHECK_NULL_PTR(model_name, buf);

    readLen = fread(buf, 1, fileLen, fp);
    if (readLen < fileLen)
    {
        SDK_ERR_PRT(model_name, "Read file failed, fileLen = %d, readLen = %d\n", fileLen, readLen);
        goto Exit;
    }

    if (fp)
    {
        fclose(fp);
    }

    return buf;

Exit:
    if (fp)
    {
        fclose(fp);
    }
    return NULL;
}

FH_SINT32 readFileToAddr(FH_CHAR *filename, FH_VOID *addr)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 readLen = 0;
    FH_SINT32 fileLen = 0;
    FILE *fp = NULL;
    FH_ADDR buf = addr;

    SDK_CHECK_NULL_PTR(model_name, filename);
    SDK_CHECK_NULL_PTR(model_name, buf);

    ret = openFile(&fp, filename);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    fseek(fp, 0, SEEK_END);
    fileLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    readLen = fread(buf, 1, fileLen, fp);
    if (readLen < fileLen)
    {
        SDK_ERR_PRT(model_name, "Read file failed, fileLen = %d, readLen = %d\n", fileLen, readLen);
        goto Exit;
    }

    if (fp)
    {
        fclose(fp);
    }

    return ret;

Exit:
    if (fp)
    {
        fclose(fp);
    }
    return FH_SDK_FAILED;
}

FH_SINT32 getFileSize(FH_CHAR *path)
{
    struct stat statbuf;
    FH_SINT32 ret = 0;

    SDK_CHECK_NULL_PTR(model_name, path);

    ret = stat(path, &statbuf);
    if (0 != ret)
    {
        SDK_ERR_PRT(model_name, "read file error, path:%s", path);
        return 0;
    }
    else
    {
        return statbuf.st_size;
    }

Exit:
    return -1;
}

FH_SINT32 readFileRepeatByLength(FILE *fp, FH_UINT32 length, FH_ADDR addr)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_UINT32 readLen;

    readLen = fread(addr, 1, length, fp);

    if (!readLen)
    { // repeat
        fseek(fp, 0, SEEK_SET);
        readLen = fread(addr, 1, length, fp);
    }

    return ret;
}

FH_SINT32 saveDataToFile(FH_ADDR data, FH_UINT32 dataSize, FH_CHAR *fileName)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FILE *fp = NULL;
    FH_UINT32 readCount = 0;

    SDK_CHECK_NULL_PTR(model_name, data);

    if (dataSize == 0)
    {
        SDK_ERR_PRT(model_name, "dataSize = 0!\n");
        return FH_SDK_FAILED;
    }

    fp = fopen(fileName, "w");
    if (!fp)
    {
        SDK_ERR_PRT(model_name, "Open File[%s] failed fd == NULL!\n", fileName);
        return FH_SDK_FAILED;
    }
    readCount = fwrite(data, 1, dataSize, fp);
    if (readCount != dataSize)
    {
        SDK_ERR_PRT(model_name, "readCount != dataSize!\n");
    }

Exit:
    if (fp)
    {
        fclose(fp);
    }

    return ret;
}

FH_UINT64 get_us(FH_VOID)
{
    FH_UINT64 usec = 0;
#ifdef __LINUX_OS__
    struct timeval tv;

    gettimeofday(&tv, NULL);
    usec = (FH_UINT64)tv.tv_sec * 1000 * 1000 + tv.tv_usec;
#endif

#ifdef __RTTHREAD_OS__
    usec = read_pts();
#endif
    return usec;
}

FH_UINT64 get_ms(FH_VOID)
{
    FH_UINT64 usec = 0;
#ifdef __LINUX_OS__
    struct timeval tv;

    gettimeofday(&tv, NULL);
    usec = (FH_UINT64)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

#ifdef __RTTHREAD_OS__
    usec = read_pts() / 1000;
#endif
    return usec;
}

FH_VOID nv12_to_yuy2(FH_CHAR *nv12_ydata, FH_CHAR *nv12_uvdata, FH_CHAR *yuy2_data, FH_SINT32 width, FH_SINT32 height)
{
    int *pdwY1 = (int *)nv12_ydata;
    int *pdwY2 = (int *)(nv12_ydata + width);
    int *pdwUV = (int *)(nv12_uvdata);

    int *pdwYUYV1 = (int *)(yuy2_data);
    int *pdwYUYV2 = (int *)(yuy2_data + (width << 1));
    int halfWidth = width >> 1;
    int quarterWidth = width >> 2;
    int halfHeight = height >> 1;

    register int dwUV, Y1, Y2, UV;
    register int line, i;

    for (line = 0; line < halfHeight; line++)
    {
        for (i = 0; i < quarterWidth; i++)
        {
            UV = *pdwUV;
            Y1 = pdwY1[i];
            Y2 = pdwY2[i];

            dwUV = ((UV << 16) & 0xff000000) | ((UV << 8) & 0xff00);
            *pdwYUYV1 = (Y1 & 0xff) | ((Y1 << 8) & 0xff0000) | dwUV;
            *pdwYUYV2 = (Y2 & 0xff) | ((Y2 << 8) & 0xff0000) | dwUV;

            pdwYUYV1++;
            pdwYUYV2++;

            dwUV = ((UV) & 0xff000000) | ((UV >> 8) & 0xff00);
            *pdwYUYV1 = ((Y1 >> 16) & 0xff) | ((Y1 >> 8) & 0xff0000) | dwUV;
            *pdwYUYV2 = ((Y2 >> 16) & 0xff) | ((Y2 >> 8) & 0xff0000) | dwUV;

            pdwYUYV1++;
            pdwYUYV2++;

            pdwUV++;
        }

        pdwY1 += halfWidth;
        pdwY2 += halfWidth;
        pdwYUYV1 += halfWidth;
        pdwYUYV2 += halfWidth;
    }
}
