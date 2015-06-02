import sys, os, glob
from subproc import run_subproc
from binCrop import binCrop
from PIL import Image

def dm4_to_tiff(fullfilename,x1,y1,x2,y2,bin):
    """ Convert the .dm4 file to a .tiff after cropping and binning. """
    # Initalize some filename variables
    path,f = os.path.split(fullfilename)
    filebasename = f[:-4]
    f = os.path.join(path, f)
    if(' ' in f):
        os.rename(f,f.replace(' ','_'))
        f = f.replace(' ','_')
        filebasename = filebasename.replace(' ','_')
    basename = f[:-4]
    # Convert from .dm4 to .q2bz
    if(not os.path.isfile(basename+'.q2bz') and not os.path.isfile('tiff/{0}.tiff'.format(filebasename))):
        print('Converting {0} to .q2bz...'.format(f))
        run_subproc("convertDM3ToQuoc " + f)
        os.rename(filebasename+'.q2bz',basename+'.q2bz')
    else:
        print('{0}.q2bz already exists!'.format(basename))
    # Convert from .q2bz to .tiff
    if(not os.path.isfile('tiff/{0}.tiff'.format(filebasename))):
        print('Cropping, binning, and saving {0} as .tiff...'.format(f[:-4]+'.q2bz'))
        editedImage = binCrop(f[:-4]+'.q2bz', x1, y1, x2, y2, bin)
        # The next two lines save the resulting cropped and binned image as a .tiff file
        im = Image.fromarray(editedImage)
        im.save('tiff/{0}.tiff'.format(filebasename))
    else:
        print('{0}.tiff already exists!'.format(basename))
    # Delete the .q2bz file to save hard drive space
    #os.remove(basename+'.q2bz')

def main():
    f   = sys.argv[1]
    x1  = int(sys.argv[2])
    y1  = int(sys.argv[3])
    x2  = int(sys.argv[4])
    y2  = int(sys.argv[5])
    bin = int(sys.argv[6])

    dm4_to_tiff(f,x1,y1,x2,y2,bin)


if __name__ == '__main__':
    main()
