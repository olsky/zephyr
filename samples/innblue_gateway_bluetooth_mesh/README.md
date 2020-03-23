# innblue_gateway_bluetooth_mesh
gateway the nRF52 mesh

## Quick start

```
mkdir ~/workspace
cd ~/workspace
west init -m https://github.com/olsky/innblue_gateway_bluetooth_mesh.git
west update
```

## Build for Thingy-91

### Build the nrf91 Chip

```
cd inn-gw-ble
west build  -p -b nrf9160_pca20035ns
west flash
```

### Build the nrf52 Chip

clone innblue_mesh_node_base git project

Build and flash in Segger the project ```dfu_pca20035_s140.emProject```



Now restart the Thingy-91 board from power switch.


## Build for DK-91

### Build the nrf91 Chip

```
cd inn-gw-ble
west build  -p -b nrf9160_pca10090ns
west flash
```

### Build the nrf52 Chip

clone innblue_mesh_node_base git project

Build and flash in Segger the project ```dfu_pca10090_s140.emProject```


Now restart the DK-91 board from power switch.



## Usage

### General Info

https://innblue.atlassian.net/wiki/spaces/MP/pages/75169793/Gateway+Node

### Mqtt pub/sub

Following configurable behaviour is in place ( configure the proj.conf file):

* The board will listen to the ```CONFIG_NET_CONFIG_PEER_IPV4_ADDR``` mqtt server ip address.
* The board will use the ```CONFIG_MQTT_PORT``` for non-tls connection.
* The board will publish messages to the ```CONFIG_MQTT_PUB_TOPIC```.
* The board will subscribe to commands from the ```CONFIG_MQTT_SUB_TOPIC```.
