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

fastOption = ""
if "--fast" in sys.argv:
    fastOption = " --fast"

build = subprocess.call(["make ds_rb"], shell=True)
if build != 0:
    print ("FAILED TO COMPILE")
    sys.exit(255)
build = subprocess.call(["make ds_rb_lf"], shell=True)
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

r = subprocess.call(["python findfailures.py newcorpus " + fastOption], shell=True)
sys.exit(r)

