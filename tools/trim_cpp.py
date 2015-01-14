import os
import sys
import re

if len(sys.argv) < 2:
    print "please specify file"
    sys.exit(0)
    
file=sys.argv[1]

matchHeader = re.match('(.*)\.h', file)
if(matchHeader):
    print "reserve file:"
    print file
else:
    print "trim file:"
    print file
    os.remove(file)

