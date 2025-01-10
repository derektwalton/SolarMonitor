This repo contains the code for a UDOO dual based "solar" monitoring system, and associated
sub components for Arduino Nano based radiant controller and Arduino NodeMCU 1.0 (ESP-12E Modules)
based greenhouse and seedling room controllers.

(1) Setup of UDOO miniSD image with Solar Monitor

(a) Create UDOO miniSD image with patched ch341 driver (see below)
(b) Clone https://github.com/derektwalton/SolarMonitor to /home/udooer
(c) Build monitor binary (cd SolarMonitor/monitor; make)
(d) Build plot / data extraction binary (cd ../plot; make)
(e) Install web page stuff
    1. Move UDOO builtin web control panel to HTML 8080
       browse to 192.168.100.101/settings/advanced and set web control panel to 8080
    2. Install Apache
       sudo apt install apache2
       sudo ln -s /etc/apache2/mods-available/cgi.load /etc/apache2/mods-enabled/
    3. Install gnuplot
       sudo apt-get install gnuplot-x11
    4. Copy webpages to correct locations
       sudo cp html/* /var/www/html
       sudo cp cgi-bin/* /usr/lib/cgi-bin/
       sudo chmod +x /usr/lib/cgi-bin/*
    5. Adjust permissions so www-data can create files in plot directory
       chmod o+w plot 




(2) UDOO_ch341_driver

Describes the processing of getting a miniSD card with the UDOO image and appriately patched
CH341 drivers needed to commincate with Chinese knock off Arduino Nano.

(3) Arduino/RadiantController

Arduino form of radiant controller code for Nano.



