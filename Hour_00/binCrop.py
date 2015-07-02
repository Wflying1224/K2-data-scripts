import fileinput, bz2
#from PIL import Image
import numpy as np

#binCrop crops and image and bins it.
#x1 and y1 are the coordinates of the top left corner to be cropped
#x2 and y2 are the coordinates of the bottom right corner
#bin is the binning number that the user would like done
#image is an image file in bz2, or .q2bz file format
def binCrop(image, x1, y1, x2, y2,bin):
    """ This function crops and bins the image, loaded from a .q2bz file. """

#first, the .q2bz file is read into 'f'
    f = bz2.BZ2File(image, 'r')

    #the first 3 lines of the file are read off as they are text information
    l=f.readline()
    l=f.readline()
    l=f.readline()
    #the third line contains pixel information, so that is read into the program
    words = l.split(" ")
    xsize = int(float(words[0]))
    ysize = int(float(words[1]))
    l=f.readline() 

    #the remaining info is read into a file, this is the image
    myF = f.read()
    #a numpy array is created, but is initially 1D
    data = np.frombuffer(myF, dtype = float)
    #based on the xsize that is given in the image header, the 2d image array is created
    image = np.reshape(data, (-1,xsize))
    #the image is then cropped according to what was given as input
    cropped = image[y1:y2, x1:x2]    

    #this next bit of code does the binning according to the given bin number
    #I did not have time to add error messages if the numbers are off, I would 
    #imagine the program would just quit out
    M,N = cropped.shape
    m = M/bin
    n = N/bin
    sh = m,cropped.shape[0]//m,n,cropped.shape[1]//n
    image = cropped.reshape(sh).mean(-1).mean(1)

    #the next two lines should save the resulting cropped and binned image as a .tiff file if PIL is installed
    #im = Image.fromarray(image)
    #im.save("firstsave.tiff")
    return image
