from __future__ import print_function
import subprocess
import sys

subprocess.call(["make fuzz_rb"], shell=True)
with open("fuzz.result",'w') as f:
    subprocess.call(["./fuzz_rb"], shell=True, stdout=f, stderr=f)
with open("fuzz.result",'r') as f:
    for l in f:
        print(l,end="")
        if "Done fuzzing" in l:
            sys.exit(0)
sys.exit(255)
            
        
