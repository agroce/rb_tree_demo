from __future__ import print_function
import subprocess
import sys

subprocess.call(["make fuzz_rb"], shell=True)
with open("fuzz.result",'w') as soutf:
    with open("fuzz.error",'w') as serrf:    
    subprocess.call(["./fuzz_rb"], shell=True, stdout=soutf, stderr=serrf)
with open("fuzz.result",'r') as f:
    for l in f:
        print(l,end="")
        if "Done fuzzing" in l:
            sys.exit(0)
sys.exit(255)
            
        
