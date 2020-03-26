build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc roundrobin.c -o roundrobin.out
	gcc hpf.c -o hpf.out
	gcc srtn.c -o srtn.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out

all: build run clean

run:
	./process_generator.out
