#!/bin/bash

echo "*************************************"
echo "SETUP BEGIN ..."
echo "*************************************"

### TEST avec nouvelle install
## BASICS


echo "*************************************"
echo "INFLUXDB INSTALLATION ..."
echo "*************************************"
# Create Influx DATABASE
echo "CREATE DATABASE" &&
influx setup --username test --password test --org TamataOcean --bucket dataseed --force &&

#### SEED PILOTE PACKAGE
cd /home/pi/code/SeedPilot/systools &&
npm install

#### ADD SERVICE seedpilot( save_data.js )
echo "ADDING SERVIVE SeedPilot"
sudo cp /home/pi/code/SeedPilot/systools/seedpilot.service /etc/systemd/system/seedpilot.service
sudo systemctl enable seedpilot.service
sudo systemctl start seedpilot.service

echo "PACJAGE DEPLOY END"
#### PACKAGE DEPLOY END ####
