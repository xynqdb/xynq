**! EARLY DRAFT !\
! WORK IN PROGRESS !**\
\
\
\
&nbsp;
# Xynq
High performance object storage for spatial applications.
Designed for AR/VR, self-driving vehicles simulation, robotics and video games.

* Open-source
* Scalable: large 3D worlds support, world partitioning
* Highly available
* Maximum performance at lowest cost
* Real-time high frequency updates feeds
* Conflict-resolution / object authority model
* Native entity-component systems support
* Multiple data formats support: binary and textual

# Slang language
At the core of Xynq is the built-in programming language, **Slang**, a small language, based on [Lisp](https://en.wikipedia.org/wiki/Common_Lisp), used to manipulate real-time object data. Xynq uses slang for configuration, data transformation and querying.

# Quick start
TODO
```bash
% ./xynq --config ../../build/example.conf
```

## Configuration
Just like many other things with Xynq, configuration is done with slang using [S-expressions](https://en.wikipedia.org/wiki/S-expression).
Example of simple config:
```lisp
    ;
    ; Task system
    ;
    (task
        (num-threads auto)          ; Number of threads to use.
                                    ; If set to auto -> will auto will automatically set it
                                    ; to number of cpu cores on the machine.
        (pin-threads Yes))          ; Pin threads to cores.

    ;
    ; Tcp connections
    ;
    (tcp
        (bind "0.0.0.0:9920"        ; Addresses to listen on for tcp connections. Might add multiple ipv4 or 6 addresses.
              "::0:9920")
        (reuse-bind-addr Yes)       ; Bind socket even if someone else is listening on it. Mostly used for debugging.
                                    ; Use at production at your own risk.
        (keep-alive
            (enable  Yes)))         ; Enable/disable tcp keep-alive sends.

    ;
    ; Execute slang code once system is up. (for example - can be used to setup some initial db schemas)
    ;
    (exec
        (@locate "example-schema.slang"))   ; Will try to find file in config directory
```
## Data types
TODO

## Define new data type
Sample code below defines new structure Car with three fields x, y, z of type double.
```lisp
% nc 127.0.0.1 9920
(defstruct Car
           :x double
           :y double
           :z double)
[]
```

## Create object
Sample code below connects to the storage and creates object of type Car at position (0.0, 25.12, 12.25) and returns its guid.
```lisp
% nc 127.0.0.1 9920
(create Car
        :x 0
        :y 25.12
        :z 12.25)
["f985ebd5-8172-49eb-9ff3-f5a8d98102de"]
```

## Query objects
The following code queries all objects within bounding sphere with the center at (0, 0, 0) and radius 25.
```lisp
% nc 127.0.0.1 9920
(select Car
        (within 0 0 0 25))
[
    {_guid:"f985ebd5-8172-49eb-9ff3-f5a8d98102de", "x":0, "y":25.1200000000000009947598, "z":12.25},
    {_guid:"b55cd452-be3f-4d81-8422-b2814867ef22", "x":1, "y":1, "z":1},
    {_guid:"846ad738-6cb1-4845-a64e-ff1860298641", "x":0, "y":0, "z":0}
]
```

