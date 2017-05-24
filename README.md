This is a fork of **contiki-os** where implementation of TinyIPFIX has been done for our master thesis. The goal is to implement a monitoring tool for IoT network.

In our case we send network informations using TinyIPFIX messages formats. Implementations of TinyIPFIX can be found in *core/net/ipv6/tinyipfix* and *core/net/ipv6/ipv6flow*. The first directory contains functions to create IPFIX or TinyIPFIX message while the second directory is where we keep status of the different flows passing through the node.

Examples can be found in *examples/ipflow* for different configurations:
- with IPFIX
- with TinyIPFIX
- with TinyIPFIX and aggregator

A border router doing the conversion from TinyIPFIX to IPFIX can be found also in examples/ipflow/border-router*.
