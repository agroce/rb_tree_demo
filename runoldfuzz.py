import subprocess
import sys

subprocess.call(["make fuzz_rb"], shell=True)
subprocess.call(["./fuzz_rb >& fuzz.result"], shell=True)
with open("fuzz.result",'r') as f:
    for l in f:
        if "Done fuzzing" in l:
            sys.exit(0)
sys.exit(255)
            
        
