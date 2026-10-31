#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <rttshell.h>

void do_rmdir(char *name)
{
    DIR *fdir;
    struct stat finfo;
    struct dirent *edir;

    stat(name, &finfo);
    if (S_ISREG(finfo.st_mode))
    {
        unlink(name);
        return;
    }
    else if (S_ISDIR(finfo.st_mode))
    {
        fdir = opendir(name);
        if (fdir == NULL)
        {
            printf("DIR(%s) not exists.\n", name);
            return;
        }
        chdir(name);
        while ((edir = readdir(fdir)) != NULL)
        {
            if (strcmp(edir->d_name, ".") == 0)
                continue;
            if (strcmp(edir->d_name, "..") == 0)
                continue;

            struct stat stinfo;
            stat(edir->d_name, &stinfo);
            if (S_ISDIR(stinfo.st_mode))
            {
                do_rmdir(edir->d_name);
            }
            else
            {
                unlink(edir->d_name);
            }
        }
        closedir(fdir);
        chdir("..");
        unlink(name);
    }
    else
    {
        printf("Invalid file name or path: %s.\n", name);
    }
}
