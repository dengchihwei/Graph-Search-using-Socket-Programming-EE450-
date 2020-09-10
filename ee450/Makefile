all:
	gcc -o client client.c
	gcc -o aws aws.c
	gcc -o serverA serverA.c
	gcc -o serverB serverB.c

.PHONY: serverA
serverA:
	./serverA

.PHONY: serverB
serverB:
	./serverB

.PHONY: aws
aws:
	./aws
