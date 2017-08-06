FROM alpine:3.6
RUN apk update && apk add nasm gcc musl-dev make
WORKDIR /exp
COPY . .
