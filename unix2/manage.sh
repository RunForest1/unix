#!/bin/bash

docker volume create shared_volume

docker build -t concurrent-fs .

for i in {1..50}; do
	echo "Запуск контейнера $i"
	docker run -d -v shared_volume:/shared --name "concurrent-fs-$i" concurrent-fs
done

echo "Все 50 контейнеров запущено!"
