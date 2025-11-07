#!/bin/sh

SHARED_DIR="/shared"
LOCK_FILE="${SHARED_DIR}/.lock"

CONTAINER_ID=$(cat /proc/sys/kernel/random/uuid 2>del/null | cut -c1-8)

FILE_COUNTER=0

echo "Cont ID: ${CONTAINER_ID}"

while true; do
	FILE_NAME=$(flock -x "${LOCK_FILE}" -c '
		i=1
		while [ $i -le 999 ]; do
			potential_name=$(printf "%03d" "$i")
			file_path=\"${SHARED_DIR}/\${potential_name}\"
			#Проверка свободно ли имя?
			if [ ! -f "${file_path}" ]; then
				echo "${CONTAINER_ID}${FILE_COUNTER}" > "\${file_path}"
				echo "${potential_name}"
				exit 0
			fi
			i=$((i + 1))
		done
		#Если нет свободного имени
		exit 1
	')

	FILE_COUNTER=$((FILE_COUNTER + 1))
	echo "[CREATE] Контейнер: ${CONTAINER_ID} | Файл: ${FILE_NAME} | Счет: ${FILE_COUNTER}"
	sleep 1
	if [ -n "${FILE_NAME}" ]; then
		rm -f "${SHARED_DIR}/${FILE_NAME}"
		echo "[DELETE] Контейнер:${CONTAINER_ID} Файл: ${FILE_NAME}"
	fi

	sleep 1
done
