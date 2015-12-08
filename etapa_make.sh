#!/bin/bash
mkdir build   
cd build
cmake -DETAPA_1=OFF -DETAPA_2=OFF -DETAPA_3=OFF -DETAPA_4=OFF -DETAPA_5=OFF -DETAPA_6=ON ..
make
