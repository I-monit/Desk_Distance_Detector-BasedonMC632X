import os
import sys

if __name__ == "__main__":
    argc = len(sys.argv)
    if argc < 3:
        print("Invalid arguments")
    else:
        f = open("Makefile.app", "a+", encoding='utf-8')
        if sys.argv[1] != "LNKLIBS":
            f.write("\n{} =\n".format(sys.argv[1]))
            for i in range(argc - 2):
                f.write("{} += ".format(sys.argv[1]))
                f.write(sys.argv[i+2])
                f.write("\n")
        else:
            f.write("\nLDFLAGS += -Wl,--start-group\n")
            for i in range(argc - 2):
                f.write("LDFLAGS += ".format(sys.argv[1]))
                f.write(sys.argv[i+2])
                f.write("\n")
            f.write("LDFLAGS += -Wl,--end-group\n\n")
            f.write("all:\n")
            f.write("\t# add your make code here\n")
            f.write("\t# $(CROSS_COMPILE)gcc -c test.c $(CFLAGS) -o test.o\n")
            f.write("\t# $(CROSS_COMPILE)gcc test.o $(LDFLAGS) -o test\n")

        f.close()

