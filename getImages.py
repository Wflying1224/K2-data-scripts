import sys, os, glob
from subproc import run_subproc
from getImage import dm4_to_tiff

def remove_directory_spaces(path):
    """ Remove spaces in all directories. """
    fixed = True
    while(fixed):
        fixed = False
        for dirpath, dirnames, filenames in os.walk(path):
            if(' ' in dirpath):
                os.rename(dirpath,dirpath.replace(' ','_'))
                print("Renamed directory {0} to {1}".format(dirpath,dirpath.replace(' ','_')))
                fixed = True
                break

def submit_job(fullfilename,x1,y1,x2,y2,bin):
    """ Submit a job using the necessary information. """
    run_subproc('qsub submit.sh {0} {1} {2} {3} {4} {5}'.format(fullfilename,x1,y1,x2,y2,bin))

def main():
    """ This is the main program you will run. """
    path = sys.argv[1]
    remove_directory_spaces(path)
    with open('parameters.txt') as f:
        x1 = int(f.readline().strip().split()[0])
        y1 = int(f.readline().strip().split()[0])
        x2 = int(f.readline().strip().split()[0])
        y2 = int(f.readline().strip().split()[0])
        bin = int(f.readline().strip().split()[0])
    for dirpath, dirnames, filenames in os.walk(path):
        for f in filenames:
            if('.dm4' in f):
                fullfilename = os.path.join(dirpath,f)
                #submit_job(fullfilename, x1, y1, x2, y2, bin)
                dm4_to_tiff(fullfilename, x1, y1, x2, y2, bin)
                #run_subproc('python getImage.py {0} {1} {2} {3} {4} {5}'.format(fullfilename,x1,y1,x2,y2,bin),True)
                #return 0
        


if __name__ == '__main__':
    main()
