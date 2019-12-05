OISR="$ISR"
ISR="\n"

echo -n "> "
while read num; do
	cp Testprogramme/$num*/progs.c SPOS/SPOS/progs.c
	echo -n "> "
done

ISR="$OISR"
