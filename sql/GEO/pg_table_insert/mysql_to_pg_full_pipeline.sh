#!/usr/bin/bash

# Configurazione database MySQL
MYSQL_USER=""
MYSQL_PASS=""
MYSQL_DB=""
MYSQL_HOST="localhost"

# Directory di lavoro
EXPORT_DIR="tmp/mysql_data_export"
IMPORT_DIR="tmp/pg_data_import"
OUTPUT_DIR="."

# File di output combinato
COMBINED_FILE="geo.sql"

# Funzione per mostrare l'uso
usage() {
    echo "Uso: $0 -u mysql_user -p mysql_password -d mysql_database [-h mysql_host]"
    echo ""
    echo "Opzioni:"
    echo "  -u    Username MySQL"
    echo "  -p    Password MySQL"
    echo "  -d    Nome database MySQL"
    echo "  -h    Host MySQL (default: localhost)"
    exit 1
}

# Parse parametri
while getopts "u:p:d:h:" opt; do
    case $opt in
        u) MYSQL_USER="$OPTARG" ;;
        p) MYSQL_PASS="$OPTARG" ;;
        d) MYSQL_DB="$OPTARG" ;;
        h) MYSQL_HOST="$OPTARG" ;;
        *) usage ;;
    esac
done

# Verifica parametri obbligatori
if [ -z "$MYSQL_USER" ] || [ -z "$MYSQL_PASS" ] || [ -z "$MYSQL_DB" ]; then
    echo "Errore: Tutti i parametri -u, -p, -d sono obbligatori"
    usage
fi

echo "=========================================="
echo "MYSQL TO POSTGRESQL CONVERSION PIPELINE"
echo "=========================================="
echo "MySQL Host: $MYSQL_HOST"
echo "MySQL Database: $MYSQL_DB"
echo "MySQL User: $MYSQL_USER"
echo "Export Directory: $EXPORT_DIR"
echo "Import Directory: $IMPORT_DIR"
echo "Output File: $COMBINED_FILE"
echo ""

# Crea le directory di lavoro
mkdir -p "$EXPORT_DIR"
mkdir -p "$IMPORT_DIR"
> "$COMBINED_FILE"  # Svuota il file se esiste

echo "Step 1: Esportazione tabelle da MySQL..."
echo "------------------------------------------"

# Ottieni la lista delle tabelle
TABLES=$(mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" -e "SHOW TABLES;" "$MYSQL_DB" 2>/dev/null | tail -n +2)

if [ $? -ne 0 ]; then
    echo "Errore: Impossibile connettersi al database MySQL"
    exit 1
fi

if [ -z "$TABLES" ]; then
    echo "Errore: Nessuna tabella trovata nel database $MYSQL_DB"
    exit 1
fi

echo "Tabelle trovate:"
echo "$TABLES"
echo ""

# Esporta ogni tabella
for TABLE in $TABLES; do
    echo "Esportando tabella: $TABLE"
    mysqldump -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" \
        --no-create-info \
        --complete-insert \
        --no-create-db \
        --skip-add-locks \
        --skip-disable-keys \
        --skip-comments \
        "$MYSQL_DB" "$TABLE" > "$EXPORT_DIR/$TABLE.sql" 2>/dev/null

    if [ $? -eq 0 ]; then
        echo "✓ $TABLE.sql creato"
    else
        echo "✗ Errore nell'export di $TABLE"
    fi
done

echo ""
echo "Step 2: Conversione per PostgreSQL..."
echo "--------------------------------------"

# Converti ogni file esportato
for FILE_PATH in "$EXPORT_DIR"/*.sql; do
    # Controlla se esistono file .sql
    if [ ! -f "$FILE_PATH" ]; then
        echo "Nessun file .sql trovato in $EXPORT_DIR"
        continue
    fi

    # Estrae il nome del file senza path e senza estensione
    BASENAME=$(basename "$FILE_PATH" .sql)

    # Imposta le variabili
    TABLE_NAME=$BASENAME
    INPUT_FILE="$FILE_PATH"
    OUTPUT_FILE="$IMPORT_DIR/$TABLE_NAME.pg.sql"

    echo "Convertendo: $BASENAME"

    # Copia il file di input nel file di output
    cp "$INPUT_FILE" "$OUTPUT_FILE"

    # 1. Sostituisci i backtick vuoti con il nome della tabella
    sed -i "s/INSERT INTO \`\`/INSERT INTO $TABLE_NAME/" "$OUTPUT_FILE"

    # 2. Rimuovi i backtick dai nomi delle colonne
    sed -i 's/`//g' "$OUTPUT_FILE"

    # 3. Gestisci gli apostrofi nei valori (come d'Ivrea)
    sed -i "s/\\\\'/''/g" "$OUTPUT_FILE"

    # 4. Aggiungi il risultato al file combinato
    cat "$OUTPUT_FILE" >> "$COMBINED_FILE"
    echo "" >> "$COMBINED_FILE"  # Aggiungi riga vuota tra le tabelle

    echo "✓ $TABLE_NAME convertita e aggiunta a $COMBINED_FILE"
done

echo ""
echo "Step 3: Pulizia finale..."
echo "--------------------------------------"

# Aggiungi header al file combinato
sed -i "1i\\-- File combinato generato automaticamente da MySQL a PostgreSQL\\-- Data: $(date)\\n" "$COMBINED_FILE"

echo ""
echo "=========================================="
echo "CONVERSIONE COMPLETATA!"
echo "=========================================="
echo "File PostgreSQL generati in: $IMPORT_DIR"
echo "File combinato generato: $COMBINED_FILE"
echo "File originali MySQL in: $EXPORT_DIR"
echo ""
echo "Per importare in PostgreSQL:"
echo "psql -U postgres_user -d postgres_db -f $COMBINED_FILE"
