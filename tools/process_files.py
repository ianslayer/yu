import os
import sys
import subprocess

if len(sys.argv) < 2:
    print "please specify path"
    sys.exit(0)

path=sys.argv[1]

hasCmd = 0
if(len(sys.argv) > 2):
    cmdFile=sys.argv[2] 
    hasCmd = 1
    
for subdir, dirs, files in os.walk(path):
    for file_name in files:
        full_path = subdir + "\\" + file_name
        if(hasCmd):
            subprocess.call(cmdFile + " " + full_path, shell=True)
        else:
            print full_path
