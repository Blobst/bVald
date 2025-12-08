# Building on Linux
## Requirements
  + clang-18 and above


First Run 'make shared-Linux'.

Then cp the 'libjsonval.so' to /usr/local/lib/ with 'sudo' so that the program knows where it is  'sudo cp lib/libjsonval.so /usr/local/lib'.

After that run 'sudo ldconfig' idk why but it works.

Then 'make'.
