#S satoriNOW

This project is a CLI application intended to offer command line functionality to users of the Satori Network.

https://satorinet.io/

This project is not associated with the Satori project or the Satori Association.

# build

make ;

make install  (optional)

# run

This project is still in development, but requires the execution of the server:

/path/to/project/build/satorinow

and a client-side CLI

/path/to/project/build/satoricli help

# valgrind

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./build/satorinow 

# usage

## HELP

Use:

> satoricli help

to display the list of available operations.

```
$ satoricli help

help				- Display supported commands
neuron addresses		- Display the specified neuron's wallet addresses
neuron delegate			- Display the specified neuron's delegate status
neuron parent status		- Display the specified neuron's parent status report
neuron ping			- Ping the specified neuron
neuron pool participants 	- Display the specified neuron's pool participants
neuron register 		- Register a protected neuron.
neuron stats 			- Display neuron stats
neuron system metrics 		- Display neuron system metrics
neuron unlock 			- Generate an authenticated session on the specified neuron.
neuron vault 			- Access the specified neuron's vault and display the CSRF token
neuron vault transfer 		- Transfer the specified amount of satori from the vault to the specified wallet address
repository backup 		- Backup the repository
repository password 		- Change the repository password
repository show 		- Display the contents of the repository
shutdown 			- Shutdown the SatoriNOW daemon
```

## REPOSITORY

Upon first use of the SatoriNOW repository, the SatoriNOW CLI will request a repository password. This password will be
requested when necessary. This password can (and should) be different than an existing neuron password. Remember to store
this password in a secure location as it cannot be recovered.

### SHOW

Use:

> satoricli repository show

```
$ satoricli repository show
	HOST	                NICKNAME	PASSWORD
	192.168.1.100:24601	satori-001	******************************
```

to display the contents of the SatoriNOW CLI repository. The repository is encrypted and intended to store Satori Neuron
host, port, nickname, and password information.

### REGISTER NEURON

Use:

> satoricli neuron register _host:ip_ _nickname_

to register a neuron.

```
$ satoricli neuron register 192.168.1.100:24601 satori-001
Repository Password:

Remember to store your SatoriNOW repository password in a secure location.

You must add your Neuron password to your SatoriNOW repository.
Neuron Password:
Neuron Registered.
```

## SATORI NEURONS

Once you have registered your Satori Neuron(s) you can utilize SatoriNOW CLI operations. You can reference neurons via
host:port or nicknames.

### NEURON ADDRESSES

Use:

> satoricli neuron addresses _nickname_

to display the address(es) assigned to the specified neuron.

```
$ satoricli neuron addresses satori-001
Repository Password:

Remember to store your SatoriNOW repository password in a secure location.

Neuron Authenticated.
Neuron wallet addresses to follow:

'satori-001' is mining to wallet address: EXe...1gfR
```

### NEURON DELEGATE INFORMATION

Use:

> satoricli neuron delegate _nickname_ [json]

to display the current delegate information for the specified neuron. Use the option 'json' argument
to display the raw JSON returned from the specified neuron.

```
$ satoricli neuron delegate satori-087

Neuron delegate to follow:

            NICKNAME	                             WALLET	                              VAULT	   OFFER	ACCEPTING	ALIAS
          satori-087	 ETU972nu9naUffZuUkVFoHGpq2AZJdBjFi	 ETU972nu9naUffZuUkVFoHGpq2AZJdBjFi	50.00000000	       NO	public neuron pool
```

### NEURON PARENT STATUS

Use:

> satoricli neuron parent status _nickname_

to display the current parent status of the specified Satori neuron.

```
$ satoricli neuron parent status satori-001
Neuron Authenticated.
Neuron parent status to follow:

PARENT	 CHILD	CHARITY	AUTO	    WALLET	     VAULT	  REWARD	POINTED		DATE
 36517	731078	     NO	  NO	EWKF...T2xk	EUPc...FnJz	0.00000000	    YES		2024-12-11 04:38:52.241291+00:00
 36517	776732	     NO	  NO	EV4e...NH4C	EMuQ...6cpy	0.05762304	    YES		2025-01-17 00:26:15.725830+00:00
 36517	742754	     NO	  NO	ERC1...9e8r	EeRP...vqeQ	0.05793529	    YES		2024-12-31 01:50:36.573821+00:00
 36517	731075	     NO	  NO	ETTY...Cc65	EYB2...jZDA	0.05782454	    YES		2024-12-11 04:04:00.604563+00:00
 36517	741079	     NO	  NO	EHAJ...uuyg	EWus...7pJM	0.05847362	    YES		2024-12-17 15:46:51.311670+00:00
 . . .
 36517	403071	     NO	  NO	Ecyq...vl5i	EXim...Jy7V	0.06010722	    YES		2024-10-25 05:41:16.628615+00:00
 36517	686802	     NO	  NO	EQSd...jvoy	Ecb1...k8qX	0.06010722	    YES		2024-11-13 03:41:35.127929+00:00
 36517	687051	     NO	  NO	ELo2...Uuve	EaYo...p6es	0.06010722	    YES		2024-11-13 04:24:26.517365+00:00
 36517	441369	     NO	  NO	Ebzb...D68d	ENUR...J9uE	0.06010722	    YES		2024-10-25 06:00:05.547183+00:00
 36517	702652	     NO	  NO	EHBh...NZmm	EPeg...FZpN	0.06010722	    YES		2024-11-19 04:58:31.925626+00:00
 36517	687080	     NO	  NO	EVmg...coF9	EfdU...w86p	0.06010722	    YES		2024-11-13 04:46:17.533370+00:00
 36517	403327	     NO	  NO	EfgR...9afy	ELMd...V9w1	0.06010722	    YES		2024-10-25 05:32:36.711479+00:00

NEURON 'satori-001' HAS 36 DELEGATED NEURONS
```

### NEURON PING

Use:

> satoricli neuron ping _nickname_

to ping the specified Satori neuron. This will display the timestamp returned by the neuron as well as the time required
to service the HTTP ping request.

```
$ satoricli neuron ping satori-001
'satori-001' reports current time '2025-01-28 22:34:51', ping time: 1003.985336 ms

```

### NEURON STATS

Use:

> satoricli neuron stats _nickname_

to display the current stats of the specified Satori neuron. If you do not specify a _nickname_, all neurons in the
repository will be queried and displayed.

```
$ satoricli neuron stats satori-001
satori-001: This Neuron has participated in 630 competitions today, with an average placement of 52 out of 100

```

### NEURON SYSTEM METRICS

Use:

> satoricli neuron system metrics _nickname_

to display the current system metrics of the specified Satori neuron.

```
$ satoricli neuron system metrics satori-001
Neuron Authenticated.
Neuron system metrics to follow:

	                BOOT TIME: 1734803853.000000
	                      CPU: 
	                CPU COUNT: 16
	        CPU USAGE PERCENT: 84.900000
	                DISK FREE: 1637072609280
	             DISK PERCENT: 14.400000
	               DISK TOTAL: 2014094303232
	                DISK USED: 274635747328
	            MEMORY ACTIVE: 35858862080
	         MEMORY AVAILABLE: 32068087808
	           MEMORY BUFFERS: 2003234816
	            MEMORY CACHED: 26643701760
	              MEMORY FREE: 6133522432
	          MEMORY INACTIVE: 20354072576
	           MEMORY PERCENT: 52.100000
	            MEMORY SHARED: 1964589056
	              MEMORY SLAB: 2649280512
	             MEMORY TOTAL: 66954407936
	              MEMORY USED: 32173948928
	 MEMORY AVAILABLE PERCENT: 47.895409
	          MEMORY TOTAL GB: 62
	                SWAP FREE: 0
	             SWAP PERCENT: 0.000000
	                 SWAP SIN: 0
	                SWAP SOUT: 0
	               SWAP TOTAL: 0
	                SWAP USED: 0
	                TIMESTAMP: 1738088255.672162
	                   UPTIME: 3284402.672160
	                  VERSION: 0.3.8


```

### NEURON TRANSFER SATORI

Use:

> satoricli neuron vault transfer _amount_ satori _wallet_address_ _nickname_

to transfer 0.01 $SATORI from the 'satori-001' vault to the specified destination wallet

```
satoricli neuron vault transfer 0.01 satori EUv...89nP satori-001

```

**Experimental**: This doesn't work yet

### NEURON UNLOCK

Use:

> satoricli neuron unlock _nickname_

to unlock a Satori neuron and display the session cookie

```
$ satoricli neuron unlock satori-001
Neuron Unlocked. Session Cookie to follow:
eyJhdXRoZW50aWNhdGVkIjp0cnVlLCJjc3JmX3Rva2VuIjoiZGQyMmQ4NmE0MjI3YjQ5ZDdmMGNiOWQ2MWNjYjNjZWE5NTk1ZjlhMiJ9.Z5pLeg.4eTONcfpjPhw5xrxFzfzEzTKcH4

```

### NEURON VAULT

Use:

> satoricli neuron vault _nickname_

```
$ satoricli neuron vault satori-001
Connecting Neuron. CSRF token to follow:
ImFkOWY5ZTkxMDdiMDdlNzRiYzM3NmU5ZWE1NzI2Nzc2YjA0MDI1ZDgi.Z5pxUA.ql6kmCgQZnHHK2lojoFUi8Xwyu8

```

