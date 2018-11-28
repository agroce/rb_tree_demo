from __future__ import print_function
import subprocess
import sys
import glob
import os

corpus = sys.argv[1]
extratime = sys.argv[2]

cmd = "./ds_rb --input_test_file "

crashes = {}
fatals = {}

build = subprocess.call(["make"], shell=True)
if build != 0:
    print ("FAILED TO COMPILE")
    sys.exit(255)

print("STARTING FROM", len(glob.glob(corpus + "/*")), "TESTS")

subprocess.call(["rm -rf crash-*"], shell=True)
subprocess.call(["rm -rf newcorpus"], shell=True)
subprocess.call(["cp -r " + corpus + " newcorpus"], shell=True)
with open("fuzz.out", 'w') as ffile:
    subprocess.call(["./ds_rb_lf newcorpus -detect_leaks=0 -use_value_profile=1 -max_total_time=" + extratime], shell=True,
                        stderr=ffile, stdout=ffile)
subprocess.call("mv crash-* newcorpus", shell=True)
coverages = []
for line in open("fuzz.out"):
    if "cov:" in line:
        coverages.append(int(line.split()[-1]))
if len(coverages) > 0:
    print("COVERAGE CHANGE WITH NEW FUZZING:", coverages[0], "TO", coverages[-1])
print("THERE ARE NOW", len(glob.glob("newcorpus/*")), "TESTS")
corpus = "newcorpus"

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
    if fatalline is not None:
        if fatalline not in fatals:
            fatals[fatalline] = (stepcount, f)
        else:
            if stepcount < fatals[fatalline][0]:
                fatals[fatalline] = (stepcount, f)
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
