# satoriNOW

This project is a CLI application intended to offer command line functionality to users of the Satori Network.

https://satorinet.io/

This project is not associated with the Satori project or the Satori Association.

# build

make ;

make install

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
$ ./build/satoricli help

help - Display supported commands
neuron parent status - Display the specified neuron's parent status report
neuron register - Register a protected neuron.
neuron stats - Display neuron stats
neuron system metrics - Display neuron system metrics
neuron unlock - Generate an authenticated session on the specified neuron.
neuron vault transfer - Transfer the specified amount of satori from the vault to the specified wallet address
repository backup - Backup the repository
repository password - Change the repository password
repository show - Display the contents of the repository
shutdown - Shutdown the SatoriNOW daemon
```

## REPOSITORY

Upon first use of the SatoriNOW repository, the SatoriNOW CLI will request a repository password. This password will be
requested when necessary. This password can (and should) be different than an existing neuron password. Remember to store
this password in a secure location as it cannot be recovered.

### SHOW

Use:

> satoricli repository show

```
$ ./build/satoricli repository show
	HOST	NICKNAME	PASSWORD
	192.168.1.100:24601	satori-001	******************************
```

to display the contents of the SatoriNOW CLI repository. The repository is encrypted and intended to store Satori Neuron
host, port, nickname, and password information.

### REGISTER NEURON

Use:

> satoricli neuron register _host:ip_ _nickname_

to register a neuron.

```
$ ./build/satoricli neuron register 192.168.1.100:24601 satori-001
Repository Password:

Remember to store your SatoriNOW repository password in a secure location.

You must add your Neuron password to your SatoriNOW repository.
Neuron Password:
Neuron Registered.
```

## SATORI NEURONS

Once you have registered your Satori Neuron(s) you can utilize SatoriNOW CLI operations. You can reference neurons via
host:port or nicknames.

### NEURON PARENT STATUS

Use:

> satoricli neuron parent status _nickname_

to display the current parent status of the specified Satori neuron.

```
$ ./build/satoricli neuron parent status satori-001
Neuron Authenticated.
Neuron parent status to follow:

PARENT	 CHILD	CHARITY	AUTO	    WALLET	     VAULT	  REWARD	POINTED		DATE
 36517	731078	     NO	  NO	EWKF...T3xj	EUPc...FnJz	0.00000000	    YES		2024-12-11 04:38:52.241291+00:00
 36517	776732	     NO	  NO	EV4e...THPC	EMuQ...6cpy	0.05762304	    YES		2025-01-17 00:26:15.725830+00:00
 36517	742754	     NO	  NO	ERC1...9Skr	EeRP...vqeQ	0.05793529	    YES		2024-12-31 01:50:36.573821+00:00
 36517	731075	     NO	  NO	ETTY...CG65	EYB2...jZDA	0.05782454	    YES		2024-12-11 04:04:00.604563+00:00
 36517	741079	     NO	  NO	EHAJ...uuHg	EWus...7pJM	0.05847362	    YES		2024-12-17 15:46:51.311670+00:00
 36517	742763	     NO	  NO	EVqq...XQJk	EXQ7...CVQf	0.05850580	    YES		2024-12-26 01:37:05.237101+00:00
 36517	742816	     NO	  NO	ENR8...odwF	ENk9...FkS9	0.05852359	    YES		2024-12-27 01:02:22.303726+00:00
 36517	495599	     NO	  NO	EcWL...3YzZ	EfRU...Fka2	0.05868400	    YES		2024-10-25 05:01:17.081909+00:00
 36517	776982	     NO	  NO	EdZF...PecP	EQpw...65LC	0.06010722	    YES		2025-01-17 00:42:02.601209+00:00
 36517	776983	     NO	  NO	EU8b...HMi5	EQDr...j4ir	0.06010722	    YES		2025-01-17 01:01:35.680172+00:00
 36517	686837	     NO	  NO	EJJx...niAb	EgEC...HMzh	0.06010722	    YES		2024-11-13 03:49:19.895696+00:00
 36517	776985	     NO	  NO	ENEC...LAWF	Ebik...Hy14	0.06010722	    YES		2025-01-21 00:49:26.391670+00:00
 36517	477758	     NO	  NO	EWco...Mki5	ETmK...znX7	0.06010722	    YES		2024-10-25 05:04:47.792558+00:00
 36517	453006	     NO	  NO	EZkd...WJSi	EYnj...Tx6c	0.06010722	    YES		2024-10-25 05:18:32.565503+00:00
 36517	560206	     NO	  NO	ELNd...V9Fk	EWqR...JL5w	0.06010722	    YES		2024-10-25 04:43:46.001991+00:00
 36517	579722	     NO	  NO	ENoN...D6xi	EUmY...j62r	0.06010722	    YES		2024-10-25 04:28:46.058991+00:00
 36517	560205	     NO	  NO	EaAF...u3wt	EKRT...4ijq	0.06010722	    YES		2024-10-25 04:33:16.287086+00:00
 36517	283254	     NO	  NO	EgUW...Qkbd	EeSN...oywG	0.06010722	    YES		2024-10-25 06:05:33.822054+00:00
 36517	559880	     NO	  NO	EUw8...i9bn	EbUt...7n3r	0.06010722	    YES		2024-10-25 04:56:02.608659+00:00
 36517	441503	     NO	  NO	EPhp...edyb	EYWP...pB7R	0.06010722	    YES		2024-10-25 05:25:24.045841+00:00
 36517	477717	     NO	  NO	EZNC...RCDW	EcNq...rji5	0.06010722	    YES		2024-10-25 05:13:27.953333+00:00
 36517	688905	     NO	  NO	EX1A...nJN6	ETNX...Udkd	0.06010722	    YES		2024-11-14 03:41:33.645863+00:00
 36517	586154	     NO	  NO	Eg8g...bhZA	ESWu...4aV5	0.06010722	    YES		2024-10-25 04:22:14.693348+00:00
 36517	267523	     NO	  NO	EKVH...bCPM	EecH...YppS	0.06010722	    YES		2024-10-25 06:17:40.749995+00:00
 36517	441294	     NO	  NO	ENrL...4v1M	EHo4...HjAL	0.06010722	    YES		2024-10-25 05:51:13.993719+00:00
 36517	686404	     NO	  NO	Eext...i4wd	Edkp...Tq1Y	0.06010722	    YES		2024-11-13 03:28:51.592963+00:00
 36517	702680	     NO	  NO	EbMd...iavo	EYrm...U351	0.06010722	    YES		2024-11-19 05:12:35.566161+00:00
 36517	688871	     NO	  NO	EcDR...zAwR	EVAi...stTF	0.06010722	    YES		2024-11-14 03:20:13.051704+00:00
 36517	702651	     NO	  NO	EKYq...MxQN	EPDT...qKUL	0.06010722	    YES		2024-11-19 04:33:03.429253+00:00
 36517	403071	     NO	  NO	Ecyq...vgji	EXim...Jy7V	0.06010722	    YES		2024-10-25 05:41:16.628615+00:00
 36517	686802	     NO	  NO	EQSd...jd4y	Ecb1...keqX	0.06010722	    YES		2024-11-13 03:41:35.127929+00:00
 36517	687051	     NO	  NO	ELo2...UVWe	EaYo...pGes	0.06010722	    YES		2024-11-13 04:24:26.517365+00:00
 36517	441369	     NO	  NO	Ebzb...D2hd	ENUR...J9SE	0.06010722	    YES		2024-10-25 06:00:05.547183+00:00
 36517	702652	     NO	  NO	EHBh...NZam	EPeg...FZhN	0.06010722	    YES		2024-11-19 04:58:31.925626+00:00
 36517	687080	     NO	  NO	EVmg...cMF9	EfdU...wn6p	0.06010722	    YES		2024-11-13 04:46:17.533370+00:00
 36517	403327	     NO	  NO	EfgR...95fy	ELMd...V9Q1	0.06010722	    YES		2024-10-25 05:32:36.711479+00:00

NEURON 'satori-001' HAS 36 DELEGATED NEURONS
```

