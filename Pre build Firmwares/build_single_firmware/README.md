# Merge arduino output files

In Arduino IDE version2, select 'Export Compiled Binary' under the 'Sketch' menu. The IDE will compile the source code. The binary files can be found in the 'build' subdirectory of the directory revealed using 'Show Sketch Folder'. 

Copy the three '*.bin' files into the '/Pre build Firmwares/build_single_firmware' directory and execute the `make_BP32_LPF2_generic.bat` or 'make_BP32_UartRemote.bat` to generate a single binary file for uploading to the LMS-ESP32.

We use the `merge_bin` option of `esptool`. The python version of esptool gives an error. The executable (copied from the Arduino IDE environment) works correctly under Windows.