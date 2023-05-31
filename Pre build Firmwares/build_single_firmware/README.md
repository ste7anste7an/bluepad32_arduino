# Merge arduino output files

The files you find in this directory are obtained from the Arduino IDE. in Arduino IDE go to Preferences and select 'show verbose output during' compilation and upload. Now you can see where the different binary files can be found. the batch files `make_BP32_UartRemote.bat`, `make_BP32_LPF2_LEGO.bat` and `make_BP32_ULPF_Pybricks.bat` create single-file firmwares for each of the platforms. These single-file firmware are placed in the directory one level up.

We use the `merge_bin` option of `esptool`. The python version of esptool gives an error. The executable (copied from the Arduino IDE environment) works correctly.