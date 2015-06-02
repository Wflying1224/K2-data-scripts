0) cd into the directory that contains your data. The "Hour_00" folder should be in here. From now on I will call this the "cwd" which stands for current working directory.
1) Do a git clone on your github link. Move all the files that were cloned into the folder that was created into your cwd (ie move them up one directory). Delete the now-empty folder.
2) Modify parameters.txt with the parameters you want.

3) To run the conversion, type this:
   python getImages.py
   This will do a few things:
   a) It will first create a 'tiff/' directory if it doesn't already exist.
   a) Second, it will recursively look through all the directories in your current directory and rename them by replacing
      any spaces with underscores (file will not be renamed, only directories; files are renamed during execution).
      That means any hardcoded directory name you used before may no longer work.
   b) It will then recursively go back through all the directories in your current directory and do the conversion
      on any .dm4 file it finds. It will also rename files by replacing spaces with underscores.
      Note that if you run the program twice,  it will not do the conversion a second time for any file.
      That means you can stop the python program at any time and resume it. If you do want to overwrite previous
      results, you will have to delete the corresponding .q2bz and .tiff files before running getImage.py.
      Be careful that the program is not stopped while writing to a .q2bz or a .tiff; if that happens, you will
      need to manually delete the partially-written file so that it will be overwritten on the next run.

Further notes:
You will need to compile the code in this repository as well:  https://github.com/ayankovich/NonRigid_Registration
Compiling instructions can be found in the README.txt in that repository (the 'make' command takes about 10 minutes).
Once compiled, copy libquocmesh.so from 'NonRigid_Registration/quocGCC/libquocmesh.so' into your cwd, overwriting the one that was downloaded in the repository.
Copying this libquocmesh.so is only necessary if this file does not run on your system/architecture.

There is another file that is used which is not included in this repository but is also created during the compilation of the non-rigid registration code.
That file is called 'convertDM3ToQuoc' and is in the directory 'NonRigid_Registration/quocGCC/tools/image/converter/' after compiling.
It is assumed that this file is accessible via your PATH environment variable, or that you have copied/linked it to the cwd.
NOT TRUE! NEED TO FIGURE THIS OUT!