if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <character>"
    exit 1
fi


if ! [[ "$1" =~ ^[[:alnum:]]$ ]]; then
    echo "Argumentul trebuie să fie un singur caracter alfanumeric."
    exit 1
fi

count=0

while IFS= read -r line; do
    if [[ "$line" =~ ^[[:upper:]] && "$line" =~ $1 && "$line" =~ ^[[:alnum:][:space:],.!?]+$ && ("$line" =~ [?!.]$) && !("$line" =~ (, si|,si)) ]]; then
        count=$((count + 1))
    fi
done


echo "$count"
