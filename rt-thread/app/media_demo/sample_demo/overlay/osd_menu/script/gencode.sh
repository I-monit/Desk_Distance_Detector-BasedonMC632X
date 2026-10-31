# gen code
# menu_create.py <input_ini> <output folder>

python3 menu_create.py ./in/main.ini ./menu ./callback
python3 menu_create.py ./in/ae.ini ./menu ./callback
python3 menu_create.py ./in/awb.ini ./menu ./callback
python3 menu_create.py ./in/dayNight.ini ./menu ./callback
python3 menu_create.py ./in/imageEnhance.ini ./menu ./callback
python3 menu_create.py ./in/videoOutput.ini ./menu ./callback
python3 menu_create.py ./in/sleep.ini ./menu ./callback
python3 menu_create.py ./in/confirm.ini ./menu ./callback
cp ./configure.c ./menu/configure.c
cp ./configure.h ./menu/configure.h
