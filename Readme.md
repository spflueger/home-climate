# Efficient and multi probe home climate station

## Goals:

1. The system should be able to tell with high confidence when its beneficial to open windows (in wintertime) to reduce the relative humidity inside the apartment/house.
2. The system should be able to tell when mold can appear inside the building. Walls will become wet due to temperatures below the dewpoint.

## Requirements:

1. Measure dew point ($< 1\degree C$) and absolute humidity ($g/m^3$). This translates to accuracies of the temperature and humidity measurements of for example $0.5\degree C$ and $2\%$ (see [Table]([#Dew-point-systematic-error-estimation]) below)
2. Low maintenance (avoid many battery renewals)
3. Collect data over variable time frame (at least 1 month)
4. Allow plug and play of new measurement stations
5. Allow easy recalibration of measurement stations
   -  via socket, so chips can easily be removed for reprogramming
   -  measurement stations can also receive data (e.g. calibration data).
  
   The first option is easier as it works with the simple unidirectional communication of the measurement stationsThis puts way more logic on the measurement stations.

6. Generally: low as possible power usage (also to 2.)


### Dew point systematic error estimation

Table shows systematic errors on dewpoints in $\degree C$ as a function of the
temperature (vertical in $\degree C$) and relative humidity (horizontal in $\%$) accuracies. The calculation was made at a temperature of $20\degree C$ and a humidity of  $50\%$ RH:

|         |  0.5 |  1.0 |  1.5 |  2.0 |  2.5 |  3.0 |  3.5 |  4.0 |  4.5 |  5.0 |
|---------|------|------|------|------|------|------|------|------|------|------|
| **0.1** | 0.24 | 0.39 | 0.54 | 0.69 | 0.84 | 0.98 | 1.13 | 1.28 | 1.43 | 1.58 |
| **0.2** | 0.33 | 0.48 | 0.63 | 0.78 | 0.93 | 1.08 | 1.22 | 1.37 | 1.52 | 1.67 |
| **0.3** | 0.42 | 0.57 | 0.72 | 0.87 | 1.02 | 1.17 | 1.32 | 1.47 | 1.61 | 1.76 |
| **0.4** | 0.52 | 0.67 | 0.81 | 0.96 | 1.11 | 1.26 | 1.41 | 1.56 | 1.71 | 1.85 |
| **0.5** | 0.61 | 0.76 | 0.91 | 1.05 | 1.20 | 1.35 | 1.50 | 1.65 | 1.80 | 1.95 | 
| **0.6** | 0.70 | 0.85 | 1.00 | 1.15 | 1.30 | 1.44 | 1.59 | 1.74 | 1.89 | 2.04 |
| **0.7** | 0.79 | 0.94 | 1.09 | 1.24 | 1.39 | 1.54 | 1.68 | 1.83 | 1.98 | 2.13 |
| **0.8** | 0.88 | 1.03 | 1.18 | 1.33 | 1.48 | 1.63 | 1.78 | 1.93 | 2.07 | 2.22 |
| **0.9** | 0.98 | 1.13 | 1.27 | 1.42 | 1.57 | 1.72 | 1.87 | 2.02 | 2.17 | 2.31 |
| **1.0** | 1.07 | 1.22 | 1.37 | 1.51 | 1.66 | 1.81 | 1.96 | 2.11 | 2.26 | 2.41 |
| **1.1** | 1.16 | 1.31 | 1.46 | 1.61 | 1.76 | 1.90 | 2.05 | 2.20 | 2.35 | 2.50 |
| **1.2** | 1.25 | 1.40 | 1.55 | 1.70 | 1.85 | 2.00 | 2.14 | 2.29 | 2.44 | 2.59 |


The dewpoint is also related to the absolute humidity and therefore a vital quantity for the achievement of the goals. It becomes clear, that the temperature has to be measured at least with $1\degree C$ accuracy (but also precision). Otherwise no confident estimates for when to open the window or when walls become wet/moldy can be given.
[TODO: calculate confidence]

## Design choices:

- Which sensors to use? 
  - BME-280
  - DHT-22
  - **HDC1080**
  - SHT21
  
  Reasoning: The HDC offers best accuracy and precision @ low power consumption.

- How to transfer data from measurement stations to Raspberry Pi?
  - Wifi: Wifi is easy to use, but has high power needs. Also cannot send data if the wifi is off.
  - **433 Mhz radio modules**: Lower power consumption and can always transfer data. But: More work to get running. Also bi-directional and interference free communication is more difficult to achieve. 

  Reasoning: Measurement stations should run as long as possible on battery. Bi-directional communication is not needed. Different measurement stations can be timed slighly different to avoid sending overlaps. This [project\[1\]][1] has achieved very long runtimes together with a low power MC like the ATtiny85.

- How to store data?

  Use a Raspberry Pi or similar board with a wifi module. During night the wifi can be off and data can still be accumulated via the radio transmission. The Pi can store the data and also host a small server to provide a dashboard for visualization.
    - Power Consumption Raspberry Pi Zero W: $100\mathrm{mA} * 5.2\mathrm{V} = 520\mathrm{mW}$
  

- Which radio modules to use?
  
  The main problem here is having a good receiver, as the transmitters all work with similar intensities (that also should not be boosted...). Other [blogs\[3\]][3] which tested several receivers had exceptional results with the **RBX8 receiver** module (reception through several walls over 10m w/o antenna!)

- Which station chips to use?
  
  -  **ATtiny85** (2.7-5V):
     - low power
     - cheap
     - has ADC and I$^2$C
  
     Reasoning: inspired by this [project\[1\]][1]. 


## Calibration of Humidity Sensors:

[Link][2]

## Technical Notes:

- To improve signal to noise use coded radio transmission. Sacrifice bandwidth for signal to noise improvements.
- Every 2min send data from outside - 1sec -> for two sensors its an overlap every 4 hours, which means no data
- Send battery status every 1h?

## Data Storage and Dashboard:

I used [Graphite\[4\]][4] for data aggregation and the open source variant of [Grafana\[5\]][5] for visualization.

One measurement station records a temperature and humidity value every 2min. This corresponds roughly to a data size of 21 mb in 10 years. In reality graphite (actually carbon) will reduce the storage space by a factor of more than 6, due to the retention settings I defined below. So running this system with several sensors is also a breeze.

Both applications run via docker on a Raspberry Pi 3 Model B Rev 1.2. The setup is rather straight forward, but below are my tips and tricks that might save you a little time.

### Setup Notes:

- Use docker for graphite and grafana (easiest install)
- I changed the default data retention settings in `storage-schema.conf` (`/opt/graphite/conf` in the docker container) from 
  ```
  [default_1min_for_1day]
  pattern = .*
  retentions = 10s:6h,1m:6d,10m:1800d
  ```
  to
  ```
  [default_2min_for_2days]
  pattern = .*
  retentions = 2m:2d,4m:8d,12m:2y,1h:10y
  ```
  Note that the next higher frequency has to be multiple of the previous one in order for the data aggregation to work accurately. So $2*2m = 4m$, $3*4m = 12m$ etc.
- Add scripts to crontab (`crontab -e`) to automatically start everything once the system is booted. Next to the two docker containers thats also the code reading the sensor and the radio receiver. `nohup` can be useful when spawning the scripts.
- To avoid the login in the Grafana to see the dashboard an anonymous user can be created. I created a new user with viewer privileges and a simpler password. That solution is easier.

### Metrics:

The metrics available through graphite are humidities and temperatures. However its possible to create new metrics from those basic metrics via transformations. Specifically I'm interested in

- **Dewpoints**: To detect when mold can appear on wall inside the apartment, since the dewpoint shows below which temperature water is condensating (together with the temperature on the coldest wall part inside the apartment). The formula to use to calculated the dewpoint is [Magnus formula\[7\]][7]
  ```python3
  alias(
    divideSeries(
      sumSeries(
        divideSeries(
          scale(kitchen.temperature, 17.62), 
          offset(kitchen.temperature,243.12)
        ), 
        log(scale(kitchen.humidity, 0.01))
      ), 
      sumSeries(
        scale(invert(offset(kitchen.temperature, 243.12)), 17.62), 
        scale(log(scale(kitchen.humidity, 0.01)),-0.004113195)
      )
    ),
    "kitchen"
  )
  ```
- **Absolute humidities**: To prevent moldy walls, by reducing the humidity inside the apartment via air exchange from outside. That requires to measure the absolute humidities inside and outside. Of course this also has to be energy efficient, in the sense that we do not want the windows to be open all the time and cool down the apartment unnecessarily. Hence there is a sweet spot when to open windows and when not. Weather forecast can also be taken into account here, but this is next level for now.

## References:

- [diy-funk-wetterstation-mit-dht22-attiny85-und-radiohead][1]
- [humidity-calibration][2]
- [wolles-elektronikkiste][3]
- [graphite][4]
- [grafana][5]
- [coding-theory][6]
- [magnus-formula][7]

[1]: https://crycode.de/diy-funk-wetterstation-mit-dht22-attiny85-und-radiohead
[2]: https://www.s-elabor.de/k00002.html
[3]: https://wolles-elektronikkiste.de/433-mhz-funk-mit-dem-arduino
[4]: https://graphiteapp.org/
[5]: https://grafana.com/
[6]: https://en.wikipedia.org/wiki/Coding_theory
[7]: https://en.wikipedia.org/wiki/Dew_point#Calculating_the_dew_point






