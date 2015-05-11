
Panda
====

**Perspective** : a little perspective is warranted here, a seed could grow into a tree.

Why the project's name is panda ?
---
**Answer** : I'm crazy about technology, I hope I can find the answer in the unknown world.

HOWTO
---

- get source code

		$ mkdir panda-git
        $ git clone https://github.com/pacepi/panda.git

- build code

		$ source build/setupenv
        $ make all (or "make help" for help)

- build example
		
		$ cd example
		$ make all

- run examples

		$ cd target/bin
        $ ./panda_server -c config.ini -m ../lib/panda_module_test.so


	you need to configure `config.ini` like this :

		[balancer]
		ip=pwdip
		port=1234

		[monitor]
		daemon=y
		nodes=2
		log_path=/home/pace/log

		[node]
		heartbeat=30
		log_path=/home/pace/log

