#!/usr/bin/bash
TABLE=$1
FILE=$TABLE.sql
OUTPUT=$TABLE.pg.sql

echo "Nome tabella: geo.$TABLE"
echo "Nome del file in input: $FILE"
echo "Nome del file in output: $OUTPUT"


# Aggiunge il nome della tabella
sed -i "s/INSERT INTO \`\`/INSERT INTO geo.$TABLE/" $FILE

# Aggiusta i nomi dei campi
sed -i 's/`//g' $FILE

# Modifica i single quote escaped
sed -i "s/\\\\'/''/" $FILE
