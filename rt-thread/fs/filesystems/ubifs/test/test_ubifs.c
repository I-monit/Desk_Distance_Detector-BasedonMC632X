
#include <rtconfig.h>
#include <dfs_file.h>
#include "ubi_glue.h"
#include <unistd.h>
#include <rt_ubi_api.h>
/* #include <dfs_private.h> */
#include <rttshell.h>

const static char MLK_Speech[] = "I am happy to join with you today in what will go down in history as the greatest demonstration for freedom in the history of our nation.\n\
Five score years ago, a great American, in whose symbolic shadow we stand today, signed the Emancipation Proclamation. This momentous decree came as a great beacon light of hope to millions of Negro slaves who had been seared in the flames of withering injustice. It came as a joyous daybreak to end the long night of their captivity.\n\
\n\
But one hundred years later, the Negro still is not free. One hundred years later, the life of the Negro is still sadly crippled by the manacles of segregation and the chains of discrimination. One hundred years later, the Negro lives on a lonely island of poverty in the midst of a vast ocean of material prosperity. One hundred years later, the Negro is still languished in the corners of American society and finds himself an exile in his own land. And so we've come here today to dramatize a shameful condition.\n\
\n\
In a sense we've come to our nation's capital to cash a check. When the architects of our republic wrote the magnificent words of the Constitution and the Declaration of Independence, they were signing a promissory note to which every American was to fall heir. This note was a promise that all men, yes, black men as well as white men, would be guaranteed the \"unalienable Rights\" of \"Life, Liberty and the pursuit of Happiness.\" It is obvious today that America has defaulted on this promissory note, insofar as her citizens of color are concerned. Instead of honoring this sacred obligation, America has given the Negro people a bad check, a check which has come back marked \"insufficient funds.\"\n\
\n\
But we refuse to believe that the bank of justice is bankrupt. We refuse to believe that there are insufficient funds in the great vaults of opportunity of this nation. And so, we've come to cash this check, a check that will give us upon demand the riches of freedom and the security of justice.\n\
\n\
We have also come to this hallowed spot to remind America of the fierce urgency of Now. This is no time to engage in the luxury of cooling off or to take the tranquilizing drug of gradualism. Now is the time to make real the promises of democracy. Now is the time to rise from the dark and desolate valley of segregation to the sunlit path of racial justice. Now is the time to lift our nation from the quicksands of racial injustice to the solid rock of brotherhood. Now is the time to make justice a reality for all of God's children.\n\
\n\
It would be fatal for the nation to overlook the urgency of the moment. This sweltering summer of the Negro's legitimate discontent will not pass until there is an invigorating autumn of freedom and equality. Nineteen sixty-three is not an end, but a beginning. And those who hope that the Negro needed to blow off steam and will now be content will have a rude awakening if the nation returns to business as usual. And there will be neither rest nor tranquility in America until the Negro is granted his citizenship rights. The whirlwinds of revolt will continue to shake the foundations of our nation until the bright day of justice emerges.\n\
\n\
But there is something that I must say to my people, who stand on the warm threshold which leads into the palace of justice : In the process of gaining our rightful place, we must not be guilty of wrongful deeds. Let us not seek to satisfy our thirst for freedom by drinking from the cup of bitterness and hatred. We must forever conduct our struggle on the high plane of dignity and discipline. We must not allow our creative protest to degenerate into physical violence. Again and again, we must rise to the majestic heights of meeting physical force with soul force.\n\
\n\
The marvelous new militancy which has engulfed the Negro community must not lead us to a distrust of all white people, for many of our white brothers, as evidenced by their presence here today, have come to realize that their destiny is tied up with our destiny. And they have come to realize that their freedom is inextricably bound to our freedom.\n\
\n\
We cannot walk alone.\n\
\n\
And as we walk, we must make the pledge that we shall always march ahead.\n\
\n\
We cannot turn back.\n\
\n\
There are those who are asking the devotees of civil rights, \"When will you be satisfied?\" We can never be satisfied as long as the Negro is the victim of the unspeakable horrors of police brutality. We can never be satisfied as long as our bodies, heavy with the fatigue of travel, cannot gain lodging in the motels of the highways and the hotels of the cities. **We cannot be satisfied as long as the negro's basic mobility is from a smaller ghetto to a larger one. We can never be satisfied as long as our children are stripped of their self-hood and robbed of their dignity by signs stating: \"For Whites Only.\"** We cannot be satisfied as long as a Negro in Mississippi cannot vote and a Negro in New York believes he has nothing for which to vote. No, no, we are not satisfied, and we will not be satisfied until \"justice rolls down like waters, and righteousness like a mighty stream.\"\n\
\n\
I am not unmindful that some of you have come here out of great trials and tribulations. Some of you have come fresh from narrow jail cells. And some of you have come from areas where your quest --quest for freedom left you battered by the storms of persecution and staggered by the winds of police brutality. You have been the veterans of creative suffering. Continue to work with the faith that unearned suffering is redemptive. Go back to Mississippi, go back to Alabama, go back to South Carolina, go back to Georgia, go back to Louisiana, go back to the slums and ghettos of our northern cities, knowing that somehow this situation can and will be changed.\n\
\n\
Let us not wallow in the valley of despair, I say to you today, my friends.\n\
\n\
And so even though we face the difficulties of today and tomorrow, I still have a dream. It is a dream deeply rooted in the American dream.\n\
\n\
I have a dream that one day this nation will rise up and live out the true meaning of its creed: \"We hold these truths to be self-evident, that all men are created equal.\"\n\
\n\
I have a dream that one day on the red hills of Georgia, the sons of former slaves and the sons of former slave owners will be able to sit down together at the table of brotherhood.\n\
\n\
I have a dream that one day even the state of Mississippi, a state sweltering with the heat of injustice, sweltering with the heat of oppression, will be transformed into an oasis of freedom and justice.\n\
\n\
I have a dream that my four little children will one day live in a nation where they will not be judged by the color of their skin but by the content of their character.\n\
\n\
I have a dream today !\n\
\n\
I have a dream that one day, down in Alabama, with its vicious racists, with its governor having his lips dripping with the words of \"interposition\" and \"nullification\" --one day right there in Alabama little black boys and black girls will be able to join hands with little white boys and white girls as sisters and brothers.\n\
\n\
I have a dream today !\n\
\n\
I have a dream that one day every valley shall be exalted, and every hill and mountain shall be made low, the rough places will be made plain, and the crooked places will be made straight; \"and the glory of the Lord shall be revealed and all flesh shall see it together.\"\n\
\n\
This is our hope, and this is the faith that I go back to the South with.\n\
\n\
With this faith, we will be able to hew out of the mountain of despair a stone of hope. With this faith, we will be able to transform the jangling discords of our nation into a beautiful symphony of brotherhood. With this faith, we will be able to work together, to pray together, to struggle together, to go to jail together, to stand up for freedom together, knowing that we will be free one day.\n\
\n\
And this will be the day --this will be the day when all of God's children will be able to sing with new meaning:\n\
\n\
My country 'tis of thee, sweet land of liberty, of thee I sing. Land where my fathers died, land of the Pilgrim's pride,    From every mountainside, let freedom ring!\n\
\n\
And if America is to be a great nation, this must become true.\n\
\n\
And so let freedom ring from the prodigious hilltops of New Hampshire.\n\
\n\
Let freedom ring from the mighty mountains of New York.\n\
\n\
Let freedom ring from the heightening Alleghenies of Pennsylvania.\n\
\n\
Let freedom ring from the snow-capped Rockies of Colorado.\n\
\n\
Let freedom ring from the curvaceous slopes of California.\n\
\n\
But not only that:\n\
\n\
Let freedom ring from Stone Mountain of Georgia.\n\
\n\
Let freedom ring from Lookout Mountain of Tennessee.\n\
\n\
Let freedom ring from every hill and molehill of Mississippi.\n\
\n\
From every mountainside, let freedom ring.\n\
\n\
And when this happens, and when we allow freedom ring, when we let it ring from every village and every hamlet, from every state and every city, we will be able to speed up that day when all of God's children, black men and white men, Jews and Gentiles, Protestants and Catholics, will be able to join hands and sing in the words of the old Negro spiritual:\n\
\n\
Free at last !Free at last !\n\
\n\
Thank God Almighty, we are free at last !";

#define TEST_DIR1 "/test/test/test3/test4/test5/test6/test7/test8/test9/test10/test11/test12/test13/test14/test15/test16/test17/test18/test19/test20/"
#define TEST_DIR "/"
#define PAGESIZE 4096
#define HALFPAGE_PLUS 2500

static int gJustCheck = 0;
static int ubifs_test_checkdata(char *fileName, const char *dataCheckBegin, int dataSize)
{
    struct dfs_fd fd;
    if (dfs_file_open(&fd, fileName, O_RDONLY) < 0)
    {
        rt_kprintf("Open %s failed\n", fileName);
        return -1;
    }
    char *dataBuf = rt_malloc(dataSize + 4);
    int readedSize = dfs_file_read(&fd, dataBuf, dataSize);
    if (readedSize != dataSize)
    {
        rt_kprintf("size error.%d!=%d\n", readedSize, dataSize);
        rt_free(dataBuf);
        dfs_file_close(&fd);
        return -1;
    }
    if (memcmp(dataBuf, dataCheckBegin, dataSize) != 0)
    {
        rt_kprintf("%s mem not equal\n", fileName);

        int i = 0;
        for (i = 0; i < dataSize; i++)
        {
            if (dataBuf[i] != dataCheckBegin[i])
            {
                rt_kprintf("%s\n",&dataBuf[i]);
                break;
            }
        }
        rt_kprintf("diff begin:%d\n", i);
        rt_kprintf("-------------------------------------readed-------------------------------------\n");
        rt_kprintf("%s\n", dataBuf + i);
        rt_kprintf("\n------------------------------------------buf-------------------------------------\n");
        rt_kprintf("%s\n", dataCheckBegin + i);
        rt_kprintf("\n-------------------------------------------end-------------------------------------\n");
        rt_free(dataBuf);
        dfs_file_close(&fd);
        return -1;
    }
    rt_kprintf("%s check OK!!! \n", strrchr(fileName, '/') + 1);
    rt_free(dataBuf);
    dfs_file_close(&fd);
    return 0;
}
static void ubifs_test_write_1_page(char *dirname)
{
    struct dfs_fd fd;
    uint32_t toWriteLength = 100;
    uint32_t length;
    uint32_t checkLength = 200;
    char fileName[128] = {0};
    snprintf(fileName, sizeof(fileName), "%s/test_1_page.txt", dirname);
    if (gJustCheck == 0)
    {
        rt_kprintf("test write 1 page data,one part: %d .%d,%d \n", 100, O_RDWR, O_WRONLY);
        if (dfs_file_open(&fd, fileName, O_RDWR) < 0)
        {
            rt_kprintf("Open %s failed\n", fileName);
            return;
        }
        length = dfs_file_write(&fd, MLK_Speech, toWriteLength);
        length += dfs_file_write(&fd, MLK_Speech + length, toWriteLength);
        rt_kprintf(" write %d bytes\n", length);
        dfs_file_close(&fd);
        if (checkLength != length)
        {
            rt_kprintf("checklength error.%d,%d\n", checkLength, length);
            checkLength = length;
        }
    }
    ubifs_test_checkdata(fileName, MLK_Speech, checkLength);
}

static void ubifs_test_write_2_page(char *dirname)
{
    struct dfs_fd fd;
    uint32_t toWriteLength = HALFPAGE_PLUS;
    uint32_t length;
    char fileName[128] = {0};
    snprintf(fileName, sizeof(fileName), "%s/test_2_page.txt", dirname);
    uint32_t checkLength = HALFPAGE_PLUS * 2;
    if (gJustCheck == 0)
    {
        rt_kprintf("test write 2 page data,one part: %d \n", HALFPAGE_PLUS);
        if (dfs_file_open(&fd, fileName, O_RDWR) < 0)
        {
            rt_kprintf("Open %s failed\n", fileName);
            return;
        }
        length = dfs_file_write(&fd, MLK_Speech, toWriteLength);
        length += dfs_file_write(&fd, MLK_Speech + length, toWriteLength);
        rt_kprintf(" write %d bytes\n", length);
        dfs_file_close(&fd);
        if (checkLength != length)
        {
            rt_kprintf("checklength error.%d,%d\n", checkLength, length);
            checkLength = length;
        }
    }
    ubifs_test_checkdata(fileName, MLK_Speech, checkLength);
}
static void ubifs_test_write_pagealign(char *dirname)
{
    struct dfs_fd fd;
    uint32_t toWriteLength = PAGESIZE;
    uint32_t writedLength = 0;
    char fileName[128] = {0};
    snprintf(fileName, sizeof(fileName), "%s/test_pagealign.txt", dirname);
    uint32_t checkLength = sizeof(MLK_Speech);
    if (gJustCheck == 0)
    {
        rt_kprintf("test write all %d pagealign\n", PAGESIZE);
        if (dfs_file_open(&fd, fileName, O_RDWR) < 0)
        {
            rt_kprintf("Open %s failed\n", fileName);
            return;
        }
        while (writedLength < sizeof(MLK_Speech))
        {
            toWriteLength = (sizeof(MLK_Speech) - writedLength) < PAGESIZE ? (sizeof(MLK_Speech) - writedLength) : PAGESIZE;
            writedLength += dfs_file_write(&fd, MLK_Speech + writedLength, toWriteLength);
        }
        rt_kprintf(" write %d bytes\n", writedLength);
        dfs_file_close(&fd);
        if (checkLength != writedLength)
        {
            rt_kprintf("checklength error.%d,%d\n", checkLength, writedLength);
            checkLength = writedLength;
        }
    }
    ubifs_test_checkdata(fileName, MLK_Speech, checkLength);
}
static void ubifs_test_write_all(char *dirname)
{
    struct dfs_fd fd;
    uint32_t writedLength = 0;
    char fileName[128] = {0};
    snprintf(fileName, sizeof(fileName), "%s/test_all_data.txt", dirname);
    uint32_t checkLength = sizeof(MLK_Speech);
    if (gJustCheck == 0)
    {
        rt_kprintf("test write all data\n");
        if (dfs_file_open(&fd, fileName, O_RDWR) < 0)
        {
            rt_kprintf("Open %s failed\n", fileName);
            return;
        }
        writedLength = dfs_file_write(&fd, MLK_Speech, sizeof(MLK_Speech));
        rt_kprintf(" write %d bytes\n", writedLength);
        dfs_file_close(&fd);
        if (checkLength != writedLength)
        {
            rt_kprintf("checklength error.%d,%d\n", checkLength, writedLength);
            checkLength = writedLength;
        }
        ubifs_test_checkdata(fileName, MLK_Speech, checkLength); /* seek will write,don't check just read. */
    }
}
static void ubifs_test_write_seek(char *dirname)
{
    struct dfs_fd fd;
    char buf[] = "---------------------------------this is test--------------------------------------";
    char buf1[200];
    char fileName[128] = {0};
    snprintf(fileName, sizeof(fileName), "%s/test_all_data.txt", dirname);
    char nbuf[PAGESIZE + 600];

    if (gJustCheck == 0) /* write->seek-->write--->seeknext-->write-->seek-->read */
    {
        rt_kprintf("test seek and write data\n");
        if (dfs_file_open(&fd, fileName, O_RDWR) < 0)
        {
            rt_kprintf("Open %s failed\n", fileName);
            return;
        }
        char cbuf[PAGESIZE + 600] = {0};
        memcpy(cbuf, MLK_Speech, sizeof(cbuf));

        dfs_file_write(&fd, buf, sizeof(buf)); /* 0 */
        memcpy(cbuf, buf, sizeof(buf));
        dfs_file_lseek(&fd, PAGESIZE / 4);
        dfs_file_write(&fd, buf, sizeof(buf)); /* 1024 */
        memcpy(cbuf + PAGESIZE / 4, buf, sizeof(buf));
        dfs_file_lseek(&fd, PAGESIZE + 100);   /* next page */
        dfs_file_write(&fd, buf, sizeof(buf)); /* 4196 */
        memcpy(cbuf + PAGESIZE + 100, buf, sizeof(buf));
        dfs_file_lseek(&fd, PAGESIZE + 400);
        dfs_file_write(&fd, buf, sizeof(buf)); /* 4496 */
        memcpy(cbuf + PAGESIZE + 400, buf, sizeof(buf));
        dfs_file_read(&fd, buf1, sizeof(buf1)); /* 4496 */

        dfs_file_lseek(&fd, 0);
        dfs_file_read(&fd, nbuf, sizeof(nbuf)); /* 0 */
        if (memcmp(nbuf, cbuf, sizeof(nbuf)) != 0)
        {
            rt_kprintf(" pos 0: error:%s\n", nbuf);
        }
        dfs_file_close(&fd);
    }

    memcpy(nbuf, MLK_Speech, sizeof(nbuf));
    memcpy(nbuf, buf, sizeof(buf));
    memcpy(nbuf + PAGESIZE / 4, buf, sizeof(buf));
    memcpy(nbuf + PAGESIZE + 100, buf, sizeof(buf));
    memcpy(nbuf + PAGESIZE + 400, buf, sizeof(buf));
    rt_kprintf("seek: ");
    ubifs_test_checkdata(fileName, nbuf, sizeof(nbuf));
}
void ubifs_test(const char *rwflag, char *dir)
{
    if (rwflag[0] == 'r')
        gJustCheck = 1;
    else
        gJustCheck = 0;
    ubifs_test_write_1_page(dir);
    ubifs_test_write_2_page(dir);
    ubifs_test_write_all(dir);
    ubifs_test_write_pagealign(dir);
    ubifs_test_write_seek(dir);
}
void ubifs_test_filestat(const char *fullpath)
{
    struct stat stat;
    dfs_file_stat(fullpath, &stat);
    rt_kprintf("\tFile\t\t%s\n", fullpath);
    rt_kprintf("\tinode\t\t%d\n", stat.st_ino);
    rt_kprintf("\tmode\t\t%d\n", stat.st_mode);
    rt_kprintf("\tnlink\t\t%d\n", stat.st_nlink);
    rt_kprintf("\tsize\t\t%d\n", stat.st_size);
    rt_kprintf("\tblksize\t\t%d\n", stat.st_blksize);
    rt_kprintf("\tblocks\t\t%d\n", stat.st_blocks);
}
static int ubifs_test_write_file(char *fileName, int justcheck)
{
    struct dfs_fd fd;
    uint32_t toWriteLength = 100;
    uint32_t length;
    uint32_t checkLength = 200;
    if (justcheck == 0)
    {
        rt_kprintf("test write 1 page data,one part: %d .%d,%d \n", 100, O_RDWR, O_WRONLY);
        if (dfs_file_open(&fd, fileName, O_RDWR) < 0)
        {
            rt_kprintf("Open %s failed\n", fileName);
            return -1;
        }
        length = dfs_file_write(&fd, MLK_Speech, toWriteLength);
        length += dfs_file_write(&fd, MLK_Speech + length, toWriteLength);
        rt_kprintf(" write %d bytes\n", length);
        dfs_file_close(&fd);
        if (checkLength != length)
        {
            rt_kprintf("checklength error.%d,%d\n", checkLength, length);
            checkLength = length;
        }
    }
    ubifs_test_checkdata(fileName, MLK_Speech, checkLength);
    return 0;
}

extern int mkdir(const char *path, mode_t mode);
static void ubifs_test_createnode(const char *rwflag)
{
    char path[256] = {0};
    int i = 0;
    if (rwflag[0] == 'w')
    {
        strcpy(path, "nodes");
        mkdir(path, 777);
        while (i < 100000)
        {
            sprintf(path, "nodes/nodetest%d", i);
            if (mkdir(path, 777) != 0)
            {
                rt_kprintf("create dir failed, %d nodes\n", i * 2);
                break;
            }
            sprintf(path, "nodes/nodetest%d/test.txt", i);
            if (ubifs_test_write_file(path, 0) != 0)
            {
                rt_kprintf("write file failed, %d nodes\n", i * 2);
                break;
            }
            usleep(100);
            i++;
        }
        rt_kprintf("create %d nodes\n", i * 2);
    }
    else
    {
        while (i < 100000)
        {
            sprintf(path, "nodes/nodetest%d/test.txt", i);
            if (ubifs_test_write_file(path, 1) != 0)
            {
                rt_kprintf("check file failed, %d nodes\n", i * 2);
                break;
            }
            i++;
        }
    }
}

int cmd_ubifs_nodetest(int argc, char **argv)
{
    if (argc == 2)
    {
        ubifs_test_createnode(argv[1]);
    }
    else
    {
        ubifs_test_createnode("0");
    }
    return 0;
}
SHELL_CMD_EXPORT(cmd_ubifs_nodetest, test ubifs nodenum);
static void ubifs_test_dir(const char *rwflag)
{
    if (rwflag[0] == 'w')
    {
        char path[256] = {0};
        char *name = TEST_DIR1;
        char *next;
        int pos = 0;
        while (*name == '/')
            name++;

        if (!name || *name == '\0')
        {
            rt_kprintf("no sub path?\n");
            return;
        }
        strcpy(path, "/");
        pos = 1;
        while (1)
        {
            next = strchr(name, '/');
            if (next)
            {
                int length = next - name + 1;
                memcpy(path + pos, name, length);
                printf("%s\n", path);
                mkdir(path, 777);
                pos += length;
                /* Remove all leading slashes.  */
                name = next;
                while (*name == '/')
                    name++;
            }
            else
            {
                break;
            }
        }
        ubifs_test_write_file(TEST_DIR1 "/test.txt", 0);
    }
    else
    {
        ubifs_test_write_file(TEST_DIR1 "/test.txt", 1);
    }
}

int cmd_ubifs_dirtest(int argc, char **argv)
{
    if (argc == 2)
    {
        ubifs_test_dir(argv[1]);
    }
    else
    {
        ubifs_test_dir("w");
    }
    return 0;
}
SHELL_CMD_EXPORT(cmd_ubifs_dirtest, test ubifs dir);

int cmd_ubifs_filetest(int argc, char **argv)
{
    /* nt index; */
    extern void ubifs_test(const char *rw, char *dir);

    if (argc != 3)
    {
        rt_kprintf("Usage: ubifs_filetest [w/r] dir\n");
        return 0;
    }
    /* for (index = 1; index < argc; index++) */
    {
        ubifs_test(argv[1], argv[2]);
    }
    return 0;
}
SHELL_CMD_EXPORT(cmd_ubifs_filetest, test file write and read);
#include <sys/time.h>
int cmd_ubifs_rwspeedtest(int argc, char **argv)
{
    struct dfs_fd fd;
    uint32_t onePieceLength = 4096;
    unsigned long length = 0;
    unsigned long mCount = 10 * 1024 * 1024 / onePieceLength;
    uint32_t i = 0;
    int ret = 0;
    char *fileName = TEST_DIR "test_speed.txt";
    if (argc == 1)
    {
        rt_kprintf("Usage: ubifs_rwspeedtest r/w\n");
        return 0;
    }
    if (dfs_file_open(&fd, fileName, O_RDWR) < 0)
    {
        rt_kprintf("Open %s failed\n", fileName);
        return -1;
    }
    struct timeval time1, time2;
    gettimeofday(&time1, NULL);
    if (argv[1][0] == 'w')
    {
        while (i < mCount)
        {
            ret = dfs_file_write(&fd, MLK_Speech, onePieceLength);
            if (ret <= 0)
            {
                rt_kprintf("write error,ret=%d,i=%d,writeCount=%d\n", ret, i, mCount);
                break;
            }
            length += ret;
            i++;
        }
        rt_kprintf("write speed: ");
    }
    else
    {
        char tmp[onePieceLength];
        while (i < mCount)
        {
            ret = dfs_file_read(&fd, tmp, onePieceLength);
            if (ret <= 0)
            {
                rt_kprintf("read error,ret=%d,i=%d,read=%d\n", ret, i, mCount);
                break;
            }
            length += ret;
            i++;
        }
        rt_kprintf("read speed: ");
    }
    if (length >= onePieceLength)
    {
        gettimeofday(&time2, NULL);
        long usec = (time2.tv_sec - time1.tv_sec) * 1000 * 1000 + (time2.tv_usec - time1.tv_usec); /* ms */
        printf(" %ld(bytes)/%ld(us), %.2f MB/s\n", length, usec, (length >> 20) * 1000.0 * 1000 / usec);

        unsigned long mtd_writel;
        unsigned long mtd_writet;
        unsigned long crc_time;
        rt_kprintf("mtdwrite: Length:%ld,time:%ld\n crcTime:%ld us\n", mtd_writel, mtd_writet, crc_time);
    }
    dfs_file_close(&fd);
    return 0;
}
SHELL_CMD_EXPORT(cmd_ubifs_rwspeedtest, test file read / write speed);
int cmd_ubifs_stat(int argc, char **argv)
{
    int index;
    extern void ubifs_test_filestat(const char *filePath);

    if (argc == 1)
    {
        rt_kprintf("Usage: ubifs_stat filePath\n");
        return 0;
    }
    for (index = 1; index < argc; index++)
    {
        ubifs_test_filestat(argv[index]);
    }
    return 0;
}
SHELL_CMD_EXPORT(cmd_ubifs_stat, file stat);

int cmd_ubi_mkvol(int argc, char **argv)
{
/*    rt_ubi_create_vol_autosize("inand0","wf1",1,-1);*/
    if (argc != 2)
    {
        rt_kprintf("if mounted,umount first,Usage: ubi_mkvol mtdName \n");
        return 0;
    }
    char *mtdName = argv[1];
/*    dfs_unmount("/");*/
    dfs_mkfs("ubifs", mtdName);
    /*char *volName = argv[2];
   char *strVolSize = argv[3];
   char *strVolWrite = argv[4];
   int mVolSize = atoi(strVolSize) * 1024 * 1024;
   int mVolWrite = atoi(strVolWrite);
   glue_ubi_detach_mtd(mtdName);
   glue_ubi_mtd_erase(mtdName);
   glue_ubi_create_vol(mtdName, volName, mVolSize, mVolWrite, -1, 1);
    rt_kprintf("mtd:%s,volName:%s,%d,%d\n", mtdName, volName, mVolSize, mVolWrite);*/
    return 0;
}
SHELL_CMD_EXPORT(cmd_ubi_mkvol, create volume);

int cmd_ubi_nanderaseall(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("Usage: nanderaseall mtdname\n");
        return 0;
    }
    char *mtdName = argv[1];
    return glue_ubi_mtd_erase(mtdName);
}
SHELL_CMD_EXPORT(cmd_ubi_nanderaseall, erase nand mtd);
int cmd_ubi_nandrwspeed(int argc, char **argv)
{
    if (argc != 3)
    {
        rt_kprintf("Usage: nandrwspeed mtdname w/r\n");
        return 0;
    }
    char *mtdName = argv[1];
    void *mtd = glue_ubi_mtd_getdevice(mtdName);
    if (mtd == NULL)
    {
        rt_kprintf("get mtd error\n");
        return -1;
    }
    uint32_t onePieceLength = 2048;
    unsigned long length = 0;
    unsigned long mCount = 10 * 1024 * 1024 / onePieceLength;
    uint32_t i = 0;
    int ret = 0;
    struct timeval time1, time2;
    gettimeofday(&time1, NULL);
    size_t retLent = 0;
    if (argv[2][0] == 'w')
    {
        while (i < mCount)
        {
            ret = glue_ubi_mtd_write(mtd, i * onePieceLength, onePieceLength, &retLent, (unsigned char *)MLK_Speech);
            if (ret < 0)
            {
                rt_kprintf("write error,ret=%d,i=%d,writeCount=%d\n", ret, i, mCount);
                break;
            }
            length += retLent;
            i++;
        }
        rt_kprintf("write speed: ");
    }
    else
    {
        unsigned char tmp[onePieceLength];
        while (i < mCount)
        {
            ret = glue_ubi_mtd_read(mtd, i * onePieceLength, onePieceLength, &retLent, tmp);
            if (ret < 0)
            {
                rt_kprintf("read error,ret=%d,i=%d,read=%d\n", ret, i, mCount);
                break;
            }
            length += retLent;
            i++;
        }
        rt_kprintf("read speed: ");
    }
    if (length >= onePieceLength)
    {
        gettimeofday(&time2, NULL);
        unsigned long msec = (time2.tv_sec - time1.tv_sec) * 1000 + (time2.tv_usec - time1.tv_usec) / 1000; /* ms */
        rt_kprintf(" %d(bytes)/%ld(ms), %d MB/s\n", length, msec, (length >> 20) * 1000 / msec);
    }
    glue_ubi_mtd_putdevice(mtd);
    return ret;
}
SHELL_CMD_EXPORT(cmd_ubi_nandrwspeed, nand rw speed);

int cmd_ubifs_mount(int argc, char **argv)
{
    if (argc != 4)
    {
        rt_kprintf("Usage: ubifs_mount mtdName path ubix_0\n");
        return 0;
    }
    /* har mVolName[256] = { 0 }; */
    char *mtdName = argv[1];
    char *path = argv[2];
    char *vol = argv[3];

    int ret = glue_ubi_attach_mtd(mtdName);
    if (ret == 0)
    {
        ret = dfs_mount(mtdName, path, "ubifs", 0, vol);
        rt_kprintf("mount volName:%s,%s,%s.%d\n", mtdName, vol, path, ret);
    }
    return ret;
}
SHELL_CMD_EXPORT(cmd_ubifs_mount, mount ubifs);

int cmd_ubifs_umount(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("Usage: ubifs_umount path\n");
        return 0;
    }
    /* glue_ubiexit(); */
    char *path = argv[1];
    dfs_unmount(path);
    return 0;
}
SHELL_CMD_EXPORT(cmd_ubifs_umount, unmount ubifs);

int cmd_ubifs_exit(int argc, char **argv)
{
    exit(0);
    return 0;
}
SHELL_CMD_EXPORT(cmd_ubifs_exit, exit);

int cmd_ubifs_trunctest(int argc, char **argv)
{
    struct dfs_fd fd;
    char *fileName = TEST_DIR "test_speed.txt";
    if (dfs_file_open(&fd, fileName, O_TRUNC) < 0)
    {
        rt_kprintf("Open %s failed\n", fileName);
        return 0;
    }
    dfs_file_close(&fd);
    return 0;
}
SHELL_CMD_EXPORT(cmd_ubifs_trunctest, trunc);

int cmd_ubifs_writefull(int argc, char **argv)
{
    struct dfs_fd fd;
    int writedLength = 0;
    char *fileName = TEST_DIR "test_full.txt";
    if (dfs_file_open(&fd, fileName, O_RDWR) < 0)
    {
        rt_kprintf("Open %s failed\n", fileName);
        return 0;
    }
    do
    {
        writedLength = dfs_file_write(&fd, MLK_Speech, sizeof(MLK_Speech));
        if (sizeof(MLK_Speech) != writedLength)
        {
            rt_kprintf("write error.%d,%d\n", sizeof(MLK_Speech), writedLength);
            break;
        }
    } while (1);
    dfs_file_close(&fd);
    return 0;
}

SHELL_CMD_EXPORT(cmd_ubifs_writefull, write file until fs full);

static unsigned char gThreadParam[32] = {0};

void ubifs_multithread_rw(void *param)
{
    unsigned char *p = (unsigned char *)param;
    char dirname[256] = {0};
    printf("-------thread % d run-------\n", *p);
    snprintf(dirname, sizeof(dirname), TEST_DIR "multi/m%d", *p);
    mkdir(dirname, 777);
    ubifs_test_write_1_page(dirname);
    sleep(10);
    ubifs_test_write_2_page(dirname);
    sleep(10);
    ubifs_test_write_all(dirname);
    /* rt_thread_sleep(10); */
    sleep(1);
    ubifs_test_write_pagealign(dirname);
    printf("-------thread %d exit-------\n", *p);
}
int cmd_ubifs_multithread(int argc, char **argv)
{
    rt_thread_t thread;
    int i = 0;
    mkdir(TEST_DIR "multi", 777);
    for (i = 0; i < 5; i++)
    {
        gThreadParam[i] = i;
        thread = rt_thread_create("ubi_thread", ubifs_multithread_rw, &gThreadParam[i],
                                  10 * 1024, 30, 20);
        if (thread != RT_NULL)
            rt_thread_startup(thread);
    }

    return 0;
}

SHELL_CMD_EXPORT(cmd_ubifs_multithread, multi thread test);
