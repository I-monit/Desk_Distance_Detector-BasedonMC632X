chmod 755 gencode.sh
chmod 755 genstring.sh
# gen code from ini file
./gencode.sh
# gen string
./genstring.sh
 python OsdStringCombine.py
