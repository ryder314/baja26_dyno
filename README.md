# baja26_dyno
All files related to the motor dyno built by/for Texas A&M's Baja SAE 2026

The motor dyno makes use of a mechanical hydraulic pump connected to the
engine, which pumps hydraulic fluid around a tank with a restrictor controlled
by a stepper motor.
The designed PCB makes use of an XFW-HX711 module to read a load cell, an A4988
stepper driver for controlling flow restriction, as well as up to two RPM sensors
for engine RPM and eventually for primary/secondary sheave RPM when testing with
the e-CVT.
The design uses a Luckfox Lyra as the devboard to process and store data. This
board runs Buildroot Linux and handles all reading of inputs, calculation of
outputs, and storage/display of results.

![alt text](https://github.com/ryder314/baja26_dyno/blob/fa81208130a97caf0380c46af1897c0dbfd13af9/Pictures/3d_pcb_view.png "3D View of the PCB for the Motor Dyno")
