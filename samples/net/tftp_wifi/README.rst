.. tftp_wifi:

TFTP With WiFi
###########

Overview
********
The purpose is to demonstrate the usage of TFTP Client with WiFi. The application firstly 
sets up WiFi, connects with an Access Point and tries to get a file from the TFTP Server. 

* single thread
* multi threading

Building and Running
********************

This application is currently only supported and tested on the STM32 Disco IOT Kit. See 
here: https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html

Though this application will work with any platform supporting WiFi, compatibility cannot 
be guranteed. 

.. zephyr-app-commands::
   :zephyr-app: samples/net/tftp_wifi
   :host-os: unix
   :board: disco_l475_iot1
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

    uart:~$ *** Booting Zephyr OS build zephyr-v2.1.0  ***
	[00:00:00.540,000] <inf> net_config: Initializing network
	uart:~$ Hello World! disco_l475_iot1
	Connected
	[00:00:07.209,000] <dbg> tftp_client.tftp_send_request: tftp_get success - Server returned: 1
	[00:00:07.209,000] <inf> tftp_client: Server sent Block Number: 1
	[00:00:07.209,000] <inf> tftp_client: Client acking Block Number: 1
	[00:00:07.762,000] <inf> tftp_client: Server sent Block Number: 2
	[00:00:07.762,000] <inf> tftp_client: Client acking Block Number: 2
	[00:00:08.290,000] <inf> tftp_client: Server sent Block Number: 3
	[00:00:08.290,000] <inf> tftp_client: Client acking Block Number: 3
	[00:00:08.304,000] <inf> tftp_client: Server send duplicate (already acked) block again. Block Number: 3
	[00:00:08.304,000] <inf> tftp_client: Client already trying to receive Block Number: 4
