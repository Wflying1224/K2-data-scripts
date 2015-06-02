import subprocess, shlex, sys

def run_subproc(args,printIt=False):
    """ Run any command line function. """
    # Python 2.7 required (3.4 doesn't work)
    sargs = shlex.split(args)
    if(printIt):
        print(sargs)
    p = subprocess.Popen(sargs,stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output = []
    for nextline in iter(p.stdout.readline,""):
        if(printIt == True):
            sys.stdout.write(nextline)
            sys.stdout.flush()
        output.append(nextline)
    poutput = p.stdout.read()
    perr = p.stderr.read()
    preturncode = p.wait()
    return ''.join(output)


