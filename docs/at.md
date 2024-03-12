
```at
# https://www.everythingrf.com/community/nb-iot-frequency-bands-in-europe
# https://en.wikipedia.org/wiki/List_of_LTE_networks_in_Europe
# +CNMP: ((2-Automatic),(13-GSM Only),(38-LTE Only),(51-GSM And LTE Only))
# +CMNB: ((1-Cat-M),(2-NB-IoT),(3-Cat-M And NB-IoT))

AT+CNMP=2
AT+CMNB=3
AT+CBANDCFG="CAT-M",1,3,8,20,28
AT+CBANDCFG="NB-IOT",3,8,20
AT+COPS=?
+COPS: (3,"F-Bouygues Telecom","BYTEL","20820",7),(1,"F SFR","SFR","20810",9),(1,"F SFR","SFR","20810",7),(1,"Orange F","Orange","20801",7),(3,"F-Bouygues Telecom","BYTEL","20820",9),,(0,1,2,3,4),(0,1,2)

# Lets connect to SFR LTE-M

AT+CMNB=2
AT+COPS=1,2,"20810",7
AT+CGDCONT=1,"IP","onomondo"
AT+CNCFG=0,1,"onomondo"
# > AT+CGDCONT=1,"IP","TM"
# > AT+CNCFG=0,1,"TM"

# Check registration status

AT+CEREG?
+CEREG: 0,5

AT+COPS?
+COPS: 1,2,"20810",7

# Ping test

AT+CNACT=0,1

AT+CNACT?
+CNACT: 0,1,"100.83.225.165"

AT+SNPDPID=0
AT+SNPING4="8.8.8.8",5,16,5000
+SNPING4: 1,8.8.8.8,481
```
