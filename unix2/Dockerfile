FROM ubuntu:latest
RUN apt-get update && apt-get install -y util-linux coreutils
WORKDIR /app
COPY main.sh ./main.sh
RUN chmod +x ./main.sh
CMD ["./main.sh"] 
