0) If you are running this for the first time on a new machine, see the Further Notes at the end of this document.

1) cd into the directory that contains your data. The "Hour_00" folder should be in here. From now on I will call this the "cwd" (which stands for current working directory).
2) Do a git clone on this github repo. Move all the files that were cloned into your cwd (ie move them up one directory). Delete the now-empty folder. You can delete  quocmesh-20140902-1202.tar.bz2 if you have finished the "Further Notes".
3) Modify parameters.txt with the parameters you want.

4) To run the conversion, type this:
   python getImages.py
   This will do a few things:
   a) It will first create a 'tiff/' directory if it doesn't already exist.
   b) Second, it will recursively look through all the directories in your current directory and rename them by replacing
      any spaces with underscores (file will not be renamed, only directories; files are renamed during execution).
      That means any hardcoded directory name you used before may no longer work.
   c) It will then recursively go back through all the directories in your current directory and do the conversion
      on any .dm4 file it finds. It will also rename files by replacing spaces with underscores.
      Note that if you run the program twice,  it will not do the conversion a second time for any file.
      That means you can stop the python program at any time and resume it. If you do want to overwrite previous
      results, you will have to delete the corresponding .q2bz and .tiff files before running getImage.py.
      Be careful that the program is not stopped while writing to a .q2bz or a .tiff; if that happens, you will
      need to manually delete the partially-written file so that it will be overwritten on the next run.

Further Notes:
You will need to untar (`tar xjvf`) and compile the code in quocmesh-20140902-1202.tar.bz2. There are basic compiling instructions in the README.txt inside the tarball.
(The 'make' command takes about 10 minutes but everything else is fast).

Once compiled, copy (or link) libquocmesh.so from 'quocGCC/libquocmesh.so' into a path in your LD_LIBRARY_PATH.
In addition, you need the executable 'convertDM3ToQuoc' which was created during compilation. The file is located in 'quocGCC/tools/image/converter/'.
You must again move, copy, or link this file to a path accessible via your PATH environment variable.
These two files (libquocmesh.so and convertDM3ToQuoc) are needed by getImages.py and its subfiles.
