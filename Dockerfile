FROM debian:stable

WORKDIR /app
COPY compiler .
COPY _setup.sh .

RUN apt update -y \
    && apt full-upgrade -y \
    && apt install gcc -y \
    && apt install g++ -y \
    && apt install vim -y \
    && apt install man -y

CMD ["sh", "_setup.sh"]
