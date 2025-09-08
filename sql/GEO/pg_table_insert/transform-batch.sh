#!/usr/bin/bash

# Directory con i file MySQL
MYSQL_DIR="../mysql_table_export/"

# Controlla se la directory esiste
if [ ! -d "$MYSQL_DIR" ]; then
    echo "Errore: Directory $MYSQL_DIR non trovata!"
    exit 1
fi

# Itera su tutti i file .sql nella directory
for FILE_PATH in "$MYSQL_DIR"*.sql; do
    # Controlla se esistono file .sql
    if [ ! -f "$FILE_PATH" ]; then
        echo "Nessun file .sql trovato in $MYSQL_DIR"
        exit 1
    fi
    
    # Estrae il nome del file senza path e senza estensione
    BASENAME=$(basename "$FILE_PATH" .sql)
    
    # Imposta le variabili
    TABLE=$BASENAME
    INPUT_FILE="$FILE_PATH"
    OUTPUT_FILE="$TABLE.pg.sql"
    
    echo "==================================="
    echo "Elaborando: $BASENAME"
    echo "Nome tabella: geo.$TABLE"
    echo "File input: $INPUT_FILE"
    echo "File output: $OUTPUT_FILE"
    
    # Copia il file di input nel file di output
    cp "$INPUT_FILE" "$OUTPUT_FILE"
    
    # Aggiunge il nome della tabella
    sed -i "s/INSERT INTO \`\`/INSERT INTO geo.$TABLE/" "$OUTPUT_FILE"
    
    # Aggiusta i nomi dei campi
    sed -i 's/`//g' "$OUTPUT_FILE"
    
    # Modifica i single quote escaped
    sed -i "s/\\\\'/''/" "$OUTPUT_FILE"
    
    echo "✓ Completato: $OUTPUT_FILE"
done

echo "==================================="
echo "Conversione completata per tutti i file!"
