

Siemens 5WG1 117-2AB12

    12345
 _____________
/    ooooo    \    a
|    ooooo    |    b
|             |
|             |
|     KNX     |
\_____________/
     -| |+

Pegelwandler ADUM1201

V1 o .-----. o V2
A0 o |     | o A1
B1 o |     | o B0
G1 o `-----´ o G2   

Siem 5WG1     ADUM1201
a1   +5V   ->   V2
a2         ->   A1
a4         ->   B0
a5   GND   ->   G2

ADUM2101     Arduino
   V1    ->    VCC
   A0    ->    RXD
   B1    ->    TXD
   G1    ->    GND