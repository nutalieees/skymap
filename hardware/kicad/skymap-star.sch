EESchema Schematic File Version 4
LIBS:power
LIBS:device
LIBS:Connector_Generic
EELAYER 29 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "SkyMap Star — preliminary schematic"
Date "2026-09-17"
Rev "0.1"
Comp "SkyMap"
Comment1 "PRELIMINARY: replace generic module connectors with exact parts before PCB layout"
Comment2 "GPS UART pins are proven: GPS TX -> GPIO16, GPS RX <- GPIO17"
Comment3 "All logic rails are 3.3 V. LiPo is a single protected cell."
Comment4 "No PCB release until selected GPS/display/battery mechanical data is attached."
$EndDescr
Text Notes 700 700 0    120  ~ 24
POWER INPUT, CHARGING, AND 3.3 V RAIL
Text Notes 700 3850 0    120  ~ 24
ESP32-C3, DISPLAY, AND GPS MODULE INTERCONNECT
$Comp
L Connector_Generic:Conn_01x02 J1
U 1 1 66000001
P 1100 1550
F 0 "J1" H 1018 1767 50  0000 C CNN
F 1 "USB-C 5V input" H 1018 1676 50 0000 C CNN
	1    1100 1550
	-1   0    0    -1
$EndComp
$Comp
L Device:Fuse F1
U 1 1 66000002
P 1900 1450
F 0 "F1" V 1703 1450 50 0000 C CNN
F 1 "PTC 1A" V 1794 1450 50 0000 C CNN
	1    1900 1450
	0    1    1    0
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J2
U 1 1 66000003
P 5200 1550
F 0 "J2" H 5280 1542 50 0000 L CNN
F 1 "LiPo 1S JST-SH" H 5280 1451 50 0000 L CNN
	1    5200 1550
	1    0    0    -1
$EndComp
$Comp
L Connector_Generic:Conn_01x05 U2
U 1 1 66000004
P 3300 1450
F 0 "U2" H 3218 1867 50 0000 C CNN
F 1 "LiPo charger + power path" H 3218 1776 50 0000 C CNN
	1    3300 1450
	-1   0    0    -1
$EndComp
$Comp
L Connector_Generic:Conn_01x03 U3
U 1 1 66000005
P 4300 1450
F 0 "U3" H 4380 1492 50 0000 L CNN
F 1 "3.3V regulator >=600mA peak" H 4380 1401 50 0000 L CNN
	1    4300 1450
	1    0    0    -1
$EndComp
$Comp
L Device:C C1
U 1 1 66000006
P 2400 2100
F 0 "C1" H 2515 2146 50 0000 L CNN
F 1 "10uF / 10V" H 2515 2055 50 0000 L CNN
	1    2400 2100
	1    0    0    -1
$EndComp
$Comp
L Device:C C2
U 1 1 66000007
P 4700 2100
F 0 "C2" H 4815 2146 50 0000 L CNN
F 1 "10uF / 6.3V" H 4815 2055 50 0000 L CNN
	1    4700 2100
	1    0    0    -1
$EndComp
$Comp
L power:GND #PWR01
U 1 1 66000008
P 1500 2350
F 0 "#PWR01" H 1500 2100 50 0001 C CNN
F 1 "GND" H 1505 2177 50 0000 C CNN
	1    1500 2350
	1    0    0    -1
$EndComp
Text Label 1450 1450 0    50   ~ 0
USB_5V
Text Label 2200 1450 0    50   ~ 0
USB_PROTECTED
Text Label 3900 1250 0    50   ~ 0
VBAT
Text Label 3900 1350 0    50   ~ 0
SYS_3V3
Text Label 4700 1250 0    50   ~ 0
3V3
Wire Wire Line
	1200 1450 1750 1450
Wire Wire Line
	2050 1450 3000 1450
Wire Wire Line
	3500 1250 4100 1250
Wire Wire Line
	3500 1350 4100 1350
Wire Wire Line
	5000 1450 4700 1450
Wire Wire Line
	1200 1550 1500 1550
Wire Wire Line
	1500 1550 1500 2350
Wire Wire Line
	3500 1550 3700 1550
Wire Wire Line
	3700 1550 3700 2350
Wire Wire Line
	5000 1550 5000 2350
Wire Wire Line
	2400 1950 2400 1450
Wire Wire Line
	2400 2250 2400 2350
Wire Wire Line
	4700 1950 4700 1450
Wire Wire Line
	4700 2250 4700 2350
Text Notes 900 2750 0 60 ~ 12
J1 represents the USB-C 5V/GND output after CC/ESD circuitry. Add USB-C CC pull-downs, ESD, and a real charger/power-path IC when the charge current and LiPo are selected.
Text Notes 900 3000 0 60 ~ 12
Do not use a bare charger without load sharing: the map must run correctly while USB is connected and the battery is charging.
$Comp
L Connector_Generic:Conn_01x12 U1
U 1 1 66000010
P 3300 5100
F 0 "U1" H 3218 5817 50 0000 C CNN
F 1 "ESP32-C3-WROOM-02" H 3218 5726 50 0000 C CNN
	1    3300 5100
	-1   0    0    -1
$EndComp
$Comp
L Connector_Generic:Conn_01x07 J3
U 1 1 66000011
P 6750 4650
F 0 "J3" H 6830 4692 50 0000 L CNN
F 1 "GC9A01 display module" H 6830 4601 50 0000 L CNN
	1    6750 4650
	1    0    0    -1
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J4
U 1 1 66000012
P 6750 5750
F 0 "J4" H 6830 5742 50 0000 L CNN
F 1 "UART GPS module" H 6830 5651 50 0000 L CNN
	1    6750 5750
	1    0    0    -1
$EndComp
$Comp
L Switch:SW_Push SW1
U 1 1 66000013
P 2200 5900
F 0 "SW1" V 2246 5852 50 0000 R CNN
F 1 "BOOT" V 2155 5852 50 0000 R CNN
	1    2200 5900
	0    -1   -1   0
$EndComp
$Comp
L Switch:SW_Push SW2
U 1 1 66000014
P 2600 5900
F 0 "SW2" V 2646 5852 50 0000 R CNN
F 1 "RESET" V 2555 5852 50 0000 R CNN
	1    2600 5900
	0    -1   -1   0
$EndComp
Text Label 3900 4550 0    50   ~ 0
3V3
Text Label 3900 4650 0    50   ~ 0
GND
Text Label 3900 4750 0    50   ~ 0
TFT_SCLK_GPIO4
Text Label 3900 4850 0    50   ~ 0
TFT_MOSI_GPIO6
Text Label 3900 4950 0    50   ~ 0
TFT_CS_GPIO7
Text Label 3900 5050 0    50   ~ 0
TFT_DC_GPIO2
Text Label 3900 5150 0    50   ~ 0
TFT_RST_GPIO3
Text Label 3900 5250 0    50   ~ 0
GPS_RX_GPIO16
Text Label 3900 5350 0    50   ~ 0
GPS_TX_GPIO17
Text Label 3900 5450 0    50   ~ 0
BOOT_GPIO9
Text Label 3900 5550 0    50   ~ 0
EN_RESET
Wire Wire Line
	3500 4550 5000 4550
Wire Wire Line
	3500 4650 5000 4650
Wire Wire Line
	3500 4750 5000 4750
Wire Wire Line
	3500 4850 5000 4850
Wire Wire Line
	3500 4950 5000 4950
Wire Wire Line
	3500 5050 5000 5050
Wire Wire Line
	3500 5150 5000 5150
Wire Wire Line
	3500 5250 5000 5250
Wire Wire Line
	3500 5350 5000 5350
Wire Wire Line
	3500 5450 5000 5450
Text Label 5900 4350 0 50 ~ 0
3V3
Text Label 5900 4450 0 50 ~ 0
GND
Text Label 5900 4550 0 50 ~ 0
TFT_SCLK_GPIO4
Text Label 5900 4650 0 50 ~ 0
TFT_MOSI_GPIO6
Text Label 5900 4750 0 50 ~ 0
TFT_CS_GPIO7
Text Label 5900 4850 0 50 ~ 0
TFT_DC_GPIO2
Text Label 5900 4950 0 50 ~ 0
TFT_RST_GPIO3
Wire Wire Line
	5900 4350 6550 4350
Wire Wire Line
	5900 4450 6550 4450
Wire Wire Line
	5900 4550 6550 4550
Wire Wire Line
	5900 4650 6550 4650
Wire Wire Line
	5900 4750 6550 4750
Wire Wire Line
	5900 4850 6550 4850
Wire Wire Line
	5900 4950 6550 4950
Text Label 5900 5550 0 50 ~ 0
3V3
Text Label 5900 5650 0 50 ~ 0
GND
Text Label 5900 5750 0 50 ~ 0
GPS_TX_GPIO16
Text Label 5900 5850 0 50 ~ 0
GPS_RX_GPIO17
Wire Wire Line
	5900 5550 6550 5550
Wire Wire Line
	5900 5650 6550 5650
Wire Wire Line
	5900 5750 6550 5750
Wire Wire Line
	5900 5850 6550 5850
Text Notes 700 6600 0 60 ~ 12
J3 and J4 are intentionally generic pending exact module part numbers. Confirm pin order and footprint before assigning PCB footprints.
Text Notes 700 6800 0 60 ~ 12
Place the GPS antenna at the top star point; obey its datasheet copper/ground/metal keep-out. Put the ESP32 antenna at a separate board edge.
$EndSCHEMATC
