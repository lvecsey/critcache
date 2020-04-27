Take a UUID and map it to what we refer to as a csv_line, or a row of data.

You need to know the number of columns and types of data you will be encoding.

## Start the server, binding it to address and port:

  critcache-server 192.168.2.14:7375 

## Run some client commands, using the following form:

./critcache-client bindip:port serverip:port cmd key [value]

critcache-client 192.168.2.14:5250 192.168.2.14:7375 add bbcb9199-eefc-4b2f-9f76-3b332d7a9d4f Lester,Vecsey,07417
critcache-client 192.168.2.14:5250 192.168.2.14:7375 get bbcb9199-eefc-4b2f-9f76-3b332d7a9d4f
critcache-client 192.168.2.14:5250 192.168.2.14:7375 del bbcb9199-eefc-4b2f-9f76-3b332d7a9d4f Lester,Vecsey,07417

The client program will actually try to bind to the specified port, and up to 10 ports above it to find an open port. You can also edit test.env and use ./run_tests.sh

## Further testing

You can also run a **stress test** to the server:

make gen_stressclient
./gen_stressclient 192.168.2.14:5250 192.168.2.14:7375 1000000

And you should get a total elapsed time, and average time of send/recv.

The included critbit code is from https://github.com/agl/critbit
