# ElectricMotorcycle

This was my Electronics Engineering Technology capstone project. 

A fully-electric dirtbike, built using an Arduino Mega.

### Key Features:
 * Fully-custom analog sensing boards, using OPAMPs and voltage protection for the Arduino
 * Custom firmware, written from scratch'
 * Electrical systems all designed by hand, with current safeties, and ALL IO is isolated in and out of the processor, to remove electrical noise.
 * Modular removable panels, with quick disconnects. Each panel comes off with four screws, and at most three connectors
 * Fully custom charging circuit and battery management
 * Touchscreen HMI, displaying telemetry and hosting a few settings, and a faults window
 * LOTS of faults, for communications, voltage, current, temperatures, safeties
 * Info prompts for when a safety stops the system from going into 'D' (drive)
 * Safety systems, disconnects on any major faults; main safety relay disconnects by a hardware-controlled switch, in case of processor lock-up
 * All lights and features required to make the bike road-legal
