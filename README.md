# rhpman-sim

This NS-3 simulator code module that attempts to implement the RHPMAN scheme
and reproduce its performance evaluation as described by the following paper:

> K. Shi and H. Chen, "RHPMAN: Replication in highly partitioned mobile
> ad hoc networks," International Journal of Distributed Sensor Networks
> Jul. 2014.


This repository is a continuation of the work started by Keefer Rourke to complete
the implementation. 


## Motivation

Our lab<sup>[1]</sup> is investigating ad-hoc mobile mesh networks (MANETs) and
their properties with the hopes that we may inform development of quality mesh
network protocols and network applications.
A challenge with MANETs is their inherently mobile nature.
Much research has been done to discover good algorithms for data distribution
and replication within MANETs, however many of these studies are unreproducible
as-is.

Having identified the RHPMAN scheme as described by Shi and Chen 2014 as a
promising data replication scheme, this project was created to attempt to
implement it on the [ns-3 simulator](https://www.nsnam.org/), and run the scheme
under a similar setting as is described in the performance evaluation conducted
by Shi and Chen.

*Note*: Shi and Chen used the OPNET simulator for their study, however this
simulator does not appear to be widely available anymore. Hence, we have chosen
to use the latest version of the popular ns-3 network simulator instead.

## Building this project

Reproducibility is key in simulation studies, so here's how to build the
project!

 1. This module makes use of [Google protocol buffers] for the network serialization
    install the protobuf code generator for C++ on your system. 

 2. Download and build copy of the ns-3.32 all-in-one distribution.

    ```sh
    wget https://www.nsnam.org/release/ns-allinone-3.32.tar.bz2
    tar xjvf ns-allinone-3.32.tar.bz2
    cd ns-allinone-3.32
    python3 ./build.py --enable-examples --enable-tests
    ```

 3. Change directories to the `contrib/` folder of the ns-3.32 source
    distribution.

    ```sh
    cd ns-3.32/contrib/
    ```

 4. Clone this repository.

    ```sh
    git clone git@github.com:marshallasch/rhpman.git
    ```

 5. Change directory back to the `ns-3.32` folder of the source distribution
   and re configure the module through the `waf` tool. 

   ```sh
   cd ..
   ./waf configure --enable-tests --enable-examples
   ```

 6. Change directory back to the `ns-3.32` folder of the source distribution
   and run this simulation through the `waf` tool. This will compile the
   simulation code and start executing the code.

   ```sh
   cd ..
   ./waf --run 'rhpman-example`
   ```

## Running the simulation

If you're familiar with ns-3, then you should know that the simulation is run
via the `waf` build tool. Arguments to this program *must* be part of the same
string that is passed to `./waf --run` (that's just how it works :shrug:).

Every parameter of the simulation is configurable. Run the following to see
all the configurable parameters. The default values are as described in the
RHPMAN paper cited at the top of this document.

```
./waf --run 'rhpman-example --printHelp'  # <-- mind the quotes!
```

You can view an animation of the simulation using `NetAnim`, which is included
with the ns-3 all-in-one distribution. To do so, run the following:

```
./waf --run 'rhpman-example --animation-xml=path/to/rhpman.xml
```

This will generate an XML file at the specified path. You can then open this
file with `NetAnim` to view what happens during the simulation run.

## Code style

This project is formatted according to the `.clang-format` file included in this
repository. It intentionally deviates from the code style used by the ns-3
library and simulator developers.


## Important assumptions

This **MUST** be run only on IPv4 based networks, the node ID that is used is a 32 bit
identifier that is equal to the IPv4 address of the node.
This ID is used to communicate and assign the nodes as replica holder nodes.
This could be modified to use IPv6 addresses by using a 128 bit value instead. 

## License

While ns-3 is itself licensed under the GNU General Public License v2, the code
in this repository is made available under the Internet Systems Consortium (ISC)
License.

A copy of this license is included in this repository, and embedded in the top
of each source file.

<!-- links -->

[1]: https://danielgillis.wordpress.com/students/
[Google Protbuf]: https://developers.google.com/protocol-buffers
