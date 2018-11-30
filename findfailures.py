from __future__ import print_function
import subprocess
import sys
import glob

corpus = sys.argv[1]
fast = "--fast" in sys.argv

cmd = "./ds_rb --input_test_file "

crashes = {}
fatals = {}

build = subprocess.call(["make ds_rb"], shell=True)
if build != 0:
    print ("FAILED TO COMPILE")
    sys.exit(255)
build = subprocess.call(["make ds_rb_lf"], shell=True)
if build != 0:
    print ("FAILED TO COMPILE")
    sys.exit(255)    

for f in glob.glob(corpus + "/*"):
    with open("test.out", 'w') as outf:
        subprocess.call([cmd + f], shell=True, stdout=outf, stderr=outf)
    crashline = None
    fatalline = None
    stepcount = 0
    with open("test.out", 'r') as inf:
        for line in inf:
            if "Crash" in line:
                crashline = line
            if "UndefinedBehaviorSanitizer" in line:
                crashline = line
            if "AddressSanitizer" in line:
                crashline = line
            if "FATAL" in line:
                fatalline = line
            if "STEP" in line:
                stepcount += 1
    if crashline is not None:
        if crashline not in crashes:
            crashes[crashline] = (stepcount, f)
        else:
            if stepcount < crashes[crashline][0]:
                crashes[crashline] = (stepcount, f)
        if fast:
            print("ABORTING AFTER DETECTING ONE FAILURE")            
            break
    if fatalline is not None:
        if fatalline not in fatals:
            fatals[fatalline] = (stepcount, f)
        else:
            if stepcount < fatals[fatalline][0]:
                fatals[fatalline] = (stepcount, f)
        if fast:
            print("ABORTING AFTER DETECTING ONE FAILURE")
            break
for fatal in fatals:
    print (fatal, fatals[fatal])
for crash in crashes:
    print (crash, crashes[crash])

print (len(fatals), "FATALS", len(crashes), "CRASHES")
if len(fatals) > 0:
    sys.exit(255)
if len(crashes) > 0:
    sys.exit(255)
sys.exit(0)
