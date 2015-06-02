1) Do a git clone on your github link. Modify parameters.txt.
2) Create a new directory called 'tiff/' in the current working directory

2) To run the conversion, type this:
   python getImages.py
   This will do a few things:
   a) Firts it will recursively look through all the directories in your current directory and rename them by replacing
      any spaces with underscores (file will not be renamed, only directories; files are renamed during execution).
      That means any hardcoded directory name you used before may no longer work.
   b) It will then recursively go back through all the directories in your current directory and do the conversion
      on any .dm4 file it finds. It will also rename files by replacing spaces with underscores.
      Note that if you run the program twice,  it will not do the conversion a second time for any file.
      That means you can stop the python program at any time and resume it. If you do want to overwrite previous
      results, you will have to delete the corresponding .q2bz and .tiff files before running getImage.py.
      Be careful that the program is not stopped while writing to a .q2bz or a .tiff; if that happens, you will
      need to manually delete the partially-written file so that it will be overwritten on the next run.

	  
	  
TODO:
Explain libquocmesh.so
Git clone into the top level directory, which is the folder wtih Hour_00 in it.