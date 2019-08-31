# Test Bank 
https://bitbucket.org/mbari/seestar/src/c7686e714579/SeeStar_III/Software/Autonomous/test/

Not every test from MBARI's Repo has been included within this sub-folder. It is UCSB's desire to not include SeeStar's GUI (Graphical User Interface), therefore Arduino sketches testing that functionality will not be found. MBARI's Repository does still have those sketches. 

To be non-repetative an explanation of each test's functionality maybe found on MBARI's repo, however an extensive explanation can be found within each of test sketches.

* Every test should be uploaded, tested, and documented on the SeeStar device. Tests are recommended to be done in order from CamPowerOnOffTest --> WakeUpPeriodicallyTest. This will verify the operational status of each component tested and will ease the testing of the next module.

The usage of a multimeter is recommended to verify the voltage output/input of pins mentioned within each test sketch.

* Before using the SeeStar Autonomous Deployment code certify that: Camera SD-card has been formatted, Camera has properly set up (By manufacturer's guidlines) , and LED is functional.
